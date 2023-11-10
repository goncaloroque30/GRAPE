// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

namespace GRAPE {
    class Doc29Aircraft;
    class Doc29Noise;
    class LTOEngine;
    class SFI;

    /**
    * @brief An Aircraft combines all the different implemented models into a single instance which can be used by operations.
    */
    struct Aircraft {
        explicit Aircraft(std::string_view NameIn) noexcept;
        Aircraft(std::string_view NameIn, const Doc29Aircraft* Doc29AcftIn, const SFI* SFIIn, const LTOEngine* LTOEngineIn, const Doc29Noise* Doc29NsIn) noexcept;

        std::string Name;

        const Doc29Aircraft* Doc29Acft = nullptr; // Performance

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

        /**
        * @return True if #Doc29Acft is not nullptr.
        */
        [[nodiscard]] bool validDoc29Performance() const { return Doc29Acft; }

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
