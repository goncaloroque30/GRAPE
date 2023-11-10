// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

namespace GRAPE {
    class PerformanceOutput;
    class PerformanceRunOutput;

    class ReceptorOutput;
    class NoiseSingleEventOutput;
    class NoiseCumulativeMetric;
    struct NoiseCumulativeOutput;

    class EmissionsRunOutput;
    class EmissionsOperationOutput;

    namespace IO::CSV {
        void exportDoc29Performance(const std::string& CsvPath);
        void exportDoc29PerformanceAerodynamicCoefficients(const std::string& CsvPath);
        void exportDoc29PerformanceThrustRatings(const std::string& CsvPath);
        void exportDoc29PerformanceThrustRatingsPropeller(const std::string& CsvPath);
        void exportDoc29PerformanceProfilesPoints(const std::string& CsvPath);
        void exportDoc29PerformanceProfilesArrivalSteps(const std::string& CsvPath);
        void exportDoc29PerformanceProfilesDepartureSteps(const std::string& CsvPath);

        void exportDoc29Noise(const std::string& CsvPath);
        void exportDoc29NoiseNpd(const std::string& CsvPath);
        void exportDoc29NoiseSpectrum(const std::string& CsvPath);

        void exportLTO(const std::string& CsvPath);

        void exportSFI(const std::string& CsvPath);

        void exportFleet(const std::string& CsvPath);

        void exportAirports(const std::string& CsvPath);
        void exportRunways(const std::string& CsvPath);
        void exportRoutesSimple(const std::string& CsvPath);
        void exportRoutesVectors(const std::string& CsvPath);
        void exportRoutesRnp(const std::string& CsvPath);

        void exportFlights(const std::string& CsvPath);
        void exportTracks4d(const std::string& CsvPath);
        void exportTracks4dPoints(const std::string& CsvPath);

        void exportScenarios(const std::string& CsvPath);
        void exportScenariosOperations(const std::string& CsvPath);

        void exportPerformanceRuns(const std::string& CsvPath);
        void exportPerformanceRunsAtmospheres(const std::string& CsvPath);

        void exportNoiseRuns(const std::string& CsvPath);
        void exportNoiseRunsReceptorsPoints(const std::string& CsvPath);
        void exportNoiseRunsReceptorsGrids(const std::string& CsvPath);
        void exportNoiseRunsCumulativeMetrics(const std::string& CsvPath);
        void exportNoiseRunsCumulativeMetricsWeights(const std::string& CsvPath);

        void exportEmissionsRuns(const std::string& CsvPath);

        void exportDoc29Files(const std::string& FolderPath);
        void exportDatasetFiles(const std::string& FolderPath);
        void exportInputDataFiles(const std::string& FolderPath);
        void exportAllFiles(const std::string& FolderPath);

        void exportPerformanceOutput(const PerformanceOutput& PerfOut, const std::string& CsvPath);
        void exportPerformanceRunOutput(const PerformanceRunOutput& PerfRunOut, const std::string& CsvPath);

        void exportNoiseSingleEventOutput(const NoiseSingleEventOutput& NsSingleEventOutput, const ReceptorOutput& ReceptOut, const std::string& CsvPath);
        void exportNoiseCumulativeMetricOutput(const NoiseCumulativeMetric& NsCumMetric, const NoiseCumulativeOutput& NsCumMetricOut, const ReceptorOutput& ReceptOut, const std::string& CsvPath);

        void exportEmissionsSegmentOutput(const EmissionsOperationOutput& EmiOpOut, const std::string& CsvPath);
        void exportEmissionsRunOutput(const EmissionsRunOutput& FlEmiRunOutput, const std::string& CsvPath);
    }
}
