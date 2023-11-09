// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Column.h"
#include "Statement.h"
#include "Table.h"

// Avoid including sqlite3.h in header file
struct sqlite3;

namespace GRAPE {
    class Database {
        friend class Statement; // Access to sqlite3*
    public:
        /**
        * @brief Default constructor, invalid state after construction.
        */
        Database() = default;

        /**
        * @brief Construct and open database at FilePath.
        */
        explicit Database(const std::filesystem::path& FilePath);

        /**
        * @brief Create a new database at FilePath, with the serialized data in Buffer.
        */
        Database(const std::filesystem::path& FilePath, const std::uint8_t* Buffer, int BufferSize);

        /**
        * @brief Opens a new connection to the same file.
        */
        Database(const Database& Other);
        Database(Database&&) = delete;

        /**
        * @brief Opens a new connection to the same file.
        */
        Database& operator=(const Database&);
        Database& operator=(Database&&) = delete;

        ~Database() { close(); }

        /**
        * @brief May be called only once per class instance lifetime. Open sqlite3 database at FilePath.
        * ASSERT !valid()
        *
        * @return True on success.
        */
        bool open(const std::filesystem::path& FilePath);

        /**
        * @brief May be called only once per class instance lifetime. Create new sqlite3 database at FilePath and execute CreateSql.
        * ASSERT !valid()
        *
        * @return True on success.
        */
        bool create(const std::filesystem::path& FilePath, const char* CreateSql = nullptr);

        /**
        * @brief May be called only once per class instance lifetime. Create new sqlite3 database at FilePath with the contents of Buffer. BufferSize should be the number of bytes in Buffer.
        * ASSERT !valid()
        *
        * @return True on success.
        */
        bool create(const std::filesystem::path& FilePath, const std::uint8_t* Buffer, int BufferSize);

        void close();

        /**
        * @brief Run an integrity check and a foreign key check on the database.
        * ASSERT valid()
        *
        * @return True on success.
        */
        bool verify() const;

        /**
        * @brief Run the VACUUM command on the database.
        * ASSERT valid()
        */
        void vacuum() const;

        /**
        * @return The full filesystem::path used to open this connection.
        */
        [[nodiscard]] std::filesystem::path path() const { return m_FilePath; }

        /**
        * @return The stem (filename without the extension) of the result from path().
        */
        [[nodiscard]] std::string name() const;

        /**
        * @return True if open() was called and sqlite3_open returned SQLITE_OK (sqlite3* is not nullptr).
        */
        [[nodiscard]] bool valid() const { return m_File; }

        /**
        * @return The application ID set in the database.
        * ASSERT valid().
        */
        [[nodiscard]] int applicationId() const;

        /**
        * @return The user version set in the database.
        * ASSERT valid().
        */
        [[nodiscard]] int userVersion() const;

        /**
        * @brief Start an immediate transaction to the database, blocks any other threads from starting a transaction.
        * ASSERT valid().
        * ASSERT result either SQLITE_OK or SQLITE_BUSY.
        */
        void beginTransaction() const;

        /**
        * @brief Commit the transaction to the database, blocks any other threads from starting a transaction.
        * ASSERT valid().
        * ASSERT result either SQLITE_OK or SQLITE_BUSY.
        */
        void commitTransaction() const;

        /**
        * @brief Execute a single statement.
        * ASSERT valid().
        * ASSERT result is SQLITE_OK.
        */
        void execute(const std::string& Query) const;

        /**
        * @brief Set the SQLite application ID to ID.
        */
        void setApplicationId(int Id) const;

        /**
        * @brief Set the SQLite user version to UserVersion.
        */
        void setUserVersion(int UserVersion) const;

        /**
        * @brief Insert all values into Tbl
        * If InsertVars is empty: ASSERT Table Size = Number of Vals.
        * If InsertVars not empty: ASSERT InsertVars Size = Number of Vals.
        */
        template <std::size_t Size, typename... Types>
        void insert(const Table<Size>& Tbl, std::initializer_list<std::size_t> InsertVars, const std::tuple<Types...>& Vals) const;

