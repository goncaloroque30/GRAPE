// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include <regex>

namespace GRAPE {
    template <typename Enum> requires std::is_enum_v<Enum>&& std::is_unsigned_v<std::underlying_type_t<Enum>>
    class Unit {
    public:
        Unit() { GRAPE_ASSERT(false, "Unit template not specialized."); }

        // Access data
        [[nodiscard]] Enum si() const { return m_Si; }
        [[nodiscard]] Enum selected() const { return Selected; }
        [[nodiscard]] const auto& names() const { return m_Names; }
        [[nodiscard]] const auto& shortNames() const { return m_ShortNames; }
        [[nodiscard]] const std::string& name() const { return m_Names.at(magic_enum::enum_integer(Selected)); }
        [[nodiscard]] const std::string& name(Enum Un) const { return m_Names.at(magic_enum::enum_integer(Un)); }
        [[nodiscard]] const std::string& shortName() const { return m_ShortNames.at(magic_enum::enum_integer(Selected)); }
        [[nodiscard]] const std::string& shortName(Enum Un) const { return m_ShortNames.at(magic_enum::enum_integer(Un)); }
        [[nodiscard]] std::size_t decimals() const { return m_Decimals.at(magic_enum::enum_integer(Selected)); }

        // Set data
        void setDecimals(std::size_t Decimals) { m_Decimals.at(magic_enum::enum_integer(Selected)) = Decimals; }
        void setDecimals(Enum Un, std::size_t Decimals) { m_Decimals.at(magic_enum::enum_integer(Un)) = Decimals; }

        // Conversions
        [[nodiscard]] double toSi(double Value) const { return m_Conversions.at(magic_enum::enum_integer(Selected)).From(Value); } // Input is in selected unit | Output is in SI
        [[nodiscard]] double toSi(double Value, const std::string& UnitStr) const { return m_Conversions.at(fromString(UnitStr)).From(Value); }
        [[nodiscard]] double toSiDelta(double Value, const std::string& UnitStr) const { return toSi(Value, UnitStr); } // Input is in selected unit | Output is in SI
        [[nodiscard]] double toSiDelta(double Value) const { return toSi(Value); } // Input is in selected unit | Output is in SI
        [[nodiscard]] double fromSi(double Value) const { return m_Conversions.at(magic_enum::enum_integer(Selected)).To(Value); } // Input in in SI | Output is in selected unit
        [[nodiscard]] double fromSiDelta(double Value) const { return fromSi(Value); } // Input in in SI | Output is in selected unit
        [[nodiscard]] double from(Enum From, double Value) const { return fromSi(m_Conversions.at(magic_enum::enum_integer(From)).From(Value)); } // Input is in From unit | Output is in selected unit
        [[nodiscard]] double to(Enum To, double Value) const { return m_Conversions.at(magic_enum::enum_integer(To)).To(toSi(Value)); } // Input is in selected unit | Output is in To unit
    public:
        // Editable Data
        Enum Selected;

    private:
        struct Conversions {
            std::function<double(double)> To;   // From Si to Unit
            std::function<double(double)> From; // From Unit to Si
        };

        Enum m_Si;
        std::array<Conversions, magic_enum::enum_count<Enum>()> m_Conversions;
        std::array<std::string, magic_enum::enum_count<Enum>()> m_Names = {};
        std::array<std::string, magic_enum::enum_count<Enum>()> m_ShortNames = {};
        std::array<std::size_t, magic_enum::enum_count<Enum>()> m_Decimals = {};
    private:
        [[nodiscard]] std::size_t fromString(const std::string& UnitStr) const;
    };

    template<typename Enum> requires std::is_enum_v<Enum>&& std::is_unsigned_v<std::underlying_type_t<Enum>>
    inline std::size_t Unit<Enum>::fromString(const std::string& UnitStr) const {
        std::smatch m;

        if (std::regex_search(UnitStr, m, std::regex("[^a-z0-9/]*([a-z0-9/]+)[^a-z0-9/]*$", std::regex::icase)))
            for (auto it = m_ShortNames.begin(); it != m_ShortNames.end(); ++it)
            {
                const auto index = std::distance(m_ShortNames.begin(), it);
                if (*it == m[1])
                    return index;
            }

        return magic_enum::enum_integer(Selected);
    }

