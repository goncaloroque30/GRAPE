// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "FuelFlowCalculator.h"

namespace GRAPE {
    void FuelFlowCalculator::calculate(const OperationArrival& Op, PerformanceOutput& Perf) const {
        for (auto& pt : Perf | std::views::values)
            pt.FuelFlowPerEng = 0.0;
    }

    void FuelFlowCalculator::calculate(const OperationDeparture& Op, PerformanceOutput& Perf) const {
        for (auto& pt : Perf | std::views::values)
            pt.FuelFlowPerEng = 0.0;
    }

    const Atmosphere& FuelFlowCalculator::atmosphere(const Operation& Op) const {
        return m_Spec.Atmospheres.atmosphere(Op.Time);
    }
}
