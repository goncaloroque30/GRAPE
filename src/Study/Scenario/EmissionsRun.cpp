// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "EmissionsRun.h"

#include "Scenario.h"

#include "Aircraft/Aircraft.h"

namespace GRAPE {
    EmissionsRun::EmissionsRun(PerformanceRun& PerfRun, std::string_view NameIn) : Name(NameIn), m_PerformanceRun(PerfRun) {}

    // Definitions in .cpp due to cyclic reference
    PerformanceRun& EmissionsRun::parentPerformanceRun() const {
        return m_PerformanceRun;
    }

    Scenario& EmissionsRun::parentScenario() const {
        return parentPerformanceRun().parentScenario();
    }

    bool EmissionsRun::valid() const {
        bool valid = true;

        if (EmissionsRunSpec.EmissionsMdl == EmissionsModel::BFFM2)
        {
            const std::function log = [&](const std::string& Err) {
                Log::dataLogic()->error("Running emissions run '{}' of performance run '{}' of scenario '{}' with Boeing Fuel Flow Method 2. {}", Name, parentPerformanceRun().Name, parentScenario().Name, Err);
                };

            for (auto flight : parentScenario().FlightArrivals)
            {
                const auto& acft = flight.get().aircraft();
                if (!acft.validLTOEngine())
                {
                    log(std::format("Arrival flight '{}' with aircraft '{}' has no LTO engine selected.", flight.get().Name, acft.Name));
                    valid = false;
                }
            }

            for (auto flight : parentScenario().FlightDepartures)
            {
                const auto& acft = flight.get().aircraft();
                if (!acft.validLTOEngine())
                {
                    log(std::format("Departure flight '{}' with aircraft '{}' has no LTO engine selected.", flight.get().Name, acft.Name));
                    valid = false;
                }
            }

            for (auto track4d : parentScenario().Track4dArrivals)
            {
                const auto& acft = track4d.get().aircraft();
                if (!acft.validLTOEngine())
                {
                    log(std::format("Arrival track 4D '{}' with aircraft '{}' has no LTO engine selected.", track4d.get().Name, acft.Name));
                    valid = false;
                }
            }

            for (auto track4d : parentScenario().Track4dDepartures)
            {
                const auto& acft = track4d.get().aircraft();
                if (!acft.validLTOEngine())
                {
                    log(std::format("Departure track 4D '{}' with aircraft '{}' has no LTO engine selected.", track4d.get().Name, acft.Name));
                    valid = false;
                }
            }
        }
        return valid;
    }

    const std::shared_ptr<EmissionsRunJob>& EmissionsRun::createJob(const Database& Db, Constraints& Blocks) {
        m_EmissionsRunOutput = std::make_unique<EmissionsRunOutput>(*this, Db);

        std::size_t threadCount = static_cast<std::size_t>(std::thread::hardware_concurrency());
        m_Job = std::make_shared<EmissionsRunJob>(Blocks, *this, threadCount);

        return m_Job;
    }
}
