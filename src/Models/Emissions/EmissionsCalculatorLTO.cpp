// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "EmissionsCalculatorLTO.h"

namespace GRAPE {
    EmissionsCalculatorLTO::EmissionsCalculatorLTO(const PerformanceSpecification& PerfSpec, const EmissionsSpecification& EmissionsSpec) : EmissionsCalculator(PerfSpec, EmissionsSpec) {}

    EmissionsOperationOutput EmissionsCalculatorLTO::calculateEmissions(const Operation& Op, const PerformanceOutput& PerfOut) const {
        GRAPE_ASSERT(Op.aircraft().LTOEng);
        GRAPE_ASSERT(m_LTOEngines.contains(Op.aircraft().LTOEng));

        const auto& ltoEng = m_LTOEngines.at(Op.aircraft().LTOEng);
        EmissionsOperationOutput out;

        for (auto it = std::next(PerfOut.begin()); it != PerfOut.end(); ++it)
        {
            const auto& [startCumGroundDistance, startPt] = *std::prev(it);
            const auto& [endCumGroundDistance, endPt] = *it;

            if (pointAfterDistanceLimits(startCumGroundDistance))
                break;

            if (!segmentInDistanceLimits(startCumGroundDistance, endCumGroundDistance))
                continue;

            if (!segmentInAltitudeLimits(std::min(startPt.AltitudeMsl, endPt.AltitudeMsl), std::max(startPt.AltitudeMsl, endPt.AltitudeMsl)))
                continue;

            EmissionsSegmentOutput segOut;
            segOut.Index = std::distance(PerfOut.begin(), std::prev(it));

            // Segment Values
            const double segFuelFlow = std::midpoint(startPt.FuelFlowPerEng, endPt.FuelFlowPerEng);
            if (segFuelFlow < Constants::Precision)
                continue;

            const double segGroundSpeed = std::midpoint(startPt.Groundspeed, endPt.Groundspeed);
            const double segTime = (endCumGroundDistance - startCumGroundDistance) / segGroundSpeed;

            segOut.Fuel = segFuelFlow * segTime * Op.aircraft().EngineCount * Op.Count;

            segOut.Emissions.HC = ltoEng.hcEI(startPt.FlPhase) * segOut.Fuel;
            segOut.Emissions.CO = ltoEng.coEI(startPt.FlPhase) * segOut.Fuel;
            segOut.Emissions.NOx = ltoEng.noxEI(startPt.FlPhase) * segOut.Fuel;
            segOut.Emissions.nvPM = ltoEng.nvPMEI(startPt.FlPhase) * segOut.Fuel;
            segOut.Emissions.nvPMNumber = ltoEng.nvPMNumberEI(startPt.FlPhase) * segOut.Fuel;

            out.addSegmentOutput(segOut);
        }

        return out;
    }
}
