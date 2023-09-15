// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "FuelFlowCalculatorLTO.h"

#include "Aircraft/FuelEmissions/LTO.h"

namespace GRAPE {
    void FuelFlowCalculatorLTO::calculate(const OperationArrival& Op, PerformanceOutput& Perf) const {
        GRAPE_ASSERT(m_FuelFlowGenerators.contains(Op.aircraft().LTOEng));

        const auto& gen = m_FuelFlowGenerators.at(Op.aircraft().LTOEng);
        const auto& atm = atmosphere(Op);

        for (auto& pt : Perf | std::views::values)
            pt.FuelFlowPerEng = gen.fuelFlow(pt.FlPhase, pt.AltitudeMsl, pt.TrueAirspeed, atm);
    }

    void FuelFlowCalculatorLTO::calculate(const OperationDeparture& Op, PerformanceOutput& Perf) const {
        GRAPE_ASSERT(m_FuelFlowGenerators.contains(Op.aircraft().LTOEng));

        const auto& gen = m_FuelFlowGenerators.at(Op.aircraft().LTOEng);
        const auto& atm = atmosphere(Op);

        for (auto& pt : Perf | std::views::values)
            pt.FuelFlowPerEng = gen.fuelFlow(pt.FlPhase, pt.AltitudeMsl, pt.TrueAirspeed, atm);
    }

    void FuelFlowCalculatorLTO::addLTOEngine(const LTOEngine* LTOEng) {
        // Check if generator already added
        if (m_FuelFlowGenerators.contains(LTOEng))
            return;

        // Add the generator
        auto [ltoFuelFlowGen, added] = m_FuelFlowGenerators.add(LTOEng, *LTOEng); // Forwards LTOEng to the constructor of LTOFuelFlowGenerator
    }

    void FuelFlowCalculatorLTODoc9889::calculate(const OperationArrival& Op, PerformanceOutput& Perf) const {
        GRAPE_ASSERT(m_FuelFlowGenerators.contains(Op.aircraft().LTOEng));

        const auto& acft = Op.aircraft();
        const auto& gen = m_FuelFlowGenerators.at(acft.LTOEng);
        const auto& atm = atmosphere(Op);

        for (auto& pt : Perf | std::views::values)
            pt.FuelFlowPerEng = gen.fuelFlow(pt.FlPhase, pt.AltitudeMsl, pt.TrueAirspeed, atm, pt.CorrNetThrustPerEng / acft.MaximumSeaLevelStaticThrust);
    }

    void FuelFlowCalculatorLTODoc9889::calculate(const OperationDeparture& Op, PerformanceOutput& Perf) const {
        GRAPE_ASSERT(m_FuelFlowGenerators.contains(Op.aircraft().LTOEng));

        const auto& acft = Op.aircraft();
        const auto& gen = m_FuelFlowGenerators.at(acft.LTOEng);
        const auto& atm = atmosphere(Op);

        for (auto& pt : Perf | std::views::values)
            pt.FuelFlowPerEng = gen.fuelFlow(pt.FlPhase, pt.AltitudeMsl, pt.TrueAirspeed, atm, pt.CorrNetThrustPerEng / acft.MaximumSeaLevelStaticThrust);
    }
}
