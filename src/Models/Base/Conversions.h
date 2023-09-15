// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "AtmosphericConstants.h"

#include <cmath>
#include <numbers>

namespace GRAPE {
    /// Time (Seconds)
    // Minutes
    constexpr double toMinutes(double Seconds) { return Seconds / 60.0; }
    constexpr double fromMinutes(double Minutes) { return Minutes / toMinutes(1.0); }

    // Hours
    constexpr double toHours(double Seconds) { return Seconds / 3600.0; }
    constexpr double fromHours(double Hours) { return Hours / toHours(1.0); }

    /// Angle (Degrees)
    constexpr double toRadians(double Degrees) { return Degrees * Constants::Pi / 180.0; }
    constexpr double fromRadians(double Radians) { return Radians / toRadians(1); }

    /// Distance (Meters)
    // Kilometers
    constexpr double toKilometers(double Meters) { return Meters / 1000.0; }
    constexpr double fromKilometers(double Kilometers) { return Kilometers / toKilometers(1.0); }

    // Feet
    constexpr double toFeet(double Meters) { return Meters / 0.304800; }
    constexpr double fromFeet(double Feet) { return Feet / toFeet(1.0); }

    // Nautical Miles
    constexpr double toNauticalMiles(double Meters) { return Meters / 1852.0; }
    constexpr double fromNauticalMiles(double NauticalMiles) { return NauticalMiles / toNauticalMiles(1.0); }

    // Miles
    constexpr double toMiles(double Meters) { return Meters / 1609.344; }
    constexpr double fromMiles(double Miles) { return Miles / toMiles(1.0); }

    /// Weight (Kilograms)
    // Grams
    constexpr double toGrams(double Kilograms) { return Kilograms * 1000.0; }
    constexpr double fromGrams(double Grams) { return Grams / toGrams(1.0); }

    // Pounds
    constexpr double toPounds(double Kilograms) { return Kilograms * 2.204623; }
    constexpr double fromPounds(double Pounds) { return Pounds / toPounds(1.0); }

    // Ounces
    constexpr double toOunces(double Kilograms) { return toPounds(Kilograms) * 16.0; }
    constexpr double fromOunces(double Ounces) { return Ounces / toOunces(1.0); }

    // Metric Tons
    constexpr double toMetricTons(double Kilograms) { return Kilograms / 1000.0; }
    constexpr double fromMetricTons(double MetricTons) { return MetricTons / toMetricTons(1.0); }

    /// Weight per Time (Kilograms per Second)
    // Kilograms per Minute
    constexpr double toKilogramsPerMinute(double KilogramsPerSecond) { return KilogramsPerSecond / toMinutes(1.0); }
    constexpr double fromKilogramsPerMinute(double KilogramsPerMinute) { return KilogramsPerMinute / fromMinutes(1.0); }

    // Pounds per Second
    constexpr double toPoundsPerSecond(double KilogramsPerSecond) { return toPounds(KilogramsPerSecond); }
    constexpr double fromPoundsPerSecond(double PoundsPerSecond) { return fromPounds(PoundsPerSecond); }

    // Pounds per Minute
    constexpr double toPoundsPerMinute(double KilogramsPerSecond) { return toPounds(KilogramsPerSecond) / toMinutes(1.0); }
    constexpr double fromPoundsPerMinute(double PoundsPerMinute) { return fromPounds(PoundsPerMinute) / fromMinutes(1.0); }

    /// Weight per Weight (Kilograms per Kilogram)
    // Grams per Kilogram
    constexpr double toGramsPerKilogram(double KilogramsPerKilogram) { return toGrams(KilogramsPerKilogram); }
    constexpr double fromGramsPerKilogram(double GramsPerKilogram) { return fromGrams(GramsPerKilogram); }

    // Pounds per Pound
    constexpr double toPoundsPerPound(double KilogramsPerKilogram) { return toPounds(KilogramsPerKilogram) / toPounds(1.0); }
    constexpr double fromPoundsPerPound(double PoundsPerPound) { return fromPounds(PoundsPerPound) / fromPounds(1.0); }

    // Ounces per Pound
    constexpr double toOuncesPerPound(double KilogramsPerKilogram) { return toOunces(KilogramsPerKilogram) / toPounds(1.0); }
    constexpr double fromOuncesPerPound(double OuncesPerPound) { return fromOunces(OuncesPerPound) / fromPounds(1.0); }

