// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Database/Database.h"
#include "Emissions/EmissionsOutput.h"
#include "Operation/Operation.h"

namespace GRAPE {
    class EmissionsRun;
    class PerformanceRun;
    class Scenario;

    class EmissionsRunOutput {
    public:
        explicit EmissionsRunOutput(const EmissionsRun& EmissionsRn, const Database& Db);

        // Access Data (Not Thread Safe)
        [[nodiscard]] auto begin() const { return m_OperationOutputs.begin(); }
        [[nodiscard]] auto end() const { return m_OperationOutputs.end(); }

        [[nodiscard]] const EmissionsRun& parentEmissionsRun() const;
        [[nodiscard]] const PerformanceRun& parentPerformanceRun() const;
        [[nodiscard]] const Scenario& parentScenario() const;

        [[nodiscard]] double totalFuel() const { return m_TotalFuel; }
        [[nodiscard]] const EmissionValues& totalEmissions() const { return m_TotalEmissions; }
        [[nodiscard]] const EmissionsOperationOutput& operationOutput(const Operation& Op) const;
        [[nodiscard]] const EmissionsOperationOutput operationOutputWithSegments(const Operation& Op) const;

        // Status Checks (Not thread Safe)
        [[nodiscard]] std::size_t size() const { return m_OperationOutputs.size(); }
        [[nodiscard]] bool empty() const { return m_OperationOutputs.empty(); }

        // Change Data (Thread Safe)
        void createOutput() const;
        void addOperationOutput(const Operation& Op, const EmissionsOperationOutput& EmissionsOpOut, bool SaveSegments = false);
        void clear();

        friend class ScenariosManager;
    private:
        // EmissionsRunOutput belongs to EmissionsRun and can't be reassigned
        // Its lifetime is coupled to a EmissionsRun
        const EmissionsRun& m_EmissionsRun;

        // Output
        double m_TotalFuel = 0.0;
        EmissionValues m_TotalEmissions;
        GrapeMap<const Operation*, EmissionsOperationOutput> m_OperationOutputs;

        Database m_Db;
        mutable std::mutex m_Mutex;
    private:
        void saveOperation(const Operation& Op, const EmissionsOperationOutput& EmissionsOpOut) const;
        void saveSegments(const Operation& Op, const EmissionsOperationOutput& EmissionsOpOut) const;

        const EmissionsOperationOutput loadSegments(const Operation& Op) const;
    };
}
