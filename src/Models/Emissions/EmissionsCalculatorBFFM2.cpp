// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "EmissionsCalculatorBFFM2.h"

namespace GRAPE {
    EmissionsCalculatorBFFM2::EmissionsCalculatorBFFM2(const PerformanceSpecification& PerfSpec, const EmissionsSpecification& EmissionsSpec) : EmissionsCalculator(PerfSpec, EmissionsSpec) {}

    EmissionsOperationOutput EmissionsCalculatorBFFM2::calculateEmissions(const Operation& Op, const PerformanceOutput& PerfOut) const {
        GRAPE_ASSERT(Op.aircraft().LTOEng);
        GRAPE_ASSERT(m_LTOEngines.contains(Op.aircraft().LTOEng));

        const auto& atm = m_PerfSpec.Atmospheres.atmosphere(Op.Time);
        const auto& ltoEng = m_LTOEngines.at(Op.aircraft().LTOEng);
        const auto& emissionsGenerator = m_EmissionsGenerators.at(&ltoEng);

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

            const double segAltitude = std::midpoint(startPt.AltitudeMsl, endPt.AltitudeMsl);
            const double segTrueAirspeed = std::midpoint(startPt.TrueAirspeed, endPt.TrueAirspeed);
            const double segGroundSpeed = std::midpoint(startPt.Groundspeed, endPt.Groundspeed);
            const double segTime = (endCumGroundDistance - startCumGroundDistance) / segGroundSpeed;

            segOut.Fuel = segFuelFlow * segTime * Op.aircraft().EngineCount * Op.Count;

            const auto [hc, co, nox] = emissionsGenerator.emissionIndexes(segFuelFlow, segAltitude, segTrueAirspeed, atm);
            segOut.Emissions.HC = hc * segOut.Fuel;
            segOut.Emissions.CO = co * segOut.Fuel;
            segOut.Emissions.NOx = nox * segOut.Fuel;
            segOut.Emissions.nvPM = ltoEng.nvPMEI(startPt.FlPhase) * segOut.Fuel;
            segOut.Emissions.nvPMNumber = ltoEng.nvPMNumberEI(startPt.FlPhase) * segOut.Fuel;

            out.addSegmentOutput(segOut);
        }

        return out;
    }

    void EmissionsCalculatorBFFM2::addLTOEngine(const LTOEngine* LTOEng) {
        // Call base to add LTOEng to the list
        EmissionsCalculator::addLTOEngine(LTOEng);

        // Get added engine
        GRAPE_ASSERT(m_LTOEngines.contains(LTOEng));
        const auto& ltoEng = m_LTOEngines.at(LTOEng);

        // Check if generator already added
        if (m_EmissionsGenerators.contains(&ltoEng))
            return;

        // Add the generator
        auto [ltoFuelFlowGen, added] = m_EmissionsGenerators.add(&ltoEng, ltoEng); // Forwards ltoEng to the constructor of BFFM2EmissionsGenerator
        GRAPE_ASSERT(added);
    }
}
