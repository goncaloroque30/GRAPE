// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Base/BaseModels.h"
#include "Aircraft/FuelEmissions/LTO.h"

namespace GRAPE {
    /**
    * @brief Defines the parameters needed by a fuel & emissions run.
    */
    struct EmissionsSpecification {
        bool CalculateGasEmissions = true;
        bool CalculateParticleEmissions = true;

        EmissionsModel EmissionsMdl = EmissionsModel::Segments;
        bool BFFM2Model = true;

        EmissionsParticleSmokeNumberModel ParticleSmokeNumberModel = EmissionsParticleSmokeNumberModel::FOA4;

        std::array<double, 4> LTOCycle{ { 1560.0, 240.0, 132.0, 42.0 } };

        double ParticleEffectiveDensity = 1000.0;
        double ParticleGeometricStandardDeviation = 1.8;
        std::array<double, 4> ParticleGeometricMeanDiameter{ { 20e-9, 20e-9, 40e-9, 40e-9 } };

        bool SaveSegmentResults = false;

        double FilterMinimumAltitude = -Constants::Inf;
        double FilterMaximumAltitude = Constants::Inf;
        double FilterMinimumCumulativeGroundDistance = -Constants::Inf;
        double FilterMaximumCumulativeGroundDistance = Constants::Inf;

        /**
        * @brief Throwing set method for #LTOCycle.
        *
        * Throws if Seconds < 0.
        */
        void setLTOCycle(LTOPhase Phase, double Seconds);

        /**
        * @brief Throwing set method for #ParticleEffectiveDensity.
        *
        * Throws if ParticleEffectiveDensityIn <= 0.
        */
        void setParticleEffectiveDensity(double ParticleEffectiveDensityIn);

        /**
        * @brief Throwing set method for #ParticleGeometricStandardDeviation.
        *
        * Throws if ParticleGeometricStandardDeviationIn <= 0.
        */
        void setParticleGeometricStandardDeviation(double ParticleGeometricStandardDeviationIn);

        /**
        * @brief Throwing set method for #ParticleGeometricMeanDiameter.
        *
        * Throws if GeometricMeanDiameter <= 0.
        */
        void setParticleGeometricMeanDiameter(LTOPhase Phase, double GeometricMeanDiameter);

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