    /// Volume (Cubic Meters)
    // Liters
    constexpr double toLiters(double Meters3) { return Meters3 * 1000.0; }
    constexpr double fromLiters(double Liters) { return Liters / toLiters(1.0); }

    // US Gallons
    constexpr double toUsGallons(double CubicMeters) { return CubicMeters * 264.172; }
    constexpr double fromUsGallons(double Gallons) { return Gallons / toUsGallons(1.0); }

    // US Quarts
    constexpr double toUsQuarts(double Meters3) { return Meters3 * 1056.69; }
    constexpr double fromUsQuarts(double Quarts) { return Quarts / toUsQuarts(1.0); }

    /// Temperature (Kelvin)
    // Celsius
    constexpr double toCelsius(double Kelvin) { return Kelvin - 273.15; }
    constexpr double fromCelsius(double Celsius) { return Celsius - toCelsius(0.0); }

    constexpr double toCelsiusDelta(double KelvinDelta) { return KelvinDelta; }
    constexpr double fromCelsiusDelta(double CelsiusDelta) { return CelsiusDelta; }

    // Fahrenheit
    constexpr double toFahrenheit(double Kelvin) { return (Kelvin - 273.15) * 9.0 / 5.0 + 32.0; }
    constexpr double fromFahrenheit(double Fahrenheit) { return (Fahrenheit - 32.0) * 5.0 / 9.0 + 273.15; }

    constexpr double toFahrenheitDelta(double KelvinDelta) { return KelvinDelta * 9.0 / 5.0; }
    constexpr double fromFahrenheitDelta(double FahrenheitDelta) { return FahrenheitDelta * 5.0 / 9.0; }

    /// Speed (Meter per Second)
    // Feet per Second
    constexpr double toFeetPerSecond(double MetersPerSec) { return MetersPerSec * toFeet(1.0); }
    constexpr double fromFeetPerSecond(double FeetPerSec) { return FeetPerSec * fromFeet(1.0); }

    // Feet per Minute
    constexpr double toFeetPerMinute(double MetersPerSec) { return MetersPerSec * toFeet(1.0) / toMinutes(1.0); }
    constexpr double fromFeetPerMinute(double FeetPerMin) { return FeetPerMin * fromFeet(1.0) / fromMinutes(1.0); }

    // Kilometer per Hour
    constexpr double toKilometersPerHour(double MetersPerSec) { return MetersPerSec * toKilometers(1.0) / toHours(1.0); }
    constexpr double fromKilometersPerHour(double KilometersPerHour) { return KilometersPerHour * fromKilometers(1.0) / fromHours(1.0); }

    // Knots
    constexpr double toKnots(double MetersPerSec) { return MetersPerSec * toNauticalMiles(1.0) / toHours(1.0); }
    constexpr double fromKnots(double Knots) { return Knots * fromNauticalMiles(1.0) / fromHours(1.0); }

    /// Acceleration (Meter per square Second)
    // Feet per square Second
    constexpr double toFeetPerSecond2(double MetersPerSec2) { return MetersPerSec2 * toFeet(1.0); }
    constexpr double fromFeetPerSecond2(double FeetPerSec2) { return FeetPerSec2 * fromFeet(1.0); }

    // Miles per square Second
    constexpr double toMilesPerSecond2(double MilesPerSec2) { return MilesPerSec2 * toMiles(1.0); }
    constexpr double fromMilesPerSecond2(double MilesPerSec2) { return MilesPerSec2 * fromMiles(1.0); }

    // Nautical Miles per square Second
    constexpr double toNauticalMilesPerSecond2(double MetersPerSec2) { return MetersPerSec2 * toNauticalMiles(1.0); }
    constexpr double fromNauticalMilesPerSecond2(double NauticalMilesPerSec2) { return NauticalMilesPerSec2 * fromNauticalMiles(1.0); }

    /// Density (Kilograms per cubic Meter)
    // Pounds per cubic Feet
    constexpr double toPoundsPerFeet3(double KilogramPerMeter3) { return KilogramPerMeter3 * toPounds(1.0) / (toFeet(1.0) * toFeet(1.0) * toFeet(1.0)); }
    constexpr double fromPoundsPerFeet3(double PoundsPerFeet3) { return PoundsPerFeet3 * fromPounds(1.0) / (fromFeet(1.0) * fromFeet(1.0) * fromFeet(1.0)); }

