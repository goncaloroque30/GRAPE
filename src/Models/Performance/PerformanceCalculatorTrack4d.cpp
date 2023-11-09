// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "PerformanceCalculatorTrack4d.h"

namespace GRAPE {
    std::optional<PerformanceOutput> PerformanceCalculatorTrack4d::calculate(const Track4dArrival& Track4dArr) const {
        if (Track4dArr.empty())
        {
            Log::models()->error("Calculating performance output for arrival track 4D '{}'. No performance output generated, operation has no points.", Track4dArr.Name);
            return {};
        }

        if (Track4dArr.size() < static_cast<std::size_t>(m_Spec.Tracks4dMinimumPoints))
        {
            Log::models()->error("Calculating performance output for arrival track 4D '{}'. No performance output generated, operation has {} points, less than specified minimum {}.", Track4dArr.Name, Track4dArr.size(), m_Spec.Tracks4dMinimumPoints);
            return {};
        }

        PerformanceOutput perfOutput;
        const auto& atm = m_Spec.Atmospheres.atmosphere(Track4dArr.Time);


        double cumGroundDist = 0.0;
        for (auto it = Track4dArr.rbegin(); it != Track4dArr.rend(); ++it)
        {
            const auto& pt = *it;

            if (!m_Spec.Tracks4dRecalculateCumulativeGroundDistance)
            {
                cumGroundDist = pt.CumulativeGroundDistance;
            }
            else if (it != Track4dArr.rbegin())
            {
                const auto& prevPt = *std::prev(it);
                cumGroundDist -= m_Spec.CoordSys->distance(pt.Longitude, pt.Latitude, prevPt.Longitude, prevPt.Latitude);
            }

            if (!pointInDistanceLimits(cumGroundDist) || !pointInAltitudeLimits(pt.AltitudeMsl))
                continue;

            double groundspeed = pt.Groundspeed;
            if (m_Spec.Tracks4dRecalculateGroundspeed)
            {
                if (it != std::prev(Track4dArr.rend()))
                {
                    const auto& nextPt = *std::next(it);
                    const double heading = m_Spec.CoordSys->heading(pt.Longitude, pt.Latitude, nextPt.Longitude, nextPt.Latitude);
                    groundspeed = pt.TrueAirspeed - atm.headwind(heading);
                }
                else
                {
                    const auto& prevPt = *std::prev(it);
                    const double headingEnd = m_Spec.CoordSys->headingEnd(prevPt.Longitude, prevPt.Latitude, pt.Longitude, pt.Latitude);
                    groundspeed = pt.TrueAirspeed - atm.headwind(headingEnd);
                }
            }

            auto [newPt, added] = perfOutput.addPoint(PerformanceOutput::PointOrigin::Track4d, pt.FlPhase, cumGroundDist, pt.Longitude, pt.Latitude, pt.AltitudeMsl, pt.TrueAirspeed, groundspeed, pt.CorrNetThrustPerEng, pt.BankAngle, pt.FuelFlowPerEng);

            if (!added)
                Log::models()->warn("Calculating performance output for arrival track 4D '{}'. Point at index {} overlaps previous point and will not be added.", Track4dArr.Name, Track4dArr.size() - std::distance(Track4dArr.rbegin(), it));
        }

        if (m_Spec.Tracks4dRecalculateFuelFlow)
            m_FuelFlow->calculate(Track4dArr, perfOutput);

        // Segmentation and Filtering
        const std::size_t deletedCount = segmentAndFilter(Track4dArr, perfOutput);

        // Check Size
        if (Track4dArr.size() < 2)
        {
            Log::models()->error("Calculating performance output for arrival track 4D '{}'. No performance output generated, operation has less than 2 points after segmenting and filtering.", Track4dArr.Name);
            return {};
        }

        // Warn if deleted
        if (deletedCount)
            Log::models()->info("Calculating performance output for arrival track 4D '{}'. Deleted {} points due to minimum ground distance filtering.", Track4dArr.Name, deletedCount);

        return perfOutput;
    }

