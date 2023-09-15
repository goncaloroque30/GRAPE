// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "FuelFlowCalculator.h"

#include "Aircraft/FuelEmissions/LTOFuelFlowGenerator.h"

namespace GRAPE {
    class FuelFlowCalculatorLTO : public FuelFlowCalculator {
    public:
        explicit FuelFlowCalculatorLTO(const PerformanceSpecification& PerfSpec) : FuelFlowCalculator(PerfSpec) {}

        /**
        * @brief Calls the LTO fuel flow calculator for each point in Perf.
        */
        virtual void calculate(const OperationArrival& Op, PerformanceOutput& Perf) const override;

        /**
        * @brief Calls the LTO fuel flow calculator for each point in Perf.
        */
        virtual void calculate(const OperationDeparture& Op, PerformanceOutput& Perf) const override;

        /**
        * @brief Adds LTOEng to the list of supported engines by this calculator. Fuel flows may only be calculated for LTOEngines which were added.
        */
        void addLTOEngine(const LTOEngine* LTOEng) override;
    protected:
        GrapeMap<const LTOEngine*, const LTOFuelFlowGenerator> m_FuelFlowGenerators;
    };

    class FuelFlowCalculatorLTODoc9889 : public FuelFlowCalculatorLTO {
    public:
        explicit FuelFlowCalculatorLTODoc9889(const PerformanceSpecification& PerfSpec) : FuelFlowCalculatorLTO(PerfSpec) {}

        /**
        * @brief Calls the LTO fuel flow calculator for each point in Perf.
        */
        void calculate(const OperationArrival& Op, PerformanceOutput& Perf) const override;

        /**
        * @brief Calls the LTO fuel flow calculator for each point in Perf.
        */
        void calculate(const OperationDeparture& Op, PerformanceOutput& Perf) const override;
    };
}
