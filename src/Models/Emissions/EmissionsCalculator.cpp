// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "EmissionsCalculator.h"

#include "Aircraft/Aircraft.h"

namespace GRAPE {
    EmissionsCalculator::EmissionsCalculator(const PerformanceSpecification& PerfSpec, const EmissionsSpecification& EmissionsSpec) : m_PerfSpec(PerfSpec), m_EmissionsSpec(EmissionsSpec) {}

    EmissionsOperationOutput EmissionsCalculator::calculateEmissions(const Operation& Op, const PerformanceOutput& PerfOut) {
        EmissionsOperationOutput out;

        auto startIt = PerfOut.begin();

        for (auto endIt = std::next(PerfOut.begin()); endIt != PerfOut.end(); ++endIt)
        {
            const auto& [startCumGroundDistance, startPt] = *startIt;
            const auto& [endCumGroundDistance, endPt] = *endIt;

            if (pointAfterDistanceLimits(startCumGroundDistance))
                break;

            if (!segmentInDistanceLimits(startCumGroundDistance, endCumGroundDistance))
            {
                startIt = endIt;
                continue;
            }

            if (!segmentInAltitudeLimits(std::min(startPt.AltitudeMsl, endPt.AltitudeMsl), std::max(startPt.AltitudeMsl, endPt.AltitudeMsl)))
            {
                startIt = endIt;
                continue;
            }

            EmissionsSegmentOutput segOut;
            segOut.Index = std::distance(PerfOut.begin(), startIt);


            const double segFuelFlow = std::midpoint(startPt.FuelFlowPerEng, endPt.FuelFlowPerEng);
            if (segFuelFlow < Constants::Precision)
                continue;

            const double segSpeed = std::midpoint(startPt.Groundspeed, endPt.Groundspeed);
            const double segTime = (endCumGroundDistance - startCumGroundDistance) / segSpeed;

            segOut.Fuel = segFuelFlow * segTime * Op.aircraft().EngineCount * Op.Count;
            out.addSegmentOutput(segOut);

            startIt = endIt;
        }

        return out;
    }

    bool EmissionsCalculator::pointAfterDistanceLimits(double CumulativeGroundDistance) const {
        return CumulativeGroundDistance > m_EmissionsSpec.FilterMaximumCumulativeGroundDistance;
    }

    bool EmissionsCalculator::segmentInDistanceLimits(double StartCumulativeGroundDistance, double EndCumulativeGroundDistance) const {
        return StartCumulativeGroundDistance >= m_EmissionsSpec.FilterMinimumCumulativeGroundDistance && EndCumulativeGroundDistance < m_EmissionsSpec.FilterMaximumCumulativeGroundDistance;
    }

    bool EmissionsCalculator::segmentInAltitudeLimits(double LowerAltitude, double HigherAltitude) const {
        return LowerAltitude >= m_EmissionsSpec.FilterMinimumAltitude && HigherAltitude <= m_EmissionsSpec.FilterMaximumAltitude;
    }
}
