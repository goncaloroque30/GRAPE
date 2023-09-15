// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "ProfileOutput.h"

#include "Airport/RouteOutput.h"
#include "Base/Math.h"

namespace GRAPE {
    void ProfileOutput::addPoint(double CumulativeGroundDistance, double AltitudeMsl, double TrueAirspeed, double Groundspeed, double Thrust, double BankAngle, FlightPhase FlPhase) {
        m_Profile.try_emplace(CumulativeGroundDistance, AltitudeMsl, TrueAirspeed, Groundspeed, Thrust, BankAngle, FlPhase);
    }

    void ProfileOutput::recalculateBankAngle(const RouteOutput& Rte) {
        for (auto& [CumGroundDist, Pt] : m_Profile)
            Pt.BankAngle = bankAngle(Pt.Groundspeed, Rte.turnRadius(CumGroundDist));
    }

    ProfileOutput::Point ProfileOutput::interpolate(double CumulativeGroundDistance) const {
        GRAPE_ASSERT(!empty());

        const auto nextNode = m_Profile.lower_bound(CumulativeGroundDistance);

        // Point is after profile end
        if (nextNode == m_Profile.end())
        {
            if (size() == 1)
                return m_Profile.begin()->second; // Return single point in the container

            // Extrapolate altitude from two last points, keep all other variables equal to the last point
            const auto& [p1CumDist, p1] = *std::prev(nextNode, 2);
            const auto& [p2CumDist, p2] = *std::prev(nextNode, 1);
            const double iFactor = (CumulativeGroundDistance - p1CumDist) / (p2CumDist - p1CumDist);
            const double altMsl = distanceInterpolation(p1.AltitudeMsl, p2.AltitudeMsl, iFactor);
            return Point{ altMsl, p2.TrueAirspeed, p2.Groundspeed, p2.Thrust, p2.BankAngle, p2.FlPhase };
        }

        // Point already in the container
        const auto& [NextCumGroundDist, NextPt] = *nextNode;
        if (std::abs(NextCumGroundDist - CumulativeGroundDistance) < Constants::Precision)
            return NextPt;

        // Point before profile start
        if (nextNode == m_Profile.begin())
        {
            if (size() == 1)
                return m_Profile.begin()->second; // Return single point in the container

            // Extrapolate altitude from first two points, keep all other variables equal to the first point
            const auto& [p1CumDist, p1] = *nextNode;
            const auto& [p2CumDist, p2] = *std::next(nextNode);
            const double iFactor = (CumulativeGroundDistance - p1CumDist) / (p2CumDist - p1CumDist);
            const double altMsl = distanceInterpolation(p1.AltitudeMsl, p2.AltitudeMsl, iFactor);
            return Point{ altMsl, p1.TrueAirspeed, p1.Groundspeed, p1.Thrust, p1.BankAngle, p1.FlPhase };
        }

        // Point between defined route
        const auto& [PrevCumGroundDist, PrevPt] = *std::prev(nextNode);
        const double iFactor = (CumulativeGroundDistance - PrevCumGroundDist) / (NextCumGroundDist - PrevCumGroundDist);
        const double altMsl = distanceInterpolation(PrevPt.AltitudeMsl, NextPt.AltitudeMsl, iFactor);
        const double trueAirspeed = timeInterpolation(PrevPt.TrueAirspeed, NextPt.TrueAirspeed, iFactor);
        const double groundSpeed = timeInterpolation(PrevPt.Groundspeed, NextPt.Groundspeed, iFactor);
        const double thrust = timeInterpolation(PrevPt.Thrust, NextPt.Thrust, iFactor);
        const double bankAngl = distanceInterpolation(PrevPt.BankAngle, NextPt.BankAngle, iFactor);
        return Point{ altMsl, trueAirspeed, groundSpeed, thrust, bankAngl, PrevPt.FlPhase };
    }
}
