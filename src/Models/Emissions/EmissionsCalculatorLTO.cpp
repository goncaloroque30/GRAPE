// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "EmissionsCalculatorLTO.h"

namespace GRAPE {
    LTOFuelEmissionsCalculator::LTOFuelEmissionsCalculator(const PerformanceSpecification& PerfSpec, const EmissionsSpecification& EmissionsSpec) : EmissionsCalculator(PerfSpec, EmissionsSpec) {}

    EmissionsOperationOutput LTOFuelEmissionsCalculator::calculateEmissions(const Operation& Op, const PerformanceOutput& PerfOut) {
        GRAPE_ASSERT(m_EmissionsGenerators.contains(Op.aircraft().LTOEng));

        const auto& atm = m_PerfSpec.Atmospheres.atmosphere(Op.Time);
        const auto& emissionsGenerator = m_EmissionsGenerators(Op.aircraft().LTOEng);

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

            out.addSegmentOutput(segOut);

            startIt = endIt;
        }

        return out;
    }

    void LTOFuelEmissionsCalculator::addLTOEngine(const LTOEngine* LTOEng) {
        // Check if generator already added
        if (m_EmissionsGenerators.contains(LTOEng))
            return;

        // Add the generator
        auto [ltoFuelFlowGen, added] = m_EmissionsGenerators.add(LTOEng, *LTOEng); // Forwards LTOEng to the constructor of LTOEmissionsGenerator
    }
}
