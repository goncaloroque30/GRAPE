// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "Database.h"

#pragma warning ( push )
#pragma warning ( disable : GRAPE_VENDOR_WARNINGS )
#include <sqlite3.h>
#pragma warning ( pop )

namespace GRAPE {
    Database::Database(const std::filesystem::path& FilePath) { open(FilePath); }

    Database::Database(const std::filesystem::path& FilePath, const std::uint8_t* Buffer, int BufferSize) { create(FilePath, Buffer, BufferSize); }

    Database::Database(const Database& Other) {
        m_FilePath = Other.m_FilePath;

        if (Other.m_File)
            open(m_FilePath);
    }

    Database& Database::operator=(const Database& Other) {
        if (this == &Other)
            return *this;

        m_FilePath = Other.m_FilePath;

        if (Other.m_File)
            open(m_FilePath);

        return *this;
    }

    bool Database::open(const std::filesystem::path& FilePath) {
        GRAPE_ASSERT(!valid(), "Database already opened!");

        std::error_code err;
        m_FilePath = weakly_canonical(FilePath, err);
        if (err)
        {
            Log::database()->error("Error opening file '{}': '{}'.", FilePath.string(), err.message());
            return false;
        }

        if (const int errOpen = sqlite3_open_v2(FilePath.string().c_str(), &m_File, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_EXRESCODE, nullptr))
        {
            Log::database()->error("SQLite error opening file '{}': '{}'.", FilePath.string(), sqlite3_errstr(errOpen));
            close();
            return false;
        }

        if (const int errReadOnly = sqlite3_db_readonly(m_File, "main"))
        {
            GRAPE_ASSERT(errReadOnly == 1); // -1 returned if database name ("main") doesn't exist.
            Log::database()->error("SQLite error opening file '{}': The file is read only.", FilePath.string());
            close();
            return false;
        }

        execute("PRAGMA foreign_keys = ON");
        execute("PRAGMA busy_timeout = 10");
        return true;
    }

    bool Database::create(const std::filesystem::path& FilePath, const char* CreateSql) {
        if (!open(FilePath))
            return false;

        const int errExec = sqlite3_exec(m_File, CreateSql, nullptr, nullptr, nullptr);
        GRAPE_ASSERT(errExec == SQLITE_OK, "SQLite error executing create statement: '{2}'", sqlite3_errstr(errExec));

        return true;
    }

    bool Database::create(const std::filesystem::path& FilePath, const std::uint8_t* Buffer, int BufferSize) {
        sqlite3* memoryDb = nullptr;
        if (const int errOpen = sqlite3_open_v2(":memory:", &memoryDb, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr))
        {
            Log::database()->error("SQLite error opening in memory database: '{}'.", sqlite3_errstr(errOpen));
            sqlite3_close(memoryDb);
            return false;
        }

        if (const int errDeserialize = sqlite3_deserialize(memoryDb, "main", const_cast<std::uint8_t*>(Buffer), BufferSize, BufferSize, SQLITE_DESERIALIZE_READONLY))
        {
            Log::database()->error("SQLite error deserializing embedded database: '{}'.", sqlite3_errstr(errDeserialize));
            sqlite3_close(memoryDb);
            return false;
        }

        if (!open(FilePath))
        {
            sqlite3_close(memoryDb);
            return false;
        }

        sqlite3_backup* newDb = sqlite3_backup_init(m_File, "main", memoryDb, "main");
        if (const int errStep = sqlite3_backup_step(newDb, -1) != SQLITE_DONE)
        {
            Log::database()->error("SQLite error saving in memory database to '{}': '{}'.", FilePath.string(), sqlite3_errstr(errStep));
            sqlite3_close(memoryDb);
            sqlite3_backup_finish(newDb);
            return false;
        }

        sqlite3_backup_finish(newDb);
        sqlite3_close(memoryDb);
        return true;
    }

    void Database::close() {
        sqlite3_close(m_File);
        m_File = nullptr;
    }

    bool Database::verify() const {
        GRAPE_ASSERT(valid());

        bool ret = true;
        // Integrity Check
        {
            Statement stmt(*this, "PRAGMA integrity_check");
            stmt.step();
            if (stmt.getColumn(0).getString() != "ok")
            {
                std::size_t errorCount = 0;
                while (stmt.hasRow())
                {
                    Log::database()->error("Integrity check error '{}'.", stmt.getColumn(0).getString());
                    ++errorCount;
                    stmt.step();
                }
                ret = false;
            }
        }

        // Foreign Key Check
        {
            Statement stmt(*this, "PRAGMA foreign_key_check");
            stmt.step();
            if (stmt.hasRow())
            {
                std::size_t errorCount = 0;
                while (stmt.hasRow())
                {
                    Log::database()->error("Foreign key error on table {}, row id {} with parent table {}.", stmt.getColumn(0).getString(), stmt.getColumn(1).getString(), stmt.getColumn(2).getString());
                    ++errorCount;
                    stmt.step();
                }
                ret = false;
            }
        }

        if (ret)
            Log::database()->info("Integrity check passed for '{}'.", path().string());

        return ret;
    }

    void Database::vacuum() const {
        GRAPE_ASSERT(valid());

        if (const int errExec = sqlite3_exec(m_File, "VACUUM", nullptr, nullptr, nullptr))
            Log::database()->error(std::format("Cleaning the study. SQLite error: '{}'.", sqlite3_errstr(errExec)));
        else
            Log::database()->info("Successfully cleaned the study.");
    }

    std::string Database::name() const { return path().stem().string(); }

    int Database::applicationId() const {
        GRAPE_ASSERT(valid());

        Statement stmt(*this, "PRAGMA application_id");
        stmt.step();
        return stmt.getColumn(0);
    }

    int Database::userVersion() const {
        GRAPE_ASSERT(valid());

        Statement stmt(*this, "PRAGMA user_version");
        stmt.step();
        return stmt.getColumn(0);
    }

    void Database::beginTransaction() const {
        GRAPE_ASSERT(valid());

        const int err = sqlite3_exec(m_File, "BEGIN IMMEDIATE TRANSACTION", nullptr, nullptr, nullptr);

        if (err == SQLITE_OK)
            return;

        if (err == SQLITE_BUSY)
        {
            GRAPE_DEBUG_INFO("Beginning transaction, database was BUSY. Trying again...");
            beginTransaction();
            return;
        }

        GRAPE_ASSERT(false, "SQLite error beginning transaction: '{2}'.", sqlite3_errstr(err));
    }

    void Database::commitTransaction() const {
        GRAPE_ASSERT(valid());

        const int err = sqlite3_exec(m_File, "COMMIT TRANSACTION", nullptr, nullptr, nullptr);

        if (err == SQLITE_OK)
            return;

        if (err == SQLITE_BUSY)
        {
            GRAPE_DEBUG_INFO("Committing transaction, database was BUSY. Trying again...");
            commitTransaction();
            return;
        }

        GRAPE_ASSERT(false, "SQLite error committing transaction: '{2}'.", sqlite3_errstr(err));
    }

    void Database::execute(const std::string& Query) const {
        GRAPE_ASSERT(valid());

        const int errExec = sqlite3_exec(m_File, Query.c_str(), nullptr, nullptr, nullptr);
        GRAPE_ASSERT(errExec == SQLITE_OK, "SQLite error executing statement: '{2}'", sqlite3_errstr(errExec));
    }

    void Database::setApplicationId(int Id) const {
        execute(std::format("PRAGMA application_id = {}", Id));
    }

    void Database::setUserVersion(int UserVersion) const {
        execute(std::format("PRAGMA user_version = {}", UserVersion));
    }
}
