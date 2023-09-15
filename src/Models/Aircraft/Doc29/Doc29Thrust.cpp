// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "Doc29Thrust.h"

#include "Base/Atmosphere.h"
#include "Base/Math.h"

namespace GRAPE {
    void Doc29ThrustRatingPropeller::Coefficients::setEfficiency(double PeIn) {
        if (!(PeIn > 0.0 && PeIn <= 1.0))
            throw GrapeException("Propeller efficiency must be higher than 0 and not higher than 1.");
        Pe = PeIn;
    }

    void Doc29ThrustRatingPropeller::Coefficients::setPower(double PpIn) {
        if (!(PpIn > 0.0))
            throw GrapeException("Propeller net propulsive power must be higher than 0.");
        Pp = PpIn;
    }

    std::pair<Doc29ThrustRatingPropeller::Coefficients&, bool> Doc29ThrustRatingPropeller::addCoefficients(Rating ThrustRating, double Efficiency, double Power) {
        Coefficients coeffs;
        coeffs.setEfficiency(Efficiency);
        coeffs.setPower(Power);

        return addCoefficients(ThrustRating, coeffs);
    }

    std::pair<Doc29ThrustRatingPropeller::Coefficients&, bool> Doc29ThrustRatingPropeller::addCoefficients(Rating ThrustRating, const Coefficients& CoeffsIn) {
        if (!(ThrustRating == Rating::MaximumTakeoff || ThrustRating == Rating::MaximumClimb))
            throw GrapeException(std::format("Thrust rating {} not supported by propeller thrust.", Ratings.toString(ThrustRating)));

        return Coeffs.add(ThrustRating, CoeffsIn);
    }

    double Doc29ThrustRating::calculate(Rating ThrustRating, double CalibratedAirspeed, double Altitude, double EngineBreakpointTemperature, const Atmosphere& Atm) const {
        GRAPE_ASSERT(Coeffs.contains(ThrustRating));

        const double temp = Atm.temperature(Altitude);
        if (temp <= EngineBreakpointTemperature)
            return thrust(ThrustRating, CalibratedAirspeed, Altitude, temp);

        switch (ThrustRating)
        {
        case Rating::MaximumTakeoff:
            {
                if (Coeffs.contains(Rating::MaximumTakeoffHighTemperature))
                    return thrust(Rating::MaximumTakeoffHighTemperature, CalibratedAirspeed, Altitude, temp);

                return thrustHighTemperature(Rating::MaximumTakeoff, CalibratedAirspeed, temp, EngineBreakpointTemperature);
            }
        case Rating::MaximumClimb:
            {
                if (Coeffs.contains(Rating::MaximumClimbHighTemperature))
                    return thrust(Rating::MaximumClimbHighTemperature, CalibratedAirspeed, Altitude, temp);

                return thrustHighTemperature(Rating::MaximumClimb, CalibratedAirspeed, temp, EngineBreakpointTemperature);
            }
        case Rating::Idle:
            {
                if (Coeffs.contains(Rating::IdleHighTemperature))
                    return thrust(Rating::IdleHighTemperature, CalibratedAirspeed, Altitude, temp);
                return thrustHighTemperature(Rating::Idle, CalibratedAirspeed, temp, EngineBreakpointTemperature);
            }
        default: return thrust(ThrustRating, CalibratedAirspeed, Altitude, temp);
        }
    }

    double Doc29ThrustRating::thrust(Rating ThrustRating, double CalibratedAirspeed, double Altitude, double Temperature) const {
        const auto& [E, F, Ga, Gb, H] = Coeffs.at(ThrustRating);
        return E + F * CalibratedAirspeed + Ga * Altitude + Gb * Altitude * Altitude + H * toCelsius(Temperature);
    }

    double Doc29ThrustRating::thrustHighTemperature(Rating ThrustRating, double CalibratedAirspeed, double Temperature, double EngineBreakpointTemperature) const {
        const auto& [E, F, Ga, Gb, H] = Coeffs.at(ThrustRating);
        return F * CalibratedAirspeed + (E + H * toCelsius(Temperature)) * (1 - 0.006 * toCelsius(Temperature)) / (1 - 0.006 * toCelsius(EngineBreakpointTemperature));
    }

    double Doc29ThrustRatingPropeller::calculate(Rating ThrustRating, double CalibratedAirspeed, double Altitude, double EngineBreakpointTemperature, const Atmosphere& Atm) const {
        GRAPE_ASSERT(Coeffs.contains(ThrustRating));
        const auto& [Pe, Pp] = Coeffs(ThrustRating);
        return Pe * Pp / (CalibratedAirspeed / std::sqrt(Atm.densityRatio(Altitude))) / Atm.pressureRatio(Altitude);
    }

    void Doc29Thrust::accept(Doc29ThrustVisitor& Vis) { Vis.visitDoc29Thrust(*this); }
    void Doc29Thrust::accept(Doc29ThrustVisitor& Vis) const { Vis.visitDoc29Thrust(*this); }

    void Doc29ThrustRating::accept(Doc29ThrustVisitor& Vis) { Vis.visitDoc29ThrustRating(*this); }
    void Doc29ThrustRating::accept(Doc29ThrustVisitor& Vis) const { Vis.visitDoc29ThrustRating(*this); }

    void Doc29ThrustRatingPropeller::accept(Doc29ThrustVisitor& Vis) { Vis.visitDoc29ThrustPropeller(*this); }
    void Doc29ThrustRatingPropeller::accept(Doc29ThrustVisitor& Vis) const { Vis.visitDoc29ThrustPropeller(*this); }
}
