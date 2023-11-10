// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "PerformanceRun.h"

#include "Scenario.h"

#include "Aircraft/Aircraft.h"

namespace GRAPE {
    PerformanceRun::PerformanceRun(Scenario& Scen, std::string_view NameIn) : Name(NameIn), m_ParentScenario(Scen) {}

    bool PerformanceRun::valid() const {
        bool valid = true;

        if (parentScenario().empty())
        {
            Log::dataLogic()->error("Running performance run '{}' of scenario '{}'. No operations selected for this scenario.", Name, parentScenario().Name);
            valid = false;
        }

        if ((PerfRunSpec.FlightsPerformanceMdl != PerformanceModel::None || PerfRunSpec.Tracks4dCalculatePerformance) && PerfRunSpec.Atmospheres.empty())
        {
            Log::dataLogic()->error("Running performance run '{}' of scenario '{}'. At least one atmosphere must be provided.", Name, parentScenario().Name);
            valid = false;
        }

        for (auto op : parentScenario().FlightArrivals)
        {
            if (PerfRunSpec.FlightsPerformanceMdl == PerformanceModel::Doc29)
            {
                if (!op.get().hasRoute())
                {
                    Log::dataLogic()->error("Running performance run '{}' of scenario '{}' with Doc29 performance model. Arrival flight '{}' has no arrival route selected.", Name, parentScenario().Name, op.get().Name);
                    valid = false;
                }

                if (!op.get().aircraft().validDoc29Performance())
                {
                    Log::dataLogic()->error("Running performance run '{}' of scenario '{}' with Doc29 performance model. Arrival flight '{}' with aircraft '{}' has no Doc29 aircraft selected.", Name, parentScenario().Name, op.get().Name, op.get().aircraft().Name);
                    valid = false;
                }
                else if (!op.get().hasDoc29Profile())
                {
                    Log::dataLogic()->error("Running performance run '{}' of scenario '{}' with Doc29 performance model. Arrival flight '{}' with aircraft '{}' has no Doc29 profile selected.", Name, parentScenario().Name, op.get().Name, op.get().aircraft().Name);
                    valid = false;
                }
            }

            if (PerfRunSpec.FuelFlowMdl == FuelFlowModel::SFI && !op.get().aircraft().validSFI())
            {
                Log::dataLogic()->error("Running performance run '{}' of scenario '{}' with SFI fuel flow model. Arrival flight '{}' with aircraft '{}' has no SFI ID selected.", Name, parentScenario().Name, op.get().Name, op.get().aircraft().Name);
                valid = false;
            }
            else if (PerfRunSpec.FuelFlowMdl == FuelFlowModel::LTO && !op.get().aircraft().validLTOEngine())
            {
                Log::dataLogic()->error("Running performance run '{}' of scenario '{}' with LTO fuel flow model. Arrival flight '{}' with aircraft '{}' has no LTO Engine selected.", Name, parentScenario().Name, op.get().Name, op.get().aircraft().Name);
                valid = false;
            }
            else if (PerfRunSpec.FuelFlowMdl == FuelFlowModel::LTODoc9889 && !op.get().aircraft().validLTOEngine())
            {
                Log::dataLogic()->error("Running performance run '{}' of scenario '{}' with LTO Doc9889 fuel flow model. Arrival flight '{}' with aircraft '{}' has no LTO Engine selected.", Name, parentScenario().Name, op.get().Name, op.get().aircraft().Name);
                valid = false;
            }
        }

        for (auto op : parentScenario().FlightDepartures)
        {
            if (PerfRunSpec.FlightsPerformanceMdl == PerformanceModel::Doc29)
            {
                if (!op.get().hasRoute())
                {
                    Log::dataLogic()->error("Running performance run '{}' of scenario '{}' with Doc29 performance model. Departure flight '{}' has no departure route selected.", Name, parentScenario().Name, op.get().Name);
                    valid = false;
                }

                if (!op.get().aircraft().Doc29Acft)
                {
                    Log::dataLogic()->error("Running performance run '{}' of scenario '{}' with Doc29 performance model. Departure flight '{}' with aircraft '{}' has no Doc29 aircraft selected.", Name, parentScenario().Name, op.get().Name, op.get().aircraft().Name);
                    valid = false;
                }
                else if (!op.get().hasDoc29Profile())
                {
                    Log::dataLogic()->error("Running performance run '{}' of scenario '{}' with Doc29 performance model. Departure flight '{}' with aircraft '{}' has no Doc29 profile selected.", Name, parentScenario().Name, op.get().Name, op.get().aircraft().Name);
                    valid = false;
                }
            }

            if (PerfRunSpec.FuelFlowMdl == FuelFlowModel::SFI && !op.get().aircraft().validSFI())
            {
                Log::dataLogic()->error("Running performance run '{}' of scenario '{}' with SFI fuel flow model. Departure flight '{}' with aircraft '{}' has no SFI ID selected.", Name, parentScenario().Name, op.get().Name, op.get().aircraft().Name);
                valid = false;
            }
            else if (PerfRunSpec.FuelFlowMdl == FuelFlowModel::LTO && !op.get().aircraft().validLTOEngine())
            {
                Log::dataLogic()->error("Running performance run '{}' of scenario '{}' with LTO fuel flow model. Departure flight '{}' with aircraft '{}' has no LTO Engine selected.", Name, parentScenario().Name, op.get().Name, op.get().aircraft().Name);
                valid = false;
            }
            else if (PerfRunSpec.FuelFlowMdl == FuelFlowModel::LTODoc9889 && !op.get().aircraft().validLTOEngine())
            {
                Log::dataLogic()->error("Running performance run '{}' of scenario '{}' with LTO Doc9889 fuel flow model. Departure flight '{}' with aircraft '{}' has no LTO Engine selected.", Name, parentScenario().Name, op.get().Name, op.get().aircraft().Name);
                valid = false;
            }
        }

        for (auto op : parentScenario().Track4dArrivals)
        {
            if (PerfRunSpec.FuelFlowMdl == FuelFlowModel::SFI && !op.get().aircraft().validSFI())
            {
                Log::dataLogic()->error("Running performance run '{}' of scenario '{}' with SFI fuel flow model. Arrival track 4D '{}' with aircraft '{}' has no SFI ID selected.", Name, parentScenario().Name, op.get().Name, op.get().aircraft().Name);
                valid = false;
            }
            else if (PerfRunSpec.FuelFlowMdl == FuelFlowModel::LTO && !op.get().aircraft().validLTOEngine())
            {
                Log::dataLogic()->error("Running performance run '{}' of scenario '{}' with LTO fuel flow model. Arrival track 4D '{}' with aircraft '{}' has no LTO Engine selected.", Name, parentScenario().Name, op.get().Name, op.get().aircraft().Name);
                valid = false;
            }
            else if (PerfRunSpec.FuelFlowMdl == FuelFlowModel::LTODoc9889 && !op.get().aircraft().validLTOEngine())
            {
                Log::dataLogic()->error("Running performance run '{}' of scenario '{}' with LTO Doc9889 fuel flow model. Arrival track 4D '{}' with aircraft '{}' has no LTO Engine selected.", Name, parentScenario().Name, op.get().Name, op.get().aircraft().Name);
                valid = false;
            }
        }

        for (auto op : parentScenario().Track4dDepartures)
        {
            if (PerfRunSpec.FuelFlowMdl == FuelFlowModel::SFI && !op.get().aircraft().validSFI())
            {
                Log::dataLogic()->error("Running performance run '{}' of scenario '{}' with SFI fuel flow model. Departure track 4D '{}' with aircraft '{}' has no SFI ID selected.", Name, parentScenario().Name, op.get().Name, op.get().aircraft().Name);
                valid = false;
            }
            else if (PerfRunSpec.FuelFlowMdl == FuelFlowModel::LTO && !op.get().aircraft().validLTOEngine())
            {
                Log::dataLogic()->error("Running performance run '{}' of scenario '{}' with LTO fuel flow model. Departure track 4D '{}' with aircraft '{}' has no LTO Engine selected.", Name, parentScenario().Name, op.get().Name, op.get().aircraft().Name);
                valid = false;
            }
            else if (PerfRunSpec.FuelFlowMdl == FuelFlowModel::LTODoc9889 && !op.get().aircraft().validLTOEngine())
            {
                Log::dataLogic()->error("Running performance run '{}' of scenario '{}' with LTO Doc9889 fuel flow model. Departure track 4D '{}' with aircraft '{}' has no LTO Engine selected.", Name, parentScenario().Name, op.get().Name, op.get().aircraft().Name);
                valid = false;
            }
        }

        return valid;
    }

    Scenario& PerformanceRun::parentScenario() const { return m_ParentScenario; }

    const std::shared_ptr<PerformanceRunJob>& PerformanceRun::createJob(const Database& Db, OperationsManager& Ops) {
        m_PerfRunOutput = std::make_unique<PerformanceRunOutput>(*this, Db);

        std::size_t threadCount = static_cast<std::size_t>(std::thread::hardware_concurrency());
        m_Job = std::make_shared<PerformanceRunJob>(Ops, *this, threadCount);

        return m_Job;
    }
}
