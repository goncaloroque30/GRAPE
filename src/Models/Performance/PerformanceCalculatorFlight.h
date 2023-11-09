// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "PerformanceCalculator.h"

#include "Airport/RouteOutput.h"
#include "Operation/Flight.h"

namespace GRAPE {
    /**
    * @brief Base class for calculating the PerformanceOutput for Flight Operation.
    */
    class PerformanceCalculatorFlight : public PerformanceCalculator {
    public:
        explicit PerformanceCalculatorFlight(const PerformanceSpecification& Spec) : PerformanceCalculator(Spec) {}
        virtual ~PerformanceCalculatorFlight() = default;

        /**
        * Default implementation returns empty output.
        */
        [[nodiscard]] virtual std::optional<PerformanceOutput> calculate(const FlightArrival&, const RouteOutput&) const { return PerformanceOutput(); };

        /**
        * Default implementation returns empty output.
        */
        [[nodiscard]] virtual std::optional<PerformanceOutput> calculate(const FlightDeparture&, const RouteOutput&) const { return PerformanceOutput(); };
    };
}
