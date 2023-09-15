// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "EmissionsRunOutput.h"

#include "Emissions/EmissionsSpecification.h"

namespace GRAPE {
    class Constraints;
    class Database;
    class PerformanceRun;
    class Scenario;

    class EmissionsRun {
    public:
        EmissionsRun(PerformanceRun& PerfRun, std::string_view NameIn);

        std::string Name;
        EmissionsSpecification EmissionsRunSpec;

        // Access Data
        [[nodiscard]] PerformanceRun& parentPerformanceRun() const;
        [[nodiscard]] Scenario& parentScenario() const;

        // Status Checks
        [[nodiscard]] bool valid() const;

        // Job
        friend class EmissionsRunJob;
        [[nodiscard]] const auto& job() const { return m_Job; }
        const std::shared_ptr<EmissionsRunJob>& createJob(const Database& Db, Constraints& Blocks);

        // Output
        [[nodiscard]] auto& output() const { return *m_EmissionsRunOutput; }
    private:
        std::reference_wrapper<PerformanceRun> m_PerformanceRun;

        std::shared_ptr<EmissionsRunJob> m_Job;
        std::unique_ptr<EmissionsRunOutput> m_EmissionsRunOutput;
    };
}
