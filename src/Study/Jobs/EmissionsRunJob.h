// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Job.h"

#include "Emissions/EmissionsCalculator.h"

namespace GRAPE {
    class Constraints;
    class EmissionsRun;

    class EmissionsRunJob : public Job {
    public:
        // Constructors & Destructor
        EmissionsRunJob(Constraints& Blocks, EmissionsRun& EmissionsRn, std::size_t ThreadCount);
        EmissionsRunJob(const EmissionsRunJob&) = delete;
        EmissionsRunJob(EmissionsRunJob&&) = delete;
        EmissionsRunJob& operator=(const EmissionsRunJob&) = delete;
        EmissionsRunJob& operator=(EmissionsRunJob&&) = delete;
        virtual ~EmissionsRunJob() override = default;

        bool queue() override; // Main thread
        void run() override; // Job thread
        void stop() override; // Main thread
        void reset() override; // Main thread

        // Main thread
        float progress() const override { return static_cast<float>(m_CalculatedCount) / static_cast<float>(m_TotalCount); }
    private:
        Constraints& m_Blocks;

        EmissionsRun& m_EmissionsRun;

        std::unique_ptr<EmissionsCalculator> m_EmissionsCalculator = nullptr;

        std::size_t m_TotalCount = 0;
        std::atomic_size_t m_CalculatedCount = 0;

        std::size_t m_ThreadCount;
        std::vector<std::unique_ptr<JobThread>> m_JobThreads{};

        MtQueue m_Tasks;
    };
}
