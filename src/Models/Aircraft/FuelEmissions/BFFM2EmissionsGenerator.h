// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "LTO.h"

namespace GRAPE {
    class Atmosphere;

    /**
    * @brief Implements the Boeing Fuel Flow Method 2 to retrieve emission indexes for a certain aircraft state.
    *
    * The log relationships are created on construction.
    * See https://doi.org/10.4271/2006-01-1987 for the description of the emissions model.
    */
    class BFFM2EmissionsGenerator {
    public:
        explicit BFFM2EmissionsGenerator(const LTOEngine& LTOEng);

        /**
        * @brief Calculate the emissions indexes at altitude.
        * @return HCEI, COEI, NOxEI
        */
        [[nodiscard]] std::tuple<double, double, double> emissionIndexes(double FuelFlow, double AltitudeMsl, double TrueAirspeed, const Atmosphere& Atm) const;
    private:
        std::array<double, LTOPhases.size()> m_LogCorrectedFuelFlow{};

        struct Line {
            double Slope;
            double Intersect;
        };

        // Bilinear Fit HC
        double m_LogHCFuelFlowIntersect = Constants::NaN;
        std::array<Line, 2> m_HCLines{};
        double m_HCEmissionIndexHighFuelFlow = Constants::NaN;

        // Bilinear Fit CO
        double m_LogCOFuelFlowIntersect = Constants::NaN;
        std::array<Line, 2> m_COLines{};
        double m_COEmissionIndexHighFuelFlow = Constants::NaN;

        // Piecewise Linear Fit NOX
        std::array<Line, 3> m_NOxLines{};
    };
}
