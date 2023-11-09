// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "PerformanceCalculator.h"

#include "Operation/Track4d.h"

namespace GRAPE {
    /**
    * @brief Base class for calculating the PerformanceOutput for Track4d Operation.
    */
    class PerformanceCalculatorTrack4dEmpty : public PerformanceCalculator {
    public:
        explicit PerformanceCalculatorTrack4dEmpty(const PerformanceSpecification& Spec) : PerformanceCalculator(Spec) {}
        virtual ~PerformanceCalculatorTrack4dEmpty() = default;

        /**
        * Default implementation returns empty output.
        */
        [[nodiscard]] virtual std::optional<PerformanceOutput> calculate(const Track4dArrival& Op) const { return PerformanceOutput(); }

        /**
        * Default implementation returns empty output.
        */
        [[nodiscard]] virtual std::optional<PerformanceOutput> calculate(const Track4dDeparture& Track4dDep) const { return PerformanceOutput(); }
    };

    /**
    * @brief Track 4D operations performance calculator.
    */
    class PerformanceCalculatorTrack4d : public PerformanceCalculatorTrack4dEmpty {
    public:
        explicit PerformanceCalculatorTrack4d(const PerformanceSpecification& Spec) : PerformanceCalculatorTrack4dEmpty(Spec) {}
        virtual ~PerformanceCalculatorTrack4d() = default;

        /**
        * @brief Transforms a Track4dArrival into a PerformanceOutput.
        */
        [[nodiscard]] std::optional<PerformanceOutput> calculate(const Track4dArrival& Op) const override;

        /**
        * @brief Transforms a Track4dDeparture into a PerformanceOutput.
        */
        [[nodiscard]] std::optional<PerformanceOutput> calculate(const Track4dDeparture& Track4dDep) const override;
    };
}
