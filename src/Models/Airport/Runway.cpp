// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "Runway.h"

#include "Airport.h"

namespace GRAPE {
    // Default runway at airport location
    Runway::Runway(const Airport& AirportIn, std::string_view NameIn) noexcept : Name(NameIn), m_Airport(AirportIn) {
        Longitude = m_Airport.get().Longitude;
        Latitude = m_Airport.get().Latitude;
        Elevation = m_Airport.get().Elevation;
    }

    const Airport& Runway::parentAirport() const noexcept {
        return m_Airport;
    }

    std::pair<RouteArrival&, bool> Runway::addArrival(const std::string& ArrName, Route::Type RteType) {
        std::unique_ptr<RouteArrival> newRte;
        switch (RteType)
        {
        case Route::Type::Simple: newRte = std::make_unique<RouteArrivalSimple>(*this, ArrName); break;
        case Route::Type::Vectors: newRte = std::make_unique<RouteArrivalVectors>(*this, ArrName); break;
        case Route::Type::Rnp: newRte = std::make_unique<RouteArrivalRnp>(*this, ArrName); break;
        default: GRAPE_ASSERT(false); break;
        }
        const auto& [emplacedRte, emplaced] = ArrivalRoutes.add(ArrName, std::move(newRte));
        return { *emplacedRte , emplaced };
    }

    std::pair<RouteDeparture&, bool> Runway::addDeparture(const std::string& DepName, Route::Type RteType) {
        std::unique_ptr<RouteDeparture> newRte;
        switch (RteType)
        {
        case Route::Type::Simple: newRte = std::make_unique<RouteDepartureSimple>(*this, DepName); break;
        case Route::Type::Vectors: newRte = std::make_unique<RouteDepartureVectors>(*this, DepName); break;
        case Route::Type::Rnp: newRte = std::make_unique<RouteDepartureRnp>(*this, DepName); break;
        default: GRAPE_ASSERT(false); break;
        }
        const auto& [emplacedRte, emplaced] = DepartureRoutes.add(DepName, std::move(newRte));
        return { *emplacedRte , emplaced };
    }

    void Runway::setLongitude(double LongitudeIn) {
        if (!(LongitudeIn >= -180.0 && LongitudeIn <= 180.0))
            throw GrapeException("Longitude must be between -180.0 and 180.0.");
        Longitude = LongitudeIn;
    }

    void Runway::setLatitude(double LatitudeIn) {
        if (!(LatitudeIn >= -90.0 && LatitudeIn <= 90.0))
            throw GrapeException("Latitude must be between -90.0 and 90.0.");
        Latitude = LatitudeIn;
    }

    void Runway::setLength(double LengthIn) {
        if (!(LengthIn > 0.0))
            throw GrapeException("Length must be higher than 0.");
        Length = LengthIn;
    }

    void Runway::setHeading(double HeadingIn) {
        if (!(HeadingIn >= 0.0 && HeadingIn <= 360.0))
            throw GrapeException("Heading must be between 0 and 360.");
        Heading = HeadingIn;
    }

    void Runway::setGradient(double GradientIn) {
        if (!(GradientIn >= -1.0 || GradientIn <= 1.0))
            throw GrapeException("Gradient must be between -1.0 and 1.0.");
        Gradient = GradientIn;
    }
}
