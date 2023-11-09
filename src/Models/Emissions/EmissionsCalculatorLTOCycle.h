// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "EmissionsCalculator.h"

namespace GRAPE {
    /**
    * @brief Calculates emissions according to the LTO Cycle
    */
    class EmissionsCalculatorLTOCycle : public EmissionsCalculator {
    public:
        EmissionsCalculatorLTOCycle(const PerformanceSpecification& PerfSpec, const EmissionsSpecification& EmissionsSpec);

        /**
        * @brief Implements the basic EI formula to obtain emissions. LTO phase times are set in the emissions specification. PerformanceOutput is ignored.
        */
        [[nodiscard]] EmissionsOperationOutput calculateEmissions(const Operation& Op, const PerformanceOutput& PerfOut) const override;
    };
}
