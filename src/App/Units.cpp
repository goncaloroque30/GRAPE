// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "Units.h"

#include "Base/Conversions.h"

namespace GRAPE {
    static std::function g_Si = [](double Value) { return Value; };

    // Angle
    template <>
    Unit<Units::Angle>::Unit() : Selected(Units::Angle::Degrees),
        m_Si(Units::Angle::Degrees),
        m_Conversions{ { { g_Si, g_Si }, { toRadians, fromRadians } } },
        m_Names{ "Degrees", "Radians" },
        m_ShortNames{ "deg", "rad" },
        m_Decimals{ 6, 6 } {}

    // Distance
    template <>
    Unit<Units::Distance>::Unit() : Selected(Units::Distance::Meters),
        m_Si(Units::Distance::Meters),
        m_Conversions{ { { g_Si, g_Si }, { toKilometers, fromKilometers }, { toFeet, fromFeet }, { toNauticalMiles, fromNauticalMiles }, { toMiles, fromMiles } } },
        m_Names{ "Meters", "Kilometers", "Feet", "Nautical Miles", "Miles" },
        m_ShortNames{ "m", "km", "ft", "nm", "mi" },
        m_Decimals{ 0, 3, 0, 3, 3 } {}

    // Distance per Force
    template <>
    Unit<Units::DistancePerForce>::Unit() : Selected(Units::DistancePerForce::FeetPerPoundOfForce),
        m_Si(Units::DistancePerForce::MetersPerNewton),
        m_Conversions{ { { g_Si, g_Si }, { toFeetPerPoundOfForce, fromFeetPerPoundOfForce } } },
        m_Names{ "Meter per Newton", "Feet per Pound of Force" },
        m_ShortNames{ "m/N", "ft/lbf" },
        m_Decimals{ 6, 6 } {}

    // Force
    template <>
    Unit<Units::Force>::Unit() : Selected(Units::Force::Newtons),
        m_Si(Units::Force::Newtons),
        m_Conversions{ { { g_Si, g_Si }, { toKilonewtons, fromKilonewtons }, { toPoundsOfForce, fromPoundsOfForce }, { toPoundals, fromPoundals } } },
        m_Names{ "Newtons", "Kilonewtons", "Pounds of Force", "Poundals" },
        m_ShortNames{ "N", "kN", "lbf", "pdl", },
        m_Decimals{ 0, 0, 0, 0 } {}

    // Force per Distance
    template <>
    Unit<Units::ForcePerDistance>::Unit() : Selected(Units::ForcePerDistance::PoundsOfForcePerFoot),
        m_Si(Units::ForcePerDistance::NewtonsPerMeter),
        m_Conversions{ { { g_Si, g_Si }, { toPoundsOfForcePerFoot, fromPoundsOfForcePerFoot } } },
        m_Names{ "Newtons per Meter", "Pounds of Force per Foot" },
        m_ShortNames{ "N/m", "lbf/ft" },
        m_Decimals{ 6, 6 } {}

    // Force per Distance2
    template <>
    Unit<Units::ForcePerDistance2>::Unit() : Selected(Units::ForcePerDistance2::PoundsOfForcePerSquareFoot),
        m_Si(Units::ForcePerDistance2::NewtonsPerSquareMeter),
        m_Conversions{ { { g_Si, g_Si }, { toPoundsOfForcePerFoot2, fromPoundsOfForcePerFoot2 } } },
        m_Names{ "Newtons per Square Meter", "Pounds of Force per Square Foot" },
        m_ShortNames{ "N/m2", "lbf/ft2" },
        m_Decimals{ 6, 6 } {}

    // Force per Speed
    template <>
    Unit<Units::ForcePerSpeed>::Unit() : Selected(Units::ForcePerSpeed::PoundsOfForcePerFeetPerSecond),
        m_Si(Units::ForcePerSpeed::NewtonsPerMeterPerSecond),
        m_Conversions{ { { g_Si, g_Si }, { toPoundsOfForcePerKnot, fromPoundsOfForcePerKnot } } },
        m_Names{ "Newtons per Meter per Second", "Pounds of Force per Knot" },
        m_ShortNames{ "N/m/s", "lbf/kt" },
        m_Decimals{ 6, 6 } {}

