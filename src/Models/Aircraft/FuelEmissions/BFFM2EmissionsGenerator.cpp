// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "BFFM2EmissionsGenerator.h"

#include "Base/Atmosphere.h"
#include "Base/Math.h"

namespace GRAPE {
    BFFM2EmissionsGenerator::BFFM2EmissionsGenerator(const LTOEngine& LTOEng) {
        // Log and Corrected Fuel Flows
        for (std::size_t i = 0; i < LTOPhases.size(); i++)
            m_LogCorrectedFuelFlow.at(i) = std::log10(std::max(Constants::Precision, LTOEng.FuelFlows.at(i) * LTOEng.FuelFlowCorrectionFactors.at(i)));

        // Set minimum EIs to 0 due to log10
        std::array<double, LTOPhases.size()> emissionIndexesHC{};
        std::array<double, LTOPhases.size()> emissionIndexesCO{};
        std::array<double, LTOPhases.size()> emissionIndexesNOx{};
        for (std::size_t i = 0; i < LTOPhases.size(); ++i)
        {
            emissionIndexesHC.at(i) = std::max(LTOEng.EmissionIndexesHC.at(i), Constants::Precision);
            emissionIndexesCO.at(i) = std::max(LTOEng.EmissionIndexesCO.at(i), Constants::Precision);
            emissionIndexesNOx.at(i) = std::max(LTOEng.EmissionIndexesNOx.at(i), Constants::Precision);
        }

        // HC and CO

        // High fuel flow values as average of 85% (Climb Out) and 100% (Takeoff) thrust values
        const double logHCEmissionIndexHighFuelFlow = std::log10(std::midpoint(emissionIndexesHC.at(2), emissionIndexesHC.at(3)));
        m_HCEmissionIndexHighFuelFlow = std::pow(10.0, logHCEmissionIndexHighFuelFlow);

        const double logCOEmissionIndexHighFuelFlow = std::log10(std::midpoint(emissionIndexesCO.at(2), emissionIndexesCO.at(3)));
        m_COEmissionIndexHighFuelFlow = std::pow(10.0, logCOEmissionIndexHighFuelFlow);

        // Slope and intersect between 7% (Idle) and 30% (Approach) values
        m_HCLines.at(0).Slope = (std::log10(emissionIndexesHC.at(1)) - std::log10(emissionIndexesHC.at(0))) / (m_LogCorrectedFuelFlow.at(1) - m_LogCorrectedFuelFlow.at(0)); // slope = (y1 - y0) / (x1 - x0)
        m_HCLines.at(0).Intersect = std::log10(emissionIndexesHC.at(0)) - m_HCLines.at(0).Slope * m_LogCorrectedFuelFlow.at(0); // intersect = y0 - slope * x0

        m_COLines.at(0).Slope = (std::log10(emissionIndexesCO.at(1)) - std::log10(emissionIndexesCO.at(0))) / (m_LogCorrectedFuelFlow.at(1) - m_LogCorrectedFuelFlow.at(0)); // slope = (y1 - y0) / (x1 - x0)
        m_COLines.at(0).Intersect = std::log10(emissionIndexesCO.at(0)) - m_COLines.at(0).Slope * m_LogCorrectedFuelFlow.at(0); // intersect = y0 - slope * x0

        // Log fuel flow intersection between first line and high fuel flow emission index values (xI = (yI - intersect) / slope)
        const double logHCFuelFlowIntersection = (logHCEmissionIndexHighFuelFlow - m_HCLines.at(0).Intersect) / m_HCLines.at(0).Slope;
        const double logCOFuelFlowIntersection = (logCOEmissionIndexHighFuelFlow - m_COLines.at(0).Intersect) / m_COLines.at(0).Slope;

        // HC - Line between 30% (Approach) and high fuel flow value at 85% (Climb Out) needed if intersection is after the 100% (Takeoff) value OR the value at 30% is smaller than high fuel flow value
        if (logHCFuelFlowIntersection >= m_LogCorrectedFuelFlow.at(3) || std::log10(emissionIndexesHC.at(1)) < logHCEmissionIndexHighFuelFlow)
        {
            m_HCLines.at(1).Slope = (logHCEmissionIndexHighFuelFlow - std::log10(emissionIndexesHC.at(1))) / (m_LogCorrectedFuelFlow.at(2) - m_LogCorrectedFuelFlow.at(1)); // slope = (y1 - y0) / (x1 - x0)
            m_HCLines.at(1).Intersect = std::log10(emissionIndexesHC.at(1)) - m_HCLines.at(1).Slope * m_LogCorrectedFuelFlow.at(1); // intersect = y0 - slope * x0
            m_LogHCFuelFlowIntersect = m_LogCorrectedFuelFlow.at(2); // After the 85% (Climb Out) value the high value is used
        }
        else
        {
            m_HCLines.at(1) = m_HCLines.at(0); // Line before and after 30% (Approach) is the same
            m_LogHCFuelFlowIntersect = logHCFuelFlowIntersection; // The intersect is at the intersect with the high value
        }

        // CO - Line between 30% (Approach) and high fuel flow value at 85% (Climb Out) needed if intersection is after the 100% (Takeoff) OR the value at 30% is smaller than high fuel flow value (second condition not described in the paper)
        if (logCOFuelFlowIntersection >= m_LogCorrectedFuelFlow.at(3) || std::log10(emissionIndexesCO.at(1)) < logCOEmissionIndexHighFuelFlow)
        {
            m_COLines.at(1).Slope = (logCOEmissionIndexHighFuelFlow - std::log10(emissionIndexesCO.at(1))) / (m_LogCorrectedFuelFlow.at(2) - m_LogCorrectedFuelFlow.at(1)); // slope = (y1 - y0) / (x1 - x0)
            m_COLines.at(1).Intersect = std::log10(emissionIndexesCO.at(1)) - m_COLines.at(1).Slope * m_LogCorrectedFuelFlow.at(1); // intersect = y0 - slope * x0
            m_LogCOFuelFlowIntersect = m_LogCorrectedFuelFlow.at(2); // After the 85% (Climb Out) value the high value is used
        }
        else
        {
            m_COLines.at(1) = m_COLines.at(0); // Line before and after 30% (Approach) is the same
            m_LogCOFuelFlowIntersect = logCOFuelFlowIntersection; // The intersect is at the intersect with the high value
        }

        // NOx
        for (std::size_t i = 0; i < LTOPhases.size() - 1; i++)
        {
            m_NOxLines.at(i).Slope = (std::log10(emissionIndexesNOx.at(i + 1)) - std::log10(emissionIndexesNOx.at(i))) / (m_LogCorrectedFuelFlow.at(i + 1) - m_LogCorrectedFuelFlow.at(i));
            m_NOxLines.at(i).Intersect = std::log10(emissionIndexesNOx.at(i)) - m_LogCorrectedFuelFlow.at(i) * m_NOxLines.at(i).Slope;
        }
    }

