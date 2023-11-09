// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

namespace GRAPE {
    class PerformanceRun;
    class NoiseRun;

    namespace IO::GPKG {
        void exportAirports(const std::string& Path);
        void exportPerformanceRunOutput(const PerformanceRun& PerfRun, const std::string& Path);
        void exportNoiseRunOutput(const NoiseRun& NsRun, const std::string& Path);
    }
}