    // Force per Temperature 
    template <>
    Unit<Units::ForcePerTemperature>::Unit() : Selected(Units::ForcePerTemperature::PoundsOfForcePerCelsius),
        m_Si(Units::ForcePerTemperature::NewtonsPerKelvin),
        m_Conversions{ { { g_Si, g_Si }, { toPoundsOfForcePerCelsius, fromPoundsOfForcePerCelsius } } },
        m_Names{ "Newtons per Kelvin", "Pounds of Force per Celsius" },
        m_ShortNames{ "N/K", "lbf/C" },
        m_Decimals{ 6, 6 } {}

    // Power
    template <>
    Unit<Units::Power>::Unit() : Selected(Units::Power::Watts),
        m_Si(Units::Power::Watts),
        m_Conversions{ { { g_Si, g_Si }, { toKilowatts, fromKilowatts }, { toHorsePower, fromHorsePower } } },
        m_Names{ "Watts", "Kilowatts", "Horsepower" },
        m_ShortNames{ "W", "kW", "hp", },
        m_Decimals{ 0, 3, 1 } {}

    // Pressure
    template <>
    Unit<Units::Pressure>::Unit() : Selected(Units::Pressure::Pascals),
        m_Si(Units::Pressure::Pascals),
        m_Conversions{ { { g_Si, g_Si }, { toHectopascal, fromHectopascal }, { toBar, fromBar }, { toHectopascal, fromHectopascal }, { toInchesOfMercury, fromInchesOfMercury }, { toMillimetersOfMercury, fromMillimetersOfMercury } } },
        m_Names{ "Pascals", "Hectopascal", "Bar", "Millibar", "Inches of Mercury", "Millimeters of Mercury" },
        m_ShortNames{ "Pa", "hPa", "bar", "mbar", "inHg", "mmHg" },
        m_Decimals{ 0, 2, 0, 2, 2, 0 } {}

    // Speed
    template <>
    Unit<Units::Speed>::Unit() : Selected(Units::Speed::MetersPerSecond),
        m_Si(Units::Speed::MetersPerSecond),
        m_Conversions{ { { g_Si, g_Si }, { toFeetPerSecond, fromFeetPerSecond }, { toFeetPerMinute, fromFeetPerMinute }, { toKilometersPerHour, fromKilometersPerHour }, { toKnots, fromKnots } } },
        m_Names{ "Meters per Second", "Feet per Second", "Feet per Minute", "Kilometers per Hour", "Knots" },
        m_ShortNames{ "m/s", "ft/s", "ft/min", "km/h", "kts" },
        m_Decimals{ 0, 0, 0, 0, 0 } {}

    // Speed per Force Sqrt
    template <>
    Unit<Units::SpeedPerForceSqrt>::Unit() : Selected(Units::SpeedPerForceSqrt::KnotsPerSquareRootOfPoundOfForce),
        m_Si(Units::SpeedPerForceSqrt::MetersPerSecondPerSquareRootOfNewton),
        m_Conversions{ { { g_Si, g_Si }, { toKnotsPerPoundOfForceSqrt, fromKnotsPerPoundOfForceSqrt } } },
        m_Names{ "Meter per Second per Square Root Newton", "Knots per Square Root Pound of Force" },
        m_ShortNames{ "m/s/sqrtN", "kts/sqrtlbf" },
        m_Decimals{ 6, 6 } {}

    // Temperature
    template <>
    Unit<Units::Temperature>::Unit() : Selected(Units::Temperature::Kelvin),
        m_Si(Units::Temperature::Kelvin),
        m_Conversions{ { { g_Si, g_Si }, { toCelsius, fromCelsius }, { toFahrenheit, fromFahrenheit } } },
        m_Names{ "Kelvin", "Celsius", "Fahrenheit" },
        m_ShortNames{ "K", "C", "F", },
        m_Decimals{ 0, 0, 0 } {}

    template <>
    double Unit<Units::Temperature>::toSiDelta(double Value) const {
        return Selected != Units::Temperature::Fahrenheit ? Value : fromFahrenheitDelta(Value);
    }

