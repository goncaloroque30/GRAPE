// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "RouteCalculator.h"

#include "Airport.h"
#include "Runway.h"

#include "Base/CoordinateSystem.h"
#include "Base/Math.h"

namespace GRAPE {
    RouteOutput RouteCalculator::calculate(const Route& Rte) {
        m_Output = RouteOutput(Rte.parentRunway());
        Rte.accept(*this);
        return std::move(m_Output);
    }

    void RouteCalculator::visitArrivalSimple(const RouteArrivalSimple& Rte) {
        for (auto [Longitude, Latitude] : std::ranges::reverse_view(Rte.m_Points))
        {
            const auto& [CumulativeGroundDistance, FirstPoint] = m_Output.firstPoint(); // First as cumulative ground distance is negative for arrivals
            auto [groundDist, hdg] = m_Cs.distanceHeading(Longitude, Latitude, FirstPoint.Longitude, FirstPoint.Latitude);

            double hdgChange = headingDifference(hdg, FirstPoint.Heading);
            if (hdgChange > s_WarnHeadingChange)
                Log::models()->warn("Calculating route output of simple arrival route '{}' in runway '{}' in airport '{}'. Point at longitude {:.6f} and latitude {:.6f} changes aircraft heading by {:.0f} (more than {:.0f} degrees).", Rte.Name, Rte.parentRunway().Name, Rte.parentAirport().Name, Longitude, Latitude, hdgChange, s_WarnHeadingChange);

            m_Output.addPoint(CumulativeGroundDistance - groundDist, Longitude, Latitude, hdg);
        }
    }

    void RouteCalculator::visitDepartureSimple(const RouteDepartureSimple& Rte) {
        for (const auto& [Longitude, Latitude] : Rte.m_Points)
        {
            const auto& [CumulativeGroundDistance, LastPoint] = m_Output.lastPoint();
            auto [groundDist, hdg] = m_Cs.distanceHeadingEnd(LastPoint.Longitude, LastPoint.Latitude, Longitude, Latitude);

            double hdgChange = normalizeHeading(hdg - LastPoint.Heading);
            if (hdgChange > s_WarnHeadingChange)
                Log::models()->warn("Calculating route output of simple departure route '{}' in runway '{}' in airport '{}'. Point at longitude {:.6f} and latitude {:.6f} changes aircraft heading by {:.0f} (more than {:.0f} degrees).", Rte.Name, Rte.parentRunway().Name, Rte.parentAirport().Name, Longitude, Latitude, hdgChange, s_WarnHeadingChange);
            m_Output.addPoint(CumulativeGroundDistance + groundDist, Longitude, Latitude, hdg);
        }
        m_Output.recalculateHeadings(m_Cs);
    }

    void RouteCalculator::visitArrivalVectors(const RouteArrivalVectors& Rte) {
        for (const auto& vec : std::ranges::reverse_view(Rte.m_Vectors))
        {
            const auto& [CumulativeGroundDistance, FirstPoint] = m_Output.firstPoint(); // First as cumulative ground distance is negative for arrivals
            std::visit(Overload{
                [&](const RouteTypeVectors::Straight& StraightVec) {
                    auto [lon, lat, hdg] = m_Cs.pointHeadingEnd(FirstPoint.Longitude, FirstPoint.Latitude, StraightVec.Distance, normalizeHeading(FirstPoint.Heading + 180.0)); // Iterating in backwards direction
                    m_Output.addPoint(CumulativeGroundDistance - StraightVec.Distance, lon, lat, normalizeHeading(hdg + 180.0));
                },
                [&](const RouteTypeVectors::Turn& TurnVec) {

                    const int turnDir = TurnVec.TurnDirection == RouteTypeVectors::Turn::Direction::Left ? -1 : 1;
                    const RouteOutput::Direction dir = turnDir == 1 ? RouteOutput::Direction::RightTurn : RouteOutput::Direction::LeftTurn; // Swapped because turnDir refers to the direction from end of turn to start of turn

                    // heading towards TC from 1st point
                    const double hdg = normalizeHeading(FirstPoint.Heading + turnDir * 90.0);

                    // calculate center
                    auto [centerLon, centerLat] = m_Cs.point(FirstPoint.Longitude, FirstPoint.Latitude, TurnVec.TurnRadius, hdg);

                    // heading towards start point of arc (in direction of calculation)
                    const double hdgCenter1 = normalizeHeading(m_Cs.headingEnd(FirstPoint.Longitude, FirstPoint.Latitude, centerLon, centerLat) + 180.0);
                    const double hdgCenter2 = normalizeHeading(hdgCenter1 - turnDir * TurnVec.HeadingChange);

                    // coords of target
                    auto [targetLon, targetLat] = m_Cs.point(centerLon, centerLat, TurnVec.TurnRadius, hdgCenter2);

                    const auto ptCount = static_cast<std::size_t>(TurnVec.HeadingChange / s_ArcInterval); // Cast to std::size_t truncates the difference

                    // Add all points except last
                    for (std::size_t i = 1; i < ptCount; ++i)
                    {
                        const auto& [lCumDist, lPoint] = m_Output.firstPoint();

                        const double hdgDelta = -turnDir * s_ArcInterval * static_cast<double>(i);

                        auto [Longitude, Latitude] = m_Cs.point(centerLon, centerLat, TurnVec.TurnRadius, normalizeHeading(hdgCenter1 + hdgDelta));

                        auto [testDist1, testHdgEnd] = m_Cs.distanceHeadingEnd(lPoint.Longitude, lPoint.Latitude, Longitude, Latitude);

                        m_Output.addPoint(lCumDist - testDist1, Longitude, Latitude, normalizeHeading(testHdgEnd + 180.0), TurnVec.TurnRadius, dir);
                    }

                    // calculate last point and check heading delta
                    if (TurnVec.HeadingChange - s_ArcInterval * static_cast<double>(ptCount) > Constants::AngleThreshold)
                    {
                        const auto& [cumDist, lPt] = m_Output.firstPoint();

                        // Calculate coordinates of next point and distance to end point 
                        auto [testLon, testLat] = m_Cs.point(centerLon, centerLat, TurnVec.TurnRadius, normalizeHeading(hdgCenter1 - turnDir * s_ArcInterval * static_cast<double>(ptCount)));

                        const auto testDist = m_Cs.distance(testLon, testLat, targetLon, targetLat);

                        // If the distance is bigger than distance threshold of 1m, last point in arc to route
                        if (testDist > Constants::DistanceThreshold)
                        {
                            auto [dist, hdgEnd] = m_Cs.distanceHeadingEnd(lPt.Longitude, lPt.Latitude, testLon, testLat);
                            m_Output.addPoint(cumDist - dist, testLon, testLat, normalizeHeading(hdgEnd + 180.0), TurnVec.TurnRadius, dir);

                            const auto targetHdg = normalizeHeading(m_Cs.headingEnd(centerLon, centerLat, targetLon, targetLat) + turnDir * 90.0);
                            m_Output.addPoint(cumDist - dist - testDist, targetLon, targetLat, targetHdg);
                            return;
                        }
                    }

                    const auto& [cumDist, lPt] = m_Output.firstPoint();
                    const auto dist = m_Cs.distance(lPt.Longitude, lPt.Latitude, targetLon, targetLat);
                    const auto targetHdg = normalizeHeading(m_Cs.headingEnd(centerLon, centerLat, targetLon, targetLat) + turnDir * 90.0);
                    m_Output.addPoint(cumDist - dist, targetLon, targetLat, targetHdg);
                },
                [&](const auto&) { GRAPE_ASSERT(false); },
                }, vec);
        }
    }

    void RouteCalculator::visitDepartureVectors(const RouteDepartureVectors& Rte) {
        for (const auto& vec : Rte.m_Vectors)
        {
            const auto& [CumulativeGroundDistance, LastPoint] = m_Output.lastPoint();
            std::visit(Overload{
                [&](const RouteTypeVectors::Straight& StraightVec) {
                    const auto [lon, lat, hdgEnd] = m_Cs.pointHeadingEnd(LastPoint.Longitude, LastPoint.Latitude, StraightVec.Distance, LastPoint.Heading);
                    m_Output.addPoint(CumulativeGroundDistance + StraightVec.Distance, lon, lat, hdgEnd);
                },
                [&](const RouteTypeVectors::Turn& TurnVec) {
                    const int turnDir = TurnVec.TurnDirection == RouteTypeVectors::Turn::Direction::Left ? -1 : 1;
                    const RouteOutput::Direction dir = turnDir == 1 ? RouteOutput::Direction::RightTurn : RouteOutput::Direction::LeftTurn;

                    // Center coordinates and start heading from center to turn start
                    auto [centerLon, centerLat, centerHeadingEnd] = m_Cs.pointHeadingEnd(LastPoint.Longitude, LastPoint.Latitude, TurnVec.TurnRadius, normalizeHeading(LastPoint.Heading + turnDir * 90.0));
                    const double centerHeadingStart = normalizeHeading(centerHeadingEnd + 180.0);

                    const auto ptCount = static_cast<std::size_t>(TurnVec.HeadingChange / s_ArcInterval); // Type conversion truncates

                    for (std::size_t i = 1; i < ptCount; i++)
                    {
                        // Heading from turn center to turn point is approximate (heading from turn center to turn start + delta * i)
                        const auto& [lCumDist, lPoint] = m_Output.lastPoint();
                        const double hdgDelta = turnDir * s_ArcInterval * static_cast<double>(i);

                        // determine new point
                        auto [lonP, latP] = m_Cs.point(centerLon, centerLat, TurnVec.TurnRadius, normalizeHeading(centerHeadingStart + hdgDelta));

                        auto [dist, hdgEnd] = m_Cs.distanceHeadingEnd(lPoint.Longitude, lPoint.Latitude, lonP, latP);
                        m_Output.addPoint(lCumDist + dist, lonP, latP, hdgEnd, TurnVec.TurnRadius, dir);
                    }

                    // calculate last point and check heading delta
                    if (TurnVec.HeadingChange - s_ArcInterval * static_cast<double>(ptCount) > Constants::AngleThreshold)
                    {
                        const auto& [cumDist, lPt] = m_Output.lastPoint();

                        // Calculate coordinates of next point and distance to end point 
                        auto [testLon, testLat] = m_Cs.point(centerLon, centerLat, TurnVec.TurnRadius, centerHeadingStart + turnDir * s_ArcInterval * static_cast<double>(ptCount));
                        const auto testDist = m_Cs.distance(testLon, testLat, lPt.Longitude, lPt.Latitude);

                        // If the distance is bigger than distance threshold of 1m, last point in arc to route (end point is added in next step!)
                        if (testDist > Constants::DistanceThreshold)
                        {
                            auto [dist, hdgEnd] = m_Cs.distanceHeadingEnd(lPt.Longitude, lPt.Latitude, testLon, testLat);
                            m_Output.addPoint(cumDist + dist, testLon, testLat, hdgEnd, TurnVec.TurnRadius, dir);

                            auto [lPtLon, lPtLat] = m_Cs.point(centerLon, centerLat, TurnVec.TurnRadius, normalizeHeading(centerHeadingStart + turnDir * TurnVec.HeadingChange));

                            auto [distEnd, hdgEndLPt] = m_Cs.distanceHeadingEnd(testLon, testLat, lPtLon, lPtLat);

                            const double targetHdg = normalizeHeading(m_Cs.headingEnd(centerLon, centerLat, lPtLon, lPtLat) + 90.0 * turnDir);

                            m_Output.addPoint(cumDist + dist + distEnd, lPtLon, lPtLat, targetHdg, Constants::Inf, RouteOutput::Direction::Straight);

                            return;
                        }
                    }
                    // If the distance is smaller than distance threshold of 1m, only add the end point
                    const auto& [prevCumDist, prevPoint] = m_Output.lastPoint();

                    // Calculate last point from center
                    auto [lPtLon, lPtLat] = m_Cs.point(centerLon, centerLat, TurnVec.TurnRadius, normalizeHeading(centerHeadingStart + turnDir * TurnVec.HeadingChange));
                    const double targetHdg = normalizeHeading(m_Cs.headingEnd(centerLon, centerLat, lPtLon, lPtLat) + 90.0 * turnDir);
                    auto [dist, hdgEnd] = m_Cs.distanceHeadingEnd(prevPoint.Longitude, prevPoint.Latitude, lPtLon, lPtLat);
                    m_Output.addPoint(prevCumDist + dist, lPtLon, lPtLat, targetHdg, Constants::Inf, RouteOutput::Direction::Straight);
                },
                [](const auto&) { GRAPE_ASSERT(false); },
                }, vec);
        }
        m_Output.recalculateHeadings(m_Cs);
    }

