// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Job.h"

#include "Noise/AtmosphericAbsorption.h"
#include "Noise/NoiseCalculator.h"

namespace GRAPE {
    class Constraints;
    class NoiseRun;

    class NoiseRunJob : public Job {
    public:
        // Constructors & Destructor
        NoiseRunJob(Constraints& Blocks, NoiseRun& NsRun, std::size_t ThreadCount);
        NoiseRunJob(const NoiseRunJob&) = delete;
        NoiseRunJob(NoiseRunJob&&) = delete;
        NoiseRunJob& operator=(const NoiseRunJob&) = delete;
        NoiseRunJob& operator=(NoiseRunJob&&) = delete;
        virtual ~NoiseRunJob() override = default;

        bool queue() override; // Main thread
        void run() override; // Job thread
        void stop() override; // Main thread
        void reset() override; // Main thread

        // Main thread
        float progress() const override { return static_cast<float>(m_CalculatedCount) / static_cast<float>(m_TotalCount); }
    private:
        Constraints& m_Blocks;

        NoiseRun& m_NoiseRun;

        std::unique_ptr<NoiseCalculator> m_NoiseCalculator = nullptr;

        std::size_t m_TotalCount = 0;
        std::atomic_size_t m_CalculatedCount = 0;

        std::size_t m_ThreadCount;
        std::vector<std::unique_ptr<JobThread>> m_JobThreads{};

        MtQueue m_Tasks;
    };
}