    std::tuple<double, double, double> BFFM2EmissionsGenerator::emissionIndexes(double FuelFlow, double AltitudeMsl, double TrueAirspeed, const Atmosphere& Atm) const {
        const double refFuelFlow = FuelFlow * std::pow(Atm.temperatureRatio(AltitudeMsl), 3.8) * std::exp(0.2 * std::pow(machNumber(TrueAirspeed, AltitudeMsl, Atm), 2)) / Atm.pressureRatio(AltitudeMsl);

        // Extremely low fuel flows have no emissions
        if (refFuelFlow < Constants::Precision)
            return { 0.0, 0.0, 0.0 };

        const double logRefFuelFlow = std::log10(refFuelFlow);

        double refEmissionIndexHC = Constants::NaN;
        double refEmissionIndexCO = Constants::NaN;
        double refEmissionIndexNOx = Constants::NaN;

        // Lower limit is the 7% values
        if (logRefFuelFlow < m_LogCorrectedFuelFlow.at(0))
        {
            const double logLowFuelFlow = m_LogCorrectedFuelFlow.at(0);
            refEmissionIndexHC = std::pow(10.0, m_HCLines.at(0).Slope * logLowFuelFlow + m_HCLines.at(0).Intersect);
            refEmissionIndexCO = std::pow(10.0, m_COLines.at(0).Slope * logLowFuelFlow + m_COLines.at(0).Intersect);
            refEmissionIndexNOx = std::pow(10.0, m_NOxLines.at(0).Slope * logLowFuelFlow + m_NOxLines.at(0).Intersect);
        }
        // Fuel flow above 85%, use high values for HC and CO and third line for NOx
        else if (logRefFuelFlow > m_LogCorrectedFuelFlow.at(2))
        {
            refEmissionIndexHC = m_HCEmissionIndexHighFuelFlow;
            refEmissionIndexCO = m_COEmissionIndexHighFuelFlow;
            refEmissionIndexNOx = std::pow(10.0, m_NOxLines.at(2).Slope * logRefFuelFlow + m_NOxLines.at(2).Intersect);
        }
        // Between 7% and 30% fuel flow, use first lines
        else if (logRefFuelFlow < m_LogCorrectedFuelFlow.at(1))
        {
            refEmissionIndexHC = std::pow(10.0, m_HCLines.at(0).Slope * logRefFuelFlow + m_HCLines.at(0).Intersect);
            refEmissionIndexCO = std::pow(10.0, m_COLines.at(0).Slope * logRefFuelFlow + m_COLines.at(0).Intersect);
            refEmissionIndexNOx = std::pow(10.0, m_NOxLines.at(0).Slope * logRefFuelFlow + m_NOxLines.at(0).Intersect);
        }
        // Fuel flow is between 30% and 85% value
        else
        {
            refEmissionIndexHC = logRefFuelFlow < m_LogHCFuelFlowIntersect ? std::pow(10.0, m_HCLines.at(1).Slope * logRefFuelFlow + m_HCLines.at(1).Intersect) : m_HCEmissionIndexHighFuelFlow;
            refEmissionIndexCO = logRefFuelFlow < m_LogCOFuelFlowIntersect ? std::pow(10.0, m_COLines.at(1).Slope * logRefFuelFlow + m_COLines.at(1).Intersect) : m_COEmissionIndexHighFuelFlow;
            refEmissionIndexNOx = std::pow(10.0, m_NOxLines.at(1).Slope * logRefFuelFlow + m_NOxLines.at(1).Intersect);
        }

        const double temperatureRatioPower = std::pow(Atm.temperatureRatio(AltitudeMsl), 3.3);
        const double pressureRatioPower = std::pow(Atm.pressureRatio(AltitudeMsl), 1.02);

        // Humidity Correction for NOx
        const double temperatureC = toCelsius(Atm.temperature(AltitudeMsl));
        const double pSat = fromHectopascal(6.107 * std::pow(10.0, 7.5 * temperatureC / (237.3 + temperatureC))); // Hectopascal = Millibar
        const double specificHumidity = 0.62197058 * Atm.relativeHumidity() * pSat / (Atm.pressure(AltitudeMsl) - Atm.relativeHumidity() * pSat);
        const double h = -19.0 * (specificHumidity - 0.00634);

        // EmissionIndexes at Altitude
        const double altEmissionIndexHC = refEmissionIndexHC * temperatureRatioPower / pressureRatioPower;
        const double altEmissionIndexCO = refEmissionIndexCO * temperatureRatioPower / pressureRatioPower;
        const double altEmissionIndexNOx = refEmissionIndexNOx * std::exp(h) * std::sqrt(pressureRatioPower / temperatureRatioPower);

        return { altEmissionIndexHC, altEmissionIndexCO, altEmissionIndexNOx };
    }

    TEST_CASE("BFFM2 Paper") {
        Atmosphere atm;
        atm.setRelativeHumidity(0.6);

        LTOEngine lto("Trent892");
        lto.FuelFlows = { 0.3, 1.0, 3.1, 3.91 };
        lto.EmissionIndexesHC = { 0.0007, 0.000001, 0.0000001, 0.00001 };
        lto.EmissionIndexesCO = { 0.01307, 0.00057, 0.0002, 0.00028 };
        lto.EmissionIndexesNOx = { 0.00533, 0.01158, 0.0333, 0.0457 };

        BFFM2EmissionsGenerator ltoGen(lto);
        auto [hcEI, coEI, noxEI] = ltoGen.emissionIndexes(0.882, fromFeet(39000.0), 0.84 * soundSpeed(fromFeet(39000.0), atm), atm);

        // 1% error permitted as intermediate values in the paper are rounded to 2nd or 3rd decimal
        CHECK_EQ(toGramsPerKilogram(coEI), doctest::Approx(0.5).epsilon(0.01));
        CHECK_EQ(toGramsPerKilogram(noxEI), doctest::Approx(15.19).epsilon(0.01));
    }
}
