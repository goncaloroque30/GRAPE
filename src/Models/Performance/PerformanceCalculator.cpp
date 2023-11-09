// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "PerformanceCalculator.h"

#include "FuelFlow/FuelFlowCalculatorLTO.h"
#include "FuelFlow/FuelFlowCalculatorLTODoc9889.h"
#include "FuelFlow/FuelFlowCalculatorSFI.h"
#include "Operation/Operation.h"

namespace GRAPE {
    PerformanceCalculator::PerformanceCalculator(const PerformanceSpecification& Spec) : m_Spec(Spec) {
        switch (m_Spec.FuelFlowMdl)
        {
        case FuelFlowModel::None: m_FuelFlow = std::make_unique<FuelFlowCalculator>(m_Spec); break;
        case FuelFlowModel::LTO: m_FuelFlow = std::make_unique<FuelFlowCalculatorLTO>(m_Spec); break;
        case FuelFlowModel::LTODoc9889: m_FuelFlow = std::make_unique<FuelFlowCalculatorLTODoc9889>(m_Spec); break;
        case FuelFlowModel::SFI: m_FuelFlow = std::make_unique<FuelFlowCalculatorSFI>(m_Spec); break;
        default: GRAPE_ASSERT(false);
        }
    }

    std::size_t PerformanceCalculator::segmentAndFilter(const Operation& Op, PerformanceOutput& PerfOutput) const {
        std::size_t ret = 0;

        if (!std::isnan(m_Spec.SpeedDeltaSegmentationThreshold))
            PerfOutput.speedSegmentation(*m_Spec.CoordSys, m_Spec.SpeedDeltaSegmentationThreshold);

        if (!std::isnan(m_Spec.FilterGroundDistanceThreshold))
            ret = PerfOutput.groundDistanceFilter(m_Spec.FilterGroundDistanceThreshold);

        return ret;
    }

    bool PerformanceCalculator::pointInDistanceLimits(double CumulativeGroundDistance) const {
        return CumulativeGroundDistance >= m_Spec.FilterMinimumCumulativeGroundDistance && CumulativeGroundDistance <= m_Spec.FilterMaximumCumulativeGroundDistance;
    }

    bool PerformanceCalculator::pointInAltitudeLimits(double Altitude) const {
        return Altitude >= m_Spec.FilterMinimumAltitude && Altitude <= m_Spec.FilterMaximumAltitude;
    }
}
