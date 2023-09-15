// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "EmissionsSpecification.h"

namespace GRAPE {
    void EmissionsSpecification::setFilterMinimumAltitude(double MinimumAltitudeIn) {
        if (std::isnan(MinimumAltitudeIn))
            throw GrapeException("Minimum altitude must be given.");

        if (!(MinimumAltitudeIn < FilterMaximumAltitude))
            throw GrapeException("Minimum altitude must be lower than maximum altitude.");

        FilterMinimumAltitude = MinimumAltitudeIn;
    }

    void EmissionsSpecification::setFilterMaximumAltitude(double MaximumAltitudeIn) {
        if (std::isnan(MaximumAltitudeIn))
            throw GrapeException("Minimum altitude must be given.");

        if (!(MaximumAltitudeIn > FilterMinimumAltitude))
            throw GrapeException("Maximum altitude must be higher than minimum altitude.");

        FilterMaximumAltitude = MaximumAltitudeIn;
    }

    void EmissionsSpecification::setFilterMinimumCumulativeGroundDistance(double MinimumCumulativeGroundDistanceIn) {
        if (std::isnan(MinimumCumulativeGroundDistanceIn))
            throw GrapeException("Minimum cumulative ground distance must be given.");
        if (!(MinimumCumulativeGroundDistanceIn < FilterMaximumCumulativeGroundDistance))
            throw GrapeException("Minimum cumulative ground distance must be lower than maximum cumulative ground distance.");

        FilterMinimumCumulativeGroundDistance = MinimumCumulativeGroundDistanceIn;
    }

    void EmissionsSpecification::setFilterMaximumCumulativeGroundDistance(double MaximumCumulativeGroundDistanceIn) {
        if (std::isnan(MaximumCumulativeGroundDistanceIn))
            throw GrapeException("Maximum cumulative ground distance must be given.");
        if (!(MaximumCumulativeGroundDistanceIn > FilterMinimumCumulativeGroundDistance))
            throw GrapeException("Maximum cumulative ground distance must be higher than minimum cumulative ground distance.");

        FilterMaximumCumulativeGroundDistance = MaximumCumulativeGroundDistanceIn;
    }
}
