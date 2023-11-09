// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "EmissionsCalculator.h"

namespace GRAPE {
    /**
    * @brief Calculates Emissions for each segment by multiplying fuel flow with the EI in the LTO Engine
    */
    class EmissionsCalculatorLTO : public EmissionsCalculator {
    public:
        EmissionsCalculatorLTO(const PerformanceSpecification& PerfSpec, const EmissionsSpecification& EmissionsSpec);

        /**
        * @brief Iterates through the points in Perf and creates segments for every two point sequence. The segment parameters (altitude, speed, thrust, fuel flow, ...) are the midpoint values of the two points.
        */
        [[nodiscard]] EmissionsOperationOutput calculateEmissions(const Operation& Op, const PerformanceOutput& PerfOut) const override;
    };
}
