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

        const std::function log = [&](const std::string& Err) {
            Log::dataLogic()->error("Running emissions run '{}' of performance run '{}' of scenario '{}'. {}", Name, parentPerformanceRun().Name, parentScenario().Name, Err);
            };

        // Performance output needed for all models except LTO Cycle
        if (EmissionsRunSpec.EmissionsMdl != EmissionsModel::LTOCycle)
        {
            if (parentScenario().flightsSize() != 0 && parentPerformanceRun().PerfRunSpec.FlightsPerformanceMdl == PerformanceModel::None)
            {
                log(std::format("Emissions model '{}' can't be applied for Flights performance model '{}'.", EmissionsModelTypes.toString(EmissionsRunSpec.EmissionsMdl), PerformanceModelTypes.toString(PerformanceModel::None)));
                valid = false;
            }

            if (parentScenario().tracks4dSize() != 0 && !parentPerformanceRun().PerfRunSpec.Tracks4dCalculatePerformance)
            {
                log(std::format("Emissions model '{}' requires Tracks 4D to be run.", EmissionsModelTypes.toString(EmissionsRunSpec.EmissionsMdl)));
                valid = false;
            }
        }

        // All emissions models require LTO Engine
        // For calculating particle emissions, either Smoke Number or nvPM EI must be provided
        for (auto flight : parentScenario().FlightArrivals)
        {
            const auto& acft = flight.get().aircraft();
            if (!acft.validLTOEngine())
            {
                log(std::format("Arrival flight '{}' with aircraft '{}' has no LTO engine selected.", flight.get().Name, acft.Name));
                valid = false;
            }
            else if (EmissionsRunSpec.CalculateParticleEmissions)
            {
                const auto& lto = *acft.LTOEng;
                if (EmissionsRunSpec.ParticleSmokeNumberModel != EmissionsParticleSmokeNumberModel::None)
                {
                    for (std::size_t i = 0; i < LTOPhases.size(); ++i)
                    {
                        if (std::isnan(lto.SmokeNumbers.at(i)) && std::isnan(lto.EmissionIndexesNVPM.at(i)))
                        {
                            log(std::format("LTO engine '{}' in aircraft '{}' for arrival flight '{}' is missing both smoke number and nvPM mass EI for LTO phase '{}'.", lto.Name, acft.Name, flight.get().Name, LTOPhases.toString(magic_enum::enum_value<LTOPhase>(i))));
                            valid = false;
                        }

                        if (std::isnan(lto.SmokeNumbers.at(i)) && std::isnan(lto.EmissionIndexesNVPMNumber.at(i)))
                        {
                            log(std::format("LTO engine '{}' in aircraft '{}' for arrival flight '{}' is missing both smoke number and nvPM number EI for LTO phase '{}'.", lto.Name, acft.Name, flight.get().Name, LTOPhases.toString(magic_enum::enum_value<LTOPhase>(i))));
                            valid = false;
                        }
                    }
                }
                else
                {
                    for (std::size_t i = 0; i < LTOPhases.size(); ++i)
                    {
                        if (std::isnan(lto.EmissionIndexesNVPM.at(i)))
                        {
                            log(std::format("LTO engine '{}' in aircraft '{}' for arrival flight '{}' is missing nvPM mass EI for LTO phase '{}'.", lto.Name, acft.Name, flight.get().Name, LTOPhases.toString(magic_enum::enum_value<LTOPhase>(i))));
                            valid = false;
                        }

                        if (std::isnan(lto.EmissionIndexesNVPMNumber.at(i)))
                        {
                            log(std::format("LTO engine '{}' in aircraft '{}' for arrival flight '{}' is missing nvPM number EI for LTO phase '{}'.", lto.Name, acft.Name, flight.get().Name, LTOPhases.toString(magic_enum::enum_value<LTOPhase>(i))));
                            valid = false;
                        }
                    }
                }
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
            else if (EmissionsRunSpec.CalculateParticleEmissions)
            {
                const auto& lto = *acft.LTOEng;
                if (EmissionsRunSpec.ParticleSmokeNumberModel != EmissionsParticleSmokeNumberModel::None)
                {
                    for (std::size_t i = 0; i < LTOPhases.size(); ++i)
                    {
                        if (std::isnan(lto.SmokeNumbers.at(i)) && std::isnan(lto.EmissionIndexesNVPM.at(i)))
                        {
                            log(std::format("LTO engine '{}' in aircraft '{}' for departure flight '{}' is missing both smoke number and nvPM mass EI for LTO phase '{}'.", lto.Name, acft.Name, flight.get().Name, LTOPhases.toString(magic_enum::enum_value<LTOPhase>(i))));
                            valid = false;
                        }

                        if (std::isnan(lto.SmokeNumbers.at(i)) && std::isnan(lto.EmissionIndexesNVPMNumber.at(i)))
                        {
                            log(std::format("LTO engine '{}' in aircraft '{}' for departure flight '{}' is missing both smoke number and nvPM number EI for LTO phase '{}'.", lto.Name, acft.Name, flight.get().Name, LTOPhases.toString(magic_enum::enum_value<LTOPhase>(i))));
                            valid = false;
                        }
                    }
                }
                else
                {
                    for (std::size_t i = 0; i < LTOPhases.size(); ++i)
                    {
                        if (std::isnan(lto.EmissionIndexesNVPM.at(i)))
                        {
                            log(std::format("LTO engine '{}' in aircraft '{}' for departure flight '{}' is missing nvPM mass EI for LTO phase '{}'.", lto.Name, acft.Name, flight.get().Name, LTOPhases.toString(magic_enum::enum_value<LTOPhase>(i))));
                            valid = false;
                        }

                        if (std::isnan(lto.EmissionIndexesNVPMNumber.at(i)))
                        {
                            log(std::format("LTO engine '{}' in aircraft '{}' for departure flight '{}' is missing nvPM number EI for LTO phase '{}'.", lto.Name, acft.Name, flight.get().Name, LTOPhases.toString(magic_enum::enum_value<LTOPhase>(i))));
                            valid = false;
                        }
                    }
                }
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
            else if (EmissionsRunSpec.CalculateParticleEmissions)
            {
                const auto& lto = *acft.LTOEng;
                if (EmissionsRunSpec.ParticleSmokeNumberModel != EmissionsParticleSmokeNumberModel::None)
                {
                    for (std::size_t i = 0; i < LTOPhases.size(); ++i)
                    {
                        if (std::isnan(lto.SmokeNumbers.at(i)) && std::isnan(lto.EmissionIndexesNVPM.at(i)))
                        {
                            log(std::format("LTO engine '{}' in aircraft '{}' for arrival track 4D '{}' is missing both smoke number and nvPM mass EI for LTO phase '{}'.", lto.Name, acft.Name, track4d.get().Name, LTOPhases.toString(magic_enum::enum_value<LTOPhase>(i))));
                            valid = false;
                        }

                        if (std::isnan(lto.SmokeNumbers.at(i)) && std::isnan(lto.EmissionIndexesNVPMNumber.at(i)))
                        {
                            log(std::format("LTO engine '{}' in aircraft '{}' for arrival track 4D '{}' is missing both smoke number and nvPM number EI for LTO phase '{}'.", lto.Name, acft.Name, track4d.get().Name, LTOPhases.toString(magic_enum::enum_value<LTOPhase>(i))));
                            valid = false;
                        }
                    }
                }
                else
                {
                    for (std::size_t i = 0; i < LTOPhases.size(); ++i)
                    {
                        if (std::isnan(lto.EmissionIndexesNVPM.at(i)))
                        {
                            log(std::format("LTO engine '{}' in aircraft '{}' for arrival track 4D '{}' is missing nvPM mass EI for LTO phase '{}'.", lto.Name, acft.Name, track4d.get().Name, LTOPhases.toString(magic_enum::enum_value<LTOPhase>(i))));
                            valid = false;
                        }

                        if (std::isnan(lto.EmissionIndexesNVPMNumber.at(i)))
                        {
                            log(std::format("LTO engine '{}' in aircraft '{}' for arrival track 4D '{}' is missing nvPM number EI for LTO phase '{}'.", lto.Name, acft.Name, track4d.get().Name, LTOPhases.toString(magic_enum::enum_value<LTOPhase>(i))));
                            valid = false;
                        }
                    }
                }
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
            else if (EmissionsRunSpec.CalculateParticleEmissions)
            {
                const auto& lto = *acft.LTOEng;
                if (EmissionsRunSpec.ParticleSmokeNumberModel != EmissionsParticleSmokeNumberModel::None)
                {
                    for (std::size_t i = 0; i < LTOPhases.size(); ++i)
                    {
                        if (std::isnan(lto.SmokeNumbers.at(i)) && std::isnan(lto.EmissionIndexesNVPM.at(i)))
                        {
                            log(std::format("LTO engine '{}' in aircraft '{}' for departure track 4D '{}' is missing both smoke number and nvPM mass EI for LTO phase '{}'.", lto.Name, acft.Name, track4d.get().Name, LTOPhases.toString(magic_enum::enum_value<LTOPhase>(i))));
                            valid = false;
                        }

                        if (std::isnan(lto.SmokeNumbers.at(i)) && std::isnan(lto.EmissionIndexesNVPMNumber.at(i)))
                        {
                            log(std::format("LTO engine '{}' in aircraft '{}' for departure track 4D '{}' is missing both smoke number and nvPM number EI for LTO phase '{}'.", lto.Name, acft.Name, track4d.get().Name, LTOPhases.toString(magic_enum::enum_value<LTOPhase>(i))));
                            valid = false;
                        }
                    }
                }
                else
                {
                    for (std::size_t i = 0; i < LTOPhases.size(); ++i)
                    {
                        if (std::isnan(lto.EmissionIndexesNVPM.at(i)))
                        {
                            log(std::format("LTO engine '{}' in aircraft '{}' for departure track 4D '{}' is missing nvPM mass EI for LTO phase '{}'.", lto.Name, acft.Name, track4d.get().Name, LTOPhases.toString(magic_enum::enum_value<LTOPhase>(i))));
                            valid = false;
                        }

                        if (std::isnan(lto.EmissionIndexesNVPMNumber.at(i)))
                        {
                            log(std::format("LTO engine '{}' in aircraft '{}' for departure track 4D '{}' is missing nvPM number EI for LTO phase '{}'.", lto.Name, acft.Name, track4d.get().Name, LTOPhases.toString(magic_enum::enum_value<LTOPhase>(i))));
                            valid = false;
                        }
                    }
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
