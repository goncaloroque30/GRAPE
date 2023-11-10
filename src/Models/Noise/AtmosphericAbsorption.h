// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Noise.h"

namespace GRAPE {
    /**
    * @brief Stores the atmospheric absorption correction factors for each one third octave band.
    */
    class AtmosphericAbsorption {
    public:
        /**
        * @brief The supported types of atmospheric correction.
        */
        enum class Type {
            None = 0,
            SaeArp866,
            SaeArp5534,
        };
        static constexpr EnumStrings<Type> Types{ "None", "SAE ARP 866", "SAE ARP 5534" };

        /**
        * @brief All correction factors initialized to 0 and #m_Type is Type::None.
        */
        AtmosphericAbsorption() = default;

        /**
        * @brief Calls applySaeArp5534.
        */
        AtmosphericAbsorption(double Temperature, double Pressure, double RelativeHumidity);

        /**
        * @brief Calls applySaeArp866.
        */
        AtmosphericAbsorption(double Temperature, double RelativeHumidity);

        [[nodiscard]] auto operator()(std::size_t Index) { return m_AtmosphericAbsorptions.at(Index); }
        [[nodiscard]] auto begin() { return m_AtmosphericAbsorptions.begin(); }
        [[nodiscard]] auto end() { return m_AtmosphericAbsorptions.end(); }

        [[nodiscard]] auto operator()(std::size_t Index) const { return m_AtmosphericAbsorptions.at(Index); }
        [[nodiscard]] auto begin() const { return m_AtmosphericAbsorptions.begin(); }
        [[nodiscard]] auto end() const { return m_AtmosphericAbsorptions.end(); }

        /**
        * @brief Get method for #m_Type.
        */
        [[nodiscard]] auto type() const { return m_Type; }

        /**
        * @brief Calculate the atmosphere correction factors according to SAE ARP 866 and set #m_Type to Type::SaeArp866.
        */
        void applySaeArp866(double Temperature, double RelativeHumidity);

        /**
        * @brief Calculate the atmosphere correction factors according to SAE ARP 5534 and set #m_Type to Type::SaeArp5534.
        */
        void applySaeArp5534(double Temperature, double Pressure, double RelativeHumidity);
    private:
        Type m_Type = Type::None;
        std::array<double, OneThirdOctaveBandsSize> m_AtmosphericAbsorptions{};
    };
}
