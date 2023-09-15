// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Base/Base.h"
#include "Base/AtmosphereSeries.h"

namespace GRAPE {
    /**
    * @brief Defines the parameters needed by a performance run.
    */
    struct PerformanceSpecification {
        // Base
        std::unique_ptr<CoordinateSystem> CoordSys = std::make_unique<Geodesic>();
        AtmosphereSeries Atmospheres;

        // Filters
        double FilterMinimumAltitude = -Constants::Inf;
        double FilterMaximumAltitude = Constants::Inf;
        double FilterMinimumCumulativeGroundDistance = -Constants::Inf;
        double FilterMaximumCumulativeGroundDistance = Constants::Inf;
        double FilterGroundDistanceThreshold = Constants::NaN;

        // Segmentation
        double SpeedDeltaSegmentationThreshold = Constants::NaN;

        // Flights
        PerformanceModel FlightsPerformanceMdl = PerformanceModel::Doc29;
        bool FlightsDoc29Segmentation = true;

        // Tracks 4D
        int Tracks4dMinimumPoints = 1;
        bool Tracks4dRecalculateCumulativeGroundDistance = false;
        bool Tracks4dRecalculateGroundspeed = false;
        bool Tracks4dRecalculateFuelFlow = false;

        // Fuel Flow
        FuelFlowModel FuelFlowMdl = FuelFlowModel::None;

        /**
        * @brief Throwing set method for FilterMinimumAltitude.
        *
        * Throws if MinimumAltitudeIn not < FilterMaximumAltitude.
        */
        void setFilterMinimumAltitude(double MinimumAltitudeIn);

        /**
        * @brief Throwing set method for FilterMaximumAltitude.
        *
        * Throws if MaximumAltitudeIn not > FilterMinimumAltitude.
        */
        void setFilterMaximumAltitude(double MaximumAltitudeIn);

        /**
        * @brief Throwing set method for FilterMinimumCumulativeGroundDistance.
        *
        * Throws if MinimumCumulativeGroundDistanceIn not < FilterMaximumCumulativeGroundDistance.
        */
        void setFilterMinimumCumulativeGroundDistance(double MinimumCumulativeGroundDistanceIn);

        /**
        * @brief Throwing set method for FilterMaximumCumulativeGroundDistance.
        *
        * Throws if MaximumCumulativeGroundDistanceIn not > FilterMinimumCumulativeGroundDistance.
        */
        void setFilterMaximumCumulativeGroundDistance(double MaximumCumulativeGroundDistanceIn);

        /**
        * @brief Throwing set method for FilterGroundDistanceThreshold.
        *
        * Throws if GroundDistanceDeltaFilterThresholdIn not at least 0.
        */
        void setFilterGroundDistanceThreshold(double GroundDistanceDeltaFilterThresholdIn);

        /**
        * @brief Throwing set method for SpeedDeltaSegmentationThreshold.
        *
        * Throws if SpeedDeltaSegmentationThresholdIn not higher than 0.
        */
        void setSegmentationSpeedDeltaThreshold(double SpeedDeltaSegmentationThresholdIn);

        /**
        * @brief Throwing set method for Tracks4dMinimumPoints.
        *
        * Throws if MinimumTrack4dPointsIn not higher than 0.
        */
        void setTracks4dMinimumPoints(int MinimumTrack4dPointsIn);
    };
}