        /**
        * @brief Update values in Tbl
        * ASSERT SetVars size = number of Vals and FilterVars size = number of FilterVals.
        */
        template <std::size_t Size, typename... SetTypes, typename... FilterTypes>
        void update(const Table<Size>& Tbl, std::initializer_list<std::size_t> SetVars, const std::tuple<SetTypes...>& Vals, std::initializer_list<std::size_t> FilterVars, const std::tuple<FilterTypes...>& FilterVals) const;

        /**
        * @brief Update values in Tbl
        * ASSERT SetVars size = TableSize and FilterVars size = number of FilterVals.
        */
        template <std::size_t Size, typename... SetTypes, typename... FilterTypes>
        void update(const Table<Size>& Tbl, const std::tuple<SetTypes...>& Vals, std::initializer_list<std::size_t> FilterVars, const std::tuple<FilterTypes...>& FilterVals) const;

        /**
        * @brief Delete values from Tbl.
        * ASSERT FilterVars size = number of Vals.
        */
        template <std::size_t Size, typename... Types>
        void deleteD(const Table<Size>& Tbl, std::initializer_list<std::size_t> FilterVars, const std::tuple<Types...>& FilterVals) const;

        /**
        * @brief Delete all values from Tbl.
        */
        template <std::size_t Size>
        void deleteD(const Table<Size>& Tbl) const;
    private:
        sqlite3* m_File = nullptr;
        std::filesystem::path m_FilePath;
    };

    template <std::size_t Size, typename... Types>
    void Database::insert(const Table<Size>& Tbl, std::initializer_list<std::size_t> InsertVars, const std::tuple<Types...>& Vals) const {
        GRAPE_ASSERT(InsertVars.size() == 0 ? Size == sizeof...(Types) : InsertVars.size() == sizeof...(Types));

        Statement stmt(*this, Tbl.queryInsert(InsertVars));
        stmt.bindValues(Vals);
        stmt.step();
    }

    template <std::size_t Size, typename... SetTypes, typename... FilterTypes>
    void Database::update(const Table<Size>& Tbl, std::initializer_list<std::size_t> SetVars, const std::tuple<SetTypes...>& Vals, std::initializer_list<std::size_t> FilterVars, const std::tuple<FilterTypes...>& FilterVals) const {
        GRAPE_ASSERT(SetVars.size() == sizeof...(SetTypes));
        GRAPE_ASSERT(FilterVars.size() == sizeof...(FilterTypes));

        Statement stmt(*this, Tbl.queryUpdate(SetVars, FilterVars));
        stmt.bindValues(Vals);
        stmt.bindValues<sizeof...(SetTypes)>(FilterVals);
        stmt.step();
    }

    template <std::size_t Size, typename... SetTypes, typename... FilterTypes>
    void Database::update(const Table<Size>& Tbl, const std::tuple<SetTypes...>& Vals, std::initializer_list<std::size_t> FilterVars, const std::tuple<FilterTypes...>& FilterVals) const {
        GRAPE_ASSERT(Size == sizeof...(SetTypes));
        GRAPE_ASSERT(FilterVars.size() == sizeof...(FilterTypes));

        Statement stmt(*this, Tbl.queryUpdate({}, FilterVars));
        stmt.bindValues(Vals);
        stmt.bindValues<Size>(FilterVals);
        stmt.step();
    }

    template <std::size_t Size, typename... Types>
    void Database::deleteD(const Table<Size>& Tbl, std::initializer_list<std::size_t> FilterVars, const std::tuple<Types...>& FilterVals) const {
        GRAPE_ASSERT(FilterVars.size() == sizeof...(Types));

        Statement stmt(*this, Tbl.queryDelete(FilterVars));
        stmt.bindValues(FilterVals);
        stmt.step();
    }

    template <std::size_t Size>
    void Database::deleteD(const Table<Size>& Tbl) const {
        Statement stmt(*this, Tbl.queryDelete());
        stmt.step();
    }
}