    /**
    * Route type Arrival RNP:
    * Points are added to the output in reverse order as added to the route (opposite to flight direction, from the runway, route steps are always added in flight direction). The cumulative ground distance is calculated with the distance to the last added point (subtraction, cumulative ground distance is negative for arrivals). The heading assigned to a point is the end heading (azimuth 2) plus 180 (converts back to flight direction).
    *
    * Track to Fix Step:
    * Radius is infinite and the turn direction is straight.
    *
    * Radius to Fix Step:
    * Radius is the distance between turn center and turn end (in flight direction, points specified in the input). A warning is issued if the radius at beginning and end of the turn are different (difference higher than threshold). First radius (in flight direction, turn center to turn end) is used for calculating points. In flight direction, the start point of the turn is determined by the previous step. The turn direction is determined by the heading difference (shortest path). Turn direction is inverted since points are added in reverse order to the flight direction. Heading difference between start and end points of the turn and the arc interval determine the number of added points. Points are added by gradually incrementing the heading until the target is reached. Last point (first in flight direction) is handled specially (see below).
    *
    * Checks for last turn point and turn start (given by previous step): if heading difference is smaller than threshold (i. e., points are on a straight line looking from the turn center), only the end point is added. If the distance between the points is smaller than a threshold, only the end point is added. Otherwise, both points are added.
     */
    void RouteCalculator::visitArrivalRnp(const RouteArrivalRnp& Rte) {
        // begin at last element, i.e. the RWY
        for (auto it = Rte.m_RnpSteps.rbegin(); it != Rte.m_RnpSteps.rend(); ++it)
        {
            auto& step = *it;
            const auto& [CumulativeGroundDistance, FirstPoint] = m_Output.firstPoint(); // First as cumulative ground distance is negative for arrivals
            std::visit(Overload{
                [&](const RouteTypeRnp::TrackToFix& TrackToFixStep) {
                    auto [groundDist, hdg] = m_Cs.distanceHeading(TrackToFixStep.Longitude, TrackToFixStep.Latitude, FirstPoint.Longitude, FirstPoint.Latitude);
                    double hdgChange = headingDifference(hdg, FirstPoint.Heading);
                    if (hdgChange > s_WarnHeadingChange)
                        Log::models()->warn("Calculating route output of arrival RNP route '{}' in runway '{}' in airport '{}'. Track to fix at longitude {:.6f} and latitude {:.6f} changes aircraft heading by {:.0f} (more than {:.0f} degrees).", Rte.Name, Rte.parentRunway().Name, Rte.parentAirport().Name, TrackToFixStep.Longitude, TrackToFixStep.Latitude, hdgChange, s_WarnHeadingChange);
                    m_Output.addPoint(CumulativeGroundDistance - groundDist, TrackToFixStep.Longitude, TrackToFixStep.Latitude, hdg);
                },
                [&](const RouteTypeRnp::RadiusToFix& RadiusToFixStep) {
                    // contains target coords for turn
                    auto& prevStep = *(it + 1); // Contains turn initial longitude and latitude in flight direction
                    auto [prevLon, prevLat] = std::visit(Overload{ [&](const auto& AnyStepType) -> std::pair<double, double> { return { AnyStepType.Longitude, AnyStepType.Latitude }; } }, prevStep);

                    auto [radius1, centerFirstHdg] = m_Cs.distanceHeading(RadiusToFixStep.CenterLongitude, RadiusToFixStep.CenterLatitude, RadiusToFixStep.Longitude, RadiusToFixStep.Latitude);
                    const auto [radius2, centerSecondHdg] = m_Cs.distanceHeading(RadiusToFixStep.CenterLongitude, RadiusToFixStep.CenterLatitude, prevLon, prevLat);

                    // Compare radii and warn if difference is bigger than 10m
                    double radiusDiff = std::abs(radius1 - radius2);
                    if (radiusDiff > s_WarnRnpRadiusDifference)
                        Log::models()->warn("Calculating route output of arrival route '{}' in runway '{}' in airport '{}'. Radius to fix turn ending at longitude {:.6f} and latitude {:.6f} changes turn radius by {:.0f} (more than {:.0f} meters).!", Rte.Name, Rte.parentRunway().Name, Rte.parentAirport().Name, RadiusToFixStep.Longitude, RadiusToFixStep.Latitude, radiusDiff, s_WarnRnpRadiusDifference);

                    // Add start point of arc calculation (against flight direction)
                    {
                        auto [dist, hdgEnd] = m_Cs.distanceHeadingEnd(FirstPoint.Longitude, FirstPoint.Latitude, RadiusToFixStep.Longitude, RadiusToFixStep.Latitude);
                        const double hdgFlight = normalizeHeading(hdgEnd + 180.0);
                        const double hdgChange = headingDifference(hdgFlight, FirstPoint.Heading);
                        if (hdgChange > s_WarnHeadingChange)
                            Log::models()->warn("Calculating route output of arrival RNP route '{}' in runway '{}' in airport '{}'. Radius to fix to longitude {:.6f} and latitude {:.6f}: final point changes aircraft heading by {:.0f} (more than {:.0f} degrees).", Rte.Name, Rte.parentRunway().Name, Rte.parentAirport().Name, RadiusToFixStep.Longitude, RadiusToFixStep.Latitude, hdgChange, s_WarnHeadingChange);
                        m_Output.addPoint(CumulativeGroundDistance - dist, RadiusToFixStep.Longitude, RadiusToFixStep.Latitude, hdgFlight);
                    }

                    const int turnDir = turnDirection(centerFirstHdg, centerSecondHdg);
                    // Invert definition since arc is calculated in reverse (-1 -> right turn, else -> left turn)
                    const RouteOutput::Direction dir = turnDir == -1 ? RouteOutput::Direction::RightTurn : RouteOutput::Direction::LeftTurn;

                    const auto hdgDiff = headingDifference(centerFirstHdg, centerSecondHdg);
                    const auto ptCount = static_cast<std::size_t>(hdgDiff / s_ArcInterval); // Type conversion truncates

                    // Add all points except last
                    for (std::size_t i = 1; i < ptCount; ++i)
                    {
                        const auto& [lCumDist, lPoint] = m_Output.firstPoint();

                        const double hdgDelta = turnDir * s_ArcInterval * static_cast<double>(i);

                        auto [lon, lat] = m_Cs.point(RadiusToFixStep.CenterLongitude, RadiusToFixStep.CenterLatitude, radius1, normalizeHeading(centerFirstHdg + hdgDelta));

                        auto [dist, hdgEnd] = m_Cs.distanceHeadingEnd(lPoint.Longitude, lPoint.Latitude, lon, lat);
                        const double hdgFlight = normalizeHeading(hdgEnd + 180.0);
                        const double hdgChange = headingDifference(hdgFlight, lPoint.Heading);
                        if (hdgChange > s_WarnHeadingChange)
                            Log::models()->warn("Calculating route output of arrival RNP route '{}' in runway '{}' in airport '{}'. Radius to fix to longitude {:.6f} and latitude {:.6f}: intermediate point changes aircraft heading by {:.0f} (more than {:.0f} degrees).", Rte.Name, Rte.parentRunway().Name, Rte.parentAirport().Name, RadiusToFixStep.Longitude, RadiusToFixStep.Latitude, hdgChange, s_WarnHeadingChange);

                        m_Output.addPoint(lCumDist - dist, lon, lat, hdgFlight, radius1, dir);
                    }

                    // calculate last point and check heading delta
                    if (hdgDiff - s_ArcInterval * static_cast<double>(ptCount) > Constants::AngleThreshold)
                    {
                        const auto& [cumDist, lPoint] = m_Output.firstPoint();

                        // Calculate coordinates of next point and distance to end point 
                        auto [testLon, testLat] = m_Cs.point(RadiusToFixStep.CenterLongitude, RadiusToFixStep.CenterLatitude, radius1, centerFirstHdg + turnDir * s_ArcInterval * static_cast<double>(ptCount));
                        const auto testDist = m_Cs.distance(testLon, testLat, prevLon, prevLat);

                        // If the distance is bigger than distance threshold of 1m, last point in arc to route (end point is added in next step!)
                        if (testDist > Constants::DistanceThreshold)
                        {
                            auto [dist, hdgEnd] = m_Cs.distanceHeadingEnd(lPoint.Longitude, lPoint.Latitude, testLon, testLat);
                            const double hdgFlight = normalizeHeading(hdgEnd + 180.0);
                            const double hdgChange = headingDifference(hdgFlight, lPoint.Heading);
                            if (hdgChange > s_WarnHeadingChange)
                                Log::models()->warn("Calculating route output of arrival RNP route '{}' in runway '{}' in airport '{}'. Radius to fix to longitude {:.6f} and latitude {:.6f}: intermediate point changes aircraft heading by {:.0f} (more than {:.0f} degrees).", Rte.Name, Rte.parentRunway().Name, Rte.parentAirport().Name, RadiusToFixStep.Longitude, RadiusToFixStep.Latitude, hdgChange, s_WarnHeadingChange);

                            m_Output.addPoint(cumDist - dist, testLon, testLat, hdgFlight, radius1, dir);
                        }
                    }
                },
                [&](const auto&) { GRAPE_ASSERT(false); },
                }, step);
        }
    }

