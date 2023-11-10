// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Base/BaseModels.h"

namespace GRAPE {
    class CoordinateSystem;
    class FlightArrival;
    class FlightDeparture;
    class Operation;

    /**
    * @brief The performance output is a sequence of points stored on an ordered map. The key is the cumulative ground distance.
    */
    class PerformanceOutput {
    public:
        /**
        * @brief Possible origins of an output point.
        */
        enum class PointOrigin {
            Route = 0,
            Profile,
            RouteAndProfile,
            Track4d,
            SpeedSegmentation,
            Doc29TakeoffRollSegmentation,
            Doc29FinalApproachSegmentation,
            Doc29InitialClimbSegmentation,
        };
        static constexpr EnumStrings<PointOrigin> Origins{ "Route", "Profile", "Route & Profile", "Track 4D", "Speed Segmentation", "Doc29 Takeoff Roll Segmentation", "Doc29 Final Approach Segmentation" , "Doc29 Initial Climb Segmentation" };

        /**
        * @brief Data structure for all the parameters needed for each performance output point.
        */
        struct Point {
            PointOrigin PtOrigin;
            TimePoint Time;
            FlightPhase FlPhase;
            double Longitude, Latitude;
            double AltitudeMsl;
            double TrueAirspeed;
            double Groundspeed;
            double CorrNetThrustPerEng;
            double BankAngle;
            double FuelFlowPerEng;
        };

        PerformanceOutput() = default;
        PerformanceOutput(const PerformanceOutput&) = delete;
        PerformanceOutput(PerformanceOutput&&) = default;
        PerformanceOutput& operator=(const PerformanceOutput&) = delete;
        PerformanceOutput& operator=(PerformanceOutput&&) = default;
        ~PerformanceOutput() = default;

        [[nodiscard]] const auto& points() const { return m_Output; }
        [[nodiscard]] auto begin() const { return m_Output.begin(); }
        [[nodiscard]] auto end() const { return m_Output.end(); }

        auto begin() { return m_Output.begin(); }
        auto end() { return m_Output.end(); }

        /**
        * @return True if there are no points in the container.
        */
        [[nodiscard]] bool empty() const { return m_Output.empty(); }

        /**
        * @return The number of points in the container.
        */
        [[nodiscard]] std::size_t size() const { return m_Output.size(); }

        /**
        * @brief Adds a point to the container with TimePoint.
        * @return The newly created Point and true or the already existing Point and false.
        */
        std::pair<Point&, bool> addPoint(PointOrigin PtOrigin, TimePoint Time, FlightPhase FlPhase, double CumulativeGroundDistance, double Longitude, double Latitude, double AltitudeMsl, double TrueAirspeed, double Groundspeed, double CorrectedNetThrustPerEng, double BankAngle, double FuelFlowPerEng = Constants::NaN);

        /**
        * @brief Adds a point to the container without TimePoint.
        * @return The newly created Point and true or the already existing Point and false.
        */
        std::pair<Point&, bool> addPoint(PointOrigin PtOrigin, FlightPhase FlPhase, double CumulativeGroundDistance, double Longitude, double Latitude, double AltitudeMsl, double TrueAirspeed, double Groundspeed, double CorrectedNetThrustPerEng, double BankAngle, double FuelFlowPerEng = Constants::NaN);

        /**
        * @brief Delete all points from the container.
        */
        void clear();

        /**
        * @brief Recalculate Time with a StartTime based on ground distance and groundspeed
        */
        void recalculateTime(TimePoint StartTime);

        /**
        * @brief Go through the container and create new points between two adjacent points if their ground speed delta is smaller than SpeedDeltaMinimum.
        */
        void speedSegmentation(const CoordinateSystem& Cs, double SpeedDeltaMinimum);

        /**
        * @brief Go through the container and delete a point if the ground distance to the previous point is smaller than GroundDistanceMaximum.
        */
        std::size_t groundDistanceFilter(double GroundDistanceMaximum);
    private:
        std::map<double, Point> m_Output;
    };
}
