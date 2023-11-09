// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Database/Database.h"
#include "Operation/Operations.h"
#include "Performance/PerformanceOutput.h"

namespace GRAPE {
    class PerformanceRun;

    class PerformanceRunOutput {
    public:
        // Constructors & Destructor
        explicit PerformanceRunOutput(const PerformanceRun& PerfRun, const Database& Db);
        PerformanceRunOutput(const PerformanceRunOutput&) = delete;
        PerformanceRunOutput(PerformanceRunOutput&&) = delete;
        PerformanceRunOutput& operator=(const PerformanceRunOutput&) = delete;
        PerformanceRunOutput& operator=(PerformanceRunOutput&&) = delete;
        ~PerformanceRunOutput() = default;

        // Access Data (Not Thread Safe)
        [[nodiscard]] auto arrivalOutputs() const { return m_ArrivalOutputs; }
        [[nodiscard]] auto departureOutputs() const { return m_DepartureOutputs; }

        // Status Checks (Not thread Safe)
        [[nodiscard]] std::size_t size() const { return m_ArrivalOutputs.size() + m_DepartureOutputs.size(); }
        [[nodiscard]] bool empty() const { return m_ArrivalOutputs.empty() && m_DepartureOutputs.empty(); }
        [[nodiscard]] bool containsArrival(const OperationArrival& Op) const;
        [[nodiscard]] bool containsDeparture(const OperationDeparture& Op) const;

        // Access Data (Thread Safe)
        [[nodiscard]] PerformanceOutput output(const Operation& Op) const;
        [[nodiscard]] PerformanceOutput arrivalOutput(const OperationArrival& Op) const;
        [[nodiscard]] PerformanceOutput departureOutput(const OperationDeparture& Op) const;

        // Change Data (Thread Safe)
        void addArrivalOutput(const OperationArrival& Op, const PerformanceOutput& PerfOut);
        void addDepartureOutput(const OperationDeparture& Op, const PerformanceOutput& PerfOut);
        void clear();

        friend class ScenariosManager;
    private:
        // PerformanceRunOutput belongs to PerformanceRun and can't be reassigned
        // Its lifetime is coupled/equal to that of a PerformanceRun
        const PerformanceRun& m_PerfRun;

        // The output values are saved on disk (sqlite) and can be loaded through the access functions
        std::vector<std::reference_wrapper<const OperationArrival>> m_ArrivalOutputs;
        std::vector<std::reference_wrapper<const OperationDeparture>> m_DepartureOutputs;

        Database m_Db;
        mutable std::mutex m_Mutex;
    private:
        PerformanceOutput load(const Operation& Op) const;
        void save(const Operation& Op, const PerformanceOutput& PerfOutput) const;
    };
}
