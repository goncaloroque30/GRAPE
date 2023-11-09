// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

namespace GRAPE {
    class Atmosphere;

    /**
    * @brief Stores the values to apply the SFI fuel flow model.
    *
    * See https://arc.aiaa.org/doi/10.2514/1.42025 for the description of the fuel flow model.
    */
    struct SFI {
        explicit SFI(std::string_view NameIn) noexcept : Name(NameIn) {}

        std::string Name;

        double MaximumSeaLevelStaticThrust = 100000.0;

        double K1 = 0.0, K2 = 0.0, K3 = 0.0, K4 = 0.0;
        double A = 0.0, B1 = 0.0, B2 = 0.0, B3 = 0.0;

        /**
        * @brief Throwing set method for #MaximumSeaLevelStaticThrust.
        *
        * Throw if not in range [1, inf].
        */
        void setMaximumSeaLevelStaticThrust(double MaximumSeaLevelStaticThrustIn);

        /**
        * @brief Implements equation (1) found in the paper.
        * @return The thrust specific fuel flow.
        */
        [[nodiscard]] double departureFuelFlow(double AltitudeMsl, double TrueAirspeed, double CorrNetThrustPerEng, const Atmosphere& Atm) const noexcept;

        /**
        * @brief Implements equation (2) found in the paper.
        * @return The thrust specific fuel flow.
        */
        [[nodiscard]] double arrivalFuelFlow(double AltitudeMsl, double TrueAirspeed, double CorrNetThrustPerEng, const Atmosphere& Atm) const noexcept;
    };
}
