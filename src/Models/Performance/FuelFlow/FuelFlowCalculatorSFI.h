// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "FuelFlowCalculator.h"

namespace GRAPE {
    /**
    * @brief Implements the SFI fuel flow calculator.
    */
    class FuelFlowCalculatorSFI : public FuelFlowCalculator {
    public:
        explicit FuelFlowCalculatorSFI(const PerformanceSpecification& PerfSpec) : FuelFlowCalculator(PerfSpec) {}

        /**
        * @brief Calls the SFI fuel flow calculator for each point in Perf.
        */
        void calculate(const OperationArrival& Op, PerformanceOutput& Perf) const override;

        /**
        * @brief Calls the SFI fuel flow calculator for each point in Perf.
        */
        void calculate(const OperationDeparture& Op, PerformanceOutput& Perf) const override;
    };
}
