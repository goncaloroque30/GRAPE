// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

namespace GRAPE {
    /**
    * @brief Enum for the 4 LTO phases.
    */
    enum class LTOPhase {
        Idle = 0,
        Approach = 1,
        ClimbOut = 2,
        Takeoff = 3,
    };
    constexpr EnumStrings<LTOPhase> LTOPhases{ "Idle", "Approach", "Climb Out", "Takeoff", };

    /**
    * @brief Stores the LTO fuel and emissions values.

    * The values are stored in arrays with the same size as #LTOPhase.
    */
    class LTOEngine {
    public:
        explicit LTOEngine(std::string_view NameIn) noexcept : Name(NameIn) {}

        std::string Name;
        std::array<double, LTOPhases.size()> FuelFlows{};
        std::array<double, LTOPhases.size()> FuelFlowCorrectionFactors{ 1.100, 1.020, 1.013, 1.010 };
        std::array<double, LTOPhases.size()> EmissionIndexesHC{};
        std::array<double, LTOPhases.size()> EmissionIndexesCO{};
        std::array<double, LTOPhases.size()> EmissionIndexesNOx{};

        /**
        * @brief Throwing set method for the fuel flows.
        * Throws if FuelFlow not in [0, inf]
        */
        void setFuelFlow(LTOPhase Phase, double FuelFlow);

        /**
        * @brief Throwing set method for the fuel flow correction factors.
        * Throws if FuelFlowCorrection not in [0, inf]
        */
        void setFuelFlowCorrection(LTOPhase Phase, double FuelFlowCorrection);

        /**
        * @brief Throwing set method for the HC emission indexes.
        * Throws if HCEI not in [0, inf]
        */
        void setEmissionIndexHC(LTOPhase Phase, double HCEI);

        /**
        * @brief Throwing set method for the CO emission indexes.
        * Throws if COEI not in [0, inf]
        */
        void setEmissionIndexCO(LTOPhase Phase, double COEI);

        /**
        * @brief Throwing set method for the NOx emission indexes.
        * Throws if NOxEI not in [0, inf]
        */
        void setEmissionIndexNOx(LTOPhase Phase, double NOxEI);
    };
}
