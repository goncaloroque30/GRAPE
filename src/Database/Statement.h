// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Blob.h"

// Avoid including sqlite3.h in header file
struct sqlite3;
struct sqlite3_stmt;

namespace GRAPE {
    class Database;

    /**
    * @brief Represents a single SQLite statement to be executed with a Database connection.
    * The Database passed in the constructor must outlive the Statement instance.
    *
    * Call sequence:
    *	- [opt] bind()
    *	- step()
    *	- [opt] while(hasRow())
    *	- [opt]		getColumn()
    *	-           step()
    *	- reset()
    */
    class Statement {
        friend class Column; // Access to sqlite3_stmt*
    public:
        /**
        * @param Db The database connection in which the statement will be executed.
        * @param Query The SQL query to be executed.
        */
        Statement(const Database& Db, const std::string& Query);

        Statement(const Statement&) = delete;
        Statement(Statement&& Other) = delete;
        Statement& operator=(const Statement&) = delete;
        Statement& operator=(Statement&& Other) = delete;
        ~Statement();

        /**
        * @brief ASSERT that Index is smaller that sqlite3_column_count value after parsing string in the constructor.
        */
        void bind(int Index, std::monostate) const noexcept;

        /**
        * @brief ASSERT that Index is smaller that sqlite3_column_count value after parsing string in the constructor.
        */
        void bind(int Index, int Value) const noexcept;

        /**
        * @brief ASSERT that Index is smaller that sqlite3_column_count value after parsing string in the constructor.
        */
        void bind(int Index, double Value) const noexcept;

        /**
        * @brief ASSERT that Index is smaller that sqlite3_column_count value after parsing string in the constructor.
        */
        void bind(int Index, const char* Value) const noexcept;

        /**
        * @brief ASSERT that Index is smaller that sqlite3_column_count value after parsing string in the constructor.
        */
        void bind(int Index, const Blob& Value) const noexcept;

        /**
        * @brief ASSERT that Index is smaller that sqlite3_column_count value after parsing string in the constructor.
        */
        void bind(int Index, const std::string& Value) const noexcept;

        void step() noexcept;
        void reset() noexcept;

        /**
        * ASSERT that step() was called and returned SQLITE_ROW.
        * ASSERT that Index is smaller that sqlite3_column_count value after parsing string in the constructor.
        * @return Column value, implicitly convertible to basic types.
        */
        [[nodiscard]] Column getColumn(int Index) const noexcept;

        /**
        * @brief Check if the value returned at Index is null.
        * ASSERT that step() was called and returned SQLITE_ROW.
        * ASSERT that Index is smaller that sqlite3_column_count value after parsing string in the constructor.
        * @return True if NULL is stored at Index.
        */
        [[nodiscard]] bool isColumnNull(int Index) const noexcept;

        /**
        * @return True if a getColumn() can be called.
        */
        [[nodiscard]] bool hasRow() const noexcept { return m_HasRow; }

        /**
        * @return True if no more columns can be retrieved and step() returned SQLITE_DONE.
        */
        [[nodiscard]] bool done() const noexcept { return m_Done; }

        /**
        * @brief Binds tuple values in order starting at index 0.
        */
        template <typename... Types>
        void bindValues(const std::tuple<Types...>& Values) { bindValues(Values, std::make_index_sequence<sizeof...(Types)>()); }

        /**
        * @brief Binds tuple values in order starting at index Offset.
        */
        template <std::size_t Offset, typename... Types>
        void bindValues(const std::tuple<Types...>& Values) { bindValues<Offset>(Values, std::make_index_sequence<sizeof...(Types)>()); }

        /**
        * @brief Binds function arguments in order starting at index 0.
        */
        template <typename... Types>
        void bindValues(const Types&... Values) { bindValues(std::forward_as_tuple(Values...)); }

    private:
        sqlite3* m_Db;
        sqlite3_stmt* m_Stmt = nullptr;

        int m_ColumnCount = 0;

        bool m_HasRow = false;
        bool m_Done = false;

    private:
        /**
        * @brief Helper class called by bind(const std::tuple<Types...>& Values).
        */
        template <typename... Types, std::size_t ... Indexes>
        void bindValues(const std::tuple<Types...>& Values, std::index_sequence<Indexes...>) { (bind(static_cast<int>(Indexes), std::get<Indexes>(Values)), ...); }

        /**
        * @brief Helper class called by bind<Offset>(const std::tuple<Types...>& Values).
        */
        template <std::size_t Offset, typename... Types, std::size_t ... Indexes>
        void bindValues(const std::tuple<Types...>& Values, std::index_sequence<Indexes...>) { (bind(static_cast<int>(Indexes + Offset), std::get<Indexes>(Values)), ...); }
    };
}
