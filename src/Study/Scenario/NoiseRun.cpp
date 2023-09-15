// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "NoiseRun.h"

#include "Scenario.h"

#include "Aircraft/Aircraft.h"
#include "Aircraft/Doc29/Doc29Noise.h"

namespace GRAPE {
    NoiseCumulativeMetric::NoiseCumulativeMetric(NoiseRun& NsRun, std::string_view NameIn) : Name(NameIn), m_NoiseRun(NsRun) {
        m_TimeOfDayWeights.try_emplace(Duration(0), 1.0); // Single weight for all day with 1.0

        setTimeSpanToScenarioSpan();
    }

    // Definitions in .cpp due to cyclic reference
    NoiseRun& NoiseCumulativeMetric::parentNoiseRun() const { return m_NoiseRun; }

    PerformanceRun& NoiseCumulativeMetric::parentPerformanceRun() const { return parentNoiseRun().parentPerformanceRun(); }

    Scenario& NoiseCumulativeMetric::parentScenario() const { return parentNoiseRun().parentScenario(); }

    double NoiseCumulativeMetric::weight(const Duration& TimeOfDay) const {
        auto it = m_TimeOfDayWeights.upper_bound(TimeOfDay);
        if (it != m_TimeOfDayWeights.begin())
            --it;
        return it->second;
    }

    void NoiseCumulativeMetric::setThreshold(double ThresholdIn) {
        if (!(ThresholdIn >= 0.0))
            throw GrapeException("Threshold must be at least 0 dB.");

        Threshold = ThresholdIn;
    }

    void NoiseCumulativeMetric::setAveragingTimeConstant(double AveragingTimeConstantIn) {
        if (!(AveragingTimeConstantIn >= 0.0))
            throw GrapeException("Averaging time constant must be at least 0 dB.");

        AveragingTimeConstant = AveragingTimeConstantIn;
    }

    void NoiseCumulativeMetric::setStartTimePoint(const std::string& UtcTimeStr) {
        const auto timeOpt = utcStringToTime(UtcTimeStr);
        if (timeOpt)
            StartTimePoint = timeOpt.value();
        else
            throw GrapeException(std::format("Invalid start time '{}'.", UtcTimeStr));
    }

    void NoiseCumulativeMetric::setEndTimePoint(const std::string& UtcTimeStr) {
        const auto timeOpt = utcStringToTime(UtcTimeStr);
        if (timeOpt)
            EndTimePoint = timeOpt.value();
        else
            throw GrapeException(std::format("Invalid end time '{}'.", UtcTimeStr));
    }

    void NoiseCumulativeMetric::setTimeSpanToScenarioSpan() {
        if (!parentScenario().empty())
        {
            auto [startTime, endTime] = parentScenario().timeSpan();
            StartTimePoint = startTime;
            EndTimePoint = endTime;
        }
    }

    void NoiseCumulativeMetric::setAveragingTimeConstantToTimeSpan() {
        const auto timeDiff = Duration(EndTimePoint - StartTimePoint);
        AveragingTimeConstant = timeDiff == Duration(0) ? 0.0 : 10.0 * std::log10(static_cast<double>(timeDiff.count()));
    }

    void NoiseCumulativeMetric::setStandard(StandardCumulativeMetric Metric) {
        clearWeights();

        switch (Metric)
        {
        case StandardCumulativeMetric::Leq: break;
        case StandardCumulativeMetric::Leqd:
            {
                setBaseWeight(0.0);
                addWeight(std::chrono::hours(7), 1.0);
                addWeight(std::chrono::hours(19), 1.0);
                addWeight(std::chrono::hours(23), 0.0);
                break;
            }
        case StandardCumulativeMetric::Leqn:
            {
                setBaseWeight(1.0);
                addWeight(std::chrono::hours(7), 0.0);
                addWeight(std::chrono::hours(19), 0.0);
                addWeight(std::chrono::hours(23), 1.0);
                break;
            }
        case StandardCumulativeMetric::Ldn:
            {
                setBaseWeight(10.0);
                addWeight(std::chrono::hours(7), 1.0);
                addWeight(std::chrono::hours(22), 10.0);
                break;
            }
        case StandardCumulativeMetric::Lden:
            {
                setBaseWeight(10.0);
                addWeight(std::chrono::hours(7), 1.0);
                addWeight(std::chrono::hours(19), 3.162);
                addWeight(std::chrono::hours(23), 10.0);
                break;
            }
        default: GRAPE_ASSERT(false);
        }
    }

