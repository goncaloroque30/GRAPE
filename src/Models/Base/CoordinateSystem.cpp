// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "CoordinateSystem.h"

#include "Conversions.h"
#include "Math.h"

namespace GRAPE {
    /**
    * Converts points 1 and 2 to cartesian
    * Calculates the distance as the hypotenuse of the x and y differences
    */
    double LocalCartesian::distance(double Longitude1, double Latitude1, double Longitude2, double Latitude2) const {
        double p1X, p1Y, p2X, p2Y, z;
        m_LocalCartesian.Forward(Latitude1, Longitude1, 0, p1X, p1Y, z);
        m_LocalCartesian.Forward(Latitude2, Longitude2, 0, p2X, p2Y, z);
        return std::hypot(p2X - p1X, p2Y - p1Y);
    }

    /**
    * Converts points 1 and 2 to cartesian
    * Calculates the azimuth based on the arctangent of the angle
    */
    double LocalCartesian::heading(double Longitude1, double Latitude1, double Longitude2, double Latitude2) const {
        double p1X, p1Y, p2X, p2Y, z;
        m_LocalCartesian.Forward(Latitude1, Longitude1, 0, p1X, p1Y, z);
        m_LocalCartesian.Forward(Latitude2, Longitude2, 0, p2X, p2Y, z);
        const double hdg = fromRadians(std::atan2(p2X - p1X, p2Y - p1Y)); // x and y are inverted as 0 is at north
        return hdg < 0.0 ? hdg + 360.0 : hdg; // Convert from ]-180, 180] to ]0, 360]
    }

    std::pair<double, double> LocalCartesian::distanceHeading(double Longitude1, double Latitude1, double Longitude2, double Latitude2) const {
        return std::make_pair(distance(Longitude1, Latitude1, Longitude2, Latitude2), heading(Longitude1, Latitude1, Longitude2, Latitude2));
    }

    std::pair<double, double> LocalCartesian::distanceHeadingEnd(double Longitude1, double Latitude1, double Longitude2, double Latitude2) const {
        return distanceHeading(Longitude1, Latitude1, Longitude2, Latitude2);
    }

    /**
    * Converts point 1 to cartesian
    * Calculates point 2 coordinates with sinus and cosine of the heading
    * Returns longitude latitude by converting back to WGS84
    */
    std::pair<double, double> LocalCartesian::point(double Longitude1, double Latitude1, double Distance, double Heading) const {
        double p1X, p1Y, z;
        m_LocalCartesian.Forward(Latitude1, Longitude1, 0, p1X, p1Y, z);
        const double p2X = p1X + Distance * std::sin(toRadians(Heading));
        const double p2Y = p1Y + Distance * std::cos(toRadians(Heading));
        double lon2, lat2;
        m_LocalCartesian.Reverse(p2X, p2Y, 0, lat2, lon2, z);
        return std::make_pair(lon2, lat2);
    }

    /**
    * Converts point 1 to cartesian
    * Calculates point 2 coordinates with sinus and cosine of the heading
    * Returns longitude, latitude and heading by converting back to WGS84
    */
    std::tuple<double, double, double> LocalCartesian::pointHeadingEnd(double Longitude1, double Latitude1, double Distance, double Heading) const {
        auto [lon2, lat2] = point(Longitude1, Latitude1, Distance, Heading);
        return std::make_tuple(lon2, lat2, Heading);
    }

    std::tuple<double, double, CoordinateSystem::Intersection> LocalCartesian::intersection(double Longitude1, double Latitude1, double Longitude2, double Latitude2, double Longitude3, double Latitude3) const {
        double x1, y1, z1;
        m_LocalCartesian.Forward(Latitude1, Longitude1, 0.0, x1, y1, z1);

        double x2, y2, z2;
        m_LocalCartesian.Forward(Latitude2, Longitude2, 0.0, x2, y2, z2);

        double x3, y3, z3;
        m_LocalCartesian.Forward(Latitude3, Longitude3, 0.0, x3, y3, z3);

        // Vector P1 P2
        const double p12X = x2 - x1;
        const double p12Y = y2 - y1;

        // Vector P1 P3
        const double p13X = x3 - x1;
        const double p13Y = y3 - y1;

        const double dotParam = (p12X * p13X + p12Y * p13Y) / (p12X * p12X + p12Y * p12Y);

        // Intersection
        const double intersectX = x1 + dotParam * p12X;
        const double intersectY = y1 + dotParam * p12Y;

        double lonI, latI, altI;
        m_LocalCartesian.Reverse(intersectX, intersectY, 0.0, latI, lonI, altI);

        auto intersectionLoc = Intersection::Behind;

        // Calculate 5cm precision.
        const double prec5Cm = 0.05 / std::hypot(p12X, p12Y);

        if (dotParam > -prec5Cm)
            intersectionLoc = dotParam < 1.0 + prec5Cm ? Intersection::Between : Intersection::Ahead;

        return { lonI, latI, intersectionLoc };
    }

    std::pair<double, double> LocalCartesian::reverse(double X, double Y) const {
        double lon, lat, alt;
        m_LocalCartesian.Reverse(X, Y, 0.0, lon, lat, alt);
        return { lon, lat };
    }

    int LocalCartesian::turnDirection(double Longitude1, double Latitude1, double Longitude2, double Latitude2, double Longitude3, double Latitude3) const {
        const double hdgDiff = normalizeHeading(heading(Longitude2, Latitude2, Longitude3, Latitude3) - heading(Longitude1, Latitude1, Longitude2, Latitude2));
        return hdgDiff > 180 ? -1 : 1;
    }

    void LocalCartesian::resetE(const double Longitude0, const double Latitude0, const double Altitude0) {
        if (!(Longitude0 >= -180.0 && Longitude0 <= 180.0))
            throw GrapeException("Longitude must be between -180.0 and 180.0.");

        if (!(Latitude0 >= -90.0 && Latitude0 <= 90.0))
            throw GrapeException("Latitude must be between -90.0 and 90.0.");

        reset(Longitude0, Latitude0, Altitude0);
    }

