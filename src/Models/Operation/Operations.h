// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Flight.h"
#include "Track4d.h"

namespace GRAPE {
    struct OperationVisitor {
        virtual void visitFlightArrival(FlightArrival&) {}
        virtual void visitFlightDeparture(FlightDeparture&) {}
        virtual void visitTrack4dArrival(Track4dArrival&) {}
        virtual void visitTrack4dDeparture(Track4dDeparture&) {}
        virtual void visitFlightArrival(const FlightArrival&) {}
        virtual void visitFlightDeparture(const FlightDeparture&) {}
        virtual void visitTrack4dArrival(const Track4dArrival&) {}
        virtual void visitTrack4dDeparture(const Track4dDeparture&) {}
        virtual ~OperationVisitor() = default;
    };
}
