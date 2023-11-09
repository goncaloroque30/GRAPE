// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "AtmosphericAbsorption.h"
#include "NoiseSingleEventOutput.h"
#include "NoiseSpecification.h"
#include "Operation/Operation.h"
#include "Performance/PerformanceOutput.h"
#include "Performance/PerformanceSpecification.h"
#include "ReceptorOutput.h"

namespace GRAPE {
    /**
    * @brief Base class for calculating the noise single event output from a PerformanceOutput. It references a PerformanceSpecification, NoiseSpecification and a ReceptorOutput.
    */
    class NoiseCalculator {
    public:
        NoiseCalculator(const PerformanceSpecification& PerfSpec, const NoiseSpecification& NsSpec, const ReceptorOutput& ReceptOutput);
        virtual ~NoiseCalculator() = default;

        [[nodiscard]] virtual NoiseSingleEventOutput calculateArrivalNoise(const OperationArrival& Op, const PerformanceOutput& PerfOutput) = 0;
        [[nodiscard]] virtual NoiseSingleEventOutput calculateDepartureNoise(const OperationDeparture& Op, const PerformanceOutput& PerfOutput) = 0;
    protected:
        const PerformanceSpecification& m_PerfSpec;
        const NoiseSpecification& m_NsSpec;
        const CoordinateSystem& m_Cs;

        struct ReceptorIndexed {
            ReceptorIndexed(const Receptor& ReceptIn, std::size_t IndexIn) : Recept(ReceptIn), Index(IndexIn) {}
            const Receptor& Recept;
            std::size_t Index;
        };
        std::vector<ReceptorIndexed> m_ReceptorOutput;
    protected:
        const Atmosphere& atmosphere(const Operation& Op) const;
        AtmosphericAbsorption atmosphericAbsorption(const Operation& Op)const;
    };
}
