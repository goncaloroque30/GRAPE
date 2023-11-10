// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "EmissionsRun.h"
#include "NoiseRun.h"
#include "PerformanceRunOutput.h"

#include "Performance/PerformanceSpecification.h"

namespace GRAPE {
    class PerformanceRunJob;
    class OperationsManager;
    class Scenario;

    class PerformanceRun {
    public:
        PerformanceRun(Scenario& Scen, std::string_view NameIn);

        // Editable Data
        std::string Name;
        PerformanceSpecification PerfRunSpec;

        GrapeMap<std::string, NoiseRun> NoiseRuns; // Key is noise run name
        GrapeMap<std::string, EmissionsRun> EmissionsRuns; // Key is noise run name

        // Access data
        [[nodiscard]] Scenario& parentScenario() const;

        // Status checks
        [[nodiscard]] bool valid() const;

        // Job
        friend class PerformanceRunJob;
        [[nodiscard]] const auto& job() const { return m_Job; }
        const std::shared_ptr<PerformanceRunJob>& createJob(const Database& Db, OperationsManager& Ops);

        // Output
        [[nodiscard]] auto& output() const { return *m_PerfRunOutput; }
    private:
        std::reference_wrapper<Scenario> m_ParentScenario;

        std::shared_ptr<PerformanceRunJob> m_Job = nullptr;
        std::unique_ptr<PerformanceRunOutput> m_PerfRunOutput = nullptr;
    };
}
