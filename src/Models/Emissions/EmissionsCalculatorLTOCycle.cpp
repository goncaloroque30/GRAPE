#include "EmissionsCalculatorLTOCycle.h"

#include "Aircraft/FuelEmissions/LTO.h"
#include "Base/Math.h"

namespace GRAPE {
    namespace {
        EmissionsOperationOutput calculateEmissionsLTOCycle(double Multiplier, const LTOEngine& LTOEng, const std::array<double, 4>& LTOCycle) {
            EmissionsOperationOutput out;

            for (std::size_t i = 0; i < LTOPhases.size(); ++i)
            {
                const double time = LTOCycle.at(i);

                EmissionsSegmentOutput segOut;
                segOut.Index = i;

                segOut.Fuel = time * LTOEng.FuelFlows.at(i) * Multiplier;
                segOut.Emissions.HC = LTOEng.EmissionIndexesHC.at(i) * segOut.Fuel;
                segOut.Emissions.CO = LTOEng.EmissionIndexesCO.at(i) * segOut.Fuel;
                segOut.Emissions.NOx = LTOEng.EmissionIndexesNOx.at(i) * segOut.Fuel;
                segOut.Emissions.nvPM = LTOEng.EmissionIndexesNVPM.at(i) * segOut.Fuel;
                segOut.Emissions.nvPMNumber = LTOEng.EmissionIndexesNVPMNumber.at(i) * segOut.Fuel;

                out.addSegmentOutput(segOut);
            }

            return out;
        }
    }

    EmissionsCalculatorLTOCycle::EmissionsCalculatorLTOCycle(const PerformanceSpecification& PerfSpec, const EmissionsSpecification& EmissionsSpec) : EmissionsCalculator(PerfSpec, EmissionsSpec) {}

    EmissionsOperationOutput EmissionsCalculatorLTOCycle::calculateEmissions(const Operation& Op, const PerformanceOutput& PerfOut) const {
        GRAPE_ASSERT(Op.aircraft().LTOEng);
        GRAPE_ASSERT(m_LTOEngines.contains(Op.aircraft().LTOEng));

        const auto& ltoEng = m_LTOEngines.at(Op.aircraft().LTOEng);
        return calculateEmissionsLTOCycle(Op.Count * Op.aircraft().EngineCount, ltoEng, m_EmissionsSpec.LTOCycle);
    }

