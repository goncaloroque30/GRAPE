// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#pragma warning ( push )
#pragma warning ( disable : GRAPE_VENDOR_WARNINGS )
#include "GeographicLib/Geodesic.hpp"
#include "GeographicLib/LocalCartesian.hpp"
#pragma warning ( pop )

namespace GRAPE {
    struct CoordinateSystemVisitor;

    /**
    * @brief Interface class representing all the actions possible on a coordinate system.
    *
    * The supported types, implemented as derived classes, are stored in the enum #Type.
    */
    class CoordinateSystem {
    public:

        /**
        * @brief The supported coordinate systems, implemented as subclasses.
        */
        enum class Type : unsigned {
            Geodesic = 0,
            LocalCartesian,
        };
        static constexpr EnumStrings<Type> Types{ "Geodesic WGS84", "Local Cartesian" };

        CoordinateSystem() = default;
        virtual ~CoordinateSystem() = default;

        /**
        * @return The coordinate system type.
        */
        [[nodiscard]] virtual Type type() const = 0;

        /**
        * @return Distance between points 1 and 2 (inverse problem).
        */
        [[nodiscard]] virtual double distance(double Longitude1, double Latitude1, double Longitude2, double Latitude2) const = 0;

        /**
        * @brief Calculates the heading at point 1 in the range [0, 360[ when going from 1 to 2 (inverse problem).
        * @return Heading in the range [0.0, 360.0[.
        */
        [[nodiscard]] virtual double heading(double Longitude1, double Latitude1, double Longitude2, double Latitude2) const = 0;

        /**
        * @brief Calculates the heading at point 2 in the range [0, 360[ when going from 1 to 2 (inverse problem).
        * @return Heading in the range [0.0, 360.0[.
        */
        [[nodiscard]] virtual double headingEnd(double Longitude1, double Latitude1, double Longitude2, double Latitude2) const = 0;

        /**
        * @brief Calculate the heading at point 1 in the range [0, 360[ and distance between 1 and 2 (inverse problem).
        * @return Distance, Heading in the range [0, 360[.
        */
        [[nodiscard]] virtual std::pair<double, double> distanceHeading(double Longitude1, double Latitude1, double Longitude2, double Latitude2) const = 0;

        /**
        * @brief Calculate the heading at point 2 in the range [0, 360[ and distance between 1 and 2 (inverse problem).
        * @return Distance, Heading in the range [0, 360[.
        */
        [[nodiscard]] virtual std::pair<double, double> distanceHeadingEnd(double Longitude1, double Latitude1, double Longitude2, double Latitude2) const = 0;

        /**
        * @return Longitude, Latitude of the point at Distance and Heading from point 1 (direct problem).
        */
        [[nodiscard]] virtual std::pair<double, double> point(double Longitude1, double Latitude1, double Distance, double Heading) const = 0;

        /**
        * @return Longitude, Latitude and HeadingEnd of the point at Distance and HeadingStart from point 1 (direct problem).
        */
        [[nodiscard]] virtual std::tuple<double, double, double> pointHeadingEnd(double Longitude1, double Latitude1, double Distance, double Heading) const = 0;

        /**
        * @brief Part of the return value of intersection(), indicates if the intersection point is before 1 (#Intersection::Behind), between 1 and 2 (#Intersection::Between) or after 2 (#Intersection::Ahead).
        */
        enum class Intersection : unsigned {
            Behind = 0,
            Between,
            Ahead,
        };

        /**
        * @brief Calculates the intersection point on the line12 defined by 1 and 2 with the line passing at point 3 and perpendicular to extended line12.
        * @return Longitude, Latitude, Intersection.
        */
        [[nodiscard]] virtual std::tuple<double, double, Intersection> intersection(double Longitude1, double Latitude1, double Longitude2, double Latitude2, double Longitude3, double Latitude3) const = 0;

        /**
         * @return 1 if turn to the right | -1 if turn to the left | 0 if point on the line
         */
        [[nodiscard]] virtual int turnDirection(double Longitude1, double Latitude1, double Longitude2, double Latitude2, double Longitude3, double Latitude3) const = 0;

        // Visitor Pattern
        virtual void accept(CoordinateSystemVisitor&) = 0;
        virtual void accept(CoordinateSystemVisitor&) const = 0;
    };

    /**
    * @brief Cartesian coordinate system defined by its center location.
    *
    * Converts from WGS84 coordinates to local cartesian via geocentric coordinates.
    * Implementation of all coordinate system problems on the cartesian system.
    */
    class LocalCartesian : public CoordinateSystem {
    public:
        /**
        * @brief Creates LocalCartesian coordinate system with center at point 0.
        */
        LocalCartesian(const double Longitude0, const double Latitude0, const double Altitude0 = 0.0) : m_LocalCartesian(Latitude0, Longitude0, Altitude0, GeographicLib::Geocentric::WGS84()) {}

        [[nodiscard]] Type type() const override { return Type::LocalCartesian; }

        /**
        * @brief Calculates the cartesian coordinates of points 1 and 2 and returns the hypotenuse between those two points.
        */
        [[nodiscard]] double distance(double Longitude1, double Latitude1, double Longitude2, double Latitude2) const override;

        /**
        * @brief Calculates the cartesian coordinates of points 1 and 2 and returns the arctangent between those two points, converted to the range ]0, 360].
        * @return Heading in the range ]0, 360].
        */
        [[nodiscard]] double heading(double Longitude1, double Latitude1, double Longitude2, double Latitude2) const override;

