// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "Statement.h"

#include "Column.h"
#include "Database.h"

#pragma warning ( push )
#pragma warning ( disable : GRAPE_VENDOR_WARNINGS )
#include <sqlite3.h>
#pragma warning ( pop )

namespace GRAPE {
    Statement::Statement(const Database& Db, const std::string& Query) : m_Db(Db.m_File) {
        GRAPE_ASSERT(m_Db);

        const int err = sqlite3_prepare_v2(m_Db, Query.c_str(), -1, &m_Stmt, nullptr);
        GRAPE_ASSERT(err == SQLITE_OK, "SQLite error preparing statement: '{2}'", sqlite3_errstr(err));

        m_ColumnCount = sqlite3_column_count(m_Stmt);
    }

    Statement::~Statement() { sqlite3_finalize(m_Stmt); }

    void Statement::bind(int Index, std::monostate) const noexcept {
        GRAPE_ASSERT(Index <= sqlite3_bind_parameter_count(m_Stmt));

        const int err = sqlite3_bind_null(m_Stmt, Index + 1);
        GRAPE_ASSERT(err == SQLITE_OK, "SQLite error binding value: '{2}'", sqlite3_errstr(err));
    }

    void Statement::bind(int Index, int Value) const noexcept {
        GRAPE_ASSERT(Index <= sqlite3_bind_parameter_count(m_Stmt));

        const int err = sqlite3_bind_int(m_Stmt, Index + 1, Value);
        GRAPE_ASSERT(err == SQLITE_OK, "SQLite error binding value: '{2}'", sqlite3_errstr(err));
    }

    void Statement::bind(int Index, double Value) const noexcept {
        GRAPE_ASSERT(Index <= sqlite3_bind_parameter_count(m_Stmt));

        const int err = sqlite3_bind_double(m_Stmt, Index + 1, Value);
        GRAPE_ASSERT(err == SQLITE_OK, "SQLite error binding value: '{2}'", sqlite3_errstr(err));
    }

    void Statement::bind(int Index, const char* Value) const noexcept {
        GRAPE_ASSERT(Index <= sqlite3_bind_parameter_count(m_Stmt));

        const int err = sqlite3_bind_text(m_Stmt, Index + 1, Value, -1, SQLITE_TRANSIENT);
        GRAPE_ASSERT(err == SQLITE_OK, "SQLite error binding value: '{2}'", sqlite3_errstr(err));
    }

    void Statement::bind(int Index, const Blob& Value) const noexcept {
        GRAPE_ASSERT(Index <= sqlite3_bind_parameter_count(m_Stmt));

        const int err = sqlite3_bind_blob(m_Stmt, Index + 1, Value.data(), static_cast<int>(Value.size()), SQLITE_TRANSIENT);
        GRAPE_ASSERT(err == SQLITE_OK, "SQLite error binding value: '{2}'", sqlite3_errstr(err));
    }

    void Statement::bind(int Index, const std::string& Value) const noexcept {
        GRAPE_ASSERT(Index <= sqlite3_bind_parameter_count(m_Stmt));

        const int err = sqlite3_bind_text(m_Stmt, Index + 1, Value.c_str(), static_cast<int>(Value.size()), SQLITE_TRANSIENT);
        GRAPE_ASSERT(err == SQLITE_OK, "SQLite error binding value: '{2}'", sqlite3_errstr(err));
    }

    void Statement::step() noexcept {
        const int err = sqlite3_step(m_Stmt);

        if (err == SQLITE_ROW) { m_HasRow = true; }
        else if (err == SQLITE_DONE)
        {
            m_HasRow = false;
            m_Done = true;
        }
        else { GRAPE_ASSERT(false, "SQLite error stepping statement: '{2}'", sqlite3_errstr(err)); }
    }

    void Statement::reset() noexcept {
        m_HasRow = false;
        m_Done = false;

        const int err = sqlite3_reset(m_Stmt);
        GRAPE_ASSERT(err == SQLITE_OK, "SQLite error reseting value: '{2}'", sqlite3_errstr(err));
    }

    Column Statement::getColumn(int Index) const noexcept {
        GRAPE_ASSERT(m_HasRow);
        GRAPE_ASSERT(Index < m_ColumnCount);

        return { *this, Index };
    }

    bool Statement::isColumnNull(int Index) const noexcept {
        GRAPE_ASSERT(m_HasRow);
        GRAPE_ASSERT(Index < m_ColumnCount);

        return SQLITE_NULL == sqlite3_column_type(m_Stmt, Index);
    }
}