    /**
    * Route type Departure RNP:
    * Points are added to the output in the same order as added to the route (in flight direction, from the runway). The cumulative ground distance is calculated with the distance to the last added point. The heading assigned to a point is the end heading (azimuth 2) in flight direction.
    *
    * Track to Fix Step:
    * Radius is infinite and turn direction is straight.
    *
    * Radius to Fix Step:
    * Radius is the distance between turn center and turn start (previous point). A warning is issued if the radius at beginning and end of the turn are different (difference higher than threshold). First radius is used for calculating points. The turn direction is determined by the heading difference (shortest path). Heading difference between start and end points of the turn and the arc interval determine the number of added points. Points are added by gradually incrementing the heading until the target is reached. Last point is handled specially (see below).
    *
    * Checks for last turn point and specified turn end: if heading difference is smaller than threshold (i. e., points are on a straight line looking from the turn center), only the end point is added. If the distance between the points is higher than threshold, only the end point is added. Otherwise, both points are added.
    */
    void RouteCalculator::visitDepartureRnp(const RouteDepartureRnp& Rte) {
        for (const auto& step : Rte.m_RnpSteps)
        {
            const auto& [CumulativeGroundDistance, LastPoint] = m_Output.lastPoint();
            std::visit(Overload{
                [&](const RouteTypeRnp::TrackToFix& TrackToFixStep) {
                    auto [groundDist, hdg] = m_Cs.distanceHeadingEnd(LastPoint.Longitude, LastPoint.Latitude, TrackToFixStep.Longitude, TrackToFixStep.Latitude);
                    double hdgChange = headingDifference(LastPoint.Heading, hdg);
                    if (hdgChange > s_WarnHeadingChange)
                        Log::models()->warn("Calculating route output of departure route '{}' in runway '{}' in airport '{}'. Track to fix at longitude {:.6f} and latitude {:.6f} changes aircraft heading by {:.0f} (more than {:.0f} degrees).", Rte.Name, Rte.parentRunway().Name, Rte.parentAirport().Name, TrackToFixStep.Longitude, TrackToFixStep.Latitude, hdgChange, s_WarnHeadingChange);
                    m_Output.addPoint(CumulativeGroundDistance + groundDist, TrackToFixStep.Longitude, TrackToFixStep.Latitude, hdg);
                },
                [&](const RouteTypeRnp::RadiusToFix& RadiusToFixStep) {
                    auto [radius, centerFirstHdg] = m_Cs.distanceHeading(RadiusToFixStep.CenterLongitude, RadiusToFixStep.CenterLatitude, LastPoint.Longitude, LastPoint.Latitude);
                    auto [radius2, centerSecondHdg] = m_Cs.distanceHeading(RadiusToFixStep.CenterLongitude, RadiusToFixStep.CenterLatitude, RadiusToFixStep.Longitude, RadiusToFixStep.Latitude);

                    // Compare radii and warn if difference is bigger than 10m
                    double radiusDiff = std::abs(radius - radius2);
                    if (radiusDiff > s_WarnRnpRadiusDifference)
                        Log::models()->warn("Calculating route output of departure route '{}' in runway '{}' in airport '{}'. Radius to fix turn ending at longitude {:.6f} and latitude {:.6f} changes turn radius by {:.0f} (more than {:.0f} meters).!", Rte.Name, Rte.parentRunway().Name, Rte.parentAirport().Name, RadiusToFixStep.Longitude, RadiusToFixStep.Latitude, radiusDiff, s_WarnRnpRadiusDifference);

                    const int turnDir = turnDirection(centerFirstHdg, centerSecondHdg);
                    const RouteOutput::Direction dir = turnDir == 1 ? RouteOutput::Direction::RightTurn : RouteOutput::Direction::LeftTurn;

                    const auto hdgDiff = headingDifference(centerFirstHdg, centerSecondHdg);
                    const auto ptCount = static_cast<std::size_t>(hdgDiff / s_ArcInterval); // Type conversion truncates

                    // Add all points except last
                    for (std::size_t i = 1; i < ptCount; ++i)
                    {
                        const auto& [lCumDist, lPoint] = m_Output.lastPoint();
                        const double hdgDelta = turnDir * s_ArcInterval * static_cast<double>(i);
                        auto [Longitude, Latitude] = m_Cs.point(RadiusToFixStep.CenterLongitude, RadiusToFixStep.CenterLatitude, radius, centerFirstHdg + hdgDelta);

                        auto [dist, hdgEnd] = m_Cs.distanceHeadingEnd(lPoint.Longitude, lPoint.Latitude, Longitude, Latitude);
                        const double hdgChange = headingDifference(hdgEnd, lPoint.Heading);
                        if (hdgChange > s_WarnHeadingChange)
                            Log::models()->warn("Calculating route output of departure RNP route '{}' in runway '{}' in airport '{}'. Radius to fix to longitude {:.6f} and latitude {:.6f}: intermediate point changes aircraft heading by {:.0f} (more than {:.0f} degrees).", Rte.Name, Rte.parentRunway().Name, Rte.parentAirport().Name, RadiusToFixStep.Longitude, RadiusToFixStep.Latitude, hdgChange, s_WarnHeadingChange);

                        m_Output.addPoint(lCumDist + dist, Longitude, Latitude, hdgEnd, radius, dir);
                    }

                    // Last Point calculation. If last point of arc is very close to end point, only add end point. Else, add both.
                    if (hdgDiff - s_ArcInterval * static_cast<double>(ptCount) > Constants::AngleThreshold)
                    {
                        const auto& [lCumDist, lPoint] = m_Output.lastPoint();

                        // Calculate coordinates of next point and distance to end point 
                        auto [testLon, testLat] = m_Cs.point(RadiusToFixStep.CenterLongitude, RadiusToFixStep.CenterLatitude, radius, centerFirstHdg + turnDir * s_ArcInterval * static_cast<double>(ptCount));
                        const auto [testDist, testHdgEnd] = m_Cs.distanceHeadingEnd(testLon, testLat, RadiusToFixStep.Longitude, RadiusToFixStep.Latitude);

                        // If the distance is bigger than distance threshold of 1m, add both points to route
                        if (testDist > Constants::DistanceThreshold)
                        {
                            auto [dist, hdgEnd] = m_Cs.distanceHeadingEnd(lPoint.Longitude, lPoint.Latitude, testLon, testLat);
                            const double hdgChangeTest = headingDifference(hdgEnd, lPoint.Heading);
                            if (hdgChangeTest > s_WarnHeadingChange)
                                Log::models()->warn("Calculating route output of departure RNP route '{}' in runway '{}' in airport '{}'. Radius to fix to longitude {:.6f} and latitude {:.6f}: intermediate point changes aircraft heading by {:.0f} (more than {:.0f} degrees).", Rte.Name, Rte.parentRunway().Name, Rte.parentAirport().Name, RadiusToFixStep.Longitude, RadiusToFixStep.Latitude, hdgChangeTest, s_WarnHeadingChange);
                            m_Output.addPoint(lCumDist + dist, testLon, testLat, hdgEnd, radius, dir);

                            const double hdgChange = headingDifference(testHdgEnd, hdgEnd);
                            if (hdgChange > s_WarnHeadingChange)
                                Log::models()->warn("Calculating route output of departure RNP route '{}' in runway '{}' in airport '{}'. Radius to fix to longitude {:.6f} and latitude {:.6f}: final point changes aircraft heading by {:.0f} (more than {:.0f} degrees).", Rte.Name, Rte.parentRunway().Name, Rte.parentAirport().Name, RadiusToFixStep.Longitude, RadiusToFixStep.Latitude, hdgChange, s_WarnHeadingChange);
                            m_Output.addPoint(lCumDist + dist + testDist, RadiusToFixStep.Longitude, RadiusToFixStep.Latitude, testHdgEnd, Constants::Inf, RouteOutput::Direction::Straight);
                            return;
                        }
                    }

                    // If the distance is smaller than distance threshold of 1m, only add the end point
                    const auto& [lCumDist, lPoint] = m_Output.lastPoint();
                    auto [dist, hdgEnd] = m_Cs.distanceHeadingEnd(lPoint.Longitude, lPoint.Latitude, RadiusToFixStep.Longitude, RadiusToFixStep.Latitude);
                    const double hdgChange = headingDifference(hdgEnd, lPoint.Heading);
                        if (hdgChange > s_WarnHeadingChange)
                            Log::models()->warn("Calculating route output of departure RNP route '{}' in runway '{}' in airport '{}'. Radius to fix to longitude {:.6f} and latitude {:.6f}: final point changes aircraft heading by {:.0f} (more than {:.0f} degrees).", Rte.Name, Rte.parentRunway().Name, Rte.parentAirport().Name, RadiusToFixStep.Longitude, RadiusToFixStep.Latitude, hdgChange, s_WarnHeadingChange);

                    m_Output.addPoint(lCumDist + dist, RadiusToFixStep.Longitude, RadiusToFixStep.Latitude, hdgEnd, Constants::Inf, RouteOutput::Direction::Straight);
                },
                }, step);
        }
        m_Output.recalculateHeadings(m_Cs);
    }