    std::optional<PerformanceOutput> PerformanceCalculatorTrack4d::calculate(const Track4dDeparture& Track4dDep) const {
        if (Track4dDep.empty())
        {
            Log::models()->warn("Calculating performance output for departure track 4D '{}'. No performance output generated, operation has no points.", Track4dDep.Name);
            return {};
        }

        if (Track4dDep.size() < static_cast<std::size_t>(m_Spec.Tracks4dMinimumPoints))
        {
            Log::models()->warn("Calculating performance output for departure track 4D '{}'. No performance output generated, operation has {} points, less than specified minimum {}.", Track4dDep.Name, Track4dDep.size(), m_Spec.Tracks4dMinimumPoints);
            return {};
        }

        PerformanceOutput perfOutput;
        const auto& atm = m_Spec.Atmospheres.atmosphere(Track4dDep.Time);


        double cumGroundDist = 0.0;
        for (auto it = Track4dDep.begin(); it != Track4dDep.end(); ++it)
        {
            const auto& pt = *it;

            if (!m_Spec.Tracks4dRecalculateCumulativeGroundDistance)
            {
                cumGroundDist = pt.CumulativeGroundDistance;
            }
            else if (it != Track4dDep.begin())
            {
                const auto& prevPt = *std::prev(it);
                cumGroundDist += m_Spec.CoordSys->distance(pt.Longitude, pt.Latitude, prevPt.Longitude, prevPt.Latitude);
            }

            if (!pointInDistanceLimits(cumGroundDist) || !pointInAltitudeLimits(pt.AltitudeMsl))
                continue;

            double groundspeed = pt.Groundspeed;
            if (m_Spec.Tracks4dRecalculateGroundspeed)
            {
                if (it != std::prev(Track4dDep.end()))
                {
                    const auto& nextPt = *std::next(it);
                    const double heading = m_Spec.CoordSys->heading(pt.Longitude, pt.Latitude, nextPt.Longitude, nextPt.Latitude);
                    groundspeed = pt.TrueAirspeed - atm.headwind(heading);
                }
                else
                {
                    const auto& prevPt = *std::prev(it);
                    const double headingEnd = m_Spec.CoordSys->headingEnd(prevPt.Longitude, prevPt.Latitude, pt.Longitude, pt.Latitude);
                    groundspeed = pt.TrueAirspeed - atm.headwind(headingEnd);
                }
            }

            auto [newPt, added] = perfOutput.addPoint(PerformanceOutput::PointOrigin::Track4d, pt.FlPhase, cumGroundDist, pt.Longitude, pt.Latitude, pt.AltitudeMsl, pt.TrueAirspeed, groundspeed, pt.CorrNetThrustPerEng, pt.BankAngle, pt.FuelFlowPerEng);

            if (!added)
                Log::models()->warn("Calculating performance output for departure track 4D '{}'. Point at index {} overlaps previous point and will not be added.", Track4dDep.Name, std::distance(Track4dDep.begin(), it) + 1);
        }

        if (m_Spec.Tracks4dRecalculateFuelFlow)
            m_FuelFlow->calculate(Track4dDep, perfOutput);

        // Segmentation and Filtering
        const std::size_t deletedCount = segmentAndFilter(Track4dDep, perfOutput);

        // Check Size
        if (Track4dDep.size() < 2)
        {
            Log::models()->error("Calculating performance output for departure track 4D '{}'. No performance output generated, operation has less than 2 points after segmenting and filtering.", Track4dDep.Name);
            return {};
        }

        // Warn if deleted
        if (deletedCount)
            Log::models()->info("Calculating performance output for departure track 4D '{}'. Deleted {} points due to minimum ground distance filtering.", Track4dDep.Name, deletedCount);

        return perfOutput;
    }
}