    void NoiseCumulativeMetric::setBaseWeight(double Weight) { m_TimeOfDayWeights.at(Duration(0)) = Weight; }

    void NoiseCumulativeMetric::addWeight(const Duration& TimeOfDay, double Weight) { m_TimeOfDayWeights.try_emplace(TimeOfDay, Weight); }

    void NoiseCumulativeMetric::addWeightE(const Duration& TimeOfDay, double Weight) {
        GRAPE_ASSERT(TimeOfDay != Duration(0));

        if (m_TimeOfDayWeights.contains(TimeOfDay))
            throw GrapeException(std::format("Time of day {0:%T} already exists in noise cumulative metric '{1}' of noise run '{2}' of performance run '{3}' of scenario '{4}'.", TimeOfDay, Name, parentNoiseRun().Name, parentPerformanceRun().Name, parentScenario().Name));

        m_TimeOfDayWeights.try_emplace(TimeOfDay, Weight);
    }

    void NoiseCumulativeMetric::eraseWeight(const Duration& TimeOfDay) {
        GRAPE_ASSERT(TimeOfDay != Duration(0));
        GRAPE_ASSERT(m_TimeOfDayWeights.contains(TimeOfDay));
        m_TimeOfDayWeights.erase(TimeOfDay);
    }

    void NoiseCumulativeMetric::clearWeights() { m_TimeOfDayWeights.erase(std::next(m_TimeOfDayWeights.begin()), m_TimeOfDayWeights.end()); }

    void NoiseCumulativeMetric::updateTime(const Duration& OldTimeOfDay, const Duration& NewTimeOfDay) {
        if (m_TimeOfDayWeights.contains(NewTimeOfDay))
        {
            m_TimeOfDayWeights.erase(OldTimeOfDay);
            return;
        }

        if (NewTimeOfDay < std::chrono::seconds(1) || NewTimeOfDay >= std::chrono::hours(24))
            return;

        auto node = m_TimeOfDayWeights.extract(OldTimeOfDay);
        node.key() = NewTimeOfDay;
        m_TimeOfDayWeights.insert(std::move(node));
    }

    void NoiseCumulativeMetric::addNumberAboveThreshold(double NaThreshold) {
        if (std::ranges::find(m_NumberAboveThresholds, NaThreshold) != m_NumberAboveThresholds.end())
            return;

        if (NaThreshold < 0.0)
            return;

        m_NumberAboveThresholds.emplace_back(NaThreshold);
        std::ranges::sort(m_NumberAboveThresholds);
    }

    void NoiseCumulativeMetric::addNumberAboveThresholdE(double NaThreshold) {
        if (std::ranges::find(m_NumberAboveThresholds, NaThreshold) != m_NumberAboveThresholds.end())
            throw GrapeException(std::format("Number above threshold {} already exists in noise cumulative metric '{}' of noise run '{}' of performance run '{}' of scenario '{}'.", Threshold, Name, parentNoiseRun().Name, parentPerformanceRun().Name, parentScenario().Name));

        if (!(NaThreshold >= 0.0))
            throw GrapeException("Number above threshold must be at least 0 dB.");

        m_NumberAboveThresholds.emplace_back(NaThreshold);
        std::ranges::sort(m_NumberAboveThresholds);
    }

    void NoiseCumulativeMetric::eraseNumberAboveThreshold(double NaThreshold) { std::erase(m_NumberAboveThresholds, NaThreshold); }

    void NoiseCumulativeMetric::clearNumberAboveThresholds() { m_NumberAboveThresholds.clear(); }

    NoiseRun::NoiseRun(PerformanceRun& PerfRun, std::string_view NameIn) : Name(NameIn), m_PerformanceRun(PerfRun) {}

    // Definitions in .cpp due to cyclic reference
    PerformanceRun& NoiseRun::parentPerformanceRun() const { return m_PerformanceRun; }

    Scenario& NoiseRun::parentScenario() const { return parentPerformanceRun().parentScenario(); }

