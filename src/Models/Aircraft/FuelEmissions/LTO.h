// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Base/BaseModels.h"

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

    /*
    * @brief Converts FlightPhase to LTOPhase
    */
    inline LTOPhase ltoPhase(const FlightPhase Phase) {
        switch (Phase)
        {
        case FlightPhase::Approach: return LTOPhase::Approach; break;
        case FlightPhase::LandingRoll: return LTOPhase::Approach; break;
        case FlightPhase::TakeoffRoll: return LTOPhase::Takeoff; break;
        case FlightPhase::InitialClimb: return LTOPhase::Takeoff; break;
        case FlightPhase::Climb: return LTOPhase::ClimbOut; break;
        default: GRAPE_ASSERT(false); return LTOPhase::Idle; break;
        }
    }

    /*
    * @brief Converts FlightPhase to LTO Index
    */
    inline int ltoIndex(const FlightPhase Phase) {
        return magic_enum::enum_integer(ltoPhase(Phase));
    }

    /**
    * @brief Stores the LTO fuel and emissions values.

    * The values are stored in arrays with the same size as #LTOPhase.
    */
    class LTOEngine {
    public:
        explicit LTOEngine(std::string_view NameIn) noexcept : Name(NameIn) {}

        std::string Name;

        double MaximumSeaLevelStaticThrust = 100000.0;

        std::array<double, 4> FuelFlows{};
        std::array<double, 4> FuelFlowCorrectionFactors{ { 1.100, 1.020, 1.013, 1.010 } };
        std::array<double, 4> EmissionIndexesHC{};
        std::array<double, 4> EmissionIndexesCO{};
        std::array<double, 4> EmissionIndexesNOx{};

        bool MixedNozzle = true;
        double BypassRatio = 0.0;
        std::array<double, 4> AirFuelRatios{ { 106.0, 83.0, 51.0, 45.0 } };
        std::array<double, 4> SmokeNumbers{ { Constants::NaN, Constants::NaN, Constants::NaN, Constants::NaN } };
        std::array<double, 4> EmissionIndexesNVPM{ { Constants::NaN, Constants::NaN, Constants::NaN, Constants::NaN } };
        std::array<double, 4> EmissionIndexesNVPMNumber{ { Constants::NaN, Constants::NaN, Constants::NaN, Constants::NaN } };


        /**
        * @brief Get method for fuel flow. Converts FlightPhase to LTOPhase.
        */
        [[nodiscard]] double fuelFlow(FlightPhase FlPhase) const;

        /**
        * @brief Get method for fuel flow correction factor. Converts FlightPhase to LTOPhase.
        */
        [[nodiscard]] double fuelFlowCorrectionFactor(FlightPhase FlPhase) const;

        /**
        * @brief Get method for HC EI. Converts FlightPhase to LTOPhase.
        */
        [[nodiscard]] double hcEI(FlightPhase FlPhase) const;

        /**
        * @brief Get method for CO EI. Converts FlightPhase to LTOPhase.
        */
        [[nodiscard]] double coEI(FlightPhase FlPhase) const;

        /**
        * @brief Get method for NOx. Converts FlightPhase to LTOPhase.
        */
        [[nodiscard]] double noxEI(FlightPhase FlPhase) const;

        /**
        * @brief Get method for smoke number. Converts FlightPhase to LTOPhase.
        */
        [[nodiscard]] double smokeNumber(FlightPhase FlPhase) const;

        /**
        * @brief Get method for nvPM. Converts FlightPhase to LTOPhase.
        */
        [[nodiscard]] double nvPMEI(FlightPhase FlPhase) const;

        /**
        * @brief Get method for nvPM Number. Converts FlightPhase to LTOPhase.
        */
        [[nodiscard]] double nvPMNumberEI(FlightPhase FlPhase) const;

        /**
        * @brief Throwing set method for #MaximumSeaLevelStaticThrust.
        *
        * Throw if not in range [1, inf].
        */
        void setMaximumSeaLevelStaticThrust(double MaximumSeaLevelStaticThrustIn);

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

        /**
        * @brief Throwing set method for #BypassRatio.
        * Throws if BypassRatioIn not in [0, inf]
        */
        void setBypassRatio(double BypassRatioIn);

        /**
        * @brief Throwing set method for air to fuel ratio.
        * Throws if AirFuelRatio not in [0, inf]
        */
        void setAirFuelRatio(LTOPhase Phase, double AirFuelRatio);

        /**
        * @brief Throwing set method for the smoke number.
        * Throws if SmokeNumbers not in [0, inf]
        */
        void setSmokeNumber(LTOPhase Phase, double SmokeNumber);

        /**
        * @brief Throwing set method for the nvPM emission indexes.
        * Throws if NVPMEI not in [0, inf]
        */
        void setEmissionIndexNVPM(LTOPhase Phase, double NVPMEI);

        /**
        * @brief Throwing set method for the nvPM Number emission indexes.
        * Throws if NVPMNumberEI not in [0, inf]
        */
        void setEmissionIndexNVPMNumber(LTOPhase Phase, double NVPMNumberEI);
    };
}
