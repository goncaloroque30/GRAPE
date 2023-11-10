// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#pragma warning ( push )
#pragma warning ( disable : GRAPE_VENDOR_WARNINGS )
#include <rapidcsv.h>
#pragma warning ( pop )

#include <filesystem>

namespace GRAPE::IO {
    /**
    * @brief Wrapper around rapidcsv.
    */
    class Csv {
    public:
        Csv() = default;

        void setImport(const std::string& CsvFile, std::size_t MinColumnCount = 0);
        void setExport(const std::string& CsvFile);

        [[nodiscard]] std::size_t rowCount() const { return m_Csv.GetRowCount(); }
        [[nodiscard]] std::size_t columnCount() const { return m_Csv.GetColumnCount(); }

        [[nodiscard]] std::string columnName(std::size_t Column) const { return m_Csv.GetColumnName(Column); }
        [[nodiscard]] std::vector<std::string> columnNames() const { return m_Csv.GetColumnNames(); }

        template<typename T>
        [[nodiscard]] T getCell(std::size_t Row, std::size_t Column) const { return m_Csv.GetCell<T>(Column, Row); }

        template<typename... T>
        void setColumnNames(T... Names) {
            std::size_t i = 0;
            (m_Csv.SetColumnName(i++, Names), ...);
        }
        void setColumnName(std::size_t Column, const std::string& Name) { m_Csv.SetColumnName(Column, Name); }

        template<typename T>
        void setCell(std::size_t Row, std::size_t Column, const T& Val) { m_Csv.SetCell(Column, Row, Val); }

        void write();
    private:
        rapidcsv::Document m_Csv;
        std::filesystem::path m_FilePath;
    private:
        [[nodiscard]] static char separator(std::ifstream& stream);
    };

    template<>
    [[nodiscard]] inline std::string Csv::getCell(std::size_t Row, std::size_t Column) const {
        auto str = m_Csv.GetCell<std::string>(Column, Row);

        // Left trim
        auto start = str.find_first_not_of("\n\r\t\f\v ");
        str = (start == std::string::npos) ? "" : str.substr(start);

        // Right trim
        auto end = str.find_last_not_of("\n\r\t\f\v ");
        str = (end == std::string::npos) ? "" : str.substr(0, ++end);

        return str;
    }
}
