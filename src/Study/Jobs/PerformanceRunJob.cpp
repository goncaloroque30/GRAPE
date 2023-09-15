// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "PerformanceRunJob.h"

#include "Airport/RouteCalculator.h"
#include "Performance/PerformanceCalculatorDoc29.h"
#include "Managers/OperationsManager.h"
#include "Scenario/Scenario.h"

namespace GRAPE {
    const RouteOutput& RouteOutputGenerator::getRouteOutput(const Route& Rte) {
        // Access the container and check if route output was already calculated
        std::unique_lock lck(m_Mutex);
        if (m_RouteOutputs.contains(&Rte))
            return m_RouteOutputs(&Rte);
        lck.unlock();

        // Calculate route output
        RouteCalculator rteCalc(m_Cs);
        RouteOutput rteOut = rteCalc.calculate(Rte);

        // Access the container and add the route output if no concurring thread already did
        lck.lock();
        auto [RteOutput, added] = m_RouteOutputs.add(&Rte, std::move(rteOut));
        return RteOutput;
    }

    PerformanceRunJob::PerformanceRunJob(OperationsManager& Operations, PerformanceRun& PerfRun, std::size_t ThreadCount) : m_Operations(Operations), m_PerfRun(PerfRun), m_ThreadCount(ThreadCount) {
        m_Status.store(Status::Ready);
    }

    bool PerformanceRunJob::queue() {
        if (!m_PerfRun.valid())
            return false;

        m_Operations.constraints().performanceRunBlock(m_PerfRun);

        m_Status.store(Status::Waiting);
        return true;
    }

    void PerformanceRunJob::run() {
        // Initialize Run
        Timer perfRunTimer;
        Log::study()->info("Started performance run '{}' of scenario '{}'.", m_PerfRun.Name, m_PerfRun.parentScenario().Name);
        m_Status.store(Status::Running);

        // Initialize Job Threads
        for (std::size_t i = 0; i < m_ThreadCount; i++)
            m_JobThreads.emplace_back(std::make_unique<JobThread>(m_Tasks));

        // Initialize Run Parameters
        m_TotalCount = m_PerfRun.parentScenario().size();

        m_RouteOutputs = std::make_unique<RouteOutputGenerator>(*m_PerfRun.PerfRunSpec.CoordSys);

        switch (m_PerfRun.PerfRunSpec.FlightsPerformanceMdl)
        {
        case PerformanceModel::Doc29: m_PerformanceCalculator = std::make_unique<PerformanceCalculatorDoc29>(m_PerfRun.PerfRunSpec); break;
        default: GRAPE_ASSERT(false);
        }

        // Prepare LTO fuel flow calculations
        for (auto op : m_PerfRun.parentScenario().FlightArrivals)
            m_PerformanceCalculator->fuelFlowCalculator().addLTOEngine(op.get().aircraft().LTOEng);

        for (auto op : m_PerfRun.parentScenario().FlightDepartures)
            m_PerformanceCalculator->fuelFlowCalculator().addLTOEngine(op.get().aircraft().LTOEng);

        for (auto op : m_PerfRun.parentScenario().Track4dArrivals)
            m_PerformanceCalculator->fuelFlowCalculator().addLTOEngine(op.get().aircraft().LTOEng);

        for (auto op : m_PerfRun.parentScenario().Track4dDepartures)
            m_PerformanceCalculator->fuelFlowCalculator().addLTOEngine(op.get().aircraft().LTOEng);

        // Queue Operations
        const auto& perfRunOutput = m_PerfRun.m_PerfRunOutput;
        for (const auto flightArr : m_PerfRun.parentScenario().FlightArrivals)
        {
            m_Tasks.pushTask([&, flightArr] {
                if (const auto perfOutputOpt = m_PerformanceCalculator->calculate(flightArr, m_RouteOutputs->getRouteOutput(flightArr.get().route())))
                    perfRunOutput->addArrivalOutput(flightArr, perfOutputOpt.value());
                ++m_CalculatedCount;
                });
        }

        for (const auto flightDep : m_PerfRun.parentScenario().FlightDepartures)
        {
            m_Tasks.pushTask([&, flightDep] {
                if (const auto perfOutputOpt = m_PerformanceCalculator->calculate(flightDep, m_RouteOutputs->getRouteOutput(flightDep.get().route())))
                    perfRunOutput->addDepartureOutput(flightDep, perfOutputOpt.value());
                ++m_CalculatedCount;
                });
        }

        for (const auto track4dArr : m_PerfRun.parentScenario().Track4dArrivals)
        {
            m_Tasks.pushTask([&, track4dArr] {
                m_Operations.loadArr(track4dArr);
                if (const auto perfOutputOpt = m_PerformanceCalculator->calculate(track4dArr))
                    perfRunOutput->addArrivalOutput(track4dArr, perfOutputOpt.value());
                m_Operations.unloadArr(track4dArr, true);
                ++m_CalculatedCount;
                });
        }

        for (const auto track4dDep : m_PerfRun.parentScenario().Track4dDepartures)
        {
            m_Tasks.pushTask([&, track4dDep] {
                m_Operations.loadDep(track4dDep);
                if (const auto perfOutputOpt = m_PerformanceCalculator->calculate(track4dDep))
                    perfRunOutput->addDepartureOutput(track4dDep, perfOutputOpt.value());
                m_Operations.unloadDep(track4dDep, true);
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
            Log::study()->info(std::format("Finished performance run '{}' of scenario '{}'. Time elapsed: {:%T}.", m_PerfRun.Name, m_PerfRun.parentScenario().Name, perfRunTimer.ellapsedDuration()));
        }
    }

    void PerformanceRunJob::stop() {
        m_Status.store(Status::Stopped);
        m_Tasks.clear();
    }

    void PerformanceRunJob::reset() {
        GRAPE_ASSERT(m_Status.load() != Status::Running);
        if (m_Status.load() != Status::Ready)
            m_Operations.constraints().performanceRunUnblock(m_PerfRun);

        m_PerfRun.m_PerfRunOutput->clear();

        m_PerformanceCalculator.reset();
        m_RouteOutputs.reset();

        m_TotalCount = 0;
        m_CalculatedCount = 0;

        m_Status.store(Status::Ready);
    }
}
