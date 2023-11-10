// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Route.h"
#include "RouteOutput.h"
#include "Runway.h"

namespace GRAPE {
    /**
    * @brief Represents an Airport and its parameters. Owns runways.
    */
    class Airport {
    public:
        explicit Airport(std::string_view NameIn) noexcept : Name(NameIn) {}
        Airport(const Airport&) = delete;
        Airport(Airport&&) = delete;
        Airport& operator=(const Airport&) = delete;
        Airport& operator=(Airport&&) = delete;
        ~Airport() = default;

        // Parameters
        std::string Name;
        double Longitude = 0.0, Latitude = 0.0;
        double Elevation = 0.0;
        double ReferenceTemperature = 288.15;
        double ReferenceSeaLevelPressure = 101325.0;

        // Runways
        GrapeMap<std::string, Runway> Runways;

        /**
        * @brief Throwing set method for #Longitude. Throws if LongitudeIn not in [-180.0, 180.0].
        */
        void setLongitude(double LongitudeIn);

        /**
        * @brief Throwing set method for #Latitude. Throws if LatitudeIn not in [-90.0, 90.0].
        */
        void setLatitude(double LatitudeIn);

        /**
        * @brief Throwing set method for #ReferenceTemperature. Throws if Temperature not in [0.0, inf].
        */
        void setReferenceTemperature(double Temperature);

        /**
        * @brief Throwing set method for #ReferencePressure. Throws if PressureIn not in [0.0, inf].
        */
        void setReferenceSeaLevelPressure(double SeaLevelPressure);
    };
}
