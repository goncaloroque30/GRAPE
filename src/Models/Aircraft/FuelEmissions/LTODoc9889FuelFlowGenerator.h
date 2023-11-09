// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "LTO.h"

namespace GRAPE {
    /**
    * @brief Calculates fuel flow based on the LTO Phases with Doc9889 interpolation
    */
    class LTODoc9889FuelFlowGenerator {
    public:
        explicit LTODoc9889FuelFlowGenerator(const LTOEngine& LTOEng);

        /**
        * @brief Gets the LTOPhase from the FlightPhase and calculates fuel flow. Applies quadratic interpolation for thrust settings above 60%, as described in https://www.icao.int/publications/documents/9889_cons_en.pdf
        */
        [[nodiscard]] double fuelFlow(FlightPhase Phase, double ThrustPercentage) const;
    private:
        std::array<double, 4> m_CorrectedFuelFlows{};

        struct Quadratic {
            double A;
            double B;
            double C;
        };

        // Fuel Flows Quadratic Fit (60 - 85 - 100)
        std::array<Quadratic, 2> m_FuelFlowQuadratic{};
    };
}