    TEST_CASE("Route Simple") {
        auto cs = Geodesic();
        auto rc = RouteCalculator(cs);

        Airport apt("Airport");
        Runway rwy(apt, "Runway");
        rwy.Longitude = 7.147559;
        rwy.Latitude = 50.86735;
        rwy.Heading = 134.57796965;

        SUBCASE("Simple Departure") {
            RouteDepartureSimple rds(rwy, "Route Simple");
            rds.addPoint(7.3, 50.772);
            rds.addPoint(7.339, 50.699);
            rds.addPoint(7.308, 50.633);

            auto output = rc.calculate(rds);

            {
                const auto& [cumGroundDist, pt] = output.point(0); // RWY
                CHECK_EQ(cumGroundDist, doctest::Approx(0.0).epsilon(Constants::PrecisionTest));
                CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                CHECK_EQ(pt.Radius, Constants::Inf);
                CHECK_EQ(pt.Heading, doctest::Approx(rwy.Heading).epsilon(Constants::PrecisionTest));
                CHECK_EQ(pt.Longitude, doctest::Approx(rwy.Longitude).epsilon(Constants::PrecisionTest));
                CHECK_EQ(pt.Latitude, doctest::Approx(rwy.Latitude).epsilon(Constants::PrecisionTest));
            }
            {
                const auto& [cumGroundDist, pt] = output.point(1);
                CHECK_EQ(cumGroundDist, doctest::Approx(15096.760).epsilon(Constants::PrecisionTest));
                CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                CHECK_EQ(pt.Radius, Constants::Inf);
                CHECK_EQ(pt.Heading, doctest::Approx(161.25628140).epsilon(Constants::PrecisionTest));
                CHECK_EQ(pt.Longitude, doctest::Approx(7.3).epsilon(Constants::PrecisionTest));
                CHECK_EQ(pt.Latitude, doctest::Approx(50.772).epsilon(Constants::PrecisionTest));
            }
            {
                const auto& [cumGroundDist, pt] = output.point(2);
                CHECK_EQ(cumGroundDist, doctest::Approx(15096.760 + 8574.789).epsilon(Constants::PrecisionTest));
                CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                CHECK_EQ(pt.Radius, Constants::Inf);
                CHECK_EQ(pt.Heading, doctest::Approx(196.6333434).epsilon(Constants::PrecisionTest));
                CHECK_EQ(pt.Longitude, doctest::Approx(7.339).epsilon(Constants::PrecisionTest));
                CHECK_EQ(pt.Latitude, doctest::Approx(50.699).epsilon(Constants::PrecisionTest));
            }
            {
                const auto& [cumGroundDist, pt] = output.point(3);
                CHECK_EQ(cumGroundDist, doctest::Approx(15096.760 + 8574.789 + 7662.117).epsilon(Constants::PrecisionTest));
                CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                CHECK_EQ(pt.Radius, Constants::Inf);
                CHECK_EQ(pt.Heading, doctest::Approx(196.6333434).epsilon(Constants::PrecisionTest));
                CHECK_EQ(pt.Longitude, doctest::Approx(7.308).epsilon(Constants::PrecisionTest));
                CHECK_EQ(pt.Latitude, doctest::Approx(50.633).epsilon(Constants::PrecisionTest));
            }
        }

        SUBCASE("Simple Arrival") {
            rwy.Heading = normalizeHeading(134.57796965 + 180.0);
            RouteArrivalSimple ras(rwy, "Route Simple");
            ras.addPoint(7.308, 50.633); // P3
            ras.addPoint(7.339, 50.699); // P2
            ras.addPoint(7.3, 50.772); // P1

            auto output = rc.calculate(ras);

            {
                const auto& [cumGroundDist, pt] = output.point(0); // P3
                CHECK_EQ(cumGroundDist, doctest::Approx(-(15096.760 + 8574.789 + 7662.117)).epsilon(Constants::PrecisionTest));
                CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                CHECK_EQ(pt.Radius, Constants::Inf);
                CHECK_EQ(pt.Heading, doctest::Approx(normalizeHeading(16.609366)).epsilon(Constants::PrecisionTest));
                CHECK_EQ(pt.Longitude, doctest::Approx(7.308).epsilon(Constants::PrecisionTest));
                CHECK_EQ(pt.Latitude, doctest::Approx(50.633).epsilon(Constants::PrecisionTest));
            }
            {
                const auto& [cumGroundDist, pt] = output.point(1); // P2
                CHECK_EQ(cumGroundDist, doctest::Approx(-(15096.760 + 8574.789)).epsilon(Constants::PrecisionTest));
                CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                CHECK_EQ(pt.Radius, Constants::Inf);
                CHECK_EQ(pt.Heading, doctest::Approx(normalizeHeading(-18.713524)).epsilon(Constants::PrecisionTest));
                CHECK_EQ(pt.Longitude, doctest::Approx(7.339).epsilon(Constants::PrecisionTest));
                CHECK_EQ(pt.Latitude, doctest::Approx(50.699).epsilon(Constants::PrecisionTest));
            }
            {
                const auto& [cumGroundDist, pt] = output.point(2); // P1
                CHECK_EQ(cumGroundDist, doctest::Approx(-15096.760).epsilon(Constants::PrecisionTest));
                CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                CHECK_EQ(pt.Radius, Constants::Inf);
                CHECK_EQ(pt.Heading, doctest::Approx(normalizeHeading(-45.303864)).epsilon(Constants::PrecisionTest));
                CHECK_EQ(pt.Longitude, doctest::Approx(7.3).epsilon(Constants::PrecisionTest));
                CHECK_EQ(pt.Latitude, doctest::Approx(50.772).epsilon(Constants::PrecisionTest));
            }
            {
                const auto& [cumGroundDist, pt] = output.point(3); // Rwy
                CHECK_EQ(cumGroundDist, doctest::Approx(0).epsilon(Constants::PrecisionTest));
                CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                CHECK_EQ(pt.Radius, Constants::Inf);
                CHECK_EQ(pt.Heading, doctest::Approx(rwy.Heading).epsilon(Constants::PrecisionTest));
                CHECK_EQ(pt.Longitude, doctest::Approx(rwy.Longitude).epsilon(Constants::PrecisionTest));
                CHECK_EQ(pt.Latitude, doctest::Approx(rwy.Latitude).epsilon(Constants::PrecisionTest));
            }
        }
    }

