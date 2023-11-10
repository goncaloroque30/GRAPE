// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "RouteOutput.h"

#include "Runway.h"

#include "Base/CoordinateSystem.h"
#include "Base/Math.h"

namespace GRAPE {

    RouteOutput::RouteOutput(const Runway& Rwy) noexcept {
        m_Output.try_emplace(0.0, Rwy.Longitude, Rwy.Latitude, Rwy.Heading, Constants::Inf, Direction::Straight);
    }

    void RouteOutput::addPoint(double CumulativeGroundDistance, double Longitude, double Latitude, double Heading, double Radius, Direction Dir) {
        m_Output.try_emplace(CumulativeGroundDistance, Longitude, Latitude, Heading, Radius, Dir);
    }

    void RouteOutput::recalculateHeadings(const CoordinateSystem& Cs) {
        for (auto it = std::next(m_Output.begin()); it != m_Output.end(); ++it)
        {
            auto& [p1CumDist, p1] = *std::prev(it);
            const auto& [p2cumDist, p2] = *it;
            p1.Heading = Cs.heading(p1.Longitude, p1.Latitude, p2.Longitude, p2.Latitude);
        }

        // Set heading of last point to be the same as the previous
        if (size() >= 2)
            m_Output.rbegin()->second.Heading = std::next(m_Output.rbegin())->second.Heading;
    }

    double RouteOutput::turnRadius(double CumulativeGroundDistance) const {
        return previousPoint(CumulativeGroundDistance)->second.Radius;
    }

    double RouteOutput::heading(double CumulativeGroundDistance) const {
        return previousPoint(CumulativeGroundDistance)->second.Heading;
    }

    double RouteOutput::turnRadiusChange(double StartCumulativeGroundDistance, double EndCumulativeGroundDistance) const {
        auto startPt = previousPoint(StartCumulativeGroundDistance);
        const auto endPt = previousPoint(EndCumulativeGroundDistance);

        if (startPt == endPt)
            return Constants::NaN;

        const double startRadius = startPt->second.Radius;
        for (auto it = ++startPt; it != endPt; ++it) // Increment startPt to be sure that change is after StartCumulativeGroundDistance
        {
            if (std::abs(it->second.Radius - startRadius) > Constants::Precision)
                return it->first;
        }

        return Constants::NaN;
    }

    RouteOutput::Point RouteOutput::interpolate(const CoordinateSystem& Cs, double CumulativeGroundDistance) const {
        GRAPE_ASSERT(!empty());

        auto nextNode = m_Output.lower_bound(CumulativeGroundDistance);

        // Point is past the end, use last heading to calculate latitude and longitude
        if (nextNode == m_Output.end())
        {
            const auto& [LastCumulativeGroundDistance, LastPoint] = *m_Output.rbegin();
            auto [lon, lat] = Cs.point(LastPoint.Longitude, LastPoint.Latitude, std::abs(CumulativeGroundDistance - LastCumulativeGroundDistance), LastPoint.Heading);
            return Point{ lon, lat, LastPoint.Heading, Constants::Inf, LastPoint.Dir };
        }

        // Point is before the start, use first point data and heading + 180 to calculate latitude longitude
        if (nextNode == m_Output.begin())
        {
            const auto& [FirstCumulativeGroundDistance, FirstData] = *m_Output.begin();
            auto [lon, lat] = Cs.point(FirstData.Longitude, FirstData.Latitude, std::abs(CumulativeGroundDistance - FirstCumulativeGroundDistance), normalizeHeading(FirstData.Heading + 180.0));
            return Point{ lon, lat, FirstData.Heading, Constants::Inf, FirstData.Dir };
        }

        const auto& [NextCumulativeGroundDistance, NextPoint] = *nextNode;
        const auto& [PreviousCumulativeGroundDistance, PreviousPoint] = *--nextNode;

        // Point already in the container
        if (std::abs(NextCumulativeGroundDistance - CumulativeGroundDistance) < Constants::Precision)
            return NextPoint;

        if (std::abs(PreviousCumulativeGroundDistance - CumulativeGroundDistance) < Constants::Precision)
            return PreviousPoint;

        // Point between defined route
        auto [lon, lat] = Cs.point(PreviousPoint.Longitude, PreviousPoint.Latitude, std::abs(CumulativeGroundDistance - PreviousCumulativeGroundDistance), PreviousPoint.Heading);
        return Point{ lon, lat, PreviousPoint.Heading, PreviousPoint.Radius, PreviousPoint.Dir };
    }

    std::map<double, RouteOutput::Point>::const_iterator RouteOutput::previousPoint(double CumulativeGroundDistance) const {
        GRAPE_ASSERT(!empty());

        auto it = m_Output.upper_bound(CumulativeGroundDistance); // it key is greater or equal to CumulativeGroundDistance
        return it == m_Output.begin() ? it : --it;
        // it points to begin if CumulativeGroundDistance smaller than first point 
        // it points to end - 1 if CumulativeGroundDistance greater than last point
    }

    TEST_CASE("Turn Radius Change") {
        RouteOutput out;

        // Populate output with straight, right turn, straight (only cumulative ground distance and turn radius / direction relevant);
        out.addPoint(0.0, 0.0, 0.0, 0.0);
        out.addPoint(2000.0, 0.0, 0.0, 0.0);
        out.addPoint(3000.0, 0.0, 0.0, 0.0, 2000.0, RouteOutput::Direction::RightTurn);
        out.addPoint(3500.0, 0.0, 0.0, 0.0, 3000.0, RouteOutput::Direction::RightTurn);
        out.addPoint(4000.0, 0.0, 0.0, 0.0);

        // Test getting the turn radius at different cumulative ground distances
        CHECK_EQ(out.turnRadius(-500.0), Constants::Inf);
        CHECK_EQ(out.turnRadius(2000.0), Constants::Inf);
        CHECK_EQ(out.turnRadius(3000.0), 2000.0);
        CHECK_EQ(out.turnRadius(3200.0), 2000.0);
        CHECK_EQ(out.turnRadius(3499.0), 2000.0);
        CHECK_EQ(out.turnRadius(4500.0), Constants::Inf);

        // Test getting turn radius changes
        CHECK(std::isnan(out.turnRadiusChange(-500.0, 1000.0)));
        CHECK(std::isnan(out.turnRadiusChange(500.0, 1500.0)));
        CHECK(std::isnan(out.turnRadiusChange(3500.0, 4500.0)));
        CHECK(std::isnan(out.turnRadiusChange(3000.0, 3500.0)));
        CHECK_EQ(out.turnRadiusChange(2500, 3500.0), 3000.0);
        CHECK_EQ(out.turnRadiusChange(-500.0, 3500.0), 3000.0);
        CHECK_EQ(out.turnRadiusChange(-500.0, 5000.0), 3000.0);
    }
}
