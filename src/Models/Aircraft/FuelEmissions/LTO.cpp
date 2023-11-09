// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "LTO.h"

namespace GRAPE {
    double LTOEngine::fuelFlow(FlightPhase FlPhase) const {
        return FuelFlows.at(ltoIndex(FlPhase));
    }

    double LTOEngine::fuelFlowCorrectionFactor(FlightPhase FlPhase) const {
        return FuelFlowCorrectionFactors.at(ltoIndex(FlPhase));
    }

    double LTOEngine::hcEI(FlightPhase FlPhase) const {
        return EmissionIndexesHC.at(ltoIndex(FlPhase));
    }

    double LTOEngine::coEI(FlightPhase FlPhase) const {
        return EmissionIndexesCO.at(ltoIndex(FlPhase));
    }

    double LTOEngine::noxEI(FlightPhase FlPhase) const {
        return EmissionIndexesNOx.at(ltoIndex(FlPhase));
    }

    double LTOEngine::smokeNumber(FlightPhase FlPhase) const {
        return SmokeNumbers.at(ltoIndex(FlPhase));
    }

    double LTOEngine::nvPMEI(FlightPhase FlPhase) const {
        return EmissionIndexesNVPM.at(ltoIndex(FlPhase));
    }

    double LTOEngine::nvPMNumberEI(FlightPhase FlPhase) const {
        return EmissionIndexesNVPMNumber.at(ltoIndex(FlPhase));
    }

    void LTOEngine::setMaximumSeaLevelStaticThrust(double MaximumSeaLevelStaticThrustIn) {
        if (!(MaximumSeaLevelStaticThrustIn >= 1.0))
            throw GrapeException("Maximum sea level static thrust must be at least 1 N.");
        MaximumSeaLevelStaticThrust = MaximumSeaLevelStaticThrustIn;
    }

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

    void LTOEngine::setBypassRatio(double BypassRatioIn) {
        if (!(BypassRatioIn >= 0.0))
            throw GrapeException("Bypass ratio must be at least 0.");

        BypassRatio = BypassRatioIn;
    }

    void LTOEngine::setAirFuelRatio(LTOPhase Phase, double AirFuelRatio) {
        if (!(AirFuelRatio >= 0.0))
            throw GrapeException("Air to fuel ratio must be at least 0.");

        AirFuelRatios.at(magic_enum::enum_integer(Phase)) = AirFuelRatio;
    }

    void LTOEngine::setSmokeNumber(LTOPhase Phase, double SmokeNumber) {
        if (!(SmokeNumber >= 0.0))
            throw GrapeException("Smoke number must be at least 0.");

        SmokeNumbers.at(magic_enum::enum_integer(Phase)) = SmokeNumber;
    }

    void LTOEngine::setEmissionIndexNVPM(LTOPhase Phase, double NVPMEI) {
        if (!(NVPMEI >= 0.0))
            throw GrapeException("Non-volatile particulate matter emission index must be at least 0 kg/kg.");

        EmissionIndexesNVPM.at(magic_enum::enum_integer(Phase)) = NVPMEI;
    }

    void LTOEngine::setEmissionIndexNVPMNumber(LTOPhase Phase, double NVPMEINumber) {
        if (!(NVPMEINumber >= 0.0))
            throw GrapeException("Non-volatile particulate matter number emission index must be at least 0 #/kg.");

        EmissionIndexesNVPMNumber.at(magic_enum::enum_integer(Phase)) = NVPMEINumber;
    }

}
