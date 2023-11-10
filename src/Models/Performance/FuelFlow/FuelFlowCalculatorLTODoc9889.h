// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "FuelFlowCalculator.h"

#include "Aircraft/FuelEmissions/LTODoc9889FuelFlowGenerator.h"

namespace GRAPE {
    class FuelFlowCalculatorLTODoc9889 : public FuelFlowCalculator {
    public:
        FuelFlowCalculatorLTODoc9889(const PerformanceSpecification& PerfSpec);

        /**
        * @brief Calls the LTO fuel flow calculator for each point in Perf.
        */
        void calculate(const OperationArrival& Op, PerformanceOutput& Perf) const override;

        /**
        * @brief Calls the LTO fuel flow calculator for each point in Perf.
        */
        void calculate(const OperationDeparture& Op, PerformanceOutput& Perf) const override;

        /**
        * @brief Adds LTOEng to the list of supported engines by this calculator. Fuel flows may only be calculated for LTOEngines which were added.
        */
        void addLTOEngine(const LTOEngine* LTOEng) override;
    private:
        GrapeMap<const LTOEngine*, const LTODoc9889FuelFlowGenerator> m_FuelFlowGenerators;
        std::function<double(double, double, double, const Atmosphere&)> m_AltitudeCorrection;
    };
}
