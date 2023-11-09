// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Atmosphere.h"
#include "AtmosphericConstants.h"

namespace GRAPE {
    /**
    * @param Value The value to be rounded.
    * @param Decimals The number of decimals to round to.
    * @return The rounded value.
    */
    inline double round(double Value, int Decimals = 0) {
        const double factor = std::pow(10.0, Decimals);
        return std::round(Value * factor) / factor;
    }

    /**
    * @param Heading Any number (]-Inf, Inf[) representing an heading.
    * @return Heading converted to the range [0.0, 360.0[
    */
    inline double normalizeHeading(double Heading) {
        double normalizedHdg = std::fmod(Heading, 360.0);
        if (normalizedHdg < 0.0)
            normalizedHdg += 360.0;
        return normalizedHdg;
    }

    /**
    * @param Heading1 Any number (]-Inf, Inf[) representing an heading.
    * @param Heading2 Any number (]-Inf, Inf[) representing an heading.
    * @return The heading difference in the range [0.0, 360.0[ between Heading1 and Heading2 (always positive)
    */
    inline double headingDifference(double Heading1, double Heading2) {
        const double diff = std::fmod(Heading1 - Heading2 + 3600, 360);
        return diff <= 180 ? diff : 360 - diff;
    }

    /**
    * @param CurrentHeading Turn initial heading in the range [0, 360]
    * @param NewHeading Turn final heading in the range [0, 360]
    * @return 1 if turn to the right (CurrentHeading + X = NewHeading) | -1 if turn to the left (CurrentHeading - X = NewHeading)
    */
    constexpr int turnDirection(double CurrentHeading, double NewHeading) {
        const double diff = NewHeading - CurrentHeading;
        return diff > 180 ? -1 : diff > 0 ? 1 : diff >= -180.0 ? -1 : 1;
    }

    /**
    * @return A + T * (B - A).
    */
    inline double distanceInterpolation(double A, double B, double T) { return std::lerp(A, B, T); }

    /**
    * @return The result of \f$\sqrt{A^2 + T \times (B^2 - A^2)}\f$.
    */
    inline double timeInterpolation(double A, double B, double T) { return std::sqrt(std::lerp(A * A, B * B, T)); }

    /**
    * @return Positive value if climbing and positive angle or descending and negative angle, Inf if Angle is 0, negative value otherwise.
    */
    inline double groundDistance(double StartAltitude, double EndAltitude, double Angle) { return (EndAltitude - StartAltitude) / std::tan(toRadians(Angle)); }

    /**
    * @brief Formula B-6 from Doc.29 Volume 2 Appendix B
    * @return The result of \f$V_T \cdot \sqrt{V_C}\f$.
    */
    inline double calibratedAirspeed(double TrueAirspeed, double AltitudeMsl, const Atmosphere& Atm) { return TrueAirspeed * std::sqrt(Atm.densityRatio(AltitudeMsl)); }

    /**
    * @brief Formula B-6 from Doc.29 Volume 2 Appendix B
    * @return The result of \f$V_C / \sqrt{Sigma}\f$.
    */
    inline double trueAirspeed(double CalibratedAirspeed, double AltitudeMsl, const Atmosphere& Atm) { return CalibratedAirspeed / std::sqrt(Atm.densityRatio(AltitudeMsl)); }

    /**
    * @return The speed over ground, which decreases if the aircraft is in a climb or descend.
    */
    inline double groundSpeed(double TrueAirspeed, double Angle, double Headwind) { return TrueAirspeed * std::cos(toRadians(Angle)) - Headwind; }

    /**
    * @brief Speed of sound in the air (ideal gas)
    *
    * @return f$ SoundSpeed = \sqrt{1.4 \cdot R_{Air} \cdot Temperature}\f$
    **/
    inline double soundSpeed(double Temperature) { return std::sqrt(1.4 * Constants::RAir * Temperature); }

    /**
    * @brief Speed of sound in the air (ideal gas). Calculates temperature at AltitudeMsl using the Atm and calls soundSpeed(double)
    *
    **/
    inline double soundSpeed(double AltitudeMsl, const Atmosphere& Atm) { return soundSpeed(Atm.temperature(AltitudeMsl)); }

    /**
    * @return The result of \f$ TAS / SoundSpeed\f$ where \f$ SoundSpeed = \sqrt{1.4 \cdot R_{Air} \cdot Temperature}\f$.
    */
    inline double machNumber(double TrueAirspeed, double Temperature) { return TrueAirspeed / soundSpeed(Temperature); }

    /**
    * @return The result of \f$ TAS / SoundSpeed\f$ where \f$ SoundSpeed = \sqrt{1.4 \cdot R_{Air} \cdot Temperature}\f$.
    */
    inline double machNumber(double TrueAirspeed, double AltitudeMsl, const Atmosphere& Atm) { return TrueAirspeed / soundSpeed(AltitudeMsl, Atm); }

    /**
    * @brief Formula B-8 from Doc.29 Volume 2 Appendix B
    * @return The result of \f$\tan^-1(GS^1 / (r * g))\f$ in degrees.
    */
    inline double bankAngle(double Groundspeed, double TurnRadius) { return fromRadians(std::atan2(Groundspeed * Groundspeed, TurnRadius * Constants::g0)); }
}
