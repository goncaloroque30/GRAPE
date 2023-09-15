// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Base/BaseModels.h"

namespace GRAPE {
    /**
    * @brief Defines the parameters needed by a fuel & emissions run.
    */
    struct EmissionsSpecification {
        EmissionsModel EmissionsMdl = EmissionsModel::BFFM2;
        bool SaveSegmentResults = false;

        double FilterMinimumAltitude = -Constants::Inf;
        double FilterMaximumAltitude = Constants::Inf;
        double FilterMinimumCumulativeGroundDistance = -Constants::Inf;
        double FilterMaximumCumulativeGroundDistance = Constants::Inf;

        /**
        * @brief Throwing set method for #FilterMinimumAltitude.
        *
        * Throws is MinimumAltitudeIn is nan.
        * Throws if MinimumAltitudeIn > FilterMaximumAltitude.
        */
        void setFilterMinimumAltitude(double MinimumAltitudeIn);

        /**
        * @brief Throwing set method for #FilterMaximumAltitude.
        *
        * Throws is MaximumAltitudeIn is nan.
        * Throws if MaximumAltitudeIn < FilterMinimumAltitude.
        */
        void setFilterMaximumAltitude(double MaximumAltitudeIn);

        /**
        * @brief Throwing set method for #FilterMinimumCumulativeGroundDistance.
        *
        * Throws if MinimumCumulativeGroundDistanceIn is nan.
        * Throws if MinimumCumulativeGroundDistanceIn > FilterMaximumCumulativeGroundDistance.
        */
        void setFilterMinimumCumulativeGroundDistance(double MinimumCumulativeGroundDistanceIn);

        /**
        * @brief Throwing set method for #FilterMaximumCumulativeGroundDistance.
        *
        * Throws if MaximumCumulativeGroundDistanceIn is nan.
        * Throws if MaximumCumulativeGroundDistanceIn < FilterMinimumCumulativeGroundDistance.
        */
        void setFilterMaximumCumulativeGroundDistance(double MaximumCumulativeGroundDistanceIn);
    };
}
