// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "NoiseRunOutput.h"

#include "Noise/NoiseSpecification.h"

namespace GRAPE {
    class Constraints;
    class Database;
    class NoiseRun;
    class PerformanceRun;
    class Scenario;

    class NoiseCumulativeMetric {
    public:
        enum class StandardCumulativeMetric {
            Leq = 0,
            Leqd,
            Leqn,
            Ldn,
            Lden,
        };
        static constexpr EnumStrings<StandardCumulativeMetric> StandardCumulativeMetrics{ "Leq", "Leq,d", "Leq,n", "Ldn", "Lden" };

        // Constructors & Destructor
        explicit NoiseCumulativeMetric(NoiseRun& NsRun, std::string_view NameIn);
        NoiseCumulativeMetric(const NoiseCumulativeMetric&) = delete;
        NoiseCumulativeMetric(NoiseCumulativeMetric&&) = delete;
        NoiseCumulativeMetric& operator=(const NoiseCumulativeMetric&) = delete;
        NoiseCumulativeMetric& operator=(NoiseCumulativeMetric&&) = delete;
        ~NoiseCumulativeMetric() = default;

        // Data
        std::string Name;
        double Threshold = 0.0;
        double AveragingTimeConstant = 0.0;

        TimePoint StartTimePoint = std::chrono::round<Duration>(std::chrono::tai_clock::now());
        TimePoint EndTimePoint = std::chrono::round<Duration>(std::chrono::tai_clock::now());

        // Access Data
        [[nodiscard]] NoiseRun& parentNoiseRun() const;
        [[nodiscard]] PerformanceRun& parentPerformanceRun() const;
        [[nodiscard]] Scenario& parentScenario() const;

        [[nodiscard]] auto& weights() { return m_TimeOfDayWeights; }
        [[nodiscard]] const auto& weights() const { return m_TimeOfDayWeights; }
        [[nodiscard]] auto baseWeight() const { return m_TimeOfDayWeights.begin(); }
        [[nodiscard]] double weight(const Duration& TimeOfDay) const;

        [[nodiscard]] auto& numberAboveThresholds() { return m_NumberAboveThresholds; }
        [[nodiscard]] const auto& numberAboveThresholds() const { return m_NumberAboveThresholds; }

        // Change Data
        void setThreshold(double ThresholdIn);
        void setAveragingTimeConstant(double AveragingTimeConstantIn);
        void setStartTimePoint(const std::string& UtcTimeStr);
        void setEndTimePoint(const std::string& UtcTimeStr);
        void setTimeSpanToScenarioSpan();
        void setAveragingTimeConstantToTimeSpan();

        void setStandard(StandardCumulativeMetric Metric);
        void setBaseWeight(double Weight);
        void addWeight(const Duration& TimeOfDay, double Weight);
        void addWeightE(const Duration& TimeOfDay, double Weight);
        void eraseWeight(const Duration& TimeOfDay);
        void clearWeights();
        void updateTime(const Duration& OldTimeOfDay, const Duration& NewTimeOfDay);

        void addNumberAboveThreshold(double NaThreshold);
        void addNumberAboveThresholdE(double NaThreshold);
        void eraseNumberAboveThreshold(double NaThreshold);
        void clearNumberAboveThresholds();
    private:
        std::reference_wrapper<NoiseRun> m_NoiseRun;

        std::map<Duration, double> m_TimeOfDayWeights{};
        std::vector<double> m_NumberAboveThresholds;
    };

    class NoiseRun {
    public:
        NoiseRun(PerformanceRun& PerfRun, std::string_view NameIn);

        std::string Name;
        NoiseSpecification NsRunSpec;
        GrapeMap<std::string, NoiseCumulativeMetric> CumulativeMetrics;

        // Access Data
        [[nodiscard]] PerformanceRun& parentPerformanceRun() const;
        [[nodiscard]] Scenario& parentScenario() const;

        // Status Checks
        [[nodiscard]] bool valid() const;
        [[nodiscard]] bool skipOperation(const Operation& Op) const;

        // Job
        friend class NoiseRunJob;
        [[nodiscard]] const auto& job() const { return m_Job; }
        const std::shared_ptr<NoiseRunJob>& createJob(const Database& Db, Constraints& Blocks);

        // Output
        [[nodiscard]] auto& output() const { return *m_NoiseRunOutput; }
    private:
        std::reference_wrapper<PerformanceRun> m_PerformanceRun;

        std::shared_ptr<NoiseRunJob> m_Job;
        std::unique_ptr<NoiseRunOutput> m_NoiseRunOutput{};
    };
}
