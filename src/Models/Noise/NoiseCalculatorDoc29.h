// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Aircraft/Doc29/Doc29NoiseGenerator.h"
#include "Performance/PerformanceSpecification.h"
#include "NoiseCalculator.h"

namespace GRAPE {
    class NoiseCalculatorDoc29 : public NoiseCalculator {
    public:
        NoiseCalculatorDoc29(const PerformanceSpecification& PerfSpec, const NoiseSpecification& NsSpec, const ReceptorOutput& ReceptOutput);

        [[nodiscard]] NoiseSingleEventOutput calculateArrivalNoise(const OperationArrival& Op, const PerformanceOutput& PerfOutput);
        [[nodiscard]] NoiseSingleEventOutput calculateDepartureNoise(const OperationDeparture& Op, const PerformanceOutput& PerfOutput);

        void addDoc29NoiseArrival(const Doc29Noise* Doc29Ns);
        void addDoc29NoiseDeparture(const Doc29Noise* Doc29Ns);
    private:
        GrapeMap<const Doc29Noise*, Doc29NoiseGeneratorArrival> m_ArrivalGenerators;
        GrapeMap<const Doc29Noise*, Doc29NoiseGeneratorDeparture> m_DepartureGenerators;
    };

}
