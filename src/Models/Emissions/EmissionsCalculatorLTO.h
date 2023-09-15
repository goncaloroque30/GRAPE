// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "EmissionsCalculator.h"

#include "Aircraft/FuelEmissions/LTOEmissionsGenerator.h"

namespace GRAPE {
    /**
    * @brief Implements the BFFM2 emissions calculator.
    */
    class LTOFuelEmissionsCalculator : public EmissionsCalculator {
    public:
        LTOFuelEmissionsCalculator(const PerformanceSpecification& PerfSpec, const EmissionsSpecification& EmissionsSpec);

        /**
        * @brief Iterates through the points in Perf and creates segments for every two point sequence. The segment parameters (altitude, speed, thrust, fuel flow, ...) are the midpoint values of the two points.
        */
        [[nodiscard]] EmissionsOperationOutput calculateEmissions(const Operation& Op, const PerformanceOutput& PerfOut) override;

        /**
        * @brief Base implementation. Subclasses may implement and use the data of LTOEngines as necessary.
        */
        void addLTOEngine(const LTOEngine* LTOEng) override;
    private:
        GrapeMap<const LTOEngine*, const LTOEmissionsGenerator> m_EmissionsGenerators;
    };
}
