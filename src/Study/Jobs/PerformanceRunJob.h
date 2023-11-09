// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Job.h"

#include "Airport/Airport.h"
#include "Performance/PerformanceCalculatorFlight.h"
#include "Performance/PerformanceCalculatorTrack4d.h"

namespace GRAPE {
    class OperationsManager;
    class PerformanceRun;

    class RouteOutputGenerator {
    public:
        // Constructors & Destructor (Copy and Move implicitly deleted)
        explicit RouteOutputGenerator(const CoordinateSystem& Cs);
        ~RouteOutputGenerator() = default;

        const RouteOutput& getRouteOutput(const Route* Rte);
    private:
        const CoordinateSystem& m_Cs;
        GrapeMap<const Route*, const RouteOutput> m_RouteOutputs;

        mutable std::mutex m_Mutex;
    };

    class PerformanceRunJob : public Job {
    public:
        // Constructors & Destructor
        PerformanceRunJob(OperationsManager& Operations, PerformanceRun& PerfRun, std::size_t ThreadCount);
        PerformanceRunJob(const PerformanceRunJob&) = delete;
        PerformanceRunJob(PerformanceRunJob&&) = delete;
        PerformanceRunJob& operator=(const PerformanceRunJob&) = delete;
        PerformanceRunJob& operator=(PerformanceRunJob&&) = delete;
        virtual ~PerformanceRunJob() override = default;

        bool queue() override; // Main thread
        void run() override; // Job thread
        void stop() override; // Main thread
        void reset() override; // Main thread

        // Main thread
        float progress() const override { return static_cast<float>(m_CalculatedCount) / static_cast<float>(m_TotalCount); }
    private:
        OperationsManager& m_Operations;

        PerformanceRun& m_PerfRun;

        std::unique_ptr<PerformanceCalculatorFlight> m_FlightsCalculator = nullptr;
        std::unique_ptr<PerformanceCalculatorTrack4dEmpty> m_Tracks4dCalculator = nullptr;
        std::unique_ptr<RouteOutputGenerator> m_RouteOutputs = nullptr;

        std::size_t m_TotalCount = 0;
        std::atomic_size_t m_CalculatedCount = 0;

        std::size_t m_ThreadCount;
        std::vector<std::unique_ptr<JobThread>> m_JobThreads{};

        MtQueue m_Tasks;
    };
}