    /**
    * Visitor pattern
    */
    void LocalCartesian::accept(CoordinateSystemVisitor& Cs) {
        Cs.visitLocalCartesian(*this);
    }

    void LocalCartesian::accept(CoordinateSystemVisitor& Cs) const {
        Cs.visitLocalCartesian(*this);
    }

    /**
    * Distance is part of the inverse problem on a geodesic
    */
    double Geodesic::distance(double Longitude1, double Latitude1, double Longitude2, double Latitude2) const {
        double dist;
        m_Geodesic.Inverse(Latitude1, Longitude1, Latitude2, Longitude2, dist);
        return dist;
    }

    /**
    * hdg is part of the inverse problem on a geodesic
    */
    double Geodesic::heading(double Longitude1, double Latitude1, double Longitude2, double Latitude2) const {
        double hdg1, hdg2;
        m_Geodesic.Inverse(Latitude1, Longitude1, Latitude2, Longitude2, hdg1, hdg2);
        return normalizeHeading(hdg1);
    }

    /**
    * End hdg is part of the inverse problem on a geodesic
    */
    double Geodesic::headingEnd(double Longitude1, double Latitude1, double Longitude2, double Latitude2) const {
        double hdg1, hdg2;
        m_Geodesic.Inverse(Latitude1, Longitude1, Latitude2, Longitude2, hdg1, hdg2);
        return normalizeHeading(hdg2);
    }

    /**
    * Distance and heading are part of the inverse problem on a geodesic
    */
    std::pair<double, double> Geodesic::distanceHeading(double Longitude1, double Latitude1, double Longitude2, double Latitude2) const {
        double dist, hdg1, hdg2;
        m_Geodesic.Inverse(Latitude1, Longitude1, Latitude2, Longitude2, dist, hdg1, hdg2);
        return std::make_pair(dist, normalizeHeading(hdg1));
    }

    /**
    * Distance and heading are part of the inverse problem on a geodesic
    */
    std::pair<double, double> Geodesic::distanceHeadingEnd(double Longitude1, double Latitude1, double Longitude2, double Latitude2) const {
        double dist, hdg1, hdg2;
        m_Geodesic.Inverse(Latitude1, Longitude1, Latitude2, Longitude2, dist, hdg1, hdg2);
        return std::make_pair(dist, normalizeHeading(hdg2));
    }

    /**
    * Inverse problem
    * Determines point 2 based on distance and heading from point 1
    */
    std::pair<double, double> Geodesic::point(double Longitude1, double Latitude1, double Distance, double Heading) const {
        double lon2, lat2;
        m_Geodesic.Direct(Latitude1, Longitude1, Heading, Distance, lat2, lon2);
        return std::make_pair(lon2, lat2);
    }

    /**
    * Inverse problem
    * Determines point 2 based on distance and heading from point 1. Returns lat and lon of calculated point, and its heading.
    */
    std::tuple<double, double, double> Geodesic::pointHeadingEnd(double Longitude1, double Latitude1, double Distance, double Heading) const {
        double lon2, lat2, hdg2;
        m_Geodesic.Direct(Latitude1, Longitude1, Heading, Distance, lat2, lon2, hdg2);
        return std::make_tuple(lon2, lat2, normalizeHeading(hdg2));
    }

    /**
     * Intersection Point
     * Returns the intersection point and type ('behind', 'between' or 'ahead') between a line formed by two points (P1 & P2) and a line perpendicular to a third point (P3). Input arguments are longitude and latitude of those 3 points. Points 1 and 2 describe a geodesic. Intersection point is calculated with the shortest distance from point 3 to geodesic.
     *
     * The algorithm used can be found in chapter 3 of the paper 'Intersection and point-to-line solutions for geodesics on the ellipsoid' by S. Baselga and J.C. Martinez-Llario (https://doi.org/10.1007/s11200-017-1020-z). An iterative process is used, where a point is gradually moved towards the intersection until the distance in every iteration is smaller than the required precision.
     */
    namespace {
        // Distance I3 corresponds to sPX (equation 8) 
        double dist3X(double EquatorialRadius, double AngleA, double DistI3) {
            return EquatorialRadius * asin(sin(DistI3 / EquatorialRadius) * sin(toRadians(AngleA)));
        }

        // Distance IX corresponds to sAX (equation 10)
        double distIntersectionX(double EquatorialRadius, double AngleA, double DistI3) {
            return  2 * EquatorialRadius * atan(tan((DistI3 - dist3X(EquatorialRadius, AngleA, DistI3)) / (2 * EquatorialRadius)) * sin(toRadians((90 + AngleA) / 2)) / sin(toRadians((90 - AngleA) / 2)));
        }
    }

