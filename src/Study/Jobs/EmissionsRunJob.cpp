// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "EmissionsRunJob.h"

#include "Constraints.h"

#include "Aircraft/Aircraft.h"
#include "Emissions/EmissionsCalculatorLTO.h"
#include "Scenario/Scenario.h"

namespace GRAPE {

    EmissionsRunJob::EmissionsRunJob(Constraints& Blocks, EmissionsRun& EmissionsRn, std::size_t ThreadCount) : m_Blocks(Blocks), m_EmissionsRun(EmissionsRn), m_ThreadCount(ThreadCount) {
        m_Status.store(Status::Ready);
    }

    bool EmissionsRunJob::queue() {
        if (!m_EmissionsRun.valid())
            return false;

        // Blocks?

        m_Status.store(Status::Waiting);
        return true;
    }

    void EmissionsRunJob::run() {
        // Initialize Run
        Timer emiRunTimer;
        Log::study()->info("Started emissions run '{}' of performance run '{}' of scenario '{}'.", m_EmissionsRun.Name, m_EmissionsRun.parentPerformanceRun().Name, m_EmissionsRun.parentScenario().Name);
        m_Status.store(Status::Running);

        // Initialize Run Parameters
        switch (m_EmissionsRun.EmissionsRunSpec.EmissionsMdl)
        {
        case EmissionsModel::None: m_EmissionsCalculator = std::make_unique<EmissionsCalculator>(m_EmissionsRun.parentPerformanceRun().PerfRunSpec, m_EmissionsRun.EmissionsRunSpec); break;
        case EmissionsModel::BFFM2: m_EmissionsCalculator = std::make_unique<LTOFuelEmissionsCalculator>(m_EmissionsRun.parentPerformanceRun().PerfRunSpec, m_EmissionsRun.EmissionsRunSpec); break;
        default: GRAPE_ASSERT(false) break;
        }

        const auto& perfRunOutput = m_EmissionsRun.parentPerformanceRun().output();
        m_TotalCount = perfRunOutput.size();

        // Initialize Fuel & Emissions Run Output
        m_EmissionsRun.output().createOutput();

        // Prepare BFFM2 4 points interpolation
        for (auto op : perfRunOutput.arrivalOutputs())
            m_EmissionsCalculator->addLTOEngine(op.get().aircraft().LTOEng);

        for (auto op : perfRunOutput.departureOutputs())
            m_EmissionsCalculator->addLTOEngine(op.get().aircraft().LTOEng);

        // Initialize Job Threads
        for (std::size_t i = 0; i < m_ThreadCount; i++)
            m_JobThreads.emplace_back(std::make_unique<JobThread>(m_Tasks));

        // Queue Operations
        for (const auto opArr : perfRunOutput.arrivalOutputs())
        {
            m_Tasks.pushTask([&, opArr] {
                const auto out = m_EmissionsCalculator->calculateEmissions(opArr, perfRunOutput.arrivalOutput(opArr));
                m_EmissionsRun.output().addOperationOutput(opArr, out, m_EmissionsRun.EmissionsRunSpec.SaveSegmentResults);
                ++m_CalculatedCount;
                });
        }

        for (const auto opDep : perfRunOutput.departureOutputs())
        {
            m_Tasks.pushTask([&, opDep] {
                const auto out = m_EmissionsCalculator->calculateEmissions(opDep, perfRunOutput.departureOutput(opDep));
                m_EmissionsRun.output().addOperationOutput(opDep, out, m_EmissionsRun.EmissionsRunSpec.SaveSegmentResults);
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

        if (m_Status.load() == Status::Running)
        {
            m_Status.store(Status::Finished);
            Log::study()->info(std::format("Finished emissions run '{}' of performance run '{}' of scenario '{}'. Time elapsed: {:%T}.", m_EmissionsRun.Name, m_EmissionsRun.parentPerformanceRun().Name, m_EmissionsRun.parentScenario().Name, emiRunTimer.ellapsedDuration()));

        }
    }

    void EmissionsRunJob::stop() {
        m_Status.store(Status::Stopped);
        m_Tasks.clear();
    }

    void EmissionsRunJob::reset() {
        GRAPE_ASSERT(m_Status.load() != Status::Running);

        m_EmissionsRun.output().clear();

        m_EmissionsCalculator.reset();

        m_TotalCount = 0;
        m_CalculatedCount = 0;

        m_Status.store(Status::Ready);
    }
}
