// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Doc29Profile.h"

#include "Performance/ProfileOutput.h"

namespace GRAPE {
    class CoordinateSystem;
    class Atmosphere;
    class Doc29Aircraft;
    class Runway;
    class RouteOutput;

    /**
    * @brief Visitor class to calculate the ProfileOutput of a Doc29ProfileArrival.
    */
    struct Doc29ProfileArrivalCalculator : Doc29ProfileArrivalVisitor {
    public:
        Doc29ProfileArrivalCalculator(const CoordinateSystem& CsIn, const Atmosphere& AtmIn, const Doc29Aircraft& Doc29Acft, const Runway& RwyIn, const RouteOutput& RteOutputIn, double WeightIn, double EngineCountIn);

        std::optional<ProfileOutput> calculate(const Doc29ProfileArrival& Prof);

        void visitDoc29ProfileArrivalPoints(const Doc29ProfileArrivalPoints& Profile) override;
        void visitDoc29ProfileArrivalProcedural(const Doc29ProfileArrivalProcedural& Profile) override;

        const CoordinateSystem& Cs;
        const Atmosphere& Atm;
        const Doc29Aircraft& Doc29Acft;
        const Runway& Rwy;
        const RouteOutput& RteOutput;

        double Weight = Constants::NaN;
        double EngineCount = Constants::NaN;

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
        Doc29ProfileDepartureCalculator(const CoordinateSystem& CsIn, const Atmosphere& AtmIn, const Doc29Aircraft& Doc29PerfIn, const Runway& RwyIn, const RouteOutput& RteOutputIn, double WeightIn, double EngineCountIn, double ThrustPercentageTakeoffIn, double ThrustPercentageClimbIn);

        std::optional<ProfileOutput> calculate(const Doc29ProfileDeparture& Prof);

        void visitDoc29ProfileDeparturePoints(const Doc29ProfileDeparturePoints& Profile) override;
        void visitDoc29ProfileDepartureProcedural(const Doc29ProfileDepartureProcedural& Profile) override;

        const CoordinateSystem& Cs;
        const Atmosphere& Atm;
        const Doc29Aircraft& Doc29Acft;
        const Runway& Rwy;
        const RouteOutput& RteOutput;

        double Weight = Constants::NaN;
        double EngineCount = Constants::NaN;
        double ThrustPercentageTakeoff = Constants::NaN;
        double ThrustPercentageClimb = Constants::NaN;

        ProfileOutput ProfOutput;
    };
}
