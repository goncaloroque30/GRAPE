// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "NoiseRunJob.h"

#include "Constraints.h"
#include "Aircraft/Aircraft.h"
#include "Noise/NoiseCalculatorDoc29.h"
#include "Noise/ReceptorOutput.h"
#include "Scenario/Scenario.h"

namespace GRAPE {
    NoiseRunJob::NoiseRunJob(Constraints& Blocks, NoiseRun& NsRun, std::size_t ThreadCount) : m_Blocks(Blocks), m_NoiseRun(NsRun), m_ThreadCount(ThreadCount) { m_Status.store(Status::Ready); }

    bool NoiseRunJob::queue() {
        if (!m_NoiseRun.valid())
            return false;

        m_Blocks.noiseRunBlock(m_NoiseRun);

        m_Status.store(Status::Waiting);
        return true;
    }

    void NoiseRunJob::run() {
        // Initialize Run
        Timer nsRunTimer;
        Log::study()->info("Started noise run '{}' of performance run '{}' of scenario '{}'.", m_NoiseRun.Name, m_NoiseRun.parentPerformanceRun().Name, m_NoiseRun.parentScenario().Name);
        m_Status.store(Status::Running);

        // Initialize Run Parameters
        m_NoiseRun.m_NoiseRunOutput->setReceptorOutput(m_NoiseRun.NsRunSpec.ReceptSet->receptorList(*m_NoiseRun.parentPerformanceRun().PerfRunSpec.CoordSys));
        const ReceptorOutput& receptOutput = m_NoiseRun.m_NoiseRunOutput->receptors();
        m_NoiseRun.m_NoiseRunOutput->startCumulative();

        const auto& perfRunOutput = m_NoiseRun.parentPerformanceRun().output();
        m_TotalCount = perfRunOutput.size();

        switch (m_NoiseRun.NsRunSpec.NoiseMdl)
        {
        case NoiseModel::Doc29:
            {
                m_ThreadCount = 1;  // Doc29 runs in parallel inside the calculator

                std::unique_ptr<NoiseCalculatorDoc29> doc29NsCalculator = std::make_unique<NoiseCalculatorDoc29>(m_NoiseRun.parentPerformanceRun().PerfRunSpec, m_NoiseRun.NsRunSpec, receptOutput);

                for (auto opArr : perfRunOutput.arrivalOutputs())
                    doc29NsCalculator->addDoc29NoiseArrival(opArr.get().aircraft().Doc29Ns);
                for (auto opDep : perfRunOutput.departureOutputs())
                    doc29NsCalculator->addDoc29NoiseDeparture(opDep.get().aircraft().Doc29Ns);

                m_NoiseCalculator = std::move(doc29NsCalculator);
                break;
            }
        default: GRAPE_ASSERT(false)
            break;
        }

        // Initialize Job Threads
        for (std::size_t i = 0; i < m_ThreadCount; i++)
            m_JobThreads.emplace_back(std::make_unique<JobThread>(m_Tasks));

        // Queue Operations
        for (const auto opArr : perfRunOutput.arrivalOutputs())
        {
            if (m_NoiseRun.skipOperation(opArr))
                continue;
            m_Tasks.pushTask([&, opArr] {
                const auto noiseRes = m_NoiseCalculator->calculateArrivalNoise(opArr, perfRunOutput.arrivalOutput(opArr));
                if (m_NoiseRun.NsRunSpec.SaveSingleMetrics)
                    m_NoiseRun.m_NoiseRunOutput->addSingleEvent(opArr, noiseRes);
                m_NoiseRun.m_NoiseRunOutput->accumulate(opArr, noiseRes);
                ++m_CalculatedCount;
                });
        }

        for (const auto opDep : perfRunOutput.departureOutputs())
        {
            if (m_NoiseRun.skipOperation(opDep))
                continue;

            m_Tasks.pushTask([&, opDep] {
                const auto noiseRes = m_NoiseCalculator->calculateDepartureNoise(opDep, perfRunOutput.departureOutput(opDep));
                if (m_NoiseRun.NsRunSpec.SaveSingleMetrics)
                    m_NoiseRun.m_NoiseRunOutput->addSingleEvent(opDep, noiseRes);
                m_NoiseRun.m_NoiseRunOutput->accumulate(opDep, noiseRes);
                ++m_CalculatedCount;
                });
        }

        // Run
        if (running())
            for (const auto& jobThread : m_JobThreads)
                jobThread->run();

        // Synchronization
        for (const auto& jobThread : m_JobThreads)
            jobThread->join();
        m_JobThreads.clear();

        m_NoiseCalculator.reset();

        if (m_Status.load() == Status::Running)
        {
            m_NoiseRun.m_NoiseRunOutput->finishCumulative();
            m_Status.store(Status::Finished);
            Log::study()->info(std::format("Finished noise run '{}' of performance run '{}' of scenario '{}'. Time elapsed: {:%T}.", m_NoiseRun.Name, m_NoiseRun.parentPerformanceRun().Name, m_NoiseRun.parentScenario().Name, nsRunTimer.elapsedDuration()));

        }
    }

    void NoiseRunJob::stop() {
        m_Status.store(Status::Stopped);
        m_Tasks.clear();
    }

    void NoiseRunJob::reset() {
        GRAPE_ASSERT(m_Status.load() != Status::Running);
        if (m_Status.load() != Status::Ready)
            m_Blocks.noiseRunUnblock(m_NoiseRun);

        m_NoiseRun.m_NoiseRunOutput->clear();

        m_NoiseCalculator.reset();

        m_TotalCount = 0;
        m_CalculatedCount = 0;

        m_Status.store(Status::Ready);
    }
}
