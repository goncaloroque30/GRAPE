// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "Flight.h"

#include "Operations.h"

namespace GRAPE {
    void Flight::setWeight(double WeightIn) {
        if (!(WeightIn > 0.0))
            throw GrapeException("Weight must be higher than 0 kg.");

        Weight = WeightIn;
    }

    FlightArrival::FlightArrival(std::string_view NameIn, const RouteArrival& RouteIn, const Aircraft& AircraftIn) : Operation(NameIn, AircraftIn), OperationArrival(NameIn, AircraftIn), Flight(NameIn, AircraftIn), m_Route(RouteIn) {}

    FlightArrival::FlightArrival(std::string_view NameIn, const RouteArrival& RouteIn, const Aircraft& AircraftIn, const std::chrono::tai_seconds& TimeIn, double CountIn, double WeightIn, const Doc29ProfileArrival* Doc29ProfIn) : Operation(NameIn, AircraftIn, TimeIn, CountIn), OperationArrival(NameIn, AircraftIn), Flight(NameIn, AircraftIn, WeightIn), Doc29Prof(Doc29ProfIn), m_Route(RouteIn) {}

    FlightDeparture::FlightDeparture(std::string_view NameIn, const RouteDeparture& RouteIn, const Aircraft& AircraftIn) : Operation(NameIn, AircraftIn), OperationDeparture(NameIn, AircraftIn), Flight(NameIn, AircraftIn), m_Route(RouteIn) {}

    FlightDeparture::FlightDeparture(std::string_view NameIn, const RouteDeparture& RouteIn, const Aircraft& AircraftIn, const std::chrono::tai_seconds& TimeIn, double CountIn, double WeightIn, double ThrustPercentageTakeoffIn, double ThrustPercentageClimbIn, const Doc29ProfileDeparture* Doc29ProfIn) : Operation(NameIn, AircraftIn, TimeIn, CountIn), OperationDeparture(NameIn, AircraftIn), Flight(NameIn, AircraftIn, WeightIn), Doc29Prof(Doc29ProfIn), ThrustPercentageTakeoff(ThrustPercentageTakeoffIn), ThrustPercentageClimb(ThrustPercentageClimbIn), m_Route(RouteIn) {}

    void FlightDeparture::setThrustPercentageTakeoff(double ThrustPercentage) {
        if (!(ThrustPercentage >= 0.5 && ThrustPercentage <= 1.0))
            throw GrapeException("Thrust percentage for takeoff must be between 0.5 and 1.");

        ThrustPercentageTakeoff = ThrustPercentage;
    }

    void FlightDeparture::setThrustPercentageClimb(double ThrustPercentage) {
        if (!(ThrustPercentage >= 0.5 && ThrustPercentage <= 1.0))
            throw GrapeException("Thrust percentage for climb phase must be between 0.5 and 1.");

        ThrustPercentageClimb = ThrustPercentage;
    }

    void FlightArrival::accept(OperationVisitor& Vis) {
        Vis.visitFlightArrival(*this);
    }

    void FlightArrival::accept(OperationVisitor& Vis) const {
        Vis.visitFlightArrival(*this);
    }

    void FlightDeparture::accept(OperationVisitor& Vis) {
        Vis.visitFlightDeparture(*this);
    }

    void FlightDeparture::accept(OperationVisitor& Vis) const {
        Vis.visitFlightDeparture(*this);
    }
}
