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
        Doc29 = 0,
    };
    constexpr EnumStrings<PerformanceModel> PerformanceModelTypes{ "Doc29" };

    enum class FuelFlowModel {
        None = 0,
        LTO,
        LTODoc9889,
        SFI,
    };
    constexpr EnumStrings<FuelFlowModel> FuelFlowModelTypes{ "None", "LTO", "LTO Doc9889", "SFI" };

    enum class EmissionsModel {
        None = 0,
        BFFM2,
    };
    constexpr EnumStrings<EmissionsModel> EmissionsModelTypes{ "None", "Boeing Fuel Flow Method 2" };

    enum class NoiseModel {
        Doc29 = 0,
    };
    constexpr EnumStrings<NoiseModel> NoiseModelTypes{ "Doc29" };
}
