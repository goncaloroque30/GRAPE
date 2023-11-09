// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "EmissionsOutput.h"
#include "EmissionsSpecification.h"
#include "Operation/Operation.h"
#include "Performance/PerformanceOutput.h"
#include "Performance/PerformanceSpecification.h"

namespace GRAPE {
    /**
    * @brief Base class for calculating the fuel and emissions for each segment of an operation.
    */
    class EmissionsCalculator {
    public:
        EmissionsCalculator(const PerformanceSpecification& PerfSpec, const EmissionsSpecification& EmissionsSpec);
        virtual ~EmissionsCalculator() = default;

        /**
        * @brief Implement in concrete classes
        */
        [[nodiscard]] virtual EmissionsOperationOutput calculateEmissions(const Operation& Op, const PerformanceOutput& PerfOut) const = 0;

        /**
        * @brief Base implementation. Subclasses may implement and use the data of LTOEngines as necessary.
        */
        virtual void addLTOEngine(const LTOEngine* LTOEng);
    protected:
        const PerformanceSpecification& m_PerfSpec;
        const EmissionsSpecification& m_EmissionsSpec;

        GrapeMap<const LTOEngine*, LTOEngine> m_LTOEngines;
    protected:
        [[nodiscard]] bool pointAfterDistanceLimits(double CumulativeGroundDistance) const;
        [[nodiscard]] bool segmentInDistanceLimits(double StartCumulativeGroundDistance, double EndCumulativeGroundDistance) const;
        [[nodiscard]] bool segmentInAltitudeLimits(double LowerAltitude, double HigherAltitude) const;
    };
}
