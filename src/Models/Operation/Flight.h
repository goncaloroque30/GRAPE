// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Operation.h"

#include "Airport/Airport.h"
#include "Aircraft/Doc29/Doc29Profile.h"

namespace GRAPE {
    /**
    * @brief Implementation of a flight operation. Virtual inherits Operation.
    */
    struct Flight : virtual Operation {
        explicit Flight(std::string_view, const Aircraft&, double WeightIn = 1000.0) : Weight(WeightIn) {}
        virtual ~Flight() override = default;

        // Data
        double Weight;

        /**
        * @brief Throwing set method for #Weight.
        *
        * Throws if WeightIn not in ]0, inf].
        */
        void setWeight(double WeightIn);

        /**
        * @return The operation Type (#Type::Flight).
        */
        [[nodiscard]] Type type() const override { return Type::Flight; }

        /**
        * @return The Route associated with this flight.
        */
        [[nodiscard]] virtual const Route& route() const = 0;

        /**
        * @return The Doc29Profile associated with this flight.
        */
        [[nodiscard]] virtual const Doc29Profile* doc29Profile() const { return nullptr; }

        /**
        * @return True if a Doc29Profile is selected for this flight.
        */
        [[nodiscard]] virtual bool doc29ProfileSelected() const = 0;

    };

    /**
    * @brief An arrival flight, inherits from OperationArrival and Flight.
    */
    class FlightArrival : public OperationArrival, public Flight {
    public:
        /**
        * @brief Initializes with empty Doc29ProfileArrival and default Weight.
        */
        FlightArrival(std::string_view NameIn, const RouteArrival& RouteIn, const Aircraft& AircraftIn);

        /**
        * @brief Initializes with all variables.
        */
        FlightArrival(std::string_view NameIn, const RouteArrival& RouteIn, const Aircraft& AircraftIn, const std::chrono::tai_seconds& TimeIn, double CountIn, double WeightIn, const Doc29ProfileArrival* Doc29ProfIn = nullptr);

        virtual ~FlightArrival() override = default;

        const Doc29ProfileArrival* Doc29Prof = nullptr; // Observer Pointer

        /**
        * @return The RouteArrival associated with this flight.
        */
        [[nodiscard]] const RouteArrival& route() const override { return m_Route; }

        /**
        * @brief The associated Doc29ProfileArrival. Might be a nullptr.
        */
        [[nodiscard]] const Doc29ProfileArrival* doc29Profile() const override { return Doc29Prof; }

        /**
        * @brief Sets the associated RouteArrival to RouteArr.
        */
        void setRoute(const RouteArrival& RouteArr) { m_Route = RouteArr; }

        /**
        * @return True if a Doc29ProfileArrival is selected for this flight.
        */
        [[nodiscard]] bool doc29ProfileSelected() const override { return Doc29Prof; }

        // Visitor Pattern
        void accept(OperationVisitor& Vis) override;
        void accept(OperationVisitor& Vis) const override;
    private:
        std::reference_wrapper<const RouteArrival> m_Route;
    };

    class FlightDeparture : public OperationDeparture, public Flight {
    public:
        /**
        * @brief Initializes with empty Doc29ProfileArrival and default weight and thrust percentages.
        */
        FlightDeparture(std::string_view NameIn, const RouteDeparture& RouteIn, const Aircraft& AircraftIn);

        /**
        * @brief Initializes with all variables.
        */
        FlightDeparture(std::string_view NameIn, const RouteDeparture& RouteIn, const Aircraft& AircraftIn, const std::chrono::tai_seconds& TimeIn, double CountIn, double WeightIn, double ThrustPercentageTakeoffIn = 1.0, double ThrustPercentageClimbIn = 1.0, const Doc29ProfileDeparture* Doc29ProfIn = nullptr);

        virtual ~FlightDeparture() override = default;

        double ThrustPercentageTakeoff = 1.0;
        double ThrustPercentageClimb = 1.0;
        const Doc29ProfileDeparture* Doc29Prof = nullptr;

        /**
        * @return The RouteDeparture associated with this flight.
        */
        [[nodiscard]] const RouteDeparture& route() const override { return m_Route; }

        /**
        * @brief The associated Doc29ProfileDeparture. Might be a nullptr.
        */
        [[nodiscard]] const Doc29ProfileDeparture* doc29Profile() const override { return Doc29Prof; }

        /**
        * @brief Sets the associated RouteDeparture to RouteDep.
        */
        void setRoute(const RouteDeparture& RouteDep) { m_Route = RouteDep; }

        /**
        * @brief Throwing set method for #ThrustPercentageTakeoff.
        *
        * Throws if ThrustPercentageIn not in [0.5, 1].
        */
        void setThrustPercentageTakeoff(double ThrustPercentage);

        /**
        * @brief Throwing set method for #ThrustPercentageClimb.
        *
        * Throws if ThrustPercentageIn not in [0.5, 1].
        */
        void setThrustPercentageClimb(double ThrustPercentage);

        /**
        * @return True if a Doc29ProfileDeparture is selected for this flight.
        */
        [[nodiscard]] bool doc29ProfileSelected() const override { return Doc29Prof; }

        // Visitor Pattern
        void accept(OperationVisitor& Vis) override;
        void accept(OperationVisitor& Vis) const override;
    private:
        std::reference_wrapper<const RouteDeparture> m_Route;
    };
}