    std::tuple<double, double, CoordinateSystem::Intersection> Geodesic::intersection(double Longitude1, double Latitude1, double Longitude2, double Latitude2, double Longitude3, double Latitude3) const {
        double dist12, distIn2, distIn3, aziStartI2, aziStartI3, aziEndI2, aziEndI3;
        double aziStart21, aziEnd21, aziStart23, aziEnd23;
        double latI = Latitude1;
        double lonI = Longitude1;
        double s1I = 0.0;
        bool behind = false;
        auto intersection = Intersection::Between;

        // First iteration

        // Geometries between I, 2 and 3
        m_Geodesic.Inverse(latI, lonI, Latitude2, Longitude2, distIn2, aziStartI2, aziEndI2);
        m_Geodesic.Inverse(latI, lonI, Latitude3, Longitude3, distIn3, aziStartI3, aziEndI3);
        m_Geodesic.Inverse(Latitude1, Longitude1, Latitude2, Longitude2, dist12);

        m_Geodesic.Inverse(Latitude2, Longitude2, Latitude1, Longitude1, aziStart21, aziEnd21);
        m_Geodesic.Inverse(Latitude2, Longitude2, Latitude3, Longitude3, aziStart23, aziEnd23);

        // Detect angles that are 90 deg (avoid nan return)
        // 1st: check if P3 orthogonal to P1
        if (std::abs(headingDifference(aziStartI2, aziStartI3) - 90) < Constants::AngleThreshold)
            return { Longitude1, Latitude1, Intersection::Between };

        // 2nd: check if P3 orthogonal to P2
        if (std::abs(headingDifference(aziStart21, aziStart23) - 90) < Constants::AngleThreshold)
            return { Longitude2, Latitude2, Intersection::Between };

        // Distance IX
        double distInX = distIntersectionX(m_Geodesic.EquatorialRadius(), headingDifference(aziStartI2, aziStartI3), distIn3);

        // Check direction in 1st iteration, 5cm precision around P1 for detection of behind.
        if (distInX < -Constants::DistanceThreshold)
            behind = true;

        // Move I towards X by sIX
        m_Geodesic.Direct(latI, lonI, aziStartI2, distInX, latI, lonI);

        // begin iteration 
        while (std::abs(distInX) > Constants::Precision)
        {
            // Geometries between I, 2 and 3
            m_Geodesic.Inverse(latI, lonI, Latitude2, Longitude2, distIn2, aziStartI2, aziEndI2);
            m_Geodesic.Inverse(latI, lonI, Latitude3, Longitude3, distIn3, aziStartI3, aziEndI3);

            // Distance to (temp-)intersection
            m_Geodesic.Inverse(Latitude1, Longitude1, latI, lonI, s1I);

            // Detect angles that are 90 deg (avoid nan return). Exit loop if angle hits 90 deg. 
            if (std::abs(headingDifference(aziStartI2, aziStartI3) - 90) < Constants::AngleThreshold)
            {
                // Determine intersection type. 5cm precision around P2 for detection of ahead.
                intersection = behind ? Intersection::Behind : s1I > dist12 + 0.05 ? Intersection::Ahead : Intersection::Between;
                return { lonI, latI, intersection };
            }

            // Distance IX
            distInX = distIntersectionX(m_Geodesic.EquatorialRadius(), headingDifference(aziStartI2, aziStartI3), distIn3);

            // Move I towards X by sIX
            m_Geodesic.Direct(latI, lonI, aziStartI2, distInX, latI, lonI);
        }

        // Distance from 1 to estimated I
        m_Geodesic.Inverse(Latitude1, Longitude1, latI, lonI, s1I);

        // Determine intersection type. 5cm precision around P2 for detection of ahead.
        intersection = behind ? Intersection::Behind : s1I > dist12 + 0.05 ? Intersection::Ahead : Intersection::Between;

        return { lonI, latI, intersection };
    }

    int Geodesic::turnDirection(double Longitude1, double Latitude1, double Longitude2, double Latitude2, double Longitude3, double Latitude3) const {
        const double hdgDiff = normalizeHeading(heading(Longitude2, Latitude2, Longitude3, Latitude3) - headingEnd(Longitude1, Latitude1, Longitude2, Latitude2));
        return hdgDiff > 180 ? -1 : 1;
    }

    /**
    * Visitor Pattern
    */
    void Geodesic::accept(CoordinateSystemVisitor& Cs) {
        Cs.visitGeodesic(*this);
    }

    void Geodesic::accept(CoordinateSystemVisitor& Cs) const {
        Cs.visitGeodesic(*this);
    }