        /**
        * @brief Same as heading for a local cartesian coordinate system.
        * @return Heading in the range ]0, 360].
        */
        [[nodiscard]] double headingEnd(double Longitude1, double Latitude1, double Longitude2, double Latitude2) const override { return heading(Longitude1, Latitude1, Longitude2, Latitude2); }

        /**
        * @brief Same as distance() and heading().
        * @return Distance, Heading in the range [0, 360[.
        */
        [[nodiscard]] std::pair<double, double> distanceHeading(double Longitude1, double Latitude1, double Longitude2, double Latitude2) const override;

        /**
        * @brief Same as distance() and heading().
        * @return Distance, Heading in the range [0, 360[.
        */
        [[nodiscard]] std::pair<double, double> distanceHeadingEnd(double Longitude1, double Latitude1, double Longitude2, double Latitude2) const override;

        /**
        * @return Longitude, Latitude.
        */
        [[nodiscard]] std::pair<double, double> point(double Longitude1, double Latitude1, double Distance, double Heading) const override;

        /**
        * @brief Solution of the direct problem.
        * @return Longitude, Latitude, HeadingEnd.
        */
        [[nodiscard]] std::tuple<double, double, double> pointHeadingEnd(double Longitude1, double Latitude1, double Distance, double HeadingStart) const override;

        /**
        * @return Longitude, Latitude, Intersection.
        */
        [[nodiscard]] std::tuple<double, double, Intersection> intersection(double Longitude1, double Latitude1, double Longitude2, double Latitude2, double Longitude3, double Latitude3) const override;

        /**
        * @brief Converts a pair of coordinates in the local cartesian system to longitude latitude in the WGS84.
        * @return Longitude, Latitude.
        */
        [[nodiscard]] std::pair<double, double> reverse(double X, double Y) const;

        /**
        * @return Longitude, Latitude used as the center point of the cartesian coordinate system.
        */
        [[nodiscard]] std::pair<double, double> origin() const { return { m_LocalCartesian.LongitudeOrigin(), m_LocalCartesian.LatitudeOrigin() }; }

        [[nodiscard]] int turnDirection(double Longitude1, double Latitude1, double Longitude2, double Latitude2, double Longitude3, double Latitude3) const override;

        /**
        * @brief Change the center of the cartesian coordinate system.
        */
        void reset(const double Longitude0, const double Latitude0, const double Altitude0 = 0.0) { m_LocalCartesian.Reset(Latitude0, Longitude0, Altitude0); }

        /**
        * @brief Throwing version of reset().
        * Throws if longitude not in [-180, 180] or latitude not in [-90, 90].
        */
        void resetE(double Longitude0, double Latitude0, double Altitude0 = 0.0);

        void accept(CoordinateSystemVisitor& Cs) override;
        void accept(CoordinateSystemVisitor& Cs) const override;
    private:
        GeographicLib::LocalCartesian m_LocalCartesian;
    };

    /**
    * @brief Geodesic coordinate system with the WGS84 ellipsoid.
    */
    class Geodesic : public CoordinateSystem {
    public:
        Geodesic() : m_Geodesic(GeographicLib::Geodesic::WGS84()) {}

        [[nodiscard]] Type type() const override { return Type::Geodesic; }

        /**
        * @brief Solution of the inverse problem.
        */
        [[nodiscard]] double distance(double Longitude1, double Latitude1, double Longitude2, double Latitude2) const override;

        /**
        * @brief Solution of the inverse problem.
        */
        [[nodiscard]] double heading(double Longitude1, double Latitude1, double Longitude2, double Latitude2) const override;

        /**
        * @brief Solution of the inverse problem.
        */
        [[nodiscard]] double headingEnd(double Longitude1, double Latitude1, double Longitude2, double Latitude2) const override;

        /**
        * @brief Solution of the inverse problem.
        * @return Distance, HeadingStart.
        */
        [[nodiscard]] std::pair<double, double> distanceHeading(double Longitude1, double Latitude1, double Longitude2, double Latitude2) const override;

        /**
        * @brief Solution of the inverse problem.
        * @return Distance, HeadingEnd.
        */
        [[nodiscard]] std::pair<double, double> distanceHeadingEnd(double Longitude1, double Latitude1, double Longitude2, double Latitude2) const override;

        /**
        * @brief Solution of the direct problem.
        * @return Longitude, Latitude.
        */
        [[nodiscard]] std::pair<double, double> point(double Longitude1, double Latitude1, double Distance, double Heading) const override;

        /**
        * @brief Solution of the direct problem.
        * @return Longitude, Latitude, HeadingEnd.
        */
        [[nodiscard]] std::tuple<double, double, double> pointHeadingEnd(double Longitude1, double Latitude1, double Distance, double HeadingStart) const override;

        /**
        * @return Longitude, Latitude, Intersection.
        */
        [[nodiscard]] std::tuple<double, double, Intersection> intersection(double Longitude1, double Latitude1, double Longitude2, double Latitude2, double Longitude3, double Latitude3) const override;


        /**
         *
         */
        [[nodiscard]] int turnDirection(double Longitude1, double Latitude1, double Longitude2, double Latitude2, double Longitude3, double Latitude3) const override;

        void accept(CoordinateSystemVisitor& Cs) override;
        void accept(CoordinateSystemVisitor& Cs) const override;
    private:
        GeographicLib::Geodesic m_Geodesic;
    };

    struct CoordinateSystemVisitor {
        virtual void visitLocalCartesian(LocalCartesian& Cs) {}
        virtual void visitGeodesic(Geodesic& Cs) {}
        virtual void visitLocalCartesian(const LocalCartesian& Cs) {}
        virtual void visitGeodesic(const Geodesic& Cs) {}

        virtual ~CoordinateSystemVisitor() = default;
    };
}
