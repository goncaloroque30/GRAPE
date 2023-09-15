// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

// Avoid including sqlite3.h in header file
struct sqlite3_stmt;

namespace GRAPE {
    /**
    * @brief Constructed by statement when a specific cell (row, column) is to be retrieved.
    */
    class Column {
        friend class Statement; // Access to constructor

        /**
        * @brief Stmt must be valid while an instance of this class exists.
        * @param Stmt The sqlite3_stmt* from Stmt will be copied.
        * @param Index The column to be retrieved from the current row.
        */
        Column(const Statement& Stmt, int Index);

    public:
        /**
        * @brief Call sqlite3_column_int, performs the sqlite conversions.
        */
        [[nodiscard]] int getInt() const noexcept;

        /**
        * @brief Call sqlite3_column_double, performs the sqlite conversions.
        */
        [[nodiscard]] double getDouble() const noexcept;

        /**
        * @brief Call sqlite3_column_text, and reinterprets the unsigned char* to const char*.
        * @return The string or an empty string if null.
        */
        [[nodiscard]] std::string getString() const noexcept;

        /**
        * @brief Enables implicit conversion to int.
        */
        operator int() const noexcept { return getInt(); }

        /**
        * @brief Enables implicit conversion to double.
        */
        operator double() const noexcept { return getDouble(); }

        /**
        * @brief Enables implicit conversion to std::string.
        */
        operator std::string() const noexcept { return getString(); }

    private:
        sqlite3_stmt* m_Stmt;
        int m_Index;
    };
}
