// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "Scenario.h"

namespace GRAPE {
    std::pair<std::chrono::tai_seconds, std::chrono::tai_seconds> Scenario::timeSpan() const {
        std::chrono::tai_seconds min = std::chrono::tai_seconds::max();
        std::chrono::tai_seconds max = std::chrono::tai_seconds::min();

        for (const auto& opRef : FlightArrivals)
        {
            const auto& op = opRef.get();
            if (op.Time < min)
                min = op.Time;

            if (op.Time > max)
                max = op.Time;
        }

        for (const auto& opRef : FlightDepartures)
        {
            const auto& op = opRef.get();
            if (op.Time < min)
                min = op.Time;

            if (op.Time > max)
                max = op.Time;
        }

        for (const auto& opRef : Track4dArrivals)
        {
            const auto& op = opRef.get();
            if (op.Time < min)
                min = op.Time;

            if (op.Time > max)
                max = op.Time;
        }

        for (const auto& opRef : Track4dDepartures)
        {
            const auto& op = opRef.get();
            if (op.Time < min)
                min = op.Time;

            if (op.Time > max)
                max = op.Time;
        }

        return { min, max };
    }
}
