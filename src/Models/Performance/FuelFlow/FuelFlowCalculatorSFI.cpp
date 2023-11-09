// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "FuelFlowCalculatorSFI.h"

#include "Aircraft/FuelEmissions/SFI.h"

namespace GRAPE {
    void FuelFlowCalculatorSFI::calculate(const OperationArrival& Op, PerformanceOutput& Perf) const {
        const auto& atm = atmosphere(Op);
        const SFI& sfi = *Op.aircraft().SFIFuel;

        for (auto& pt : Perf | std::views::values)
            pt.FuelFlowPerEng = sfi.arrivalFuelFlow(pt.AltitudeMsl, pt.TrueAirspeed, pt.CorrNetThrustPerEng, atm);
    }

    void FuelFlowCalculatorSFI::calculate(const OperationDeparture& Op, PerformanceOutput& Perf) const {
        const SFI& sfi = *Op.aircraft().SFIFuel;
        const auto& atm = atmosphere(Op);

        for (auto& pt : Perf | std::views::values)
            pt.FuelFlowPerEng = sfi.departureFuelFlow(pt.AltitudeMsl, pt.TrueAirspeed, pt.CorrNetThrustPerEng, atm);
    }
}
