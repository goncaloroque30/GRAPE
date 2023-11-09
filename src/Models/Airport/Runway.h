// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Route.h"

namespace GRAPE {
    /**
    * @brief A Runway belongs to an airport and is described by its parameters. It contains arrival and departure routes.
    */
    class Runway {
    public:
        Runway(const Airport& AirportIn, std::string_view NameIn) noexcept; // Default initializes data from airport
        Runway(const Runway&) = delete;
        Runway(Runway&&) = delete;
        Runway& operator=(const Runway&) = delete;
        Runway& operator=(Runway&&) = delete;
        ~Runway() = default;

        // Parameters
        std::string Name;
        double Longitude = 0.0, Latitude = 0.0;
        double Elevation = 0.0;
        double Length = 1000.0;
        double Heading = 0.0;
        double Gradient = 0.0;

        GrapeMap<std::string, std::unique_ptr<RouteArrival>> ArrivalRoutes; // Key is route name
        GrapeMap<std::string, std::unique_ptr<RouteDeparture>> DepartureRoutes; // Key is route name

        /**
        * @return The Airport which owns this Runway.
        */
        [[nodiscard]] const Airport& parentAirport() const noexcept;

        /**
        * @brief Helper method to create an ArrivalRoute with the given type.
        * @return The newly created arrival route and true or the already existing arrival route and false.
        */
        std::pair<RouteArrival&, bool> addArrival(const std::string& ArrName, Route::Type RteType);

        /**
        * @brief Helper method to create a DepartureRoute with the given type.
        * @return The newly created departure route and true or the already existing departure route and false.
        */
        std::pair<RouteDeparture&, bool> addDeparture(const std::string& DepName, Route::Type RteType);

        /**
        * @brief Throwing set method for #Longitude. Throws if LongitudeIn not in [-180.0, 180.0].
        */
        void setLongitude(double LongitudeIn);

        /**
        * @brief Throwing set method for #Latitude. Throws if LatitudeIn not in [-90.0, 90.0].
        */
        void setLatitude(double LatitudeIn);

        /**
        * @brief Throwing set method for #Length. Throws if LengthIn not in ]0.0, inf].
        */
        void setLength(double LengthIn);

        /**
        * @brief Throwing set method for #Heading. Throws if HeadingIn not in [0.0, 360.0].
        */
        void setHeading(double HeadingIn);

        /**
        * @brief Throwing set method for #Gradient. Throws if GradientIn not in [0.0, 1.0].
        */
        void setGradient(double GradientIn);

        /**
        * @return True if both #ArrivalRoutes and #DepartureRoutes are empty.
        */
        [[nodiscard]] bool empty() const { return ArrivalRoutes.empty() && DepartureRoutes.empty(); }


        /**
        * @return The result of \f{ #Elevation + #Distance \times #Gradient \f}.
        */
        [[nodiscard]] double elevationAt(double Distance) const { return Elevation + Distance * Gradient; }

        /**
        * @return The result of \f{ #Elevation + #Length \times #Gradient \f}.
        */
        [[nodiscard]] double elevationEnd() const { return elevationAt(Length); }
    private:
        // Parent airport
        std::reference_wrapper<const Airport> m_Airport;
    };
}