    TEST_CASE("Local cartesian") {
        auto lc = LocalCartesian(0.0, 0.0);
        std::tuple<double, double, CoordinateSystem::Intersection> intersection;

        /**
         * P1 = (10.0, 50.0), P2 = (10.001, 50.001)
         * Dist = 132.333 m
         * AziStart12 = 32.804262 deg
         */
        SUBCASE("Standard") {
            lc = LocalCartesian(10.0004, 50.0005);

            // Distance
            CHECK_EQ(lc.distance(10.0, 50.0, 10.001, 50.001), doctest::Approx(132.333).epsilon(Constants::PrecisionTest));

            // Heading
            CHECK_EQ(lc.heading(10.0, 50.0, 10.001, 50.001), doctest::Approx(32.804262).epsilon(Constants::PrecisionTest));
            /// Would fail if coordinate system is positioned too far away from P1.

            // Point
            auto [lon2, lat2] = lc.point(10.0, 50.0, 132.333, 32.804262);
            CHECK_EQ(lon2, doctest::Approx(10.001).epsilon(Constants::PrecisionTest));
            CHECK_EQ(lat2, doctest::Approx(50.001).epsilon(Constants::PrecisionTest));

            auto [lon1, lat1] = lc.point(10.001, 50.001, 132.333, normalizeHeading(32.805028 + 180.0));
            CHECK_EQ(lon1, doctest::Approx(10.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(lat1, doctest::Approx(50.0).epsilon(Constants::PrecisionTest));

            // Intersection Between
            intersection = lc.intersection(10.0, 50.0, 10.001, 50.001, 10.000512, 50.000588);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(10.000565).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(50.000565).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // Intersection Ahead
            intersection = lc.intersection(10.0, 50.0, 10.001, 50.001, 10.001288, 50.001215);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(10.001236).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(50.001236).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Ahead, std::get<2>(intersection));

            // Intersection Behind
            intersection = lc.intersection(10.0, 50.0, 10.001, 50.001, 9.999771, 49.999820);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(9.999806).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(49.999806).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Behind, std::get<2>(intersection));

            // Intersection P3 = P1
            intersection = lc.intersection(10.0, 50.0, 10.001, 50.001, 10.0, 50.0);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(10.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(50.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // Intersection P3 = P2
            intersection = lc.intersection(10.0, 50.0, 10.001, 50.001, 10.001, 50.001);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(10.001).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(50.001).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // P3 = Intersection Point
            intersection = lc.intersection(10.0, 50.0, 10.001, 50.001, 10.000756, 50.000756);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(10.000756).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(50.000756).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // P3 orthogonal to P1
            intersection = lc.intersection(10.0, 50.0, 10.001, 50.001, 10.000234, 49.999903);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(10.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(50.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // P3 orthogonal to P2  
            intersection = lc.intersection(10.0, 50.0, 10.001, 50.001, 10.001293, 50.000878);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(10.001).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(50.001).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));
        }

        /**
         * P1 = (0.0, 89.999), P2 = (180.0, 89.999)
         * Dist = 223.388 m
         * AziStart12 = 0.0 deg
         */
        SUBCASE("Northpole") {
            lc = LocalCartesian(0.0, 90.0);

            // Distance
            CHECK_EQ(lc.distance(0.0, 89.999, 180.0, 89.999), doctest::Approx(223.388).epsilon(Constants::PrecisionTest));

            // Heading
            CHECK_EQ(lc.heading(0.0, 89.999, 180.0, 89.999), doctest::Approx(0.0).epsilon(Constants::PrecisionTest));

            // Point
            auto [lon2, lat2] = lc.point(0.0, 89.999, 223.388, 0.0);
            CHECK_EQ(lon2, doctest::Approx(180.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(lat2, doctest::Approx(89.999).epsilon(Constants::PrecisionTest));

            auto [lon1, lat1] = lc.point(180.0, 89.999, 223.388, 180.0);
            CHECK_EQ(lon1, doctest::Approx(0.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(lat1, doctest::Approx(89.999).epsilon(Constants::PrecisionTest));

            // Intersection Between
            intersection = lc.intersection(0.0, 89.999, 180.0, 89.999, 0.5, 89.9995);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(0.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(89.9995).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // Intersection Ahead
            intersection = lc.intersection(0.0, 89.999, 180.0, 89.999, -179.0, 89.9985);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(180.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(89.9985).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Ahead, std::get<2>(intersection));

            // Intersection Behind
            intersection = lc.intersection(0.0, 89.999, 180.0, 89.999, 1.0, 89.9985);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(0.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(89.9985).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Behind, std::get<2>(intersection));

            // Intersection P3 = P1
            intersection = lc.intersection(0.0, 89.999, 180.0, 89.999, 0.0, 89.999);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(0.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(89.999).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // Intersection P3 = P2
            intersection = lc.intersection(0.0, 89.999, 180.0, 89.999, 180.0, 89.999);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(180.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(89.999).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // Centered Intersection Between
            intersection = lc.intersection(0.0, 89.999, 180.0, 89.999, 90.0, 89.999);
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(90.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // P3 = Intersection Point
            intersection = lc.intersection(0.0, 89.999, 180.0, 89.999, 0.0, 89.999403);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(0.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(89.999403).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // P3 orthogonal to P1
            intersection = lc.intersection(0.0, 89.999, 180.0, 89.999, 10.151835, 89.998984);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(0.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(89.999).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // P3 orthogonal to P2
            intersection = lc.intersection(0.0, 89.999, 180.0, 89.999, 169.848165, 89.998984);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(180.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(89.999).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));
        }

        /**
         * P1 = (179.999, 0.0), P2 = (-179.999, 0.0)
         * Dist = 222.639 m
         * AziStart12 = 90.0 deg
         */
        SUBCASE("Equator along") {
            lc = LocalCartesian(180.0, 0.0);

            // Distance
            CHECK_EQ(lc.distance(179.999, 0.0, -179.999, 0.0), doctest::Approx(222.639).epsilon(Constants::PrecisionTest));

            // Heading
            CHECK_EQ(lc.heading(179.999, 0.0, -179.999, 0.0), doctest::Approx(90.0).epsilon(Constants::PrecisionTest));

            // Point
            auto [lon2, lat2] = lc.point(179.999, 0.0, 222.639, 90.0);
            CHECK_EQ(lon2, doctest::Approx(-179.999).epsilon(Constants::PrecisionTest));
            CHECK_EQ(lat2, doctest::Approx(0.0).epsilon(Constants::PrecisionTest));

            auto [lon1, lat1] = lc.point(-179.999, 0.0, 222.639, normalizeHeading(90.0 + 180.0));
            CHECK_EQ(lon1, doctest::Approx(179.999).epsilon(Constants::PrecisionTest));
            CHECK_EQ(lat1, doctest::Approx(0.0).epsilon(Constants::PrecisionTest));

            // Intersection Between
            intersection = lc.intersection(179.999, 0.0, -179.999, 0.0, -179.9995, 0.0002);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(-179.9995).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(0.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // Intersection Ahead
            intersection = lc.intersection(179.999, 0.0, -179.999, 0.0, -179.998, -0.00012);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(-179.998).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(0.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Ahead, std::get<2>(intersection));

            // Intersection Behind
            intersection = lc.intersection(179.999, 0.0, -179.999, 0.0, 179.998, 0.0001);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(179.998).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(0.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Behind, std::get<2>(intersection));

            // Intersection P3 = P1
            intersection = lc.intersection(179.999, 0.0, -179.999, 0.0, 179.999, 0.0);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(179.999).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(0.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // Intersection P3 = P2
            intersection = lc.intersection(179.999, 0.0, -179.999, 0.0, -179.999, 0.0);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(-179.999).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(0.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // Intersection P3 = Intersection Point
            intersection = lc.intersection(179.999, 0.0, -179.999, 0.0, 180.0, 0.0);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(180.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(0.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // P3 orthogonal to P1
            intersection = lc.intersection(179.999, 0.0, -179.999, 0.0, 179.999, 0.000723);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(179.999).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(0.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // P3 orthogonal to P2
            intersection = lc.intersection(179.999, 0.0, -179.999, 0.0, -179.999, -0.000543);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(-179.999).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(0.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));
        }

        /**
         * P1 = (18.35, 0.00015), P2 = (18.353, -0.0002)
         * Dist = 336.193 m
         * AziStart12 = 96.610273 deg
         */
        SUBCASE("Equator crossing") {
            lc = LocalCartesian(18.3515, 0.0);

            // Distance
            CHECK_EQ(lc.distance(18.35, 0.00015, 18.353, -0.0002), doctest::Approx(336.193).epsilon(Constants::PrecisionTest));

            // Heading
            CHECK_EQ(lc.heading(18.35, 0.00015, 18.353, -0.0002), doctest::Approx(96.610273).epsilon(Constants::PrecisionTest));

            // Point
            auto [lon2, lat2] = lc.point(18.35, 0.00015, 336.193, 96.610273);
            CHECK_EQ(lon2, doctest::Approx(18.353).epsilon(Constants::PrecisionTest));
            CHECK_EQ(lat2, doctest::Approx(-0.0002).epsilon(Constants::PrecisionTest));

            auto [lon1, lat1] = lc.point(18.353, -0.0002, 336.193, normalizeHeading(96.610273 + 180.0));
            CHECK_EQ(lon1, doctest::Approx(18.35).epsilon(Constants::PrecisionTest));
            CHECK_EQ(lat1, doctest::Approx(0.00015).epsilon(Constants::PrecisionTest));

            // Intersection Between
            intersection = lc.intersection(18.35, 0.00015, 18.353, -0.0002, 18.3515, -0.00001);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(18.351498).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(-0.000025).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // Intersection Ahead
            intersection = lc.intersection(18.35, 0.00015, 18.353, -0.0002, 18.354, -0.00035);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(18.354004).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(-0.000318).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Ahead, std::get<2>(intersection));

            // Intersection Behind
            intersection = lc.intersection(18.35, 0.00015, 18.353, -0.0002, 18.3485, 0.00025);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(18.348511).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(0.000323).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Behind, std::get<2>(intersection));

            // Intersection P3 = P1
            intersection = lc.intersection(18.35, 0.00015, 18.353, -0.0002, 18.35, 0.00015);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(18.35).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(0.00015).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // Intersection P3 = P2
            intersection = lc.intersection(18.35, 0.00015, 18.353, -0.0002, 18.353, -0.0002);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(18.353).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(-0.0002).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // Intersection P3 = Intersection Point
            intersection = lc.intersection(18.35, 0.00015, 18.353, -0.0002, 18.351347, -0.000007);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(18.351347).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(-0.000007).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // P3 orthogonal to P1
            intersection = lc.intersection(18.35, 0.00015, 18.353, -0.0002, 18.349959, -0.000209);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(18.35).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(0.00015).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // P3 orthogonal to P2
            intersection = lc.intersection(18.35, 0.00015, 18.353, -0.0002, 18.352938, -0.000739);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(18.353).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(-0.0002).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));
        }
    }

    TEST_CASE("Geodesic Class") {
        Geodesic geo;
        std::tuple<double, double, CoordinateSystem::Intersection> intersection;

        /**
         * P1 = (10.0, 50.0), P2 = (10.001, 50.001)
         * Dist = 132.333 m
         * AziStart12 = 32.804262 deg
         */
        SUBCASE("Standard") {
            // Distance
            CHECK_EQ(geo.distance(10.0, 50.0, 10.001, 50.001), doctest::Approx(132.333221).epsilon(Constants::PrecisionTest));

            // Heading
            CHECK_EQ(geo.heading(10.0, 50.0, 10.001, 50.001), doctest::Approx(32.804262).epsilon(Constants::PrecisionTest));

            // HeadingEnd
            CHECK_EQ(geo.headingEnd(10.0, 50.0, 10.001, 50.001), doctest::Approx(32.805028).epsilon(Constants::PrecisionTest));

            // PointHeadingEnd
            auto [lon2, lat2, hdgEnd2] = geo.pointHeadingEnd(10.0, 50.0, 132.333221, 32.804262);
            CHECK_EQ(lon2, doctest::Approx(10.001).epsilon(Constants::PrecisionTest));
            CHECK_EQ(lat2, doctest::Approx(50.001).epsilon(Constants::PrecisionTest));
            CHECK_EQ(hdgEnd2, doctest::Approx(32.805028).epsilon(Constants::PrecisionTest));

            auto [lon1, lat1, hdgEnd1] = geo.pointHeadingEnd(10.001, 50.001, 132.333221, normalizeHeading(32.805028 + 180.0));
            CHECK_EQ(lon1, doctest::Approx(10.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(lat1, doctest::Approx(50.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(hdgEnd1, doctest::Approx(normalizeHeading(32.804262 + 180.0)).epsilon(Constants::PrecisionTest));

            // Intersection Between
            intersection = geo.intersection(10.0, 50.0, 10.001, 50.001, 10.000512, 50.000588);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(10.000565).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(50.000565).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // Intersection Ahead
            intersection = geo.intersection(10.0, 50.0, 10.001, 50.001, 10.001288, 50.001215);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(10.001236).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(50.001236).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Ahead, std::get<2>(intersection));

            // Intersection Behind
            intersection = geo.intersection(10.0, 50.0, 10.001, 50.001, 9.999771, 49.999820);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(9.999806).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(49.999806).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Behind, std::get<2>(intersection));

            // Intersection P3 = P1
            intersection = geo.intersection(10.0, 50.0, 10.001, 50.001, 10.0, 50.0);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(10.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(50.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // Intersection P3 = P2
            intersection = geo.intersection(10.0, 50.0, 10.001, 50.001, 10.001, 50.001);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(10.001).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(50.001).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // P3 = Intersection Point
            intersection = geo.intersection(10.0, 50.0, 10.001, 50.001, 10.000756, 50.000756);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(10.000756).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(50.000756).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // P3 orthogonal to P1
            intersection = geo.intersection(10.0, 50.0, 10.001, 50.001, 10.000234, 49.999903);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(10.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(50.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // P3 orthogonal to P2  
            intersection = geo.intersection(10.0, 50.0, 10.001, 50.001, 10.001293, 50.000878);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(10.001).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(50.001).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));
        }

        /**
         * P1 = (6.145280, 50.800384), P2 = (6.266903, 50.849234)
         * Dist = 10147.540 m
         * AziStart12 = 57.573000 deg
         */
        SUBCASE("Long distance") {
            // Distance
            CHECK_EQ(geo.distance(6.145280, 50.800384, 6.266903, 50.849234), doctest::Approx(10147.540).epsilon(Constants::PrecisionTest));

            // Heading
            CHECK_EQ(geo.heading(6.145280, 50.800384, 6.266903, 50.849234), doctest::Approx(57.573000).epsilon(Constants::PrecisionTest));

            // HeadingEnd
            CHECK_EQ(geo.headingEnd(6.145280, 50.800384, 6.266903, 50.849234), doctest::Approx(57.667284).epsilon(Constants::PrecisionTest));

            // PointHeadingEnd
            auto [lon2, lat2, hdgEnd2] = geo.pointHeadingEnd(6.145280, 50.800384, 10147.540, 57.573);
            CHECK_EQ(lon2, doctest::Approx(6.266903).epsilon(Constants::PrecisionTest));
            CHECK_EQ(lat2, doctest::Approx(50.849234).epsilon(Constants::PrecisionTest));
            CHECK_EQ(hdgEnd2, doctest::Approx(57.667284).epsilon(Constants::PrecisionTest));

            auto [lon1, lat1, hdgEnd1] = geo.pointHeadingEnd(6.266903, 50.849234, 10147.540, normalizeHeading(57.667284 + 180.0));
            CHECK_EQ(lon1, doctest::Approx(6.145280).epsilon(Constants::PrecisionTest));
            CHECK_EQ(lat1, doctest::Approx(50.800384).epsilon(Constants::PrecisionTest));
            CHECK_EQ(hdgEnd1, doctest::Approx(normalizeHeading(57.573000 + 180.0)).epsilon(Constants::PrecisionTest));

            // Intersection Between
            intersection = geo.intersection(6.145280, 50.800384, 6.266903, 50.849234, 6.188276, 50.826079);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(6.194274).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(50.820090).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // Intersection Ahead
            intersection = geo.intersection(6.145280, 50.800384, 6.266903, 50.849234, 6.345080, 50.902254);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(6.360654).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(50.886746).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Ahead, std::get<2>(intersection));

            // Intersection Behind
            intersection = geo.intersection(6.145280, 50.800384, 6.266903, 50.849234, 6.082482, 50.788049);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(6.091754).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(50.778796).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Behind, std::get<2>(intersection));

            // Intersection P3 = P1
            intersection = geo.intersection(6.145280, 50.800384, 6.266903, 50.849234, 6.145280, 50.800384);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(6.145280).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(50.800384).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // Intersection P3 = P2
            intersection = geo.intersection(6.145280, 50.800384, 6.266903, 50.849234, 6.266903, 50.849234);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(6.266903).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(50.849234).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // Intersection P3 = Intersection Point
            intersection = geo.intersection(6.145280, 50.800384, 6.266903, 50.849234, 6.194740, 50.820277);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(6.194740).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(50.820277).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // P3 orthogonal to P1
            intersection = geo.intersection(6.145280, 50.800384, 6.266903, 50.849234, 6.152885, 50.792796);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(6.145280).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(50.800384).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // P3 orthogonal to P2
            intersection = geo.intersection(6.145280, 50.800384, 6.266903, 50.849234, 6.272978, 50.843157);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(6.266903).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(50.849234).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));
        }

        /**
         * P1 = (0.0, 89.999), P2 = (180.0, 89.999)
         * Dist = 223.388 m
         * AziStart12 = 0.0 deg
         */
        SUBCASE("Northpole") {
            // Distance
            CHECK_EQ(geo.distance(0.0, 89.999, 180.0, 89.999), doctest::Approx(223.388).epsilon(Constants::PrecisionTest));

            // Heading
            CHECK_EQ(geo.heading(0.0, 89.999, 180.0, 89.999), doctest::Approx(0.0).epsilon(Constants::PrecisionTest));

            // HeadingEnd
            CHECK_EQ(geo.headingEnd(0.0, 89.999, 180.0, 89.999), doctest::Approx(180.0).epsilon(Constants::PrecisionTest));

            // PointHeadingEnd
            auto [lon2, lat2, hdgEnd2] = geo.pointHeadingEnd(0.0, 89.999, 223.388, 0.0);
            CHECK_EQ(lon2, doctest::Approx(180.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(lat2, doctest::Approx(89.999).epsilon(Constants::PrecisionTest));
            CHECK_EQ(hdgEnd2, doctest::Approx(180.0).epsilon(Constants::PrecisionTest));

            auto [lon1, lat1, hdgEnd1] = geo.pointHeadingEnd(180.0, 89.999, 223.388, 0.0);
            CHECK_EQ(lon1, doctest::Approx(0.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(lat1, doctest::Approx(89.999).epsilon(Constants::PrecisionTest));
            CHECK_EQ(hdgEnd1, doctest::Approx(normalizeHeading(0.0 + 180.0)).epsilon(Constants::PrecisionTest));

            // Intersection Between
            intersection = geo.intersection(0.0, 89.999, 180.0, 89.999, 0.5, 89.9995);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(0.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(89.9995).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // Intersection Ahead
            intersection = geo.intersection(0.0, 89.999, 180.0, 89.999, -179.0, 89.9985);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(180.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(89.9985).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Ahead, std::get<2>(intersection));

            // Intersection Behind
            intersection = geo.intersection(0.0, 89.999, 180.0, 89.999, 1.0, 89.9985);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(0.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(89.9985).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Behind, std::get<2>(intersection));

            // Intersection P3 = P1
            intersection = geo.intersection(0.0, 89.999, 180.0, 89.999, 0.0, 89.999);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(0.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(89.999).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // Intersection P3 = P2
            intersection = geo.intersection(0.0, 89.999, 180.0, 89.999, 180.0, 89.999);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(180.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(89.999).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // Centered Intersection Between
            /// Would fail (loop): Fixed by exiting loop at 90 degrees.
            intersection = geo.intersection(0.0, 89.999, 180.0, 89.999, 90.0, 89.999);
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(90.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // P3 = Intersection Point
            intersection = geo.intersection(0.0, 89.999, 180.0, 89.999, 0.0, 89.999403);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(0.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(89.999403).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // P3 orthogonal to P1
            /// Would fail (loop): Fixed by exiting loop at 90 degrees.
            intersection = geo.intersection(0.0, 89.999, 180.0, 89.999, 10.151835, 89.998984);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(0.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(89.999).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // P3 orthogonal to P2
            /// Would fail (loop): Fixed by exiting loop at 90 degrees.
            intersection = geo.intersection(0.0, 89.999, 180.0, 89.999, 169.848165, 89.998984);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(180.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(89.999).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));
        }

        /**
         * P1 = (179.999, 0.0), P2 = (-179.999, 0.0)
         * Dist = 222.639 m
         * AziStart12 = 90.0 deg
         */
        SUBCASE("Equator along") {
            // Distance
            CHECK_EQ(geo.distance(179.999, 0.0, -179.999, 0.0), doctest::Approx(222.639).epsilon(Constants::PrecisionTest));

            // Heading
            CHECK_EQ(geo.heading(179.999, 0.0, -179.999, 0.0), doctest::Approx(90.0).epsilon(Constants::PrecisionTest));

            // HeadingEnd
            CHECK_EQ(geo.headingEnd(179.999, 0.0, -179.999, 0.0), doctest::Approx(90.0).epsilon(Constants::PrecisionTest));

            // PointHeadingEnd
            auto [lon2, lat2, hdgEnd2] = geo.pointHeadingEnd(179.999, 0.0, 222.639, 90.0);
            CHECK_EQ(lon2, doctest::Approx(-179.999).epsilon(Constants::PrecisionTest));
            CHECK_EQ(lat2, doctest::Approx(0.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(hdgEnd2, doctest::Approx(90.0).epsilon(Constants::PrecisionTest));

            auto [lon1, lat1, hdgEnd1] = geo.pointHeadingEnd(-179.999, 0.0, 222.639, normalizeHeading(90.0 + 180.0));
            CHECK_EQ(lon1, doctest::Approx(179.999).epsilon(Constants::PrecisionTest));
            CHECK_EQ(lat1, doctest::Approx(0.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(hdgEnd1, doctest::Approx(normalizeHeading(90.0 + 180)).epsilon(Constants::PrecisionTest));

            // Intersection Between
            /// Wold fail (nan) if angle hits exactly 90 degrees.
            intersection = geo.intersection(179.999, 0.0, -179.999, 0.0, -179.9995, 0.0002);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(-179.9995).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(0.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // Intersection Ahead
            intersection = geo.intersection(179.999, 0.0, -179.999, 0.0, -179.998, -0.00012);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(-179.998).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(0.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Ahead, std::get<2>(intersection));

            // Intersection Behind
            /// Wold fail (nan) if angle hits exactly 90 degrees.
            intersection = geo.intersection(179.999, 0.0, -179.999, 0.0, 179.998, 0.0001);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(179.998).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(0.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Behind, std::get<2>(intersection));

            // Intersection P3 = P1
            intersection = geo.intersection(179.999, 0.0, -179.999, 0.0, 179.999, 0.0);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(179.999).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(0.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // Intersection P3 = P2
            intersection = geo.intersection(179.999, 0.0, -179.999, 0.0, -179.999, 0.0);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(-179.999).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(0.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // Intersection P3 = Intersection Point
            intersection = geo.intersection(179.999, 0.0, -179.999, 0.0, 180.0, 0.0);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(180.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(0.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // P3 orthogonal to P1
            intersection = geo.intersection(179.999, 0.0, -179.999, 0.0, 179.999, 0.000723);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(179.999).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(0.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // P3 orthogonal to P2
            intersection = geo.intersection(179.999, 0.0, -179.999, 0.0, -179.999, -0.000543);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(-179.999).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(0.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));
        }

        /**
         * P1 = (18.35, 0.00015), P2 = (18.353, -0.0002)
         * Dist = 336.193 m
         * AziStart12 = 96.610273 deg
         */
        SUBCASE("Equator crossing") {
            // Distance
            CHECK_EQ(geo.distance(18.35, 0.00015, 18.353, -0.0002), doctest::Approx(336.193).epsilon(Constants::PrecisionTest));

            // Heading
            CHECK_EQ(geo.heading(18.35, 0.00015, 18.353, -0.0002), doctest::Approx(96.610273).epsilon(Constants::PrecisionTest));

            // HeadingEnd
            CHECK_EQ(geo.headingEnd(18.35, 0.00015, 18.353, -0.0002), doctest::Approx(96.610273).epsilon(Constants::PrecisionTest));

            // Point
            auto [lon2, lat2, hdgEnd2] = geo.pointHeadingEnd(18.35, 0.00015, 336.193, 96.610273);
            CHECK_EQ(lon2, doctest::Approx(18.353).epsilon(Constants::PrecisionTest));
            CHECK_EQ(lat2, doctest::Approx(-0.0002).epsilon(Constants::PrecisionTest));
            CHECK_EQ(hdgEnd2, doctest::Approx(96.610273).epsilon(Constants::PrecisionTest));

            auto [lon1, lat1, hdgEnd1] = geo.pointHeadingEnd(18.353, -0.0002, 336.193, normalizeHeading(96.610273 + 180.0));
            CHECK_EQ(lon1, doctest::Approx(18.35).epsilon(Constants::PrecisionTest));
            CHECK_EQ(lat1, doctest::Approx(0.00015).epsilon(Constants::PrecisionTest));
            CHECK_EQ(hdgEnd1, doctest::Approx(normalizeHeading(96.610273 + 180.0)).epsilon(Constants::PrecisionTest));

            // Intersection Between
            intersection = geo.intersection(18.35, 0.00015, 18.353, -0.0002, 18.3515, -0.00001);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(18.351498).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(-0.000025).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // Intersection Ahead
            intersection = geo.intersection(18.35, 0.00015, 18.353, -0.0002, 18.354, -0.00035);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(18.354004).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(-0.000318).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Ahead, std::get<2>(intersection));

            // Intersection Behind
            intersection = geo.intersection(18.35, 0.00015, 18.353, -0.0002, 18.3485, 0.00025);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(18.348511).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(0.000323).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Behind, std::get<2>(intersection));

            // Intersection P3 = P1
            intersection = geo.intersection(18.35, 0.00015, 18.353, -0.0002, 18.35, 0.00015);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(18.35).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(0.00015).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // Intersection P3 = P2
            intersection = geo.intersection(18.35, 0.00015, 18.353, -0.0002, 18.353, -0.0002);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(18.353).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(-0.0002).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // Intersection P3 = Intersection Point
            intersection = geo.intersection(18.35, 0.00015, 18.353, -0.0002, 18.351347, -0.000007);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(18.351347).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(-0.000007).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // P3 orthogonal to P1
            intersection = geo.intersection(18.35, 0.00015, 18.353, -0.0002, 18.349959, -0.000209);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(18.35).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(0.00015).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));

            // P3 orthogonal to P2
            intersection = geo.intersection(18.35, 0.00015, 18.353, -0.0002, 18.352937, -0.0007390);
            CHECK_EQ(std::get<0>(intersection), doctest::Approx(18.353).epsilon(Constants::PrecisionTest));
            CHECK_EQ(std::get<1>(intersection), doctest::Approx(-0.0002).epsilon(Constants::PrecisionTest));
            CHECK_EQ(CoordinateSystem::Intersection::Between, std::get<2>(intersection));
        }
    }

    TEST_CASE("Turn Direction Function") {
        SUBCASE("Geodesic class") {
            Geodesic geo;
            int output = 2;

            SUBCASE("Standard") {
                // left1
                output = geo.turnDirection(10.0, 50.0, 10.001, 50.001, 9.999830, 50.001671);
                CHECK_EQ(output, -1);

                // left2
                output = geo.turnDirection(10.0, 50.0, 10.001, 50.001, 9.998653, 49.999252);
                CHECK_EQ(output, -1);

                // right1
                output = geo.turnDirection(10.0, 50.0, 10.001, 50.001, 10.002667, 50.001298);
                CHECK_EQ(output, 1);

                // right2
                output = geo.turnDirection(10.0, 50.0, 10.001, 50.001, 10.000885, 50.000311);
                CHECK_EQ(output, 1);
            }

            SUBCASE("North pole") {
                // left1
                output = geo.turnDirection(0.0, 89.5, 180.0, 89.7, -170.331524, 89.096642);
                CHECK_EQ(output, -1);

                // left2
                output = geo.turnDirection(0.0, 89.5, 180.0, 89.7, -34.482593, 89.832157);
                CHECK_EQ(output, -1);

                // right1
                output = geo.turnDirection(0.0, 89.5, 180.0, 89.7, 138.219857, 89.098789);
                CHECK_EQ(output, 1);

                // right2
                output = geo.turnDirection(0.0, 89.5, 180.0, 89.7, 10.092353, 89.126110);
                CHECK_EQ(output, 1);
            }

            SUBCASE("Equator") {
                // left1
                output = geo.turnDirection(179.99, 0.01, -179.89, -0.008, -179.706966, 0.010142);
                CHECK_EQ(output, -1);

                // left2
                output = geo.turnDirection(179.99, 0.01, -179.89, -0.008, -179.901170, 0.153749);
                CHECK_EQ(output, -1);

                // right1
                output = geo.turnDirection(179.99, 0.01, -179.89, -0.008, -179.817119, -0.061572);
                CHECK_EQ(output, 1);

                // right2
                output = geo.turnDirection(179.99, 0.01, -179.89, -0.008, -179.945149, -0.124806);
                CHECK_EQ(output, 1);
            }
        }

        SUBCASE("Local cartesian class") {
            auto lc = LocalCartesian(0, 0);
            int output = 2;

            SUBCASE("Standard") {
                lc = LocalCartesian(10.0004, 50.0005);
                // left1
                output = lc.turnDirection(10.0, 50.0, 10.001, 50.001, 9.999830, 50.001671);
                CHECK_EQ(output, -1);

                // left2
                output = lc.turnDirection(10.0, 50.0, 10.001, 50.001, 9.998653, 49.999252);
                CHECK_EQ(output, -1);

                // right1
                output = lc.turnDirection(10.0, 50.0, 10.001, 50.001, 10.002667, 50.001298);
                CHECK_EQ(output, 1);

                // right2
                output = lc.turnDirection(10.0, 50.0, 10.001, 50.001, 10.000885, 50.000311);
                CHECK_EQ(output, 1);
            }

            SUBCASE("North pole") {
                lc = LocalCartesian(0.0, 90.0);
                // left1
                output = lc.turnDirection(0.0, 89.5, 180.0, 89.7, -170.331524, 89.096642);
                CHECK_EQ(output, -1);

                // left2
                output = lc.turnDirection(0.0, 89.5, 180.0, 89.7, -34.482593, 89.832157);
                CHECK_EQ(output, -1);

                // right1
                output = lc.turnDirection(0.0, 89.5, 180.0, 89.7, 138.219857, 89.098789);
                CHECK_EQ(output, 1);

                // right2
                output = lc.turnDirection(0.0, 89.5, 180.0, 89.7, 10.092353, 89.126110);
                CHECK_EQ(output, 1);
            }

            SUBCASE("Equator") {
                lc = LocalCartesian(179.999, 0.0);
                // left1
                output = lc.turnDirection(179.99, 0.01, -179.89, -0.008, -179.706966, 0.010142);
                CHECK_EQ(output, -1);

                // left2
                output = lc.turnDirection(179.99, 0.01, -179.89, -0.008, -179.901170, 0.153749);
                CHECK_EQ(output, -1);

                // right1
                output = lc.turnDirection(179.99, 0.01, -179.89, -0.008, -179.817119, -0.061572);
                CHECK_EQ(output, 1);

                // right2
                output = lc.turnDirection(179.99, 0.01, -179.89, -0.008, -179.945149, -0.124806);
                CHECK_EQ(output, 1);
            }
        }
    }
}
