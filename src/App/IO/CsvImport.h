// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

namespace GRAPE::IO::CSV {
    void importDoc29Performance(const std::string& CsvPath);
    void importDoc29PerformanceAerodynamicCoefficients(const std::string& CsvPath);
    void importDoc29PerformanceThrustRatings(const std::string& CsvPath);
    void importDoc29PerformanceThrustRatingsPropeller(const std::string& CsvPath);
    void importDoc29PerformanceProfilesPoints(const std::string& CsvPath);
    void importDoc29PerformanceProfilesArrivalSteps(const std::string& CsvPath);
    void importDoc29PerformanceProfilesDepartureSteps(const std::string& CsvPath);

    void importDoc29Noise(const std::string& CsvPath);
    void importDoc29NoiseNpd(const std::string& CsvPath);
    void importDoc29NoiseSpectrum(const std::string& CsvPath);

    void importLTO(const std::string& CsvPath);

    void importSFI(const std::string& CsvPath);

    void importFleet(const std::string& CsvPath);

    void importAirports(const std::string& CsvPath);
    void importRunways(const std::string& CsvPath);
    void importRoutesSimple(const std::string& CsvPath);
    void importRoutesVectors(const std::string& CsvPath);
    void importRoutesRnp(const std::string& CsvPath);

    void importFlights(const std::string& CsvPath);
    void importTracks4d(const std::string& CsvPath);
    void importTracks4dPoints(const std::string& CsvPath);

    void importScenarios(const std::string& CsvPath);
    void importScenariosOperations(const std::string& CsvPath);

    void importPerformanceRuns(const std::string& CsvPath);
    void importPerformanceRunsAtmospheres(const std::string& CsvPath);

    void importNoiseRuns(const std::string& CsvPath);
    void importNoiseRunsReceptorsGrids(const std::string& CsvPath);
    void importNoiseRunsReceptorsPoints(const std::string& CsvPath);
    void importNoiseRunsCumulativeMetrics(const std::string& CsvPath);
    void importNoiseRunsCumulativeMetricsWeights(const std::string& CsvPath);

    void importEmissionsRuns(const std::string& CsvPath);

    void importDoc29Files(const std::string& FolderPath);
    void importDatasetFiles(const std::string& FolderPath);
    void importInputDataFiles(const std::string& FolderPath);
    void importAllFiles(const std::string& FolderPath);
}
