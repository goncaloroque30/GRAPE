// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "LTOFuelFlowGenerator.h"

namespace GRAPE {
    LTOFuelFlowGenerator::LTOFuelFlowGenerator(const LTOEngine& LTOEng) {
        // Corrected fuel flows
        for (std::size_t i = 0; i < LTOPhases.size(); ++i)
            m_CorrectedFuelFlows.at(i) = LTOEng.FuelFlows.at(i) * LTOEng.FuelFlowCorrectionFactors.at(i);
    }

    double LTOFuelFlowGenerator::fuelFlow(FlightPhase Phase) const {
        const std::size_t ltoIndex = magic_enum::enum_integer(ltoPhase(Phase));
        return m_CorrectedFuelFlows.at(ltoIndex);
    }
}
