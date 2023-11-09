// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "LTODoc9889FuelFlowGenerator.h"

#include "Base/Atmosphere.h"
#include "Base/Math.h"

namespace GRAPE {
    namespace {
        constexpr std::array<double, 4> LTOThrustSettings = { { 0.07, 0.30, 0.85, 1.0 } };

        void quadraticInterpolation(double X1, double X2, double X3, double Y1, double Y2, double Y3, double& oA, double& oB, double& oC) {
            oA = (Y3 - Y1) / ((X3 - X1) * (X1 - X2)) - (Y3 - Y2) / ((X3 - X2) * (X1 - X2));
            oB = (Y3 - Y1) / (X3 - X1) - oA * (X3 + X1);
            oC = Y3 - oA * X3 * X3 - oB * X3;
        }
    }

    LTODoc9889FuelFlowGenerator::LTODoc9889FuelFlowGenerator(const LTOEngine& LTOEng) {
        // Corrected fuel flows
        for (std::size_t i = 0; i < LTOPhases.size(); ++i)
            m_CorrectedFuelFlows.at(i) = LTOEng.FuelFlows.at(i) * LTOEng.FuelFlowCorrectionFactors.at(i);

        // Fuel Flow Quadratic (60 - 85 - 100)
        // Calculate corrected ratios
        std::array<double, LTOPhases.size()> fuelFlowRatios{};
        for (std::size_t i = 0; i < LTOPhases.size(); ++i)
            fuelFlowRatios.at(i) = m_CorrectedFuelFlows.at(i) / m_CorrectedFuelFlows.back();

        int lto_index_start = 1; // point at 30%
        // Calculate quadratic factors
        for (std::size_t i = 0; i < m_FuelFlowQuadratic.size(); ++i)
        {
            auto& quad = m_FuelFlowQuadratic.at(i);
            const double idx = lto_index_start + i;
            quadraticInterpolation(LTOThrustSettings.at(idx - 1), LTOThrustSettings.at(idx), LTOThrustSettings.at(idx + 1), fuelFlowRatios.at(idx - 1), fuelFlowRatios.at(idx), fuelFlowRatios.at(idx + 1), quad.A, quad.B, quad.C);
        }
    }

    double LTODoc9889FuelFlowGenerator::fuelFlow(FlightPhase Phase, double ThrustPercentage) const {
        const std::size_t ltoIndex = magic_enum::enum_integer(ltoPhase(Phase));
        if (ThrustPercentage <= 0.60 + Constants::Precision)
        {
            return m_CorrectedFuelFlows.at(ltoIndex);
        }
        else
        {
            const auto& quad = ThrustPercentage < 0.85 ? m_FuelFlowQuadratic.at(0) : m_FuelFlowQuadratic.at(1);
            return (quad.A * ThrustPercentage * ThrustPercentage + quad.B * ThrustPercentage + quad.C) * m_CorrectedFuelFlows.back();
        }
    }

    TEST_CASE("Fuel Flow Doc9889") {
        LTOEngine lto("Trent 553-61");
        lto.FuelFlows = { 0.23, 0.6, 1.73, 2.11 };
        lto.FuelFlowCorrectionFactors = { 1.0, 1.0, 1.0, 1.0 };

        LTODoc9889FuelFlowGenerator ltoGen(lto);

        CHECK_EQ(round(ltoGen.fuelFlow(FlightPhase::Climb, 0.7), 3), doctest::Approx(1.388).epsilon(Constants::PrecisionTest));
        CHECK_EQ(round(ltoGen.fuelFlow(FlightPhase::Climb, 0.9), 3), doctest::Approx(1.853).epsilon(Constants::PrecisionTest));
    }
}