    TEST_CASE("Emissions LTO Cycle") {
        EmissionsSpecification emiSpec; // Default should be the default LTO Cycle values

        {
            LTOEngine ltoEng("01P02GE186");
            ltoEng.FuelFlows = { { 0.199, 0.650, 1.983, 2.422 } };
            ltoEng.EmissionIndexesHC = { { fromGramsPerKilogram(1.54), fromGramsPerKilogram(0.11), fromGramsPerKilogram(0.05), fromGramsPerKilogram(0.05) } };
            ltoEng.EmissionIndexesCO = { { fromGramsPerKilogram(19.23), fromGramsPerKilogram(2.13), fromGramsPerKilogram(0.04), fromGramsPerKilogram(0.04) } };
            ltoEng.EmissionIndexesNOx = { { fromGramsPerKilogram(4.73), fromGramsPerKilogram(12.47), fromGramsPerKilogram(19.72), fromGramsPerKilogram(24.94) } };

            // Totals
            const auto out = calculateEmissionsLTOCycle(1.0, ltoEng, emiSpec.LTOCycle);
            CHECK_EQ(round(out.totalFuel(), 0), doctest::Approx(830.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(round(toGrams(out.totalEmissions().HC), 0), doctest::Approx(513.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(round(toGrams(out.totalEmissions().CO), 0), doctest::Approx(6317.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(round(toGrams(out.totalEmissions().NOx), 0), doctest::Approx(11113.0).epsilon(Constants::PrecisionTest));

            // Segments
            const auto& segOuts = out.segmentOutput();
            CHECK_EQ(segOuts.size(), 4);

            {
                const auto& segOut = segOuts.at(0);
                CHECK_EQ(round(segOut.Fuel, 0), doctest::Approx(310.0).epsilon(Constants::PrecisionTest));
                CHECK_EQ(round(toGrams(segOut.Emissions.HC), 0), doctest::Approx(478.0).epsilon(Constants::PrecisionTest));
                CHECK_EQ(round(toGrams(segOut.Emissions.CO), 0), doctest::Approx(5970.0).epsilon(Constants::PrecisionTest));
                CHECK_EQ(round(toGrams(segOut.Emissions.NOx), 0), doctest::Approx(1468.0).epsilon(Constants::PrecisionTest));
            }

            {
                const auto& segOut = segOuts.at(1);
                CHECK_EQ(round(segOut.Fuel, 0), doctest::Approx(156.0).epsilon(Constants::PrecisionTest));
                CHECK_EQ(round(toGrams(segOut.Emissions.HC), 0), doctest::Approx(17.0).epsilon(Constants::PrecisionTest));
                CHECK_EQ(round(toGrams(segOut.Emissions.CO), 0), doctest::Approx(332.0).epsilon(Constants::PrecisionTest));
                CHECK_EQ(round(toGrams(segOut.Emissions.NOx), 0), doctest::Approx(1945.0).epsilon(Constants::PrecisionTest));
            }

            {
                const auto& segOut = segOuts.at(2);
                CHECK_EQ(round(segOut.Fuel, 0), doctest::Approx(262.0).epsilon(Constants::PrecisionTest));
                CHECK_EQ(round(toGrams(segOut.Emissions.HC), 0), doctest::Approx(13.0).epsilon(Constants::PrecisionTest));
                CHECK_EQ(round(toGrams(segOut.Emissions.CO), 0), doctest::Approx(10.0).epsilon(Constants::PrecisionTest));
                CHECK_EQ(round(toGrams(segOut.Emissions.NOx), 0), doctest::Approx(5162.0).epsilon(Constants::PrecisionTest));
            }

            {
                const auto& segOut = segOuts.at(3);
                CHECK_EQ(round(segOut.Fuel, 0), doctest::Approx(102.0).epsilon(Constants::PrecisionTest));
                CHECK_EQ(round(toGrams(segOut.Emissions.HC), 0), doctest::Approx(5.0).epsilon(Constants::PrecisionTest));
                CHECK_EQ(round(toGrams(segOut.Emissions.CO), 0), doctest::Approx(4.0).epsilon(Constants::PrecisionTest));
                CHECK_EQ(round(toGrams(segOut.Emissions.NOx), 0), doctest::Approx(2537.0).epsilon(Constants::PrecisionTest));
            }
        }

        {
            LTOEngine ltoEng("01P20CM136");
            ltoEng.FuelFlows = { { 0.095, 0.273, 0.826, 1.014 } };
            ltoEng.EmissionIndexesHC = { { fromGramsPerKilogram(0.51), fromGramsPerKilogram(0.05), fromGramsPerKilogram(0.04), fromGramsPerKilogram(0.05) } };
            ltoEng.EmissionIndexesCO = { { fromGramsPerKilogram(14.4), fromGramsPerKilogram(1.11), fromGramsPerKilogram(0.14), fromGramsPerKilogram(0.17) } };
            ltoEng.EmissionIndexesNOx = { { fromGramsPerKilogram(4.59), fromGramsPerKilogram(11.59), fromGramsPerKilogram(23.26), fromGramsPerKilogram(55.26) } };

            // Totals
            const auto out = calculateEmissionsLTOCycle(1.0, ltoEng, emiSpec.LTOCycle);
            CHECK_EQ(round(out.totalFuel(), 0), doctest::Approx(365.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(round(toGrams(out.totalEmissions().HC), 0), doctest::Approx(85.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(round(toGrams(out.totalEmissions().CO), 0), doctest::Approx(2229.0).epsilon(Constants::PrecisionTest)); // Wrong in EEDB
            CHECK_EQ(round(toGrams(out.totalEmissions().NOx), 0), doctest::Approx(6329.0).epsilon(Constants::PrecisionTest)); // Wrong in EEDB
        }
    }
}
