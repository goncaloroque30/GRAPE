// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

namespace GRAPE {
    enum class OperationType {
        Arrival = 0,
        Departure,
    };
    constexpr EnumStrings<OperationType> OperationTypes{ "Arrival", "Departure" };

    enum class FlightPhase {
        Approach = 0,
        LandingRoll,
        TakeoffRoll,
        InitialClimb,
        Climb,
    };
    constexpr EnumStrings<FlightPhase> FlightPhases{ "Approach", "Landing Roll", "Takeoff Roll", "Initial Climb", "Climb" };

    enum class PerformanceModel {
        None = 0,
        Doc29,
    };
    constexpr EnumStrings<PerformanceModel> PerformanceModelTypes{ "None", "Doc29" };

    enum class FuelFlowModel {
        None = 0,
        LTO,
        LTODoc9889,
        SFI,
    };
    constexpr EnumStrings<FuelFlowModel> FuelFlowModelTypes{ "None", "LTO", "LTO Doc9889", "SFI" };

    enum class FuelModel {
        None = 0,
        LTOCycle,
        FuelFlow,
    };
    constexpr EnumStrings<FuelModel> FuelModelTypes{ "None", "LTO Cycle", "Fuel Flow" };

    enum class EmissionsModel {
        LTOCycle,
        Segments,
    };
    constexpr EnumStrings<EmissionsModel> EmissionsModelTypes{ "LTO Cycle", "Segments" };

    enum class EmissionsParticleSmokeNumberModel {
        None = 0,
        FOA3,
        FOA4,
    };
    constexpr EnumStrings<EmissionsParticleSmokeNumberModel> EmissionsParticleSmokeNumberModelTypes{ "None", "FOA 3", "FOA 4" };

    enum class NoiseModel {
        Doc29 = 0,
    };
    constexpr EnumStrings<NoiseModel> NoiseModelTypes{ "Doc29" };
}