    namespace Units {
        enum class Acceleration : unsigned {
            MetersPerSquareSecond = 0,
            FeetPerSquareSecond,
            MilesPerSquareSecond,
            NauticalMilesPerSquareSecond,
        };

        enum class Angle : unsigned {
            Degrees,
            Radians,
        };

        enum class Density : unsigned {
            KilogramsPerCubicMeter = 0,
            PoundsPerCubicFeet,
        };

        enum class Distance : unsigned {
            Meters = 0,
            Kilometers,
            Feet,
            NauticalMiles,
            Miles,
        };

        enum class DistancePerForce : unsigned {
            MetersPerNewton = 0,
            FeetPerPoundOfForce,
        };

        enum class Force : unsigned {
            Newtons = 0,
            PoundsOfForce,
            Poundals,
        };

        enum class ForcePerDistance : unsigned {
            NewtonsPerMeter = 0,
            PoundsOfForcePerFoot,
        };

        enum class ForcePerDistance2 : unsigned {
            NewtonsPerSquareMeter = 0,
            PoundsOfForcePerSquareFoot,
        };

        enum class ForcePerSpeed : unsigned {
            NewtonsPerMeterPerSecond = 0,
            PoundsOfForcePerFeetPerSecond,
        };

        enum class ForcePerTemperature : unsigned {
            NewtonsPerKelvin = 0,
            PoundsOfForcePerCelsius,
        };

        enum class Power : unsigned {
            Watts = 0,
            Kilowatts,
            HorsePower,
        };

        enum class Pressure : unsigned {
            Pascals = 0,
            Hectopascal,
            Bar,
            Millibar,
            InchesOfMercury,
            MillimetersOfMercury,
        };

        enum class Speed : unsigned {
            MetersPerSecond = 0, FeetPerSecond,
            FeetPerMinute,
            KilometersPerHour,
            Knots,
        };

        enum class SpeedPerForceSqrt : unsigned {
            MetersPerSecondPerSquareRootOfNewton = 0,
            KnotsPerSquareRootOfPoundOfForce,
        };

        enum class Temperature : unsigned {
            Kelvin = 0,
            Celsius,
            Fahrenheit,
        };

        enum class Time : unsigned {
            Seconds = 0,
            Minutes,
            Hours,
        };

        enum class Weight : unsigned {
            Kilograms = 0,
            Pounds,
            MetricTons,
        };

        enum class WeightPerTime : unsigned {
            KilogramsPerSecond = 0,
            KilogramsPerMinute,
            PoundsPerSecond,
            PoundsPerMinute,
        };

        enum class WeightPerWeight : unsigned {
            KilogramsPerKilogram = 0,
            GramsPerKilogram,
            PoundsPerPound,
            OuncesPerPound,
        };

        enum class WeightSmall : unsigned {
            Kilograms = 0,
            Grams,
            Pounds,
        };

        enum class Volume : unsigned {
            CubicMeters = 0,
            Liters,
            UsGallons,
            UsQuarts,
        };
    }

    // Specializations
    template <>
    Unit<Units::Angle>::Unit();

    template <>
    Unit<Units::Distance>::Unit();

    template <>
    Unit<Units::DistancePerForce>::Unit();

    template <>
    Unit<Units::Force>::Unit();

    template <>
    Unit<Units::ForcePerDistance>::Unit();

    template <>
    Unit<Units::ForcePerDistance2>::Unit();

    template <>
    Unit<Units::ForcePerSpeed>::Unit();


    template <>
    Unit<Units::ForcePerTemperature>::Unit();

    template <>
    Unit<Units::Power>::Unit();

    template <>
    Unit<Units::Pressure>::Unit();

    template <>
    Unit<Units::Speed>::Unit();

    template <>
    Unit<Units::SpeedPerForceSqrt>::Unit();

    template <>
    Unit<Units::Temperature>::Unit();

    template <>
    double Unit<Units::Temperature>::toSiDelta(double Value) const;

    template <>
    double Unit<Units::Temperature>::toSiDelta(double Value, const std::string& UnitStr) const;

    template <>
    double Unit<Units::Temperature>::fromSiDelta(double Value) const;

    template <>
    Unit<Units::Time>::Unit();

    template <>
    Unit<Units::Weight>::Unit();

    template <>
    Unit<Units::WeightPerTime>::Unit();

    template <>
    Unit<Units::WeightPerWeight>::Unit();

    template <>
    Unit<Units::WeightSmall>::Unit();
}