    bool NoiseRun::valid() const {
        bool valid = true;

        if (NsRunSpec.ReceptSet->empty())
        {
            Log::dataLogic()->warn("Running noise run '{}' of performance run '{}' of scenario '{}'. Receptor set generates no receptors.", Name, parentPerformanceRun().Name, parentScenario().Name);
            valid = false;
        }

        const std::function log = [&](const std::string& Err) { Log::dataLogic()->error("Running noise run '{}' of performance run '{}' of scenario '{}' with Doc29 noise model. {}", Name, parentPerformanceRun().Name, parentScenario().Name, Err); };
        if (NsRunSpec.NoiseMdl == NoiseModel::Doc29)
        {
            for (auto flight : parentScenario().FlightArrivals)
            {
                auto& acft = flight.get().aircraft();
                if (!acft.Doc29Ns)
                {
                    log(std::format("Arrival flight '{}' with aircraft '{}' has no Doc29 noise entry selected.", flight.get().Name, acft.Name));
                    valid = false;
                }
                else if (!acft.Doc29Ns->valid())
                {
                    log(std::format("Arrival flight '{}' with aircraft '{}' and Doc29 noise entry '{}' has invalid NPD data.", flight.get().Name, acft.Name, acft.Doc29Ns->Name));
                    valid = false;
                }
            }

            for (auto flight : parentScenario().FlightDepartures)
            {
                auto& acft = flight.get().aircraft();
                if (!acft.Doc29Ns)
                {
                    log(std::format("Departure flight '{}' with aircraft '{}' has no Doc29 noise entry selected.", flight.get().Name, flight.get().aircraft().Name));
                    valid = false;
                }
                else if (!acft.Doc29Ns->valid())
                {
                    log(std::format("Departure flight '{}' with aircraft '{}' and Doc29 noise entry '{}' has invalid NPD data.", flight.get().Name, acft.Name, acft.Doc29Ns->Name));
                    valid = false;
                }
            }

            for (auto track4d : parentScenario().Track4dArrivals)
            {
                auto& acft = track4d.get().aircraft();
                if (!acft.Doc29Ns)
                {
                    log(std::format("Arrival track 4D '{}' with aircraft '{}' has no Doc29 noise entry selected.", track4d.get().Name, acft.Name));
                    valid = false;
                }
                else if (!acft.Doc29Ns->valid())
                {
                    log(std::format("Arrival track 4D '{}' with aircraft '{}' and Doc29 noise entry '{}' has invalid NPD data.", track4d.get().Name, acft.Name, acft.Doc29Ns->Name));
                    valid = false;
                }
            }

            for (auto track4d : parentScenario().FlightDepartures)
            {
                auto& acft = track4d.get().aircraft();
                if (!acft.Doc29Ns)
                {
                    log(std::format("Departure track 4D '{}' with aircraft '{}' has no Doc29 noise entry selected.", track4d.get().Name, acft.Name));
                    valid = false;
                }
                else if (!acft.Doc29Ns->valid())
                {
                    log(std::format("Departure track 4D '{}' with aircraft '{}' and Doc29 noise entry '{}' has invalid NPD data.", track4d.get().Name, acft.Name, acft.Doc29Ns->Name));
                    valid = false;
                }
            }
        }
        return valid;
    }

    bool NoiseRun::skipOperation(const Operation& Op) const {
        if (Op.Count < Constants::Precision)
            return true;

        return std::ranges::any_of(CumulativeMetrics | std::views::values,
            [&](const NoiseCumulativeMetric& CumMetric) { return Op.Time >= CumMetric.StartTimePoint && Op.Time <= CumMetric.EndTimePoint && CumMetric.weight(Op.timeOfDay()) < Constants::Precision; });
    }

    const std::shared_ptr<NoiseRunJob>& NoiseRun::createJob(const Database& Db, Constraints& Blocks) {
        m_NoiseRunOutput = std::make_unique<NoiseRunOutput>(*this, Db);

        std::size_t threadCount = static_cast<std::size_t>(std::thread::hardware_concurrency());
        m_Job = std::make_shared<NoiseRunJob>(Blocks, *this, threadCount);

        return m_Job;
    }
}
