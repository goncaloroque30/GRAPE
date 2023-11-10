// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Panel.h"

#include "Scenario/Scenario.h"

namespace GRAPE {
    class ScenariosPanel : public Panel {
    public:
        // Constructors & Destructor
        ScenariosPanel();

        // Selection
        void select(Scenario& Scen);
        void select(PerformanceRun& PerfRun);
        void select(NoiseRun& NsRun);
        void select(NoiseCumulativeMetric& NsCumMetric);
        void select(EmissionsRun& EmiRun);
        void selectPerformanceOutput(const Operation& Op);
        void selectNoiseSingleEventOutput(const Operation& Op);
        void selectNoiseCumulativeOutput(const NoiseCumulativeMetric& NsCumMetric);
        void selectEmissionsSegmentOutput(const Operation& Op);

        void clearSelection();
        void clearOutputSelection();

        // Status Checks
        [[nodiscard]] bool isSelected(const Scenario& Scen) const;
        [[nodiscard]] bool isSelected(const PerformanceRun& PerfRun) const;
        [[nodiscard]] bool isSelected(const NoiseRun& NsRun) const;
        [[nodiscard]] bool isSelected(const NoiseCumulativeMetric& NsCumMetric) const;
        [[nodiscard]] bool isSelected(const EmissionsRun& EmiRun) const;
        [[nodiscard]] bool isOutputSelected(const Operation& Op) const;
        [[nodiscard]] bool isOutputSelected(const NoiseCumulativeMetric& NsCumMetric) const;

        // Panel Interface
        void reset() override;
        void imGuiDraw() override;
    private:
        Scenario* m_SelectedScenario = nullptr;
        PerformanceRun* m_SelectedPerformanceRun = nullptr;
        NoiseRun* m_SelectedNoiseRun = nullptr;
        NoiseCumulativeMetric* m_SelectedNoiseCumulativeMetric = nullptr;
        EmissionsRun* m_SelectedEmissionsRun = nullptr;

        const Operation* m_SelectedOutputOperation = nullptr;
        std::unique_ptr<const PerformanceOutput> m_SelectedPerformanceOutput;
        std::unique_ptr<const NoiseSingleEventOutput> m_SelectedNoiseSingleEventOutput;
        std::unique_ptr<const EmissionsOperationOutput> m_SelectedEmissionsSegmentOutput;

        const NoiseCumulativeMetric* m_SelectedNoiseCumulativeMetricOutput = nullptr;
        const NoiseCumulativeOutput* m_SelectedNoiseCumulativeOutput = nullptr;

        enum class Selected {
            Scenario = 0,
            PerformanceRun,
            NoiseRun,
            NoiseCumulativeMetric,
            EmissionsRun,
        } m_SelectedType = Selected::Scenario;

        std::function<void()> m_Action;
    private:
        void drawScenarioNode(const std::string& ScenarioId, Scenario& Scen);
        void drawPerformanceRunNode(const std::string& PerfRunId, PerformanceRun& PerfRun);
        void drawNoiseRunNode(const std::string& NoiseRunId, NoiseRun& NsRun);
        void drawEmissionsRunNode(const std::string& EmiRunId, EmissionsRun& EmiRun);

        void drawSelectedScenario();
        void drawSelectedPerformanceRun();
        void drawSelectedNoiseRun();
        void drawSelectedEmissionsRun();
        void drawSelectedPerformanceOutput() const;
        void drawSelectedNoiseSingleEventOutput() const;
        void drawSelectedNoiseCumulativeOutput() const;
        void drawSelectedEmissionsSegmentOutput() const;
    };

}
