// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Operation/Operation.h"
#include "Performance/PerformanceOutput.h"
#include "Performance/PerformanceSpecification.h"

namespace GRAPE {
    /**
    * @brief Base class for calculating the fuel flow at each performance output point of an operation.
    */
    class FuelFlowCalculator {
    public:
        explicit FuelFlowCalculator(const PerformanceSpecification& PerfSpec) : m_Spec(PerfSpec) {}
        virtual ~FuelFlowCalculator() = default;

        /**
        * @brief Base implementation. Sets every point fuel flow to 0.
        */
        virtual void calculate(const OperationArrival& Op, PerformanceOutput& Perf) const;

        /**
        * @brief Base implementation. Sets every point fuel flow to 0.
        */
        virtual void calculate(const OperationDeparture& Op, PerformanceOutput& Perf) const;

        /**
        * @brief Base implementation. Subclasses may implement and use the data of LTOEngines as necessary.
        */
        virtual void addLTOEngine(const LTOEngine*) {}
    protected:
        const PerformanceSpecification& m_Spec;
    protected:
        const Atmosphere& atmosphere(const Operation& Op) const;
    };
}
