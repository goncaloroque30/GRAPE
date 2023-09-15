// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "PerformanceOutput.h"

#include "Base/CoordinateSystem.h"
#include "Base/Math.h"
#include "Operation/Flight.h"

namespace GRAPE {
    std::pair<PerformanceOutput::Point&, bool> PerformanceOutput::addPoint(PointOrigin PtOrigin, FlightPhase FlPhase, double CumulativeGroundDistance, double Longitude, double Latitude, double AltitudeMsl, double TrueAirspeed, double Groundspeed, double Thrust, double BankAngle, double FuelFlowPerEng) {
        auto [NewElement, emplaced] = m_Output.try_emplace(CumulativeGroundDistance, PtOrigin, FlPhase, Longitude, Latitude, AltitudeMsl, TrueAirspeed, Groundspeed, Thrust, BankAngle, FuelFlowPerEng);
        return { NewElement->second, emplaced };
    }

    void PerformanceOutput::clear() {
        m_Output.clear();
    }

    // If the speed difference between two adjacent points is higher than speed delta minimum, points are added between those two adjacent points with equal speed deltas which are at maximum speed delta minimum
    void PerformanceOutput::speedSegmentation(const CoordinateSystem& Cs, double SpeedDeltaMinimum) {
        GRAPE_ASSERT(SpeedDeltaMinimum > 0);
        for (auto it = m_Output.begin(); it != std::prev(m_Output.end(), 1);)
        {
            const auto& [CumGroundDist1, P1] = *it;
            it = std::next(it, 1);
            const auto& [CumGroundDist2, P2] = *it;

            const double speedDelta = P2.Groundspeed - P1.Groundspeed;
            const double speedDeltaAbs = std::abs(speedDelta);

            if (speedDeltaAbs > SpeedDeltaMinimum)
            {
                const double distanceDelta = std::abs(CumGroundDist1 - CumGroundDist2);

                FlightPhase flPhase = P1.FlPhase;
                if (P1.FlPhase == FlightPhase::TakeoffRoll && P2.FlPhase == FlightPhase::InitialClimb)
                    flPhase = FlightPhase::InitialClimb;

                const int segCount = 1 + static_cast<int>(speedDeltaAbs / SpeedDeltaMinimum); // Truncates
                const double speedIncrement = speedDelta / segCount;
                const double segTime = distanceDelta / std::midpoint(P1.Groundspeed, P2.Groundspeed) / segCount;
                const double hdg = Cs.heading(P1.Longitude, P1.Latitude, P2.Longitude, P2.Latitude);
                double cumGroundDistanceP1ToP2 = 0.0;

                for (int i = 1; i <= segCount - 1; i++) // First and last point already in the output
                {
                    const double segLength = (P1.Groundspeed + speedIncrement * (i - 0.5)) * segTime;
                    cumGroundDistanceP1ToP2 += segLength;
                    const double iFactor = cumGroundDistanceP1ToP2 / distanceDelta;
                    const double newAltMsl = distanceInterpolation(P1.AltitudeMsl, P2.AltitudeMsl, iFactor);
                    const double newTrueAirspeed = timeInterpolation(P1.TrueAirspeed, P2.TrueAirspeed, iFactor);
                    const double newGroundSpeed = timeInterpolation(P1.Groundspeed, P2.Groundspeed, iFactor);
                    const double newCorrNetThrustPerEng = timeInterpolation(P1.CorrNetThrustPerEng, P2.CorrNetThrustPerEng, iFactor);
                    const double newBankAngle = distanceInterpolation(P1.BankAngle, P2.BankAngle, iFactor);
                    const double newTotalFuelFlow = timeInterpolation(P1.FuelFlowPerEng, P2.FuelFlowPerEng, iFactor);

                    auto [newLon, newLat] = Cs.point(P1.Longitude, P1.Latitude, cumGroundDistanceP1ToP2, hdg);

                    addPoint(PointOrigin::SpeedSegmentation, flPhase, CumGroundDist1 + cumGroundDistanceP1ToP2, newLon, newLat, newAltMsl, newTrueAirspeed, newGroundSpeed, newCorrNetThrustPerEng, newBankAngle, newTotalFuelFlow);
                }
            }
        }
    }

    // Deletes points that are separated by less than ground distance maximum
    // Returns the number of deleted points
    std::size_t PerformanceOutput::groundDistanceFilter(double GroundDistanceMaximum) {
        GRAPE_ASSERT(GroundDistanceMaximum >= 0.0);

        std::size_t deletedCount = 0;
        for (auto it = m_Output.begin(); it != std::prev(m_Output.end(), 1); ++it)
        {
            if (std::abs(it->first - std::next(it, 1)->first) < GroundDistanceMaximum)
            {
                ++deletedCount;
                if (m_Output.erase(std::next(it, 1)) == m_Output.end())
                    break;
            }
        }
        return deletedCount;
    }
}
