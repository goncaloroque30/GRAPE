// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "Elevator.h"

#include "Elevator11.h"

namespace GRAPE::Schema {
    Elevator::Elevator() {
        m_ElevatorQueries.try_emplace(11, ElevatorQueries{
                Elevator11::g_lto_fuel_emissions,
                Elevator11::g_operations_flights,
                Elevator11::g_performance_run,
                Elevator11::g_emissions_run,
                Elevator11::g_emissions_run_output,
                Elevator11::g_emissions_run_output_operations,
                Elevator11::g_emissions_run_output_segments,
            });
    }

    void Elevator::elevate(const Database& Db, int CurrentVersion) const {
        Log::study()->info("Updating GRAPE study from version {} to version {}.", CurrentVersion, GRAPE_VERSION_NUMBER);

        Db.execute("PRAGMA foreign_keys = OFF");

        const auto skip = std::distance(m_ElevatorQueries.begin(), m_ElevatorQueries.upper_bound(CurrentVersion));
        for (const auto& [version, queries] : m_ElevatorQueries | std::views::drop(skip))
        {
            for (const auto& query : queries)
                Db.execute(std::string(query));
            Db.execute(std::format("PRAGMA user_version={}", version));
        }

        Db.execute("PRAGMA foreign_keys = ON");
    }
}

