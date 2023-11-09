// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "PerformanceOutput.h"
#include "PerformanceSpecification.h"
#include "FuelFlow/FuelFlowCalculator.h"

namespace GRAPE {
    class Operation;

    /**
    * @brief Base class for calculating the PerformanceOutput for any given Operation.
    */
    class PerformanceCalculator {
    public:
        explicit PerformanceCalculator(const PerformanceSpecification& Spec);
        virtual ~PerformanceCalculator() = default;

        /**
         * @brief Give caller access to fuel flow calculator. May be needed in order to prepare calculations.
        */
        [[nodiscard]] FuelFlowCalculator& fuelFlowCalculator() { return *m_FuelFlow; }
    protected:
        const PerformanceSpecification& m_Spec;
        std::unique_ptr<FuelFlowCalculator> m_FuelFlow{};
    protected:
        /**
        * @brief Go through PerformanceOutput and perform segmentation and filtering.
        * @return True if after segmenting and filtering the PerformanceOutput has at least 2 points, negative otherwise.
        */
        [[nodiscard]] std::size_t segmentAndFilter(const Operation& Op, PerformanceOutput& PerfOutput) const;

        /**
        * @return True if CumulativeGroundDistance in [m_Spec.FilterMinimumCumulativeGroundDistance, m_Spec.FilterMaximumCumulativeGroundDistance]
        */
        [[nodiscard]] bool pointInDistanceLimits(double CumulativeGroundDistance) const;

        /**
        * @return True if Altitude in [m_Spec.FilterMinimumAltitude, m_Spec.FilterMaximumAltitude]
        */
        [[nodiscard]] bool pointInAltitudeLimits(double Altitude) const;
    };
}
