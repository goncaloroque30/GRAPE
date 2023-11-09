// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Manager.h"

#include "Scenario/Scenario.h"

namespace GRAPE {
    class JobManager;
    class OperationsManager;

    class ScenariosManager : public Manager {
    public:
        ScenariosManager(const Database& Db, Constraints& Blocks, OperationsManager& Ops, JobManager& Jobs);
        auto& scenarios() { return m_Scenarios; }
        auto& operator()() { return m_Scenarios; }
        const Scenario& operator()(const std::string& ScenId) { return m_Scenarios(ScenId); }
        [[nodiscard]] auto begin() const { return std::views::values(m_Scenarios).begin(); }
        [[nodiscard]] auto end() const { return std::views::values(m_Scenarios).end(); }

        std::pair<Scenario&, bool> addScenario(const std::string& Name = "");
        std::pair<PerformanceRun&, bool> addPerformanceRun(Scenario& Scen, const std::string& Name = "") const;
        std::pair<NoiseRun&, bool> addNoiseRun(PerformanceRun& PerfRun, const std::string& Name = "") const;
        std::pair<NoiseCumulativeMetric&, bool> addNoiseCumulativeMetric(NoiseRun& NsRun, const std::string& Name = "") const;
        std::pair<EmissionsRun&, bool> addEmissionsRun(PerformanceRun& PerfRun, const std::string& Name = "") const;

        Scenario& addScenarioE(const std::string& Name);
        PerformanceRun& addPerformanceRunE(Scenario& Scen, const std::string& Name) const;
        NoiseRun& addNoiseRunE(PerformanceRun& PerfRun, const std::string& Name) const;
        NoiseCumulativeMetric& addNoiseCumulativeMetricE(NoiseRun& NsRun, const std::string& Name) const;
        EmissionsRun& addEmissionsRunE(PerformanceRun& PerfRun, const std::string& Name) const;

        void eraseScenarios();
        void eraseOutputs();
        void erase(const Scenario& Scen);
        void erase(const PerformanceRun& PerfRun) const;
        void erase(const NoiseRun& NsRun) const;
        void erase(const NoiseCumulativeMetric& NsCumMetric) const;
        void eraseNoiseCumulativeMetrics(NoiseRun& NsRun) const;
        void erase(const EmissionsRun& EmiRun) const;

        bool updateKey(Scenario& Scen, std::string Id);
        bool updateKey(PerformanceRun& PerfRun, std::string Id) const;
        bool updateKey(NoiseRun& NsRun, std::string Id) const;
        bool updateKey(NoiseCumulativeMetric& NsCumMetric, std::string Id) const;
        bool updateKey(EmissionsRun& EmiRun, std::string Id) const;

        void update(const PerformanceRun& PerfRun) const;
        void update(const NoiseRun& NsRun) const;
        void update(const NoiseCumulativeMetric& NsCumMetric) const;
        void update(const EmissionsRun& EmiRun) const;

        bool addFlightArrival(Scenario& Scen, const FlightArrival& Op) const;
        bool addFlightDeparture(Scenario& Scen, const FlightDeparture& Op) const;
        bool addTrack4dArrival(Scenario& Scen, const Track4dArrival& Op) const;
        bool addTrack4dDeparture(Scenario& Scen, const Track4dDeparture& Op) const;

        void addFlightArrivalE(Scenario& Scen, const std::string& OpName) const;
        void addFlightDepartureE(Scenario& Scen, const std::string& OpName) const;
        void addTrack4dArrivalE(Scenario& Scen, const std::string& OpName) const;
        void addTrack4dDepartureE(Scenario& Scen, const std::string& OpName) const;

        bool eraseFlightArrival(Scenario& Scen, const FlightArrival& Op) const;
        bool eraseFlightDeparture(Scenario& Scen, const FlightDeparture& Op) const;
        bool eraseTrack4dArrival(Scenario& Scen, const Track4dArrival& Op) const;
        bool eraseTrack4dDeparture(Scenario& Scen, const Track4dDeparture& Op) const;
        void eraseFlights(Scenario& Scen) const;
        void eraseTracks4d(Scenario& Scen) const;

        void loadFromFile();

    private:
        OperationsManager& m_Operations;
        JobManager& m_Jobs;

        GrapeMap<std::string, Scenario> m_Scenarios;
    };
}
