// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Conversions.h"

namespace GRAPE {
    /**
    * @brief Implements the ISA hydrostatic equations and all the ISA layers. Keeps track of a wind speed and direction as well as a relative humidity.
    * Allows for variation of the first layer, meaning the Atmosphere can either be in the Standard state or the non standard state.
    */
    class Atmosphere {
    public:
        /**
        * @brief Constructs with the standard state.
        */
        Atmosphere();

        /**
        * @brief Constructs with the non standard state.
        */
        Atmosphere(double TemperatureDelta, double PressureDelta) noexcept; // Standard

        Atmosphere(const Atmosphere&) noexcept = default;
        Atmosphere(Atmosphere&&) noexcept = default;
        Atmosphere& operator=(const Atmosphere&) noexcept = default;
        Atmosphere& operator=(Atmosphere&&) noexcept = default;

        /**
        * @brief Get the sea level temperature.
        */
        [[nodiscard]] double seaLevelTemperature() const noexcept;

        /**
        * @brief Get the sea level pressure.
        */
        [[nodiscard]] double seaLevelPressure() const noexcept;

        /**
        * @return Difference between the MSL temperature and the ISA MSL temperature.
        */
        [[nodiscard]] double temperatureDelta() const noexcept;

        /**
        * @return Difference between the MSL temperature and the ISA MSL temperature.
        */
        [[nodiscard]] double pressureDelta() const noexcept;

        /**
        * @return The headwind for the given heading. Negative values indicate a tailwind.
        */
        [[nodiscard]] double headwind(double Heading) const noexcept;

        /**
        * @return The crosswind for the given heading. Negative values indicate a wind from the left.
        */
        [[nodiscard]] double crosswind(double Heading) const noexcept;

        /**
        * @return The atmosphere wind speed.
        */
        [[nodiscard]] double windSpeed() const noexcept { return m_WindSpeed; }

        /**
        * @return The atmosphere wind direction in the range [0, 360] or nan if wind speed should be seen as headwind.
        */
        [[nodiscard]] double windDirection() const noexcept { return m_WindDirection; }

        /**
        * @return Relative humidity in the range [0.0 1.0].
        */
        [[nodiscard]] double relativeHumidity() const noexcept { return m_RelativeHumidity; }

        /**
        * @brief Set temperature and pressure deltas to 0.
        */
        void setStandard() noexcept;

        /**
        * @brief Changes the first layer so that the values of Temperature and Pressure would be observed at GeometricAltitude.
        * ASSERT TemperatureDelta in [-100, 100], PressureDelta in [-15000, 15000].
        */
        void setDeltas(double TemperatureDelta, double PressureDelta) noexcept;

        /**
        * @brief Change the temperature delta.
        * ASSERT TemperatureDelta in [-100, 100].
        */
        void setTemperatureDelta(double TemperatureDelta) noexcept;

        /**
        * @brief Change the pressure for the non standard layer.
        * ASSERT PressureDelta in [-15000, 15000].
        */
        void setPressureDelta(double PressureDelta) noexcept;

        /**
        * @brief Set method for #m_WindSpeed.
        * ASSERT not nan WindSpeed.
        */
        void setWindSpeed(double WindSpeed) noexcept;

        /**
        * @brief Set method for #m_WindDirection
        * ASSERT WindDirection in [0, 360].
        */
        void setWindDirection(double WindDirection) noexcept;

        /*
        * @brief Sets #m_WindSpeed to Headwind and #m_WindDirection to nan. Calling headwind(double) after returns a constant headwind and crosswind(double) returns always 0.0.
        * ASSERT not nan Headwind.
        */
        void setConstantHeadwind(double Headwind) noexcept;

        /**
        * @brief Throwing set method for #m_RelativeHumidity.
        * ASSERT RelativeHumidity in [0.0, 1.0].
        */
        void setRelativeHumidity(double RelativeHumidity) noexcept;

        /**
        * @brief Throwing version of setDeltas(double, double).
        * Throws if TemperatureDelta not in [-100, 100] or PressureDelta not in [-15000, 15000].
        */
        void setDeltasE(double TemperatureDelta, double PressureDelta);

        /**
        * @brief Throwing version of setTemperatureDelta(double).
        * Throws if TemperatureDelta not in [-100, 100].
        */
        void setTemperatureDeltaE(double TemperatureDelta);

        /**
        * @brief Throwing version of setPressure(double).
        * Throws if PressureDelta not in [-15000, 15000].
        */
        void setPressureDeltaE(double PressureDelta);

        /**
        * @brief Throwing version of setWindDirection(double).
        * Throws if WindDirection not in [0, 360].
        */
        void setWindDirectionE(double WindDirection);

        /**
        * @brief Throwing version of setRelativeHumidity(double).
        * Throws if RelativeHumidity not in [0, 1].
        */
        void setRelativeHumidityE(double RelativeHumidity);

        /**
        * @brief Atmosphere state (standard vs. non standard) is defined by the values of #m_TemperatureDelta and #m_PressureDelta.
        * @return True if any of the deltas is not 0.
        */
        [[nodiscard]] bool isNonStandard() const noexcept;

        /**
        * @brief Atmosphere is in Headwind mode if #m_WindDirection is nan.
        * @return True if #m_WindDirection is nan.
        */
        [[nodiscard]] bool isHeadwind() const noexcept;

        /**
        * @return The temperature at GeometricAltitude.
        */
        [[nodiscard]] double temperature(double GeometricAltitude) const;

        /**
        * @return The pressure at GeometricAltitude.
        */
        [[nodiscard]] double pressure(double GeometricAltitude) const;

        /**
        * @return The density at GeometricAltitude.
        */
        [[nodiscard]] double density(double GeometricAltitude) const;

        /**
        * @return The temperature ratio at GeometricAltitude (relation to constant t0).
        */
        [[nodiscard]] double temperatureRatio(double GeometricAltitude) const;

        /**
        * @return The pressure ratio at GeometricAltitude (relation to constant p0).
        */
        [[nodiscard]] double pressureRatio(double GeometricAltitude) const;

        /**
        * @return The density ratio at GeometricAltitude (relation to constant d0).
        */
        [[nodiscard]] double densityRatio(double GeometricAltitude) const;

    private:
        double m_TemperatureDelta;
        double m_PressureDelta;

        double m_WindSpeed;
        double m_WindDirection;
        double m_RelativeHumidity;
    };
}