    TEST_CASE("Route Vector" * doctest::skip(false)) {
        RouteCalculator::s_ArcInterval = 30.0;
        auto cs = Geodesic();
        auto rc = RouteCalculator(cs);

        Airport apt("Airport");
        Runway rwy(apt, "Runway");

        SUBCASE("Departure Vector") {
            RouteDepartureVectors rdv(rwy, "departure");

            SUBCASE("Simple") {
                rwy.Longitude = 7.97709391;
                rwy.Latitude = 48.97762897;
                rwy.Heading = 33.96546164;

                rdv.addStraight(3000.001);
                rdv.addTurn(10000.0, 40.0, RouteTypeVectors::Turn::Direction::Right);
                rdv.addTurn(3000.0, 69.0, RouteTypeVectors::Turn::Direction::Left);
                auto output = rc.calculate(rdv);

                {
                    const auto& [cumGroundDist, pt] = output.point(6); // 6
                    CHECK_EQ(cumGroundDist, doctest::Approx(3000.001 + 5176.379 + 1743.115 + 1552.914 + 1552.914 + 470.755).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                    CHECK_EQ(pt.Radius, Constants::Inf);
                    CHECK_EQ(pt.Heading, doctest::Approx(9.56136244).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Longitude, doctest::Approx(8.10527116).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Latitude, doctest::Approx(49.0597066).epsilon(Constants::PrecisionTest));
                }
                {
                    const auto& [cumGroundDist, pt] = output.point(5); // 5
                    CHECK_EQ(cumGroundDist, doctest::Approx(3000.001 + 5176.379 + 1743.115 + 1552.914 + 1552.914).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Dir, RouteOutput::Direction::LeftTurn);
                    CHECK_EQ(pt.Radius, doctest::Approx(3000.0).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Heading, doctest::Approx(9.56136244).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Longitude, doctest::Approx(8.10420124).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Latitude, doctest::Approx(49.05553242).epsilon(Constants::PrecisionTest));
                }
                {
                    const auto& [cumGroundDist, pt] = output.point(4); // 4
                    CHECK_EQ(cumGroundDist, doctest::Approx(3000.001 + 5176.379 + 1743.115 + 1552.914).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Dir, RouteOutput::Direction::LeftTurn);
                    CHECK_EQ(pt.Radius, doctest::Approx(3000.0).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Heading, doctest::Approx(29.05358132).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Longitude, doctest::Approx(8.09388337).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Latitude, doctest::Approx(49.04332628).epsilon(Constants::PrecisionTest));
                }
                {
                    const auto& [cumGroundDist, pt] = output.point(3); // 3
                    CHECK_EQ(cumGroundDist, doctest::Approx(3000.001 + 5176.379 + 1743.115).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                    CHECK_EQ(pt.Radius, Constants::Inf);
                    CHECK_EQ(pt.Heading, doctest::Approx(59.03980720).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Longitude, doctest::Approx(8.07566844).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Latitude, doctest::Approx(49.03614418).epsilon(Constants::PrecisionTest));
                }
                {
                    const auto& [cumGroundDist, pt] = output.point(2); // 2
                    CHECK_EQ(cumGroundDist, doctest::Approx(3000.001 + 5176.379).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Dir, RouteOutput::Direction::RightTurn);
                    CHECK_EQ(pt.Radius, doctest::Approx(10000.0).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Heading, doctest::Approx(69.02303080).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Longitude, doctest::Approx(8.05340894).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Latitude, doctest::Approx(49.03053514).epsilon(Constants::PrecisionTest));
                }
                {
                    const auto& [cumGroundDist, pt] = output.point(1); // 1
                    CHECK_EQ(cumGroundDist, doctest::Approx(3000.001).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                    CHECK_EQ(pt.Radius, Constants::Inf);
                    CHECK_EQ(pt.Heading, doctest::Approx(48.98272069).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Longitude, doctest::Approx(8).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Latitude, doctest::Approx(49).epsilon(Constants::PrecisionTest));
                }
                {
                    const auto& [cumGroundDist, pt] = output.point(0); // RWY
                    CHECK_EQ(cumGroundDist, doctest::Approx(0).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                    CHECK_EQ(pt.Radius, Constants::Inf);
                    CHECK_EQ(pt.Heading, doctest::Approx(rwy.Heading).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Longitude, doctest::Approx(rwy.Longitude).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Latitude, doctest::Approx(rwy.Latitude).epsilon(Constants::PrecisionTest));
                }
            }

            SUBCASE("Equator") {
                rwy.Longitude = -179.93702811;
                rwy.Latitude = -0.07417071;
                rwy.Heading = 350.96955855;

                SUBCASE("P_end on arc") {
                    rdv.addStraight(9200.0);
                    rdv.addTurn(5635.830, 130.969575, RouteTypeVectors::Turn::Direction::Left);
                    auto output = rc.calculate(rdv);
                    {
                        const auto& [cumGroundDist, pt] = output.point(0); // Rwy
                        CHECK_EQ(cumGroundDist, doctest::Approx(0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(rwy.Heading).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(rwy.Longitude).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(rwy.Latitude).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(1); // P2
                        CHECK_EQ(cumGroundDist, doctest::Approx(9200.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(335.9695777).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-179.95).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.008).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(2); // P3
                        CHECK_EQ(cumGroundDist, doctest::Approx(9200.0 + 2917.320).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::LeftTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5635.830).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(305.9695886).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-179.960672).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.032097).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(3); // P4
                        CHECK_EQ(cumGroundDist, doctest::Approx(9200.0 + 2917.320 + 2917.320).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::LeftTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5635.830).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(275.9695694).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-179.981882).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.047593).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(4); // P5
                        CHECK_EQ(cumGroundDist, doctest::Approx(9200.0 + 2917.320 + 2917.320 + 2917.320).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::LeftTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5635.830).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(245.9695871).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(179.992054).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.050337).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(5); // P6
                        CHECK_EQ(cumGroundDist, doctest::Approx(9200.0 + 2917.320 + 2917.320 + 2917.320 + 2917.320).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::LeftTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5635.830).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(225.4847593).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(179.968118).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.039593).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(6); // P end
                        CHECK_EQ(cumGroundDist, doctest::Approx(9200.0 + 2917.320 + 2917.320 + 2917.320 + 2917.320 + 1077.362).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(225.4847593).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(179.96121706).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.03276206).epsilon(Constants::PrecisionTest));
                    }
                }

                SUBCASE("P_end equal to last point") {
                    rdv.addStraight(9200.0);
                    rdv.addTurn(5635.830, 120.0, RouteTypeVectors::Turn::Direction::Left);
                    auto output = rc.calculate(rdv);
                    {
                        const auto& [cumGroundDist, pt] = output.point(0); // Rwy
                        CHECK_EQ(cumGroundDist, doctest::Approx(0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(rwy.Heading).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(rwy.Longitude).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(rwy.Latitude).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(1); // P2
                        CHECK_EQ(cumGroundDist, doctest::Approx(9200.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(335.9695777).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-179.95).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.008).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(2); // P3
                        CHECK_EQ(cumGroundDist, doctest::Approx(9200.0 + 2917.320).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::LeftTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5635.830).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(305.9695886).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-179.960672).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.032097).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(3); // P4
                        CHECK_EQ(cumGroundDist, doctest::Approx(9200.0 + 2917.320 + 2917.320).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::LeftTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5635.830).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(275.9695694).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-179.981882).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.047593).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(4); // P5
                        CHECK_EQ(cumGroundDist, doctest::Approx(9200.0 + 2917.320 + 2917.320 + 2917.320).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::LeftTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5635.830).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(245.9695871).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(179.992054).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.050337).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(5); // P6
                        CHECK_EQ(cumGroundDist, doctest::Approx(9200.0 + 2917.320 + 2917.320 + 2917.320 + 2917.320).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(245.9695871).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(179.968118).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.039593).epsilon(Constants::PrecisionTest));
                    }
                }
            }

            // TODO
            SUBCASE("North pole") {
                rwy.Longitude = 12.647;
                rwy.Latitude = 89.956;
                rwy.Heading = 319.9999844;

                SUBCASE("P_end on arc") {
                    rdv.addStraight(5000.001);
                    rdv.addTurn(5000.0, 106.13972020, RouteTypeVectors::Turn::Direction::Right);
                    auto output = rc.calculate(rdv);
                    {
                        const auto& [cumGroundDist, pt] = output.point(0); // Rwy
                        CHECK_EQ(cumGroundDist, doctest::Approx(0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(rwy.Heading).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(rwy.Longitude).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(rwy.Latitude).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(1); // P2
                        CHECK_EQ(cumGroundDist, doctest::Approx(5000.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(263.6433165).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-58.709728).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.969632).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(2); // P3
                        CHECK_EQ(cumGroundDist, doctest::Approx(5000.0 + 2588.190).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::RightTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5000.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(258.6790449).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-93.673943).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.959813).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(3); // P4
                        CHECK_EQ(cumGroundDist, doctest::Approx(5000.0 + 2588.190 + 2588.191).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::RightTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5000.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(261.7530114).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-120.599998).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.949825).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(4); // P5
                        CHECK_EQ(cumGroundDist, doctest::Approx(5000.0 + 2588.190 + 2588.191 + 2588.190).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::RightTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5000.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(261.6202291).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-143.80259440).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.941793).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(5); // P end (on arc)
                        CHECK_EQ(cumGroundDist, doctest::Approx(5000.0 + 2588.190 + 2588.191 + 2588.190 + 1403.805).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(261.6202291).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-155.503296).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.938687).epsilon(Constants::PrecisionTest));
                    }
                }

                SUBCASE("P_end equals last point") {
                    rdv.addStraight(5000.001);
                    rdv.addTurn(5000.0, 90.0, RouteTypeVectors::Turn::Direction::Right);
                    auto output = rc.calculate(rdv);
                    {
                        const auto& [cumGroundDist, pt] = output.point(0); // Rwy
                        CHECK_EQ(cumGroundDist, doctest::Approx(0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(rwy.Heading).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(rwy.Longitude).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(rwy.Latitude).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(1); // P2
                        CHECK_EQ(cumGroundDist, doctest::Approx(5000.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(263.6433165).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-58.709728).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.969632).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(2); // P3
                        CHECK_EQ(cumGroundDist, doctest::Approx(5000.0 + 2588.190).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::RightTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5000.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(258.6790449).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-93.673943).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.959813).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(3); // P4
                        CHECK_EQ(cumGroundDist, doctest::Approx(5000.0 + 2588.190 + 2588.191).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::RightTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5000.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(261.7530114).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-120.599998).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.949825).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(4); // P5 = P_end
                        CHECK_EQ(cumGroundDist, doctest::Approx(5000.0 + 2588.190 + 2588.191 + 2588.190).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(261.7530114).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-143.80259440).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.941793).epsilon(Constants::PrecisionTest));
                    }
                }
            }
        }

        SUBCASE("Arrival Vector") {
            RouteArrivalVectors rav(rwy, "arrival");

            SUBCASE("Simple") {
                rwy.Longitude = 7.97709391;
                rwy.Latitude = 48.97762897;
                rwy.Heading = 213.96546164;

                rav.addTurn(3000.0, 69.0, RouteTypeVectors::Turn::Direction::Right); // turn 3 -> 6
                rav.addTurn(10000.0, 40.0, RouteTypeVectors::Turn::Direction::Left); // turn 3 -> 1
                rav.addStraight(3000.001); // 1
                auto output = rc.calculate(rav);

                {
                    const auto& [cumGroundDist, pt] = output.point(0); // 6
                    CHECK_EQ(cumGroundDist, doctest::Approx(-(3000.001 + 5176.379 + 1743.115 + 1552.914 + 1552.914 + 470.755)).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                    CHECK_EQ(pt.Radius, Constants::Inf);
                    CHECK_EQ(pt.Heading, doctest::Approx(185.06218491).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Longitude, doctest::Approx(8.10527116).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Latitude, doctest::Approx(49.0597066).epsilon(Constants::PrecisionTest));
                }
                {
                    const auto& [cumGroundDist, pt] = output.point(1); // 5
                    CHECK_EQ(cumGroundDist, doctest::Approx(-(3000.001 + 5176.379 + 1743.115 + 1552.914 + 1552.914)).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Dir, RouteOutput::Direction::RightTurn);
                    CHECK_EQ(pt.Radius, doctest::Approx(3000.0).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Heading, doctest::Approx(209.06137415).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Longitude, doctest::Approx(8.10420124).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Latitude, doctest::Approx(49.05553242).epsilon(Constants::PrecisionTest));
                }
                {
                    const auto& [cumGroundDist, pt] = output.point(2); // 4
                    CHECK_EQ(cumGroundDist, doctest::Approx(-(3000.001 + 5176.379 + 1743.115 + 1552.914)).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Dir, RouteOutput::Direction::RightTurn);
                    CHECK_EQ(pt.Radius, doctest::Approx(3000.0).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Heading, doctest::Approx(239.05356247).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Longitude, doctest::Approx(8.09388337).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Latitude, doctest::Approx(49.04332628).epsilon(Constants::PrecisionTest));
                }
                {
                    const auto& [cumGroundDist, pt] = output.point(3); // 3
                    CHECK_EQ(cumGroundDist, doctest::Approx(-(3000.001 + 5176.379 + 1743.115)).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                    CHECK_EQ(pt.Radius, Constants::Inf);
                    CHECK_EQ(pt.Heading, doctest::Approx(254.03982130).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Longitude, doctest::Approx(8.07566844).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Latitude, doctest::Approx(49.03614418).epsilon(Constants::PrecisionTest));
                }
                {
                    const auto& [cumGroundDist, pt] = output.point(4); // 2
                    CHECK_EQ(cumGroundDist, doctest::Approx(-(3000.001 + 5176.379)).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Dir, RouteOutput::Direction::LeftTurn);
                    CHECK_EQ(pt.Radius, doctest::Approx(10000.0).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Heading, doctest::Approx(229.02303827).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Longitude, doctest::Approx(8.05340894).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Latitude, doctest::Approx(49.03053514).epsilon(Constants::PrecisionTest));
                }
                {
                    const auto& [cumGroundDist, pt] = output.point(5); // 1
                    CHECK_EQ(cumGroundDist, doctest::Approx(-3000.001).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                    CHECK_EQ(pt.Radius, Constants::Inf);
                    CHECK_EQ(pt.Heading, doctest::Approx(normalizeHeading(-56.01726004 - 90.0)).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Longitude, doctest::Approx(8.0).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Latitude, doctest::Approx(49.0).epsilon(Constants::PrecisionTest));
                }
                {
                    const auto& [cumGroundDist, pt] = output.point(6); // RWY
                    CHECK_EQ(cumGroundDist, doctest::Approx(0).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                    CHECK_EQ(pt.Radius, Constants::Inf);
                    CHECK_EQ(pt.Heading, doctest::Approx(rwy.Heading).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Longitude, doctest::Approx(rwy.Longitude).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Latitude, doctest::Approx(rwy.Latitude).epsilon(Constants::PrecisionTest));
                }
            }

            SUBCASE("North pole") {
                rwy.Longitude = 12.646979126;
                rwy.Latitude = 89.95599999;
                rwy.Heading = 139.99998749;

                SUBCASE("P_end on arc") {
                    rav.addTurn(5000.0, 106.13972020, RouteTypeVectors::Turn::Direction::Left); // add turn P_end -> P2
                    rav.addStraight(5000.0); // P2 -> RWY
                    auto output = rc.calculate(rav);
                    {
                        const auto& [cumGroundDist, pt] = output.point(5); // Rwy
                        CHECK_EQ(cumGroundDist, doctest::Approx(0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(rwy.Heading).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(rwy.Longitude).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(rwy.Latitude).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(4); // P2
                        CHECK_EQ(cumGroundDist, doctest::Approx(-5000.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(68.64329114).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-58.709728).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.969632).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(3); // P3
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(5000.0 + 2588.190)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::LeftTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5000.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(48.67910814).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-93.673943).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.959813).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(2); // P4
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(5000.0 + 2588.190 + 2588.191)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::LeftTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5000.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(51.75299733).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-120.599998).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.949825).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(1); // P5
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(5000.0 + 2588.190 + 2588.191 + 2588.190)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::LeftTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5000.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(58.55042535).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-143.80259440).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.941793).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(0); // P end (on arc)
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(5000.0 + 2588.190 + 2588.191 + 2588.190 + 1403.805)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(77.98944114).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-155.503296).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.938687).epsilon(Constants::PrecisionTest));
                    }
                }

                SUBCASE("P_end equal to last point") {
                    rav.addTurn(5000.0, 90.0, RouteTypeVectors::Turn::Direction::Left);
                    rav.addStraight(5000.001);
                    auto output = rc.calculate(rav);
                    {
                        const auto& [cumGroundDist, pt] = output.point(4); // Rwy
                        CHECK_EQ(cumGroundDist, doctest::Approx(0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(rwy.Heading).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(rwy.Longitude).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(rwy.Latitude).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(3); // P2
                        CHECK_EQ(cumGroundDist, doctest::Approx(-5000.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(68.64329114).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-58.709728).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.969632).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(2); // P3
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(5000.0 + 2588.190)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::LeftTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5000.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(48.67910814).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-93.673943).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.959813).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(1); // P4
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(5000.0 + 2588.190 + 2588.191)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::LeftTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5000.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(51.75299733).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-120.599998).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.949825).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(0); // P5 = P_end
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(5000.0 + 2588.190 + 2588.191 + 2588.190)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(73.55041527).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-143.80259440).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.941793).epsilon(Constants::PrecisionTest));
                    }
                }
            }

            SUBCASE("Equator") {
                rwy.Longitude = -179.93702811;
                rwy.Latitude = -0.07417071;
                rwy.Heading = 170.96955855;

                SUBCASE("P_end on arc") {
                    rav.addTurn(5635.830, 130.969575, RouteTypeVectors::Turn::Direction::Right);
                    rav.addStraight(9200.0);
                    auto output = rc.calculate(rav);
                    {
                        const auto& [cumGroundDist, pt] = output.point(6); // Rwy
                        CHECK_EQ(cumGroundDist, doctest::Approx(0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(rwy.Heading).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(rwy.Longitude).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(rwy.Latitude).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(5); // P2
                        CHECK_EQ(cumGroundDist, doctest::Approx(-9200.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(170.96956604).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-179.95).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.008).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(4); // P3
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(9200.0 + 2917.320)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::RightTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5635.830).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(155.96957401).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-179.960672).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.032097).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(3); // P4
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(9200.0 + 2917.320 + 2917.320)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::RightTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5635.830).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(125.96957382).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-179.981882).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.047593).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(2); // P5
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(9200.0 + 2917.320 + 2917.320 + 2917.320)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::RightTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5635.830).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(95.96954715).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(179.992054).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.050337).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(1); // P6
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(9200.0 + 2917.320 + 2917.320 + 2917.320 + 2917.320)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::RightTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5635.830).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(65.96956829).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(179.968118).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.039593).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(0); // P end
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(9200.0 + 2917.320 + 2917.320 + 2917.320 + 2917.320 + 1077.362)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(39.99998347).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(179.96121706).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.03276206).epsilon(Constants::PrecisionTest));
                    }
                }

                SUBCASE("P_end equal to last point") {
                    rav.addTurn(5635.830, 120.0, RouteTypeVectors::Turn::Direction::Right);
                    rav.addStraight(9200.0);
                    auto output = rc.calculate(rav);
                    {
                        const auto& [cumGroundDist, pt] = output.point(5); // Rwy
                        CHECK_EQ(cumGroundDist, doctest::Approx(0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(rwy.Heading).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(rwy.Longitude).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(rwy.Latitude).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(4); // P2
                        CHECK_EQ(cumGroundDist, doctest::Approx(-9200.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(170.96956604).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-179.95).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.008).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(3); // P3
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(9200.0 + 2917.320)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::RightTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5635.830).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(155.96957401).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-179.960672).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.032097).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(2); // P4
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(9200.0 + 2917.320 + 2917.320)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::RightTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5635.830).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(125.96957382).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-179.981882).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.047593).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(1); // P5
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(9200.0 + 2917.320 + 2917.320 + 2917.320)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::RightTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5635.830).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(95.96954715).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(179.992054).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.050337).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(0); // P6
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(9200.0 + 2917.320 + 2917.320 + 2917.320 + 2917.320)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(50.96956044).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(179.968118).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.039593).epsilon(Constants::PrecisionTest));
                    }
                }
            }
        }
    }

    TEST_CASE("Route RNP") {
        RouteCalculator::s_ArcInterval = 30.0;
        auto cs = Geodesic();
        auto rc = RouteCalculator(cs);

        Airport apt("Airport");
        Runway rwy(apt, "Runway");

        SUBCASE("Arrival") {
            RouteArrivalRnp raRnp(rwy, "arrival");

            SUBCASE("Simple") {
                rwy.Longitude = 7.0;
                rwy.Latitude = 48.0;
                rwy.Heading = 210.0;

                raRnp.addTrackToFix(8.1052711628688172, 49.059706605055823); // 6
                raRnp.addRadiusToFix(8.07566844, 49.03614418, 8.06438083, 49.06207962); // turn 6 -> 3
                raRnp.addRadiusToFix(8.0, 49.0, 8.11320921, 48.94968393); // turn 3 -> 1
                auto output = rc.calculate(raRnp);
                {
                    const auto& [cumGroundDist, pt] = output.point(0); // 6
                    CHECK_EQ(cumGroundDist, doctest::Approx(-133514.485 - 5176.379 - 1743.115 - 1552.914 - 1552.914 - 470.755).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                    CHECK_EQ(pt.Radius, Constants::Inf);
                    CHECK_EQ(pt.Heading, doctest::Approx(189.56218442).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Longitude, doctest::Approx(8.10527116).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Latitude, doctest::Approx(49.0597066).epsilon(Constants::PrecisionTest));
                }
                {
                    const auto& [cumGroundDist, pt] = output.point(1); // 5
                    CHECK_EQ(cumGroundDist, doctest::Approx(-133514.485 - 5176.379 - 1743.115 - 1552.914 - 1552.914).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Dir, RouteOutput::Direction::RightTurn);
                    CHECK_EQ(pt.Radius, doctest::Approx(3000.0).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Heading, doctest::Approx(209.06137415).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Longitude, doctest::Approx(8.10420124).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Latitude, doctest::Approx(49.05553242).epsilon(Constants::PrecisionTest));
                }
                {
                    const auto& [cumGroundDist, pt] = output.point(2); // 4
                    CHECK_EQ(cumGroundDist, doctest::Approx(-133514.485 - 5176.379 - 1743.115 - 1552.914).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Dir, RouteOutput::Direction::RightTurn);
                    CHECK_EQ(pt.Radius, doctest::Approx(3000.0).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Heading, doctest::Approx(239.05356247).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Longitude, doctest::Approx(8.09388337).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Latitude, doctest::Approx(49.04332628).epsilon(Constants::PrecisionTest));
                }
                {
                    const auto& [cumGroundDist, pt] = output.point(3); // 3
                    CHECK_EQ(cumGroundDist, doctest::Approx(-133514.485 - 5176.379 - 1743.115).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                    CHECK_EQ(pt.Radius, Constants::Inf);
                    CHECK_EQ(pt.Heading, doctest::Approx(249.03983875).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Longitude, doctest::Approx(8.07566844).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Latitude, doctest::Approx(49.03614418).epsilon(Constants::PrecisionTest));
                }
                {
                    const auto& [cumGroundDist, pt] = output.point(4); // 2
                    CHECK_EQ(cumGroundDist, doctest::Approx(-133514.485 - 5176.379).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Dir, RouteOutput::Direction::LeftTurn);
                    CHECK_EQ(pt.Radius, doctest::Approx(10000.0).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Heading, doctest::Approx(229.02303827).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Longitude, doctest::Approx(8.05340894).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Latitude, doctest::Approx(49.03053514).epsilon(Constants::PrecisionTest));
                }
                {
                    const auto& [cumGroundDist, pt] = output.point(5); // 1
                    CHECK_EQ(cumGroundDist, doctest::Approx(-133514.485).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                    CHECK_EQ(pt.Radius, Constants::Inf);
                    CHECK_EQ(pt.Heading, doctest::Approx(213.98274033).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Longitude, doctest::Approx(8).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Latitude, doctest::Approx(49).epsilon(Constants::PrecisionTest));
                }
                {
                    const auto& [cumGroundDist, pt] = output.point(6); // RWY
                    CHECK_EQ(cumGroundDist, doctest::Approx(0).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                    CHECK_EQ(pt.Radius, Constants::Inf);
                    CHECK_EQ(pt.Heading, doctest::Approx(rwy.Heading).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Longitude, doctest::Approx(rwy.Longitude).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Latitude, doctest::Approx(rwy.Latitude).epsilon(Constants::PrecisionTest));
                }
            }

            SUBCASE("Equator") {
                rwy.Longitude = -179.92;
                rwy.Latitude = -0.07;
                rwy.Heading = 160.0;

                SUBCASE("P_end on arc") {
                    raRnp.addTrackToFix(179.96121706, 0.03276206); // P_end
                    raRnp.addRadiusToFix(-179.95, 0.008, 180.0, 0.0); // turn; ends at P2

                    auto output = rc.calculate(raRnp);

                    {
                        const auto& [cumGroundDist, pt] = output.point(0); // P_end
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(9248.777 + 2917.320 + 2917.320 + 2917.320 + 2917.320 + 1077.362)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(45.48475498).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(179.96121706).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.03276206).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(1); // P6
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(9248.777 + 2917.320 + 2917.320 + 2917.320 + 2917.320)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::RightTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5635.830).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(65.96956829).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(179.968118).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.039593).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(2); // P5
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(9248.777 + 2917.320 + 2917.320 + 2917.320)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::RightTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5635.830).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(95.96954715).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(179.992054).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.050337).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(3); // P4
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(9248.777 + 2917.320 + 2917.320)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::RightTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5635.830).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(125.96957382).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-179.981882).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.047593).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(4); // P3
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(9248.777 + 2917.320)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::RightTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5635.830).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(155.96957401).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-179.960672).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.032097).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(5); // P2
                        CHECK_EQ(cumGroundDist, doctest::Approx(-9248.777).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(158.83323216).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-179.95).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.008).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(6); // Rwy
                        CHECK_EQ(cumGroundDist, doctest::Approx(0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(rwy.Heading).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(rwy.Longitude).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(rwy.Latitude).epsilon(Constants::PrecisionTest));
                    }
                }

                SUBCASE("P_end offset") {
                    raRnp.addTrackToFix(179.96605806, 0.04215148); // P_end extension
                    raRnp.addRadiusToFix(-179.95, 0.008, 180.0, 0.0); // turn, ends at P2

                    auto output = rc.calculate(raRnp);

                    {
                        const auto& [cumGroundDist, pt] = output.point(0); // P_end extension
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(9248.777 + 2917.320 + 2917.320 + 2917.320 + 3032.048)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(72.63169906).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(179.96605806).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.04215148).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(1); // P5
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(9248.777 + 2917.320 + 2917.320 + 2917.320)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::RightTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5635.830).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(95.96954715).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(179.992054).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.050337).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(2); // P4
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(9248.777 + 2917.320 + 2917.320)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::RightTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5635.830).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(125.96957382).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-179.981882).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.047593).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(3); // P3
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(9248.777 + 2917.320)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::RightTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5635.830).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(155.96957401).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-179.960672).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.032097).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(4); // P2
                        CHECK_EQ(cumGroundDist, doctest::Approx(-9248.777).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(158.83323216).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-179.95).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.008).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(5); // Rwy
                        CHECK_EQ(cumGroundDist, doctest::Approx(0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(rwy.Heading).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(rwy.Longitude).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(rwy.Latitude).epsilon(Constants::PrecisionTest));
                    }
                }

                SUBCASE("P_end equals last point") {
                    raRnp.addTrackToFix(179.96811817, 0.03959310); // P6
                    raRnp.addRadiusToFix(-179.95, 0.008, 180.0, 0.0); // turn; ends at P2

                    auto output = rc.calculate(raRnp);

                    {
                        const auto& [cumGroundDist, pt] = output.point(0); // P6
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(9248.777 + 2917.320 + 2917.320 + 2917.320 + 2917.319)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(65.96956829).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(179.96811817).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.03959310).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(1); // P5
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(9248.777 + 2917.320 + 2917.320 + 2917.320)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::RightTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5635.830).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(95.96954715).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(179.992054).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.050337).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(2); // P4
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(9248.777 + 2917.320 + 2917.320)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::RightTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5635.830).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(125.96957382).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-179.981882).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.047593).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(3); // P3
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(9248.777 + 2917.320)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::RightTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5635.830).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(155.96957401).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-179.960672).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.032097).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(4); // P2
                        CHECK_EQ(cumGroundDist, doctest::Approx(-9248.777).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(158.83323216).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-179.95).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.008).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(5); // Rwy
                        CHECK_EQ(cumGroundDist, doctest::Approx(0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(rwy.Heading).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(rwy.Longitude).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(rwy.Latitude).epsilon(Constants::PrecisionTest));
                    }
                }
            }

            SUBCASE("Northpole") {
                rwy.Longitude = 12.647;
                rwy.Latitude = 89.956;
                rwy.Heading = 140.0;

                SUBCASE("Standard") {
                    raRnp.addTrackToFix(-155.50329553, 89.93868749); // P_end
                    raRnp.addRadiusToFix(-58.709728, 89.969632, 176.50725924913968, 89.980151083490071); // (P2, TC)
                    auto output = rc.calculate(raRnp);
                    {
                        const auto& [cumGroundDist, pt] = output.point(0); // P_end
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(5000.0 + 2588.190 + 2588.191 + 2588.190 + 1403.805)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(69.91953426).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-155.50329553).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.93868749).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(1); // P5
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(5000.0 + 2588.190 + 2588.191 + 2588.190)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::LeftTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5000.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(58.55042535).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-143.80259440).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.94179317).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(2); // P4
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(5000.0 + 2588.190 + 2588.191)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::LeftTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5000.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(51.75299733).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-120.59999826).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.94982491).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(3); // P3
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(5000.0 + 2588.190)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::LeftTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5000.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(48.67910814).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-93.67394277).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.95981311).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(4); // P2
                        CHECK_EQ(cumGroundDist, doctest::Approx(-5000.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(68.64326720).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-58.70972827).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.969632).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(5); // RWY
                        CHECK_EQ(cumGroundDist, doctest::Approx(0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(rwy.Heading).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(rwy.Longitude).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(rwy.Latitude).epsilon(Constants::PrecisionTest));
                    }
                }
                SUBCASE("P_end offset") {
                    raRnp.addTrackToFix(-156.288786, 89.918763); // P_extension
                    raRnp.addRadiusToFix(-58.709728, 89.969632, 176.50725924913968, 89.980151083490071); // (P2, TC)
                    auto output = rc.calculate(raRnp);
                    {
                        const auto& [cumGroundDist, pt] = output.point(0); // P_extension
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(5000.0 + 2588.190 + 2588.191 + 2588.190 + 3067.147)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(27.27634599).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-156.288786).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.918763).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(1); // P5
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(5000.0 + 2588.190 + 2588.191 + 2588.190)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::LeftTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5000.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(58.55042535).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-143.80259440).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.94179317).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(2); // P4
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(5000.0 + 2588.190 + 2588.191)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::LeftTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5000.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(51.75299733).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-120.59999826).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.94982491).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(3); // P3
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(5000.0 + 2588.190)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::LeftTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5000.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(48.67910814).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-93.67394277).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.95981311).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(4); // P2
                        CHECK_EQ(cumGroundDist, doctest::Approx(-5000.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(68.64326720).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-58.70972827).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.969632).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(5); // RWY
                        CHECK_EQ(cumGroundDist, doctest::Approx(0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(rwy.Heading).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(rwy.Longitude).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(rwy.Latitude).epsilon(Constants::PrecisionTest));
                    }
                }
                SUBCASE("P_end equals last point") {
                    raRnp.addTrackToFix(-143.80259440, 89.94179317); // P5
                    raRnp.addRadiusToFix(-58.709728, 89.969632, 176.50725924913968, 89.980151083490071); // (P2, TC)
                    auto output = rc.calculate(raRnp);
                    {
                        const auto& [cumGroundDist, pt] = output.point(0); // P5
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(5000.0 + 2588.190 + 2588.191 + 2588.190)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(58.55042535).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-143.80259440).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.94179317).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(1); // P4
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(5000.0 + 2588.190 + 2588.191)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::LeftTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5000.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(51.75299733).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-120.59999826).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.94982491).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(2); // P3
                        CHECK_EQ(cumGroundDist, doctest::Approx(-(5000.0 + 2588.190)).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::LeftTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5000.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(48.67910814).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-93.67394277).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.95981311).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(3); // P2
                        CHECK_EQ(cumGroundDist, doctest::Approx(-5000.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(68.64326720).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-58.70972827).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.969632).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(4); // RWY
                        CHECK_EQ(cumGroundDist, doctest::Approx(0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(rwy.Heading).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(rwy.Longitude).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(rwy.Latitude).epsilon(Constants::PrecisionTest));
                    }
                }
            }
        }

        // RNP
        SUBCASE("Departure") {
            RouteDepartureRnp rdRnp(rwy, "departure");

            SUBCASE("Simple") {
                rwy.Longitude = 7.0;
                rwy.Latitude = 48.0;
                rwy.Heading = 33.23374767;

                rdRnp.addTrackToFix(8.0, 49.0); // 1
                rdRnp.addRadiusToFix(8.07566844, 49.03614418, 8.11320921, 48.94968393); // turn 1 -> 3
                rdRnp.addRadiusToFix(8.10527116, 49.05970660, 8.06438083, 49.06207962); // turn 3 -> 6
                auto output = rc.calculate(rdRnp);
                {
                    const auto& [cumGroundDist, pt] = output.point(6); // 6
                    CHECK_EQ(cumGroundDist, doctest::Approx(133514.485 + 5176.379 + 1743.115 + 1552.914 + 1552.914 + 470.755).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                    CHECK_EQ(pt.Radius, Constants::Inf);
                    CHECK_EQ(pt.Heading, doctest::Approx(9.56137623).epsilon(Constants::PrecisionTest)); // hdg??
                    CHECK_EQ(pt.Longitude, doctest::Approx(8.10527116).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Latitude, doctest::Approx(49.0597066).epsilon(Constants::PrecisionTest));
                }
                {
                    const auto& [cumGroundDist, pt] = output.point(5); // 5
                    CHECK_EQ(cumGroundDist, doctest::Approx(133514.485 + 5176.379 + 1743.115 + 1552.914 + 1552.914).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Dir, RouteOutput::Direction::LeftTurn);
                    CHECK_EQ(pt.Radius, doctest::Approx(3000.0).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Heading, doctest::Approx(9.56137623).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Longitude, doctest::Approx(8.10420124).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Latitude, doctest::Approx(49.05553242).epsilon(Constants::PrecisionTest));
                }
                {
                    const auto& [cumGroundDist, pt] = output.point(4); // 4
                    CHECK_EQ(cumGroundDist, doctest::Approx(133514.485 + 5176.379 + 1743.115 + 1552.914).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Dir, RouteOutput::Direction::LeftTurn);
                    CHECK_EQ(pt.Radius, doctest::Approx(3000.0).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Heading, doctest::Approx(29.05358132).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Longitude, doctest::Approx(8.09388337).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Latitude, doctest::Approx(49.04332628).epsilon(Constants::PrecisionTest));
                }
                {
                    const auto& [cumGroundDist, pt] = output.point(3); // 3
                    CHECK_EQ(cumGroundDist, doctest::Approx(133514.485 + 5176.379 + 1743.115).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                    CHECK_EQ(pt.Radius, Constants::Inf);
                    CHECK_EQ(pt.Heading, doctest::Approx(59.03980720).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Longitude, doctest::Approx(8.07566844).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Latitude, doctest::Approx(49.03614418).epsilon(Constants::PrecisionTest));
                }
                {
                    const auto& [cumGroundDist, pt] = output.point(2); // 2
                    CHECK_EQ(cumGroundDist, doctest::Approx(133514.485 + 5176.379).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Dir, RouteOutput::Direction::RightTurn);
                    CHECK_EQ(pt.Radius, doctest::Approx(10000.0).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Heading, doctest::Approx(69.02303080).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Longitude, doctest::Approx(8.05340894).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Latitude, doctest::Approx(49.03053514).epsilon(Constants::PrecisionTest));
                }
                {
                    const auto& [cumGroundDist, pt] = output.point(1); // 1
                    CHECK_EQ(cumGroundDist, doctest::Approx(133514.485).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                    CHECK_EQ(pt.Radius, Constants::Inf);
                    CHECK_EQ(pt.Heading, doctest::Approx(48.98272069).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Longitude, doctest::Approx(8).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Latitude, doctest::Approx(49).epsilon(Constants::PrecisionTest));
                }
                {
                    const auto& [cumGroundDist, pt] = output.point(0); // RWY
                    CHECK_EQ(cumGroundDist, doctest::Approx(0).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                    CHECK_EQ(pt.Radius, Constants::Inf);
                    CHECK_EQ(pt.Heading, doctest::Approx(rwy.Heading).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Longitude, doctest::Approx(rwy.Longitude).epsilon(Constants::PrecisionTest));
                    CHECK_EQ(pt.Latitude, doctest::Approx(rwy.Latitude).epsilon(Constants::PrecisionTest));
                }
            }

            SUBCASE("Equator") {
                rwy.Longitude = -179.92;
                rwy.Latitude = -0.07;
                rwy.Heading = 338.8332159;

                SUBCASE("P_end offset") {
                    rdRnp.addTrackToFix(-179.95, 0.008);
                    rdRnp.addRadiusToFix(179.96605806, 0.04215148, 180.0, 0.0);
                    auto output = rc.calculate(rdRnp);
                    {
                        const auto& [cumGroundDist, pt] = output.point(0); // Rwy
                        CHECK_EQ(cumGroundDist, doctest::Approx(0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(rwy.Heading).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(rwy.Longitude).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(rwy.Latitude).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(1); // P2
                        CHECK_EQ(cumGroundDist, doctest::Approx(9248.777).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(335.9695777).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-179.95).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.008).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(2); // P3
                        CHECK_EQ(cumGroundDist, doctest::Approx(9248.777 + 2917.320).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::LeftTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5635.830).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(305.9695886).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-179.960672).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.032097).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(3); // P4
                        CHECK_EQ(cumGroundDist, doctest::Approx(9248.777 + 2917.320 + 2917.320).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::LeftTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5635.830).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(275.9695694).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-179.981882).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.047593).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(4); // P5
                        CHECK_EQ(cumGroundDist, doctest::Approx(9248.777 + 2917.320 + 2917.320 + 2917.320).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::LeftTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5635.830).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(252.63172).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(179.992054).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.050337).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(5); // P_end extension
                        CHECK_EQ(cumGroundDist, doctest::Approx(9248.777 + 2917.320 + 2917.320 + 2917.320 + 3032.048).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(252.63172).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(179.96605806).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.04215148).epsilon(Constants::PrecisionTest));
                    }
                }

                SUBCASE("P_end equals last point") {
                    rdRnp.addTrackToFix(-179.95, 0.008);
                    rdRnp.addRadiusToFix(179.96811817, 0.03959310, 180.0, 0.0);
                    auto output = rc.calculate(rdRnp);
                    {
                        const auto& [cumGroundDist, pt] = output.point(0); // Rwy
                        CHECK_EQ(cumGroundDist, doctest::Approx(0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(rwy.Heading).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(rwy.Longitude).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(rwy.Latitude).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(1); // P2
                        CHECK_EQ(cumGroundDist, doctest::Approx(9248.777).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(335.9695777).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-179.95).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.008).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(2); // P3
                        CHECK_EQ(cumGroundDist, doctest::Approx(9248.777 + 2917.320).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::LeftTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5635.830).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(305.9695886).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-179.960672).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.032097).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(3); // P4
                        CHECK_EQ(cumGroundDist, doctest::Approx(9248.777 + 2917.320 + 2917.320).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::LeftTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5635.830).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(275.9695694).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-179.981882).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.047593).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(4); // P5
                        CHECK_EQ(cumGroundDist, doctest::Approx(9248.777 + 2917.320 + 2917.320 + 2917.320).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::LeftTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5635.830).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(245.9695871).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(179.992054).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.050337).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(5); // P6
                        CHECK_EQ(cumGroundDist, doctest::Approx(9248.777 + 2917.320 + 2917.320 + 2917.320 + 2917.319).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(245.9695871).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(179.968118).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.039593).epsilon(Constants::PrecisionTest));
                    }
                }

                SUBCASE("P_end on arc") {
                    rdRnp.addTrackToFix(-179.95, 0.008);
                    rdRnp.addRadiusToFix(179.96121706, 0.03276206, 180.0, 0.0);
                    auto output = rc.calculate(rdRnp);
                    {
                        const auto& [cumGroundDist, pt] = output.point(0); // Rwy
                        CHECK_EQ(cumGroundDist, doctest::Approx(0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(rwy.Heading).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(rwy.Longitude).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(rwy.Latitude).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(1); // P2
                        CHECK_EQ(cumGroundDist, doctest::Approx(9248.777).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(335.9695777).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-179.95).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.008).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(2); // P3
                        CHECK_EQ(cumGroundDist, doctest::Approx(9248.777 + 2917.320).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::LeftTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5635.830).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(305.9695886).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-179.960672).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.032097).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(3); // P4
                        CHECK_EQ(cumGroundDist, doctest::Approx(9248.777 + 2917.320 + 2917.320).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::LeftTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5635.830).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(275.9695694).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-179.981882).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.047593).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(4); // P5
                        CHECK_EQ(cumGroundDist, doctest::Approx(9248.777 + 2917.320 + 2917.320 + 2917.320).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::LeftTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5635.830).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(245.9695871).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(179.992054).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.050337).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(5); // P6
                        CHECK_EQ(cumGroundDist, doctest::Approx(9248.777 + 2917.320 + 2917.320 + 2917.320 + 2917.320).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::LeftTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5635.830).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(225.4847593).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(179.968118).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.039593).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(6); // P end
                        CHECK_EQ(cumGroundDist, doctest::Approx(9248.777 + 2917.320 + 2917.320 + 2917.320 + 2917.320 + 1077.362).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(225.4847593).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(179.961217).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(0.032762).epsilon(Constants::PrecisionTest));
                    }
                }
            }

            SUBCASE("Northpole") {
                rwy.Longitude = 12.647;
                rwy.Latitude = 89.956;
                rwy.Heading = 319.9999844;

                SUBCASE("P_end on arc") {
                    rdRnp.addTrackToFix(-58.709728, 89.969632); // P2
                    rdRnp.addRadiusToFix(-155.503296, 89.938687, 176.50725924913968, 89.980151083490071); // P_end
                    auto output = rc.calculate(rdRnp);

                    {
                        const auto& [cumGroundDist, pt] = output.point(0); // Rwy
                        CHECK_EQ(cumGroundDist, doctest::Approx(0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(rwy.Heading).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(rwy.Longitude).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(rwy.Latitude).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(1); // P2
                        CHECK_EQ(cumGroundDist, doctest::Approx(5000.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(263.6433165).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-58.709728).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.969632).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(2); // P3
                        CHECK_EQ(cumGroundDist, doctest::Approx(5000.0 + 2588.190).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::RightTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5000.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(258.6790449).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-93.673943).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.959813).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(3); // P4
                        CHECK_EQ(cumGroundDist, doctest::Approx(5000.0 + 2588.190 + 2588.191).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::RightTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5000.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(261.7530114).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-120.599998).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.949825).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(4); // P5
                        CHECK_EQ(cumGroundDist, doctest::Approx(5000.0 + 2588.190 + 2588.191 + 2588.190).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::RightTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5000.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(261.6202291).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-143.80259440).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.941793).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(5); // P end (on arc)
                        CHECK_EQ(cumGroundDist, doctest::Approx(5000.0 + 2588.190 + 2588.191 + 2588.190 + 1403.805).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(261.6202291).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-155.503296).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.938687).epsilon(Constants::PrecisionTest));
                    }
                }

                SUBCASE("P_end offset") {
                    rdRnp.addTrackToFix(-58.709728, 89.969632); // P2 
                    rdRnp.addRadiusToFix(-156.288786, 89.918763, 176.50725924913968, 89.980151083490071); // P_extension
                    auto output = rc.calculate(rdRnp);

                    {
                        const auto& [cumGroundDist, pt] = output.point(0); // Rwy
                        CHECK_EQ(cumGroundDist, doctest::Approx(0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(rwy.Heading).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(rwy.Longitude).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(rwy.Latitude).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(1); // P2
                        CHECK_EQ(cumGroundDist, doctest::Approx(5000.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(263.6433165).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-58.709728).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.969632).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(2); // P3
                        CHECK_EQ(cumGroundDist, doctest::Approx(5000.0 + 2588.190).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::RightTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5000.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(258.6790449).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-93.673943).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.959813).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(3); // P4
                        CHECK_EQ(cumGroundDist, doctest::Approx(5000.0 + 2588.190 + 2588.191).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::RightTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5000.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(261.7530114).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-120.599998).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.949825).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(4); // P5
                        CHECK_EQ(cumGroundDist, doctest::Approx(5000.0 + 2588.190 + 2588.191 + 2588.191).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::RightTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5000.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(219.7625287).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-143.80259440).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.94179317).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(5); // P extension 
                        CHECK_EQ(cumGroundDist, doctest::Approx(5000.0 + 2588.190 + 2588.191 + 2588.191 + 3067.147).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(219.7625287).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-156.288786).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.918763).epsilon(Constants::PrecisionTest));
                    }
                }

                SUBCASE("P_end equals last point") {
                    rdRnp.addTrackToFix(-58.709728, 89.969632);
                    rdRnp.addRadiusToFix(-143.80259440, 89.94179317, 176.50725924913968, 89.980151083490071); // P5
                    auto output = rc.calculate(rdRnp);
                    {
                        const auto& [cumGroundDist, pt] = output.point(0); // Rwy
                        CHECK_EQ(cumGroundDist, doctest::Approx(0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(rwy.Heading).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(rwy.Longitude).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(rwy.Latitude).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(1); // P2
                        CHECK_EQ(cumGroundDist, doctest::Approx(5000.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(263.6433165).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-58.709728).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.969632).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(2); // P3
                        CHECK_EQ(cumGroundDist, doctest::Approx(5000.0 + 2588.190).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::RightTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5000.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(258.6790449).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-93.673943).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.959813).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(3); // P4
                        CHECK_EQ(cumGroundDist, doctest::Approx(5000.0 + 2588.190 + 2588.191).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::RightTurn);
                        CHECK_EQ(pt.Radius, doctest::Approx(5000.0).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Heading, doctest::Approx(261.7530114).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-120.599998).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.949825).epsilon(Constants::PrecisionTest));
                    }
                    {
                        const auto& [cumGroundDist, pt] = output.point(4); // P5
                        CHECK_EQ(cumGroundDist, doctest::Approx(5000.0 + 2588.190 + 2588.191 + 2588.191).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Dir, RouteOutput::Direction::Straight);
                        CHECK_EQ(pt.Radius, Constants::Inf);
                        CHECK_EQ(pt.Heading, doctest::Approx(261.7530114).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Longitude, doctest::Approx(-143.80259440).epsilon(Constants::PrecisionTest));
                        CHECK_EQ(pt.Latitude, doctest::Approx(89.94179317).epsilon(Constants::PrecisionTest));
                    }
                }
            }
        }
    }
}
