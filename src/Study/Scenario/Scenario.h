// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "EmissionsRun.h"
#include "NoiseRun.h"
#include "PerformanceRun.h"

#include "Jobs/EmissionsRunJob.h"
#include "Jobs/PerformanceRunJob.h"
#include "Jobs/NoiseRunJob.h"

#include "Operation/Operations.h"

namespace GRAPE {
    class Scenario {
    public:
        explicit Scenario(std::string_view NameIn) : Name(NameIn) {}

        // Editable Data
        std::string Name;

        std::vector<std::reference_wrapper<const FlightArrival>> FlightArrivals{};
        std::vector<std::reference_wrapper<const FlightDeparture>> FlightDepartures{};
        std::vector<std::reference_wrapper<const Track4dArrival>> Track4dArrivals{};
        std::vector<std::reference_wrapper<const Track4dDeparture>> Track4dDepartures{};

        GrapeMap<std::string, PerformanceRun> PerformanceRuns; // Key is PerformanceRun Name

        // Status checks
        [[nodiscard]] std::size_t size() const { return arrivalsSize() + departuresSize(); }
        [[nodiscard]] std::size_t arrivalsSize() const { return FlightArrivals.size() + Track4dArrivals.size(); }
        [[nodiscard]] std::size_t departuresSize() const { return FlightDepartures.size() + Track4dDepartures.size(); }
        [[nodiscard]] std::size_t flightsSize() const { return FlightArrivals.size() + FlightDepartures.size(); }
        [[nodiscard]] std::size_t tracks4dSize() const { return Track4dArrivals.size() + Track4dDepartures.size(); }

        [[nodiscard]] std::pair<std::chrono::tai_seconds, std::chrono::tai_seconds> timeSpan() const;

        [[nodiscard]] bool empty() const { return FlightArrivals.empty() && FlightDepartures.empty() && Track4dArrivals.empty() && Track4dDepartures.empty(); }

        [[nodiscard]] bool contains(const FlightArrival& Op) const { return std::ranges::find_if(FlightArrivals, [&](const FlightArrival& ScenOp) { return &ScenOp == &Op; }) != FlightArrivals.end(); }
        [[nodiscard]] bool contains(const FlightDeparture& Op) const { return std::ranges::find_if(FlightDepartures, [&](const FlightDeparture& ScenOp) { return &ScenOp == &Op; }) != FlightDepartures.end(); }
        [[nodiscard]] bool contains(const Track4dArrival& Op) const { return std::ranges::find_if(Track4dArrivals, [&](const Track4dArrival& ScenOp) { return &ScenOp == &Op; }) != Track4dArrivals.end(); }
        [[nodiscard]] bool contains(const Track4dDeparture& Op) const { return std::ranges::find_if(Track4dDepartures, [&](const Track4dDeparture& ScenOp) { return &ScenOp == &Op; }) != Track4dDepartures.end(); }
    };
}
