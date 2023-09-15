// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Database/Database.h"
#include "Noise/NoiseCumulativeOutput.h"
#include "Noise/NoiseSingleEventOutput.h"
#include "Noise/ReceptorOutput.h"
#include "Operation/Operation.h"

namespace GRAPE {
    class PerformanceRun;
    class NoiseCumulativeMetric;
    class NoiseRun;
    class Scenario;

    class NoiseRunOutput {
    public:
        explicit NoiseRunOutput(const NoiseRun& NsRun, const Database& Db);

        // Access Data (Not Thread Safe)
        [[nodiscard]] const NoiseRun& parentNoiseRun() const;
        [[nodiscard]] const PerformanceRun& parentPerformanceRun() const;
        [[nodiscard]] const Scenario& parentScenario() const;

        [[nodiscard]] const ReceptorOutput& receptors() const { return m_ReceptorOutput; }

        [[nodiscard]] NoiseSingleEventOutput singleEventOutput(const Operation& Op) const;
        [[nodiscard]] NoiseSingleEventOutput singleEventOutput(const OperationArrival& Op) const;
        [[nodiscard]] NoiseSingleEventOutput singleEventOutput(const OperationDeparture& Op) const;

        [[nodiscard]] const auto& cumulativeOutputs() const { return m_CumulativeOutputs; }
        [[nodiscard]] const NoiseCumulativeOutput& cumulativeOutput(const NoiseCumulativeMetric& Metric) const;

        // Change Data (Thread Safe)
        void setReceptorOutput(ReceptorOutput&& ReceptOutput);
        void addSingleEvent(const Operation& Op, const NoiseSingleEventOutput& NsOut) const;
        void startCumulative();
        void accumulate(const Operation& Op, const NoiseSingleEventOutput& NsOut);
        void finishCumulative();
        void clear();

        friend class ScenariosManager;
    private:
        // NoiseRunOutput belongs to NoiseRun and can't be reassigned
        // Its lifetime is coupled to a NoiseRun
        const NoiseRun& m_NoiseRun;

        // Receptor Output
        ReceptorOutput m_ReceptorOutput;

        GrapeMap<const NoiseCumulativeMetric*, NoiseCumulativeOutput> m_CumulativeOutputs;
        mutable std::mutex m_CumOutMutex;

        Database m_Db;
        mutable std::mutex m_DbMutex;
    private:
        NoiseSingleEventOutput load(const Operation& Op) const;

        void saveReceptorOutput() const;
        void saveSingleEvent(const Operation& Op, const NoiseSingleEventOutput& NsOutput) const;
        void saveCumulative() const;
    };
}
