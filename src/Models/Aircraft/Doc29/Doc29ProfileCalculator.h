// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Doc29Profile.h"

#include "Airport/RouteOutput.h"
#include "Airport/Runway.h"
#include "Aircraft/Aircraft.h"
#include "Base/Base.h"
#include "Performance/ProfileOutput.h"

namespace GRAPE {
    /**
    * @brief Visitor class to calculate the ProfileOutput of a Doc29ProfileArrival.
    */
    struct Doc29ProfileArrivalCalculator : Doc29ProfileArrivalVisitor {
    public:
        Doc29ProfileArrivalCalculator(const CoordinateSystem& CsIn, const Atmosphere& AtmIn, const Aircraft& AcftIn, const Runway& RwyIn, const RouteOutput& RteOutputIn, double WeightIn);

        std::optional<ProfileOutput> calculate(const Doc29ProfileArrival& Prof);

        void visitDoc29ProfileArrivalPoints(const Doc29ProfileArrivalPoints& Profile) override;
        void visitDoc29ProfileArrivalProcedural(const Doc29ProfileArrivalProcedural& Profile) override;

        const CoordinateSystem& Cs;
        const Atmosphere& Atm;
        const Aircraft& Acft;
        const Runway& Rwy;
        const RouteOutput& RteOutput;

        double Weight = Constants::NaN;

        ProfileOutput ProfOutput;
    private:
        // Procedural steps helper functions
        void addLandingStep(const Doc29ProfileArrivalProcedural& Profile);
        void addGroundSteps(const Doc29ProfileArrivalProcedural& Profile);
        void addAirSteps(const Doc29ProfileArrivalProcedural& Profile);
        [[nodiscard]] double forceBalanceThrust(double AltitudeMsl, double R, double Angle, double Acceleration) const;
        [[nodiscard]] double acceleration(double V1, double V2, double Angle, double GroundDistance) const;
    };

    /**
    * @brief Visitor class to calculate the ProfileOutput of a Doc29ProfileDeparture.
    */
    struct Doc29ProfileDepartureCalculator : Doc29ProfileDepartureVisitor {
    public:
        Doc29ProfileDepartureCalculator(const CoordinateSystem& CsIn, const Atmosphere& AtmIn, const Aircraft& AcftIn, const Runway& RwyIn, const RouteOutput& RteOutputIn, double WeightIn, double ThrustPercentageTakeoffIn, double ThrustPercentageClimbIn);

        std::optional<ProfileOutput> calculate(const Doc29ProfileDeparture& Prof);

        void visitDoc29ProfileDeparturePoints(const Doc29ProfileDeparturePoints& Profile) override;
        void visitDoc29ProfileDepartureProcedural(const Doc29ProfileDepartureProcedural& Profile) override;

        const CoordinateSystem& Cs;
        const Atmosphere& Atm;
        const Aircraft& Acft;
        const Runway& Rwy;
        const RouteOutput& RteOutput;

        double Weight = Constants::NaN;
        double ThrustPercentageTakeoff = Constants::NaN;
        double ThrustPercentageClimb = Constants::NaN;

        ProfileOutput ProfOutput;
    };
}