    /// Pressure (Pascal)
    // Hectopascal / Millibar
    constexpr double toHectopascal(double Pascal) { return Pascal / 100.0; }
    constexpr double fromHectopascal(double Hectopascal) { return Hectopascal * 100.0; }

    // Inches of Mercury
    constexpr double toInchesOfMercury(double Pascal) { return Pascal / 3386.39; }
    constexpr double fromInchesOfMercury(double InHg) { return InHg / toInchesOfMercury(1.0); }

    // Millimeters of Mercury
    constexpr double toMillimetersOfMercury(double Pascal) { return Pascal / 133.322387415; }
    constexpr double fromMillimetersOfMercury(double MmHg) { return MmHg / toMillimetersOfMercury(1.0); }

    // Bar
    constexpr double toBar(double Pascal) { return Pascal / 100000.0; }
    constexpr double fromBar(double Bar) { return Bar / toBar(1.0); }

    /// Force (Newtons)
    // Pounds of Force
    constexpr double fromPoundsOfForce(double PoundsOfForce) { return PoundsOfForce * fromPounds(1.0) * Constants::g0; }
    constexpr double toPoundsOfForce(double Newtons) { return Newtons / fromPoundsOfForce(1.0); }

    // Poundals
    constexpr double fromPoundals(double Poundals) { return Poundals * fromPounds(1.0) * fromFeet(1.0); }
    constexpr double toPoundals(double Newtons) { return Newtons / fromPoundals(1.0); }

    /// Power (Watts)
    // Kilowatts
    constexpr double toKilowatts(double Watts) { return Watts / 1000.0; }
    constexpr double fromKilowatts(double Kilowatts) { return Kilowatts / toKilowatts(1.0); }

    // Horse Power
    constexpr double fromHorsePower(double HorsePower) { return HorsePower * 550.0 * fromPounds(1.0) * fromFeet(1.0) * Constants::g0; }
    constexpr double toHorsePower(double Newtons) { return Newtons / fromHorsePower(1.0); }

    /// Engine Coefficients
    constexpr double toPoundsOfForcePerKnot(double NewtonsPerMeterPerSecond) { return NewtonsPerMeterPerSecond * toPoundsOfForce(1.0) / toKnots(1.0); }
    constexpr double fromPoundsOfForcePerKnot(double PoundsOfForcePerKnot) { return PoundsOfForcePerKnot * fromPoundsOfForce(1.0) / fromKnots(1.0); }

    constexpr double toPoundsOfForcePerFoot(double NewtonsPerMeter) { return NewtonsPerMeter * toPoundsOfForce(1.0) / toFeet(1.0); }
    constexpr double fromPoundsOfForcePerFoot(double PoundsOfForcePerFoot) { return PoundsOfForcePerFoot * fromPoundsOfForce(1.0) / fromFeet(1.0); }

    constexpr double toPoundsOfForcePerFoot2(double NewtonsPerMeter2) { return NewtonsPerMeter2 * toPoundsOfForce(1.0) / (toFeet(1.0) * toFeet(1.0)); }
    constexpr double fromPoundsOfForcePerFoot2(double PoundsOfForcePerFeet2) { return PoundsOfForcePerFeet2 * fromPoundsOfForce(1.0) / (fromFeet(1.0) * fromFeet(1.0)); }

    constexpr double toPoundsOfForcePerCelsius(double NewtonsPerKelvin) { return NewtonsPerKelvin * toPoundsOfForce(1.0); }
    constexpr double fromPoundsOfForcePerCelsius(double PoundsOfForcePerCelsius) { return PoundsOfForcePerCelsius * fromPoundsOfForce(1.0); }

    /// Aerodynamic Coefficients
    constexpr double toFeetPerPoundOfForce(double MetersPerNewton) { return MetersPerNewton * toFeet(1.0) / toPoundsOfForce(1.0); }
    constexpr double fromFeetPerPoundOfForce(double FeetPerPoundOfForce) { return FeetPerPoundOfForce * fromFeet(1.0) / fromPoundsOfForce(1.0); }

    inline double toKnotsPerPoundOfForceSqrt(double MetersPerSecondPerSqrtNewton) { return MetersPerSecondPerSqrtNewton * toKnots(1.0) / std::sqrt(toPoundsOfForce(1.0)); }
    inline double fromKnotsPerPoundOfForceSqrt(double KnotsPerSqrtPoundOfForce) { return KnotsPerSqrtPoundOfForce * fromKnots(1.0) / std::sqrt(fromPoundsOfForce(1.0)); }
}
