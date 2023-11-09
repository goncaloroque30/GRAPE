// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Base/BaseModels.h"

namespace GRAPE {
    class RouteOutput;

    /**
    * @brief: Output class of profile calculations. Implemented as an ordered map with cumulative ground distance as key.
    *
    * Departures: 0 cumulative distance at departure threshold and positive cumulative distance afterwards
    * Arrivals: 0 cumulative distance at arrival threshold, negative cumulative distance before and positive afterwards
    */
    class ProfileOutput {
    public:
        /**
        * @brief Data structure for all the parameters needed for each profile output point.
        */
        struct Point {
            Point(double AltMsl, double TrueAirspeedIn, double GroundspeedIn, double ThrustIn, double BkAngle, FlightPhase FlPhaseIn) : AltitudeMsl(AltMsl), TrueAirspeed(TrueAirspeedIn), Groundspeed(GroundspeedIn), Thrust(ThrustIn), BankAngle(BkAngle), FlPhase(FlPhaseIn) {}
            double AltitudeMsl = Constants::NaN;
            double TrueAirspeed = Constants::NaN;
            double Groundspeed = Constants::NaN;
            double Thrust = Constants::NaN;
            double BankAngle = Constants::NaN;
            FlightPhase FlPhase = FlightPhase::Approach;
        };

        ProfileOutput() = default;
        ProfileOutput(const ProfileOutput&) = delete;
        ProfileOutput& operator=(const ProfileOutput&) = delete;
        ProfileOutput(ProfileOutput&&) = default;
        ProfileOutput& operator=(ProfileOutput&&) = default;
        ~ProfileOutput() = default;

        [[nodiscard]] const auto& points() const { return m_Profile; }
        [[nodiscard]] auto begin() const { return m_Profile.begin(); }
        [[nodiscard]] auto end() const { return m_Profile.end(); }

        [[nodiscard]] auto rbegin() const { return m_Profile.rbegin(); }
        [[nodiscard]] auto rend() const { return m_Profile.rend(); }

        /**
        * @brief Adds a point to the container.
        */
        void addPoint(double CumulativeGroundDistance, double AltitudeMsl, double TrueAirspeed, double Groundspeed, double Thrust, double BankAngle, FlightPhase FlPhase);

        /**
        * @brief Delete all points from the container and sets the total air distance to nan.
        */
        void clear() { m_Profile.clear(); }

        /**
        * @brief Changes the bank angle for all points in the container by calling bankAngle(Groundspeed, TurnRadius).
        */
        void recalculateBankAngle(const RouteOutput& Rte);

        /**
        * @return True if there are no points in the container.
        */
        [[nodiscard]] bool empty() const { return m_Profile.empty(); }

        /**
        * @return The number of points in the container.
        */
        [[nodiscard]] std::size_t size() const { return m_Profile.size(); }

        /**
        * @brief Interpolate a new point at CumulativeGroundDistance.
        *
        * ASSERT !empty()
        */
        [[nodiscard]] Point interpolate(double CumulativeGroundDistance) const;
    private:
        std::map<double, Point> m_Profile;
    };
}
