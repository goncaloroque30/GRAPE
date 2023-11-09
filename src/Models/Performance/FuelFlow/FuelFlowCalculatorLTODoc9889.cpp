// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "FuelFlowCalculatorLTODoc9889.h"

#include "Aircraft/FuelEmissions/LTO.h"
#include "Base/Math.h"

namespace GRAPE {
    namespace {
        double altitudeCorrectionNone(double FuelFlow, double AltitudeMsl, double TrueAirspeed, const Atmosphere& Atm) {
            return FuelFlow;
        }

        double altitudeCorrectionBFFM2(double FuelFlow, double AltitudeMsl, double TrueAirspeed, const Atmosphere& Atm) {
            return FuelFlow * Atm.pressureRatio(AltitudeMsl) / (std::pow(Atm.temperatureRatio(AltitudeMsl), 3.8) * std::exp(0.2 * std::pow(machNumber(TrueAirspeed, AltitudeMsl, Atm), 2)));
        }
    }

    FuelFlowCalculatorLTODoc9889::FuelFlowCalculatorLTODoc9889(const PerformanceSpecification& PerfSpec) : FuelFlowCalculator(PerfSpec) {
        m_AltitudeCorrection = m_Spec.FuelFlowLTOAltitudeCorrection ? altitudeCorrectionBFFM2 : altitudeCorrectionNone;
    }

    void FuelFlowCalculatorLTODoc9889::calculate(const OperationArrival& Op, PerformanceOutput& Perf) const {
        GRAPE_ASSERT(m_FuelFlowGenerators.contains(Op.aircraft().LTOEng));

        const auto* ltoEng = Op.aircraft().LTOEng;
        const auto& gen = m_FuelFlowGenerators.at(ltoEng);
        const auto& atm = atmosphere(Op);

        for (auto& pt : Perf | std::views::values)
            pt.FuelFlowPerEng = m_AltitudeCorrection(gen.fuelFlow(pt.FlPhase, pt.CorrNetThrustPerEng / ltoEng->MaximumSeaLevelStaticThrust), pt.AltitudeMsl, pt.TrueAirspeed, atm);
    }

    void FuelFlowCalculatorLTODoc9889::calculate(const OperationDeparture& Op, PerformanceOutput& Perf) const {
        GRAPE_ASSERT(m_FuelFlowGenerators.contains(Op.aircraft().LTOEng));

        const auto* ltoEng = Op.aircraft().LTOEng;
        const auto& gen = m_FuelFlowGenerators.at(ltoEng);
        const auto& atm = atmosphere(Op);

        for (auto& pt : Perf | std::views::values)
            pt.FuelFlowPerEng = m_AltitudeCorrection(gen.fuelFlow(pt.FlPhase, pt.CorrNetThrustPerEng / ltoEng->MaximumSeaLevelStaticThrust), pt.AltitudeMsl, pt.TrueAirspeed, atm);
    }

    void FuelFlowCalculatorLTODoc9889::addLTOEngine(const LTOEngine* LTOEng) {
        // Check if generator already added
        if (m_FuelFlowGenerators.contains(LTOEng))
            return;

        // Add the generator
        auto [ltoFuelFlowGen, added] = m_FuelFlowGenerators.add(LTOEng, *LTOEng); // Forwards LTOEng to the constructor of LTODoc9889FuelFlowGenerator
    }
}
