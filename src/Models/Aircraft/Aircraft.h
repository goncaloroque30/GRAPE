// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

namespace GRAPE {
    class Doc29Performance;
    class Doc29Noise;
    class LTOEngine;
    class SFI;

    /**
    * @brief An Aircraft combines all the different implemented models into a single instance which can be used by operations.
    */
    struct Aircraft {
        explicit Aircraft(std::string_view NameIn) noexcept;
        Aircraft(std::string_view NameIn, const Doc29Performance* Doc29AcftIn, const SFI* SFIIn, const LTOEngine* LTOEngineIn, const Doc29Noise* Doc29NsIn) noexcept;

        std::string Name;

        const Doc29Performance* Doc29Perf = nullptr; // Performance

        const SFI* SFIFuel = nullptr;      // Fuel Flow
        const LTOEngine* LTOEng = nullptr; // Fuel Flow & Emissions

        const Doc29Noise* Doc29Ns = nullptr; // Noise
        double Doc29NoiseDeltaArrivals = 0.0;
        double Doc29NoiseDeltaDepartures = 0.0;

        // Engine Count
        int EngineCount = 2;

        /**
        * @brief Throwing set method for #EngineCount.
        *
        * Throw if not in range [1, 4].
        */
        void setEngineCountE(int EngineCountIn);

        // Maximum Sea Level Static Thrust
        double MaximumSeaLevelStaticThrust = 100000.0;

        /**
        * @brief Throwing set method for #MaximumSeaLevelStaticThrust.
        *
        * Throw if not in range ]0, inf].
        */
        void setMaximumSeaLevelStaticThrustE(double MaximumSeaLevelStaticThrustIn);

        // Engine Breakpoint Temperature
        double EngineBreakpointTemperature = 303.15; // 30 C.

        /**
        * @brief Throwing set method for #EngineBreakpointTemperature.
        *
        * Throw if not in range [0, inf].
        */
        void setEngineBreakpointTemperatureE(double EngineBreakpointTemperatureIn);

        /**
        * @return  True if #Doc29Perf is not nullptr.
        */
        [[nodiscard]] bool validDoc29Performance() const { return Doc29Perf; }

        /**
        * @return  True if #SFIFuel is not nullptr.
        */
        [[nodiscard]] bool validSFI() const { return SFIFuel; }

        /**
        * @return  True if #LTOEng is not nullptr.
        */
        [[nodiscard]] bool validLTOEngine() const { return LTOEng; }

        /**
        * @return  True if #Doc29Ns is not nullptr.
        */
        [[nodiscard]] bool validDoc29Noise() const { return Doc29Ns; }
    };
}