    template <>
    double Unit<Units::Temperature>::toSiDelta(double Value, const std::string& UnitStr) const {
        const Units::Temperature unit = magic_enum::enum_value<Units::Temperature>(fromString(UnitStr));
        return unit != Units::Temperature::Fahrenheit ? Value : fromFahrenheitDelta(Value);
    }

    template <>
    double Unit<Units::Temperature>::fromSiDelta(double Value) const {
        return Selected != Units::Temperature::Fahrenheit ? Value : toFahrenheitDelta(Value);
    }

    // Time
    template <>
    Unit<Units::Time>::Unit() : Selected(Units::Time::Seconds),
        m_Si(Units::Time::Seconds),
        m_Conversions{ { { g_Si, g_Si }, { toMinutes, fromMinutes }, { toHours, fromHours } } },
        m_Names{ "Seconds", "Minutes", "Hours" },
        m_ShortNames{ "s", "min", "h" },
        m_Decimals{ 0, 0, 0 } {}

    // Weight
    template <>
    Unit<Units::Weight>::Unit() : Selected(Units::Weight::Kilograms),
        m_Si(Units::Weight::Kilograms),
        m_Conversions{ { { g_Si, g_Si }, { toPounds, fromPounds }, { toMetricTons, fromMetricTons } } },
        m_Names{ "Kilograms", "Pounds", "Metric Tons" },
        m_ShortNames{ "kg", "lb", "t" },
        m_Decimals{ 0, 0, 0 } {}

    // WeightPerTime
    template <>
    Unit<Units::WeightPerTime>::Unit() : Selected(Units::WeightPerTime::KilogramsPerSecond),
        m_Si(Units::WeightPerTime::KilogramsPerSecond),
        m_Conversions{ { { g_Si, g_Si }, { toKilogramsPerMinute, fromKilogramsPerMinute }, { toPoundsPerSecond, fromPoundsPerSecond, }, { toPoundsPerMinute, fromPoundsPerMinute } } },
        m_Names{ "Kilograms per Second", "Kilograms per Minute", "Pounds per Second", "Pounds per Minute" },
        m_ShortNames{ "kg/s", "kg/min", "lb/s", "lb/min" },
        m_Decimals{ 4, 4, 4, 4 } {}

    // WeightPerWeight
    template <>
    Unit<Units::WeightPerWeight>::Unit() : Selected(Units::WeightPerWeight::KilogramsPerKilogram),
        m_Si(Units::WeightPerWeight::KilogramsPerKilogram),
        m_Conversions{ { {g_Si, g_Si }, { toGramsPerKilogram, fromGramsPerKilogram }, { toPoundsPerPound, fromPoundsPerPound }, { toOuncesPerPound, fromOuncesPerPound } } },
        m_Names{ "Kilograms per Kilogram", "Grams per Kilogram", "Pounds per Pound", "Ounces per Pound" },
        m_ShortNames{ "kg/kg", "g/kg", "lb/lb", "oz/lb" },
        m_Decimals{ 4, 2, 4, 2 } {}

    // Weight Small
    template <>
    Unit<Units::WeightSmall>::Unit() : Selected(Units::WeightSmall::Kilograms),
        m_Si(Units::WeightSmall::Kilograms),
        m_Conversions{ { { g_Si, g_Si }, { toGrams, fromGrams }, { toPounds, fromPounds } } },
        m_Names{ "Kilograms", "Grams", "Pounds" },
        m_ShortNames{ "kg", "g", "lb" },
        m_Decimals{ 2, 0, 0 } {}

    TEST_CASE("Units Strings") {
        Unit<Units::Distance> dist;
        dist.Selected = Units::Distance::Feet;

        CHECK_EQ(fromNauticalMiles(1.0), dist.toSi(1.0, "Distance nm"));
        CHECK_EQ(fromNauticalMiles(1.0), dist.toSi(1.0, "Distance   nm#??"));
        CHECK_EQ(fromNauticalMiles(1.0), dist.toSi(1.0, "nm#??"));
        CHECK_EQ(fromNauticalMiles(1.0), dist.toSi(1.0, "ft#??nm__"));
    }
}
