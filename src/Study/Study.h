// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Constraints.h"

#include "Database/Database.h"
#include "Jobs/JobManager.h"
#include "Managers/AircraftsManager.h"
#include "Managers/AirportsManager.h"
#include "Managers/Doc29PerformanceManager.h"
#include "Managers/Doc29NoiseManager.h"
#include "Managers/LTOManager.h"
#include "Managers/OperationsManager.h"
#include "Managers/ScenariosManager.h"
#include "Managers/SFIManager.h"

namespace GRAPE {
    class Study {
    public:
        // Constructors & Destructor
        Study();
        Study(const Study&) = delete;
        Study(Study&&) = delete;
        Study& operator=(const Study&) = delete;
        Study& operator=(Study&&) = delete;
        ~Study();

        // Access Data
        [[nodiscard]] std::string name() const { return m_Database.name(); } // stem from path
        [[nodiscard]] Database& db() { return m_Database; }

        // Change Data
        bool open(const std::filesystem::path& Path);
        bool create(const std::filesystem::path& Path);
        void close();

        // Status checks
        [[nodiscard]] bool valid() const { return m_Database.valid(); }
    private:
        // Database Connection
        Database m_Database;
    public:
        //Managers
        AirportsManager Airports;
        Doc29PerformanceManager Doc29Aircrafts;
        Doc29NoiseManager Doc29Noises;
        SFIManager SFIs;
        LTOManager LTOEngines;
        AircraftsManager Aircrafts;
        OperationsManager Operations;

        JobManager Jobs;

        ScenariosManager Scenarios;

        // Constraints
        Constraints Blocks;
    private:
        void elevate(int CurrentVersion);
        void loadFile();
    };
}
