// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "PerformanceCalculatorDoc29.h"

#include "Aircraft/Doc29/Doc29ProfileCalculator.h"

namespace GRAPE {
    namespace {
        constexpr std::array Doc29DefaultHeights = { 18.9, 41.5, 68.3, 102.1, 147.5, 214.9, 334.9, 609.6, 1289.6 };
    }

    PerformanceCalculatorDoc29::PerformanceCalculatorDoc29(const PerformanceSpecification& Spec) : PerformanceCalculator(Spec) {}

    std::optional<PerformanceOutput> PerformanceCalculatorDoc29::calculate(const FlightArrival& FlightArr, const RouteOutput& RteOutput) const {
        PerformanceOutput perfOutput;

        Doc29ProfileArrivalCalculator profCalculator(*m_Spec.CoordSys, m_Spec.Atmospheres.atmosphere(FlightArr.Time), FlightArr.aircraft(), FlightArr.route().parentRunway(), RteOutput, FlightArr.Weight);
        const auto profOutputOpt = profCalculator.calculate(*FlightArr.Doc29Prof);
        if (!profOutputOpt)
        {
            Log::models()->error("Calculating performance output for arrival flight '{}' with Doc29 profile '{}'. No performance output generated, profile generated no points.", FlightArr.Name, FlightArr.Doc29Prof->Name);
            return {};
        }

        const ProfileOutput& profOutput = *profOutputOpt;

        // Add route points
        for (const auto& [cumGroundDist, rtePt] : RteOutput)
        {
            if (!pointInDistanceLimits(cumGroundDist))
                continue;

            const auto [altitudeMsl, trueAirspeed, groundspeed, thrust, unusedBankAngle, flPhase] = profOutput.interpolate(cumGroundDist);

            if (!pointInAltitudeLimits(altitudeMsl))
                continue;

            // Recalculate bank angle as route output provides exact turn radius, better than interpolated bank angle!
            double bankAngl = bankAngle(groundspeed, rtePt.Radius);
            if (rtePt.Dir == RouteOutput::Direction::RightTurn)
                bankAngl = -bankAngl;

            perfOutput.addPoint(PerformanceOutput::PointOrigin::Route, flPhase, cumGroundDist, rtePt.Longitude, rtePt.Latitude, altitudeMsl, trueAirspeed, groundspeed, thrust, bankAngl);
        }

        // Add profile points
        for (const auto& [cumGroundDist, profPt] : profOutput)
        {
            if (!pointInDistanceLimits(cumGroundDist) || !pointInAltitudeLimits(profPt.AltitudeMsl))
                continue;

            const auto [longitude, latitude, heading, radius, dir] = RteOutput.interpolate(*m_Spec.CoordSys, cumGroundDist);
            const double bankAngl = dir == RouteOutput::Direction::RightTurn ? -profPt.BankAngle : profPt.BankAngle;

            auto [perfPt, added] = perfOutput.addPoint(PerformanceOutput::PointOrigin::Profile, profPt.FlPhase, cumGroundDist, longitude, latitude, profPt.AltitudeMsl, profPt.TrueAirspeed, profPt.Groundspeed, profPt.Thrust, bankAngl);
            if (!added)
                perfPt.PtOrigin = PerformanceOutput::PointOrigin::RouteAndProfile;
        }

        // Doc29 Segmentation
        if (m_Spec.FlightsDoc29Segmentation)
        {
            const auto elev = FlightArr.route().parentRunway().Elevation;
            for (auto it = std::next(profOutput.rbegin()); it != profOutput.rend(); ++it)
            {
                auto& [p2CumDist, p2] = *it;
                double p2AltitudeAfe = p2.AltitudeMsl - elev;

                // Find the closest default height
                auto closestHeightIt = std::ranges::find_if(Doc29DefaultHeights.begin(), Doc29DefaultHeights.end(), [&](const double Height) {
                    return p2AltitudeAfe <= Height;
                    });

                if (closestHeightIt == Doc29DefaultHeights.end() || // Point altitude higher than all default heights
                    (closestHeightIt != Doc29DefaultHeights.begin() && std::abs(p2AltitudeAfe - *closestHeightIt) > std::abs(p2AltitudeAfe - *std::prev(closestHeightIt))) // Closest height was the previous one
                    )
                    --closestHeightIt;

                auto& [p1CumDist, p1] = *std::prev(it);
                const double slope = (p2.AltitudeMsl - p1.AltitudeMsl) / (p2CumDist - p1CumDist);
                const double b = p2.AltitudeMsl - slope * p2CumDist;
                double originalAltitudeAfe = p2AltitudeAfe;
                const double normalizingAltitude = *closestHeightIt;

                // Interpolation end flag
                bool end = false;
                if (std::abs(p2AltitudeAfe - Doc29DefaultHeights.back()) < Constants::Precision)
                {
                    end = true;
                }
                else if (p2AltitudeAfe > Doc29DefaultHeights.back())
                {
                    end = true;
                    originalAltitudeAfe = Doc29DefaultHeights.back();
                    ++closestHeightIt;
                }

                // Add points
                for (const auto height : std::ranges::subrange(Doc29DefaultHeights.begin(), closestHeightIt))
                {
                    const double newAltMsl = originalAltitudeAfe * height / normalizingAltitude + elev;
                    if (newAltMsl <= p1.AltitudeMsl)
                        continue;
                    const double newCumDist = (newAltMsl - b) / slope;
                    if (!pointInDistanceLimits(newCumDist))
                        continue;
                    const auto rtePt = RteOutput.interpolate(*m_Spec.CoordSys, newCumDist);
                    const auto profPt = profOutput.interpolate(newCumDist);
                    if (!pointInAltitudeLimits(profPt.AltitudeMsl))
                        continue;

                    // Recalculate bank angle as route output provides exact turn radius, better than interpolated bank angle!
                    double bankAngl = bankAngle(profPt.Groundspeed, rtePt.Radius);
                    if (rtePt.Dir == RouteOutput::Direction::RightTurn)
                        bankAngl = -bankAngl;

                    perfOutput.addPoint(PerformanceOutput::PointOrigin::Doc29FinalApproachSegmentation, profPt.FlPhase, newCumDist, rtePt.Longitude, rtePt.Latitude, profPt.AltitudeMsl, profPt.TrueAirspeed, profPt.Groundspeed, profPt.Thrust, profPt.BankAngle);
                }

                if (end)
                    break;
            }
        }

        // Fuel Flow
        m_FuelFlow->calculate(FlightArr, perfOutput);

        // Segmentation and Filtering
        const std::size_t deletedCount = segmentAndFilter(FlightArr, perfOutput);

        // Check Size
        if (perfOutput.size() < 2)
        {
            Log::models()->error("Calculating performance output for arrival flight '{}'. No performance output generated, operation has less than 2 points after segmenting and filtering.", FlightArr.Name);
            return {};
        }

        // Warn if deleted
        if (deletedCount)
            Log::models()->info("Calculating performance output for arrival flight '{}'. Deleted {} points due to minimum ground distance filtering.", FlightArr.Name, deletedCount);

        return perfOutput;
    }

