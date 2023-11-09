// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Operation.h"

namespace GRAPE {
    /**
    * @brief Implementation of a track 4D operation. Virtual inherits Operation.
    *
    * A Track4d is a sequence of points. The points are stored in a vector.
    */
    class Track4d : public virtual Operation {
    public:
        /**
        * @brief Data structure that holds all variables needed for each point of a Track4d.
        */
        struct Point {
            FlightPhase FlPhase = FlightPhase::Approach;
            double CumulativeGroundDistance = 0.0;
            double Longitude = 0.0, Latitude = 0.0;
            double AltitudeMsl = 0.0;
            double TrueAirspeed = 0.0;
            double Groundspeed = 0.0;
            double CorrNetThrustPerEng = 0.0;
            double BankAngle = 0.0;
            double FuelFlowPerEng = 0.0;

            bool operator==(const Point&) const = default;

            /**
            * @brief Throwing set method for #Longitude.
            *
            * Throws if LongitudeIn not in [-180, 180].
            */
            void setLongitude(double LongitudeIn);

            /**
            * @brief Throwing set method for #Latitude.
            *
            * Throws if LongitudeIn not in [-90, 90].
            */
            void setLatitude(double LatitudeIn);

            /**
            * @brief Throwing set method for #TrueAirspeed.
            *
            * Throws if TrueAirspeedIn not in [0, inf].
            */
            void setTrueAirspeed(double TrueAirspeedIn);

            /**
            * @brief Throwing set method for #Groundspeed.
            *
            * Throws if GroundspeedIn not in [0, inf].
            */
            void setGroundspeed(double GroundspeedIn);

            /**
            * @brief Throwing set method for #BankAngle.
            *
            * Throws if BankAngleIn not in [-90, 90].
            */
            void setBankAngle(double BankAngleIn);

            /**
            * @brief Throwing set method for #FuelFlowPerEng.
            *
            * Throws if FuelFlowPerEngIn not in [0, inf].
            */
            void setFuelFlowPerEng(double FuelFlowPerEngIn);
        };

        Track4d(std::string_view, const Aircraft&) {}
        virtual ~Track4d() override = default;

        /**
        * @return The operation Type (#Type::Track4d).
        */
        [[nodiscard]] Type type() const override { return Type::Track4d; }

        [[nodiscard]] const auto& points() const { return m_Points; }
        [[nodiscard]] auto begin() const { return m_Points.begin(); }
        [[nodiscard]] auto end() const { return m_Points.end(); }
        [[nodiscard]] auto rbegin() const { return m_Points.rbegin(); }
        [[nodiscard]] auto rend() const { return m_Points.rend(); }

        [[nodiscard]] auto begin() { return m_Points.begin(); }
        [[nodiscard]] auto end() { return m_Points.end(); }
        [[nodiscard]] auto rbegin() { return m_Points.begin(); }
        [[nodiscard]] auto rend() { return m_Points.end(); }

        /**
        * @brief Implemented in the derived classes to add a point at the beginning or end of the container.
        */
        virtual void addPoint() noexcept = 0;

        /**
        * @brief Adds a Point at the end of the vector.
        *
        * ASSERT Longitude in [-180, 180].
        * ASSERT Latitude in [-90, 90].
        * ASSERT TrueAirspeed in [0, inf].
        * ASSERT Groundspeed in [0, inf].
        * ASSERT BankAngle in [-90, 90].
        * ASSERT FuelFlowPerEng in [0, inf].
        */
        void addPoint(FlightPhase FlPhase, double CumulativeGroundDistance, double Longitude, double Latitude, double AltitudeMsl, double TrueAirspeed, double Groundspeed, double CorrNetThrustPerEng, double BankAngle, double FuelFlowPerEng) noexcept;

        /**
        * @brief Adds a point to the end of the vector.
        */
        void addPoint(const Point& Pt) noexcept;

        /**
        * @brief Insert point at Index.
        *
        * ASSERT Index <= size().
        */
        void insertPoint(std::size_t Index) noexcept;

        /**
        * @brief Delete point at Index.
        *
        * ASSERT Index < size();
        */
        void deletePoint(std::size_t Index) noexcept;

        /**
        * @brief Deletes the last point.
        *
        * ASSERT !empty().
        */
        void deletePoint() noexcept;

        /**
        * @brief Deletes all points from the vector. If Shrink is true, the vector capcacity is reduced to 0.
        */
        void clear(bool Shrink = false) noexcept;

        /**
        * @return True if the points vector is empty.
        */
        [[nodiscard]] bool empty() const { return m_Points.empty(); }

        /**
        * @return The number of points in the vector.
        */
        [[nodiscard]] std::size_t size() const { return m_Points.size(); }
    protected:
        std::vector<Point> m_Points{};
    };

    /**
    * @brief Inherits from OperationOp and Track4d.
    */
    template<OperationType OpType>
    struct Track4dOp : OperationOp<OpType>, Track4d {
        Track4dOp(std::string_view NameIn, const Aircraft& AircraftIn, const std::chrono::tai_seconds& TimeIn = std::chrono::round<std::chrono::seconds>(std::chrono::tai_clock::now()), double CountIn = 1.0) : Operation(NameIn, AircraftIn, TimeIn, CountIn), OperationOp<OpType>(NameIn, AircraftIn), Track4d(NameIn, AircraftIn) {}
        virtual ~Track4dOp() override = default;

        using Track4d::addPoint; ///< Exposes the addPoint methods from the base class.

        /**
        * @brief Specialized in each OperationType.
        *
        * if !empty()
        *	add a point to the end of the vector.
        * else
        *	Arrival: add a default point with FlightPhase::Approach.
        *	Departure: add a default point with FlightPhase::TakeoffRoll.
        */
        void addPoint() noexcept override;

        // Visitor Pattern
        void accept(OperationVisitor& Vis) override;
        void accept(OperationVisitor& Vis) const override;
    };
    typedef Track4dOp<OperationType::Arrival> Track4dArrival;
    typedef Track4dOp<OperationType::Departure> Track4dDeparture;
}
