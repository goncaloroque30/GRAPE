// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "PerformanceSpecification.h"

namespace GRAPE {
    void PerformanceSpecification::setFilterMinimumAltitude(double MinimumAltitudeIn) {
        if (!(MinimumAltitudeIn < FilterMaximumAltitude))
            throw GrapeException("Minimum altitude must be lower than maximum altitude.");

        FilterMinimumAltitude = MinimumAltitudeIn;
    }

    void PerformanceSpecification::setFilterMaximumAltitude(double MaximumAltitudeIn) {
        if (!(MaximumAltitudeIn > FilterMinimumAltitude))
            throw GrapeException("Maximum altitude must be higher than minimum altitude.");

        FilterMinimumAltitude = MaximumAltitudeIn;
    }

    void PerformanceSpecification::setFilterMinimumCumulativeGroundDistance(double MinimumCumulativeGroundDistanceIn) {
        if (!(MinimumCumulativeGroundDistanceIn < FilterMaximumCumulativeGroundDistance))
            throw GrapeException("Minimum cumulative ground distance must be lower than maximum cumulative ground distance.");

        FilterMinimumCumulativeGroundDistance = MinimumCumulativeGroundDistanceIn;
    }

    void PerformanceSpecification::setFilterMaximumCumulativeGroundDistance(double MaximumCumulativeGroundDistanceIn) {
        if (!(MaximumCumulativeGroundDistanceIn > FilterMinimumCumulativeGroundDistance))
            throw GrapeException("Maximum cumulative ground distance must be higher than minimum cumulative ground distance.");

        FilterMaximumCumulativeGroundDistance = MaximumCumulativeGroundDistanceIn;
    }

    void PerformanceSpecification::setFilterGroundDistanceThreshold(double GroundDistanceDeltaFilterThresholdIn) {
        if (!(GroundDistanceDeltaFilterThresholdIn >= 0.0))
            throw GrapeException("Ground distance delta filter threshold must be higher than 0.");

        FilterGroundDistanceThreshold = GroundDistanceDeltaFilterThresholdIn;
    }

    void PerformanceSpecification::setSegmentationSpeedDeltaThreshold(double SpeedDeltaSegmentationThresholdIn) {
        if (!(SpeedDeltaSegmentationThresholdIn > 0.0))
            throw GrapeException("Speed delta segmentation threshold must be higher than 0.");

        SpeedDeltaSegmentationThreshold = SpeedDeltaSegmentationThresholdIn;
    }

    void PerformanceSpecification::setTracks4dMinimumPoints(int MinimumTrack4dPointsIn) {
        if (!(MinimumTrack4dPointsIn > 0))
            throw GrapeException("Minimum track 4D points must be at least 1.");

        Tracks4dMinimumPoints = MinimumTrack4dPointsIn;
    }
}