    std::optional<PerformanceOutput> PerformanceCalculatorDoc29::calculate(const FlightDeparture& FlightDep, const RouteOutput& RteOutput) const {
        PerformanceOutput perfOutput;

        Doc29ProfileDepartureCalculator profCalculator(*m_Spec.CoordSys, m_Spec.Atmospheres.atmosphere(FlightDep.Time), FlightDep.aircraft(), FlightDep.route().parentRunway(), RteOutput, FlightDep.Weight, FlightDep.ThrustPercentageTakeoff, FlightDep.ThrustPercentageClimb);
        const auto profOutputOpt = profCalculator.calculate(*FlightDep.Doc29Prof);
        if (!profOutputOpt)
        {
            Log::models()->error("Calculating performance output for departure flight '{}' with Doc29 profile '{}'. No performance output generated, profile generated no points.", FlightDep.Name, FlightDep.Doc29Prof->Name);
            return {};
        }
        const ProfileOutput& profOutput = *profOutputOpt;

        // Add route points
        for (const auto& [cumGroundDist, rtePt] : RteOutput)
        {
            if (!pointInDistanceLimits(cumGroundDist))
                continue;

            const auto [altitudeMsl, trueAirspeed, groundspeed, thrust, unusedBankAngle, flPhase] = profOutput.interpolate(cumGroundDist);

            if (!pointInAltitudeLimits(altitudeMsl))
                continue;

            // Recalculate bank angle as route output provides exact turn radius
            double bankAngl = bankAngle(groundspeed, rtePt.Radius);
            if (rtePt.Dir == RouteOutput::Direction::RightTurn)
                bankAngl = -bankAngl;

            perfOutput.addPoint(PerformanceOutput::PointOrigin::Route, flPhase, cumGroundDist, rtePt.Longitude, rtePt.Latitude, altitudeMsl, trueAirspeed, groundspeed, thrust, bankAngl);
        }

        // Add profile points
        for (const auto& [cumGroundDist, profPt] : profOutput)
        {
            if (!pointInDistanceLimits(cumGroundDist) || !pointInAltitudeLimits(profPt.AltitudeMsl))
                continue;

            const auto [longitude, latitude, heading, radius, dir] = RteOutput.interpolate(*m_Spec.CoordSys, cumGroundDist);
            const double bankAngl = dir == RouteOutput::Direction::RightTurn ? -profPt.BankAngle : profPt.BankAngle;

            auto [perfPt, added] = perfOutput.addPoint(PerformanceOutput::PointOrigin::Profile, profPt.FlPhase, cumGroundDist, longitude, latitude, profPt.AltitudeMsl, profPt.TrueAirspeed, profPt.Groundspeed, profPt.Thrust, bankAngl);
            if (!added)
                perfPt.PtOrigin = PerformanceOutput::PointOrigin::RouteAndProfile;
        }

        // Doc29 Segmentation
        if (m_Spec.FlightsDoc29Segmentation)
        {
            const auto elev = FlightDep.route().parentRunway().Elevation;

            // Takeoff Roll
            {
                // Find 1st and last takeoff roll points
                const auto& [cumDist1, p1] = *perfOutput.begin();
                const double* cumDist2Ptr = nullptr;
                const PerformanceOutput::Point* p2Ptr = nullptr;
                for (auto it = perfOutput.begin(); it != std::prev(perfOutput.end()); ++it)
                {
                    const auto& [cumDistNext, nextPt] = *std::next(it);
                    if (nextPt.FlPhase != FlightPhase::TakeoffRoll)
                    {
                        const auto& [cumDist, pt] = it != perfOutput.begin() ? *it : *std::next(it); // In case there is only 1 takeoff roll point
                        cumDist2Ptr = &cumDist;
                        p2Ptr = &pt;
                        break;
                    }
                }
                const auto& cumDist2 = *cumDist2Ptr;
                const auto& p2 = *p2Ptr;

                // Interpolate
                const double distanceDelta = cumDist2 - cumDist1;
                const double speedDelta = p2.Groundspeed - p1.Groundspeed;
                const double speedDeltaAbs = std::abs(speedDelta);
                const int segCount = 1 + static_cast<int>(speedDeltaAbs / 10.0); // Truncates
                const double speedIncrement = speedDelta / segCount;
                const double thrustIncrement = (p2.CorrNetThrustPerEng - p1.CorrNetThrustPerEng) / segCount;
                const double segTime = distanceDelta / std::midpoint(p1.Groundspeed, p2.Groundspeed) / segCount;
                double cumGroundDistanceP1ToP2 = 0.0;

                for (int i = 1; i <= segCount - 1; i++) // First and last point already in the output
                {
                    const double segLength = (p1.Groundspeed + speedIncrement * (i - 0.5)) * segTime;
                    cumGroundDistanceP1ToP2 += segLength;
                    const double newCumDist = cumDist1 + cumGroundDistanceP1ToP2;
                    if (!pointInDistanceLimits(newCumDist))
                        continue;
                    const double iFactor = cumGroundDistanceP1ToP2 / distanceDelta;
                    const double newAltMsl = distanceInterpolation(p1.AltitudeMsl, p2.AltitudeMsl, iFactor);
                    if (!pointInAltitudeLimits(newAltMsl))
                        continue;
                    const double newSpeed = p1.Groundspeed + i * speedIncrement;
                    const double newCorrNetThrustPerEng = p1.CorrNetThrustPerEng + i * thrustIncrement;
                    const auto rtePt = RteOutput.interpolate(*m_Spec.CoordSys, newCumDist);
                    perfOutput.addPoint(PerformanceOutput::PointOrigin::Doc29TakeoffRollSegmentation, p1.FlPhase, newCumDist, rtePt.Longitude, rtePt.Latitude, newAltMsl, newSpeed, newSpeed, newCorrNetThrustPerEng, 0.0);
                }
            }

            // Initial Climb
            {
                for (auto it = std::next(profOutput.begin()); it != profOutput.begin(); ++it)
                {
                    auto& [p2CumDist, p2] = *it;
                    double p2AltitudeAfe = p2.AltitudeMsl - elev;

                    // Find the closest default height
                    auto closestHeightIt = std::ranges::find_if(Doc29DefaultHeights.begin(), Doc29DefaultHeights.end(), [&](const double Height) {
                        return p2AltitudeAfe <= Height;
                        });

                    if (closestHeightIt == Doc29DefaultHeights.end() || // Point altitude higher than all default heights
                        (closestHeightIt != Doc29DefaultHeights.begin() && std::abs(p2AltitudeAfe - *closestHeightIt) > std::abs(p2AltitudeAfe - *std::prev(closestHeightIt))) // Closest height was the previous one
                        )
                        --closestHeightIt;

                    auto& [p1CumDist, p1] = *std::prev(it);
                    const double slope = (p2.AltitudeMsl - p1.AltitudeMsl) / (p2CumDist - p1CumDist);
                    const double b = p2.AltitudeMsl - slope * p2CumDist;
                    double originalAltitudeAfe = p2AltitudeAfe;
                    const double normalizingAltitude = *closestHeightIt;

                    // Interpolation end flag
                    bool end = false;
                    if (std::abs(p2AltitudeAfe - Doc29DefaultHeights.back()) < Constants::Precision)
                    {
                        end = true;
                    }
                    else if (p2AltitudeAfe > Doc29DefaultHeights.back())
                    {
                        end = true;
                        originalAltitudeAfe = Doc29DefaultHeights.back();
                        ++closestHeightIt;
                    }

                    for (const auto height : std::ranges::subrange(Doc29DefaultHeights.begin(), closestHeightIt))
                    {
                        const double newAltMsl = originalAltitudeAfe * height / normalizingAltitude + elev;
                        if (newAltMsl <= p1.AltitudeMsl)
                            continue;
                        const double newCumDist = (newAltMsl - b) / slope;
                        if (!pointInDistanceLimits(newCumDist))
                            continue;
                        const auto rtePt = RteOutput.interpolate(*m_Spec.CoordSys, newCumDist);
                        auto profPt = profOutput.interpolate(newCumDist);
                        if (!pointInAltitudeLimits(profPt.AltitudeMsl))
                            continue;
                        // Recalculate bank angle as route output provides exact turn radius
                        double bankAngl = bankAngle(profPt.Groundspeed, rtePt.Radius);
                        if (rtePt.Dir == RouteOutput::Direction::RightTurn)
                            bankAngl = -bankAngl;

                        // Fix for profiles without airborne point before first interpolation height
                        if (profPt.FlPhase == FlightPhase::TakeoffRoll)
                            profPt.FlPhase = FlightPhase::InitialClimb;

                        perfOutput.addPoint(PerformanceOutput::PointOrigin::Doc29InitialClimbSegmentation, profPt.FlPhase, newCumDist, rtePt.Longitude, rtePt.Latitude, profPt.AltitudeMsl, profPt.TrueAirspeed, profPt.Groundspeed, profPt.Thrust, bankAngl);
                    }

                    if (end)
                        break;
                }
            }
        }

        // Fuel Flow
        m_FuelFlow->calculate(FlightDep, perfOutput);

        // Segmentation and Filtering
        const std::size_t deletedCount = segmentAndFilter(FlightDep, perfOutput);

        // Check Size
        if (perfOutput.size() < 2)
        {
            Log::models()->error("Calculating performance output for departure flight '{}'. No performance output generated, operation has less than 2 points after segmenting and filtering.", FlightDep.Name);
            return {};
        }

        // Warn if deleted
        if (deletedCount)
            Log::models()->info("Calculating performance output for departure flight '{}'. Deleted {} points due to minimum ground distance filtering.", FlightDep.Name, deletedCount);

        return perfOutput;
    }
}
