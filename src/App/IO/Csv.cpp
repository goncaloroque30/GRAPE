// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "Csv.h"

namespace GRAPE::IO {
    namespace {
        constexpr std::array<char, 3> s_Separators{ ',', ';', '\t' };
    }

    void Csv::setImport(const std::string& CsvFile, std::size_t MinColumnCount) {
        m_FilePath = CsvFile;

        if (!is_regular_file(m_FilePath))
            throw GrapeException("Invalid .csv file.");

        std::ifstream stream;
        stream.open(m_FilePath, std::ios::binary | std::ios::in);
        if (!stream)
            throw GrapeException("Failed to read from the file.");

        m_Csv = rapidcsv::Document(m_FilePath.string(), rapidcsv::LabelParams(), rapidcsv::SeparatorParams(separator(stream), false, rapidcsv::sPlatformHasCR, true, true));

        if (m_Csv.GetColumnCount() < MinColumnCount)
            throw GrapeException(std::format("The file must have at least {} columns.", MinColumnCount));
    }

    void Csv::setExport(const std::string& CsvFile) {
        m_FilePath = CsvFile;
        rapidcsv::SeparatorParams sepParams;
        sepParams.mAutoQuote = false;

        // Passing the file path here would attempt to read the csv file
        m_Csv = rapidcsv::Document(std::string(), rapidcsv::LabelParams(), sepParams);

        if (exists(m_FilePath))
        {
            std::ofstream stream;
            stream.open(m_FilePath, std::ios::binary | std::ios::trunc);
            if (!stream)
                throw GrapeException("Failed to write to the file.");

            Log::io()->warn("Exporting to csv file '{}'. File already exists and will be overwritten.", m_FilePath.string());
        }
    }

    char Csv::separator(std::ifstream& stream) {
        GRAPE_ASSERT(stream);

        // Detect separator by reading first 100 lines into a string and checking which supported separator occurs the most
        std::string buffer;
        std::string lineBuffer;
        std::size_t rowCount = 0;
        while (std::getline(stream, lineBuffer) || !(rowCount < 100))
        {
            buffer.append(lineBuffer);
            ++rowCount;

            if (stream.eof())
                break;
        }

        char sep = s_Separators.at(0);
        std::string::difference_type count = 0;
        for (const auto& c : s_Separators)
        {
            const auto cCount = std::ranges::count(buffer, c);
            if (cCount > count)
            {
                count = cCount;
                sep = c;
            }
        }

        return sep;
    }

    void Csv::write() {
        try { m_Csv.Save(m_FilePath.string()); }
        catch (...) { Log::io()->error("Exporting to '{}'. Failed to write to the file.", m_FilePath.string()); }
    }
}
