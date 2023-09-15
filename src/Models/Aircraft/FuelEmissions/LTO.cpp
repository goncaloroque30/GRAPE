// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "LTO.h"

namespace GRAPE {
    void LTOEngine::setFuelFlow(LTOPhase Phase, double FuelFlow) {
        if (!(FuelFlow >= 0.0))
            throw GrapeException("Fuel flow must be at least 0 kg/s.");

        FuelFlows.at(magic_enum::enum_integer(Phase)) = FuelFlow;
    }

    void LTOEngine::setFuelFlowCorrection(LTOPhase Phase, double FuelFlowCorrection) {
        if (!(FuelFlowCorrection >= 0.0))
            throw GrapeException("Fuel flow correction must be at least 0.");

        FuelFlowCorrectionFactors.at(magic_enum::enum_integer(Phase)) = FuelFlowCorrection;
    }

    void LTOEngine::setEmissionIndexHC(LTOPhase Phase, double HCEI) {
        if (!(HCEI >= 0.0))
            throw GrapeException("Hydrocarbon emission index must be at least 0 kg/kg.");

        EmissionIndexesHC.at(magic_enum::enum_integer(Phase)) = HCEI;
    }

    void LTOEngine::setEmissionIndexCO(LTOPhase Phase, double COEI) {
        if (!(COEI >= 0.0))
            throw GrapeException("Carbon monoxide emission index must be at least 0 kg/kg.");

        EmissionIndexesCO.at(magic_enum::enum_integer(Phase)) = COEI;
    }

    void LTOEngine::setEmissionIndexNOx(LTOPhase Phase, double NOxEI) {
        if (!(NOxEI >= 0.0))
            throw GrapeException("Nitrogen oxides emission index must be at least 0 kg/kg.");

        EmissionIndexesNOx.at(magic_enum::enum_integer(Phase)) = NOxEI;
    }
}
