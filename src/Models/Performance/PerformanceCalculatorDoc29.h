// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Performance/PerformanceCalculatorFlight.h"

namespace GRAPE {
    /**
    * @brief Calculates the PerformanceOutput of arrival and departure flights with the Doc29 performance model.
    */
    class PerformanceCalculatorDoc29 : public PerformanceCalculatorFlight {
    public:
        explicit PerformanceCalculatorDoc29(const PerformanceSpecification& Spec);

        [[nodiscard]] std::optional<PerformanceOutput> calculate(const FlightArrival& FlightArr, const RouteOutput& RteOutput) const override;
        [[nodiscard]] std::optional<PerformanceOutput> calculate(const FlightDeparture& FlightDep, const RouteOutput& RteOutput) const override;
    };
}
