// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "Airport.h"

namespace GRAPE {
    void Airport::setLongitude(double LongitudeIn) {
        if (!(LongitudeIn >= -180.0 && LongitudeIn <= 180.0))
            throw GrapeException("Longitude must be between -180.0 and 180.0.");
        Longitude = LongitudeIn;
    }

    void Airport::setLatitude(double LatitudeIn) {
        if (!(LatitudeIn >= -90.0 && LatitudeIn <= 90.0))
            throw GrapeException("Latitude must be between -90.0 and 90.0.");
        Latitude = LatitudeIn;
    }

    void Airport::setReferenceTemperature(double Temperature) {
        if (!(Temperature >= 0.0))
            throw GrapeException("Temperature must be at least 0 K.");
        ReferenceTemperature = Temperature;
    }

    void Airport::setReferenceSeaLevelPressure(double SeaLevelPressure) {
        if (!(SeaLevelPressure >= 0.0))
            throw GrapeException("Sea level pressure must be at least 0 Pa.");
        ReferenceSeaLevelPressure = SeaLevelPressure;
    }
}
