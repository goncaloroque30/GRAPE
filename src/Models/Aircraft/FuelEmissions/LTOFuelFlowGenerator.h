// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "LTO.h"

#include "Base/BaseModels.h"

namespace GRAPE {
    class Atmosphere;

    /**
    * @brief Calculates fuel flow based on the LTO Phases
    */
    class LTOFuelFlowGenerator {
    public:
        explicit LTOFuelFlowGenerator(const LTOEngine& LTOEng);

        /**
        * @brief Gets the LTOPhase from the FlightPhase and calculates fuel flow at altitude.
        */
        [[nodiscard]] double fuelFlow(FlightPhase Phase, double AltitudeMsl, double TrueAirspeed, const Atmosphere& Atm) const;

        /**
        * @brief Gets the LTOPhase from the FlightPhase and calculates fuel flow at altitude, applies quadratic interpolation for thrust settings above 60%, as described in https://www.icao.int/publications/documents/9889_cons_en.pdf
        */
        [[nodiscard]] double fuelFlow(FlightPhase Phase, double AltitudeMsl, double TrueAirspeed, const Atmosphere& Atm, double ThrustPercentage) const;
    private:
        std::array<double, LTOPhases.size()> m_CorrectedFuelFlows{};

        struct Quadratic {
            double A;
            double B;
            double C;
        };

        // Fuel Flows Quadratic Fit (60 - 85 - 100)
        std::array<Quadratic, 2> m_FuelFlowQuadratic{};
    };
}
