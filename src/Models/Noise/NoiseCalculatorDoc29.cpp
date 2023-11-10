// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "NoiseCalculatorDoc29.h"

#include "Aircraft/Doc29/Doc29Noise.h"

#include <execution>

namespace GRAPE {
    namespace {
        struct SegmentData {
            SegmentData(double LengthIn, double AngleIn) : Length(LengthIn), Angle(AngleIn) {}
            double Length; // Greater or equal to 0
            double Angle; // Between -90 and 90
        };

        std::vector<SegmentData> constantSegmentData(const PerformanceOutput& PerfOutput) {
            std::vector<SegmentData> outSegData;
            outSegData.reserve(PerfOutput.size() - 1);

            for (auto it = std::next(PerfOutput.begin(), 1); it != PerfOutput.end(); ++it)
            {
                auto& [CumulativeGroundDistance1, Point1] = *std::prev(it, 1);
                auto& [CumulativeGroundDistance2, Point2] = *it;
                const double groundLength = CumulativeGroundDistance2 - CumulativeGroundDistance1;
                const double verticalLength = Point2.AltitudeMsl - Point1.AltitudeMsl;
                outSegData.emplace_back(std::hypot(groundLength, verticalLength), std::atan(verticalLength / groundLength));
            }

            return outSegData;
        }
    }

    NoiseCalculatorDoc29::NoiseCalculatorDoc29(const PerformanceSpecification& PerfSpec, const NoiseSpecification& NsSpec, const ReceptorOutput& ReceptOutput) : NoiseCalculator(PerfSpec, NsSpec, ReceptOutput) {}

    NoiseSingleEventOutput NoiseCalculatorDoc29::calculateArrivalNoise(const OperationArrival& Op, const PerformanceOutput& PerfOutput) {
        GRAPE_ASSERT(m_ArrivalGenerators.contains(Op.aircraft().Doc29Ns));

        auto& arrGen = m_ArrivalGenerators(Op.aircraft().Doc29Ns);
        arrGen.applyAtmosphericAbsorption(atmosphericAbsorption(Op));
        const auto& atm = atmosphere(Op);

        NoiseSingleEventOutput outNoise;
        outNoise.fill(m_ReceptorOutput.size());

        // Constant segment data
        const auto segData = constantSegmentData(PerfOutput);

        // Iterate through receptors
        std::for_each(std::execution::par, m_ReceptorOutput.begin(), m_ReceptorOutput.end(), [this, &atm, &PerfOutput, &arrGen, &segData, &Op, &outNoise](const ReceptorIndexed& ReceptIndexed) {
            std::size_t index = ReceptIndexed.Index;
            const Receptor& recept = ReceptIndexed.Recept;

            double laMax = 0.0;
            double sel = 0.0;

            // Impedance corrections
            double corrImpedance = 10 * std::log10(416.86 / 409.81 * atm.pressureRatio(recept.Elevation) / std::sqrt(atm.temperatureRatio(recept.Elevation)));

            // Iterate through flight path
            auto& [unused1, pInit] = *PerfOutput.begin();
            std::reference_wrapper p1 = pInit;

            for (auto it = std::next(PerfOutput.begin(), 1); it != PerfOutput.end(); ++it)
            {
                auto& [unused2, p2] = *it;
                const std::size_t segIndex = std::distance(PerfOutput.begin(), it) - 1;

                // Performance output dependent correction factors
                auto [laMaxSeg, selSeg] = arrGen.calculateArrivalNoise(segData.at(segIndex).Length, segData.at(segIndex).Angle, Op.aircraft().Doc29NoiseDeltaArrivals, p1, p2, recept, m_Cs, atm);

                // Receptor dependent correction factors
                laMaxSeg += corrImpedance;
                selSeg += corrImpedance;

                // Update operation noise
                laMax = std::max(laMax, laMaxSeg);
                sel = sel + std::pow(10.0, selSeg / 10.0);

                // Prepare next segment
                p1 = p2;
            }

            sel = 10.0 * std::log10(sel);

            GRAPE_ASSERT(!std::isnan(laMax));
            GRAPE_ASSERT(!std::isnan(sel));

            outNoise.setValues(index, laMax, sel);
            }
        );

        return outNoise;
    }

    NoiseSingleEventOutput NoiseCalculatorDoc29::calculateDepartureNoise(const OperationDeparture& Op, const PerformanceOutput& PerfOutput) {
        GRAPE_ASSERT(m_DepartureGenerators.contains(Op.aircraft().Doc29Ns));

        auto& depGen = m_DepartureGenerators(Op.aircraft().Doc29Ns);
        depGen.applyAtmosphericAbsorption(atmosphericAbsorption(Op));
        const auto& atm = atmosphere(Op);

        NoiseSingleEventOutput outNoise;
        outNoise.fill(m_ReceptorOutput.size());

        // Constant segment data
        const auto segData = constantSegmentData(PerfOutput);

        // Iterate through receptors
        std::for_each(std::execution::par, m_ReceptorOutput.begin(), m_ReceptorOutput.end(), [this, &atm, &PerfOutput, &depGen, &segData, &Op, &outNoise](const ReceptorIndexed& ReceptIndexed) {
            std::size_t index = ReceptIndexed.Index;
            const Receptor& recept = ReceptIndexed.Recept;

            double laMax = 0.0;
            double sel = 0.0;

            // Impedance correction
            double corrImpedance = 10 * std::log10(416.86 / 409.81 * atm.pressureRatio(recept.Elevation) / std::sqrt(atm.temperatureRatio(recept.Elevation)));

            // Iterate through flight path
            auto& [pnused1, pInit] = *PerfOutput.begin();
            std::reference_wrapper p1 = pInit;

            for (auto it = std::next(PerfOutput.begin(), 1); it != PerfOutput.end(); ++it)
            {
                auto& [unused, p2] = *it;
                const std::size_t segIndex = std::distance(PerfOutput.begin(), it) - 1;

                // Performance output dependent correction factors
                auto [laMaxSeg, selSeg] = depGen.calculateDepartureNoise(segData.at(segIndex).Length, segData.at(segIndex).Angle, Op.aircraft().Doc29NoiseDeltaDepartures, p1, p2, recept, m_Cs, atm);

                // Receptor dependent correction factors
                laMaxSeg += corrImpedance;
                selSeg += corrImpedance;

                // Update operation noise
                laMax = std::max(laMax, laMaxSeg);
                sel = sel + std::pow(10.0, selSeg / 10.0);

                // Prepare next segment
                p1 = p2;
            }

            sel = 10.0 * std::log10(sel);

            GRAPE_ASSERT(!std::isnan(laMax));
            GRAPE_ASSERT(!std::isnan(sel));

            outNoise.setValues(index, laMax, sel);
            }
        );

        return outNoise;
    }

    void NoiseCalculatorDoc29::addDoc29NoiseArrival(const Doc29Noise* Doc29Ns) {
        if (m_ArrivalGenerators.contains(Doc29Ns))
            return;

        m_ArrivalGenerators.add(Doc29Ns, *Doc29Ns); // Forwards Doc29Noise to Doc29NoiseGeneratorArrival constructor
    }

    void NoiseCalculatorDoc29::addDoc29NoiseDeparture(const Doc29Noise* Doc29Ns) {
        if (m_DepartureGenerators.contains(Doc29Ns))
            return;

        m_DepartureGenerators.add(Doc29Ns, *Doc29Ns); // Forwards Doc29Noise to Doc29NoiseGeneratorArrival constructor
    }
}
