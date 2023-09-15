// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

namespace GRAPE {
    class CoordinateSystem;
    class Runway;

    /**
    * @brief Output class of route calculations.
    *
    * The output is implemented as a map with the cumulative ground distance as key.
    * Departures: 0 cumulative distance at departure threshold and positive cumulative distance afterwards.
    * Arrivals: 0 cumulative distance at arrival threshold and negative cumulative distance before.
    */
    class RouteOutput {
    public:
        /**
        * @brief Enum describing for each point if it is in a left or right turn or in a straight.
        */
        enum class Direction {
            Straight,
            LeftTurn,
            RightTurn,
        };
        static constexpr EnumStrings<Direction> Directions{ "Straight", "LeftTurn", "RightTurn" };

        /**
        * @brief Data structure containing the parameters for each point of the output from the route calculations.
        */
        struct Point {
            Point(double Lon, double Lat, double HeadingIn, double Rad, Direction DirIn) noexcept : Longitude(Lon), Latitude(Lat), Heading(HeadingIn), Radius(Rad), Dir(DirIn) {}
            double Longitude = Constants::NaN, Latitude = Constants::NaN;
            double Heading = Constants::NaN;
            double Radius = Constants::NaN;
            Direction Dir = Direction::Straight;
        };

        RouteOutput() = default;

        /**
        * @brief Adds the runway threshold to the output.
        */
        explicit RouteOutput(const Runway& Rwy) noexcept;

        /**
        * Arrivals: The runway threshold.
        * Departures: the last point.
        *
        * ASSERT !empty().
        */
        [[nodiscard]] const auto& lastPoint() const noexcept { GRAPE_ASSERT(!empty()); return *m_Output.rbegin(); }

        /**
        * Arrivals: The first point.
        * Departures: the runway threshold.
        *
        * ASSERT !empty().
        */
        [[nodiscard]] const auto& firstPoint() const noexcept { GRAPE_ASSERT(!empty()); return *m_Output.begin(); }

        [[nodiscard]] const auto& points() const noexcept { return m_Output; }
        [[nodiscard]] const auto& point(std::size_t Index) const noexcept { GRAPE_ASSERT(Index < size()); return *std::next(m_Output.begin(), Index); }

        [[nodiscard]] auto begin() const noexcept { return m_Output.begin(); }
        [[nodiscard]] auto end() const noexcept { return m_Output.end(); }

        /**
        * @brief Add a point to the container.
        */
        void addPoint(double CumulativeGroundDistance, double Longitude, double Latitude, double Heading, double Radius = Constants::Inf, Direction Dir = Direction::Straight);

        /**
        * @brief Recalculates the headings. For each segment (sequence of two points), the heading of the first point is set to be the heading of the segment at the start point. The segment of the last point equals the previous one.
        */
        void recalculateHeadings(const CoordinateSystem& Cs);

        /**
        * @return True if there are no points in this output.
        */
        [[nodiscard]] bool empty() const { return m_Output.empty(); }

        /**
        * @return The number of points in this output.
        */
        [[nodiscard]] std::size_t size() const { return m_Output.size(); }

        /**
        * @return The turn radius at CumulativeGroundDistance, determined by the point before CumulativeGroundDistance.
        *
        * ASSERT !empty().
        */
        [[nodiscard]] double turnRadius(double CumulativeGroundDistance) const;

        /**
       * @return The turn radius at CumulativeGroundDistance, determined by the point before CumulativeGroundDistance.
       *
       * ASSERT !empty().
       */
        [[nodiscard]] double heading(double CumulativeGroundDistance) const;

        /**
        * @brief Check if the turn radius changes between StartCumulativeGroundDistance and EndCumulativeGroundDistance, and return the cumulative ground distance at which it does.
        * @return If there is no change in turn radius, NaN. Otherwise the cumulative ground distance of the point at which the change in turn radius occurs.
        *
        * ASSERT !empty().
        */
        [[nodiscard]] double turnRadiusChange(double StartCumulativeGroundDistance, double EndCumulativeGroundDistance) const;

        /**
        * @brief Interpolates a new point at CumulativeGroundDistance. Needs the CoordinateSystem to calculate the coordinates and heading of the new point.
        *
        * ASSERT !empty().
        */
        [[nodiscard]] Point interpolate(const CoordinateSystem& Cs, double CumulativeGroundDistance) const;
    private:
        /**
        * @return The previous point to CumulativeGroundDistance in the container or the first point, if CumulativeGroundDistance is smaller than the point with the lowest cumulative ground distance value.
        *
        * ASSERT !empty().
        */
        [[nodiscard]] std::map<double, Point>::const_iterator previousPoint(double CumulativeGroundDistance) const;
    private:
        std::map<double, Point> m_Output; ///< Key is cumulative ground distance.
    };
}
