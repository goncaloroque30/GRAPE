// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "ScenariosPanel.h"

#include "Application.h"
#include "IO/CsvExport.h"
#include "IO/GpkgExport.h"
#include "UI.h"

namespace GRAPE {
    // Scenarios Panel
    ScenariosPanel::ScenariosPanel() : Panel("Scenarios") {}

    void ScenariosPanel::select(Scenario& Scen) {
        if (isSelected(Scen))
            return;

        clearSelection();
        m_SelectedType = Selected::Scenario;

        m_SelectedScenario = &Scen;
    }

    void ScenariosPanel::select(PerformanceRun& PerfRun) {
        if (isSelected(PerfRun))
            return;

        clearSelection();
        m_SelectedType = Selected::PerformanceRun;

        m_SelectedPerformanceRun = &PerfRun;
    }

    void ScenariosPanel::select(NoiseRun& NsRun) {
        if (isSelected(NsRun))
            return;

        clearSelection();
        m_SelectedType = Selected::NoiseRun;

        m_SelectedNoiseRun = &NsRun;
    }

    void ScenariosPanel::select(NoiseCumulativeMetric& NsCumMetric) {
        if (isSelected(NsCumMetric))
            return;

        m_SelectedNoiseCumulativeMetric = &NsCumMetric;
    }

    void ScenariosPanel::select(EmissionsRun& EmiRun) {
        if (isSelected(EmiRun))
            return;

        clearSelection();
        m_SelectedType = Selected::EmissionsRun;

        m_SelectedEmissionsRun = &EmiRun;
    }

    void ScenariosPanel::selectPerformanceOutput(const Operation& Op) {
        if (isOutputSelected(Op))
            return;

        clearOutputSelection();

        m_SelectedOutputOperation = &Op;
        m_SelectedPerformanceOutput = std::make_unique<const PerformanceOutput>(m_SelectedPerformanceRun->output().output(Op));
    }

    void ScenariosPanel::selectNoiseSingleEventOutput(const Operation& Op) {
        if (isOutputSelected(Op))
            return;

        clearOutputSelection();

        m_SelectedOutputOperation = &Op;
        m_SelectedNoiseSingleEventOutput = std::make_unique<const NoiseSingleEventOutput>(m_SelectedNoiseRun->output().singleEventOutput(Op));
    }

    void ScenariosPanel::selectNoiseCumulativeOutput(const NoiseCumulativeMetric& NsCumMetric) {
        if (isOutputSelected(NsCumMetric))
            return;

        clearOutputSelection();

        m_SelectedNoiseCumulativeMetricOutput = &NsCumMetric;
        m_SelectedNoiseCumulativeOutput = &m_SelectedNoiseRun->output().cumulativeOutput(NsCumMetric);
    }

    void ScenariosPanel::selectEmissionsSegmentOutput(const Operation& Op) {
        if (isOutputSelected(Op))
            return;

        clearOutputSelection();

        m_SelectedOutputOperation = &Op;
        m_SelectedEmissionsSegmentOutput = std::make_unique<const EmissionsOperationOutput>(m_SelectedEmissionsRun->output().operationOutputWithSegments(Op));
    }

    void ScenariosPanel::clearSelection() {
        m_SelectedScenario = nullptr;
        m_SelectedPerformanceRun = nullptr;
        m_SelectedNoiseRun = nullptr;
        m_SelectedNoiseCumulativeMetric = nullptr;
        m_SelectedEmissionsRun = nullptr;

        clearOutputSelection();
    }

    void ScenariosPanel::clearOutputSelection() {
        m_SelectedOutputOperation = nullptr;
        m_SelectedPerformanceOutput = nullptr;

        m_SelectedNoiseSingleEventOutput = nullptr;
        m_SelectedNoiseCumulativeMetricOutput = nullptr;
        m_SelectedNoiseCumulativeOutput = nullptr;

        m_SelectedEmissionsSegmentOutput = nullptr;
    }

    bool ScenariosPanel::isSelected(const Scenario& Scen) const {
        return m_SelectedScenario == &Scen;
    }

    bool ScenariosPanel::isSelected(const PerformanceRun& PerfRun) const {
        return m_SelectedPerformanceRun == &PerfRun;
    }

    bool ScenariosPanel::isSelected(const NoiseRun& NsRun) const {
        return m_SelectedNoiseRun == &NsRun;
    }

    bool ScenariosPanel::isSelected(const NoiseCumulativeMetric& NsCumMetric) const {
        return m_SelectedNoiseRun == &NsCumMetric.parentNoiseRun() && m_SelectedNoiseCumulativeMetric == &NsCumMetric;
    }

    bool ScenariosPanel::isSelected(const EmissionsRun& EmiRun) const {
        return m_SelectedEmissionsRun == &EmiRun;
    }

    bool ScenariosPanel::isOutputSelected(const Operation& Op) const {
        return m_SelectedOutputOperation == &Op;
    }

    bool ScenariosPanel::isOutputSelected(const NoiseCumulativeMetric& NsCumMetric) const {
        return m_SelectedNoiseCumulativeMetricOutput == &NsCumMetric;
    }

    void ScenariosPanel::reset() {
        clearSelection();
    }

    void ScenariosPanel::imGuiDraw() {
        if (!isOpen())
            return;

        auto& study = Application::study();

        ImGui::Begin(m_Name.c_str(), &m_Open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse);

        // Left side
        ImGui::BeginChild("Left Side", ImVec2(-ImGui::GetContentRegionAvail().x * UI::g_StandardLeftAlignFraction * 0.9f, 0.0f));

        // Edit Button
        UI::buttonEditRight();
        if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonLeft))
        {
            if (UI::selectableNew("Scenario"))
                study.Scenarios.addScenario();

            if (UI::selectableDelete("All"))
            {
                clearSelection();
                Application::get().queueAsyncTask([&] { study.Scenarios.eraseScenarios(); }, "Deleting all scenarios");
            }
            ImGui::EndPopup();
        }

        if (UI::beginTable("Scenarios", 3))
        {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Run", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            for (auto& [scenId, scen] : study.Scenarios())
                drawScenarioNode(scenId, scen);

            UI::endTable();
        }
        ImGui::EndChild(); // Left Side

        ImGui::SameLine();

        // Selected Data
        ImGui::BeginChild("Right Side");
        switch (m_SelectedType)
        {
        case Selected::Scenario: if (m_SelectedScenario) drawSelectedScenario(); break;
        case Selected::PerformanceRun: if (m_SelectedPerformanceRun) drawSelectedPerformanceRun(); break;
        case Selected::NoiseRun: if (m_SelectedNoiseRun) drawSelectedNoiseRun(); break;
        case Selected::EmissionsRun: if (m_SelectedEmissionsRun) drawSelectedEmissionsRun(); break;
        default: GRAPE_ASSERT(false);
        }
        ImGui::EndChild();

        // Actions outside loops
        if (m_Action)
        {
            m_Action();
            m_Action = nullptr;
        }

        ImGui::End();
    }

    void ScenariosPanel::drawScenarioNode(const std::string& ScenarioId, Scenario& Scen) {
        auto& study = Application::study();

        ImGui::TableNextRow();
        ImGui::PushID(ScenarioId.c_str());

        UI::tableNextColumn(false);

        // Selectable Row
        if (UI::selectableRowEmpty(isSelected(Scen)))
            select(Scen);

        if (ImGui::BeginPopupContextItem())
        {
            if (UI::selectableNew("Performance Run"))
                study.Scenarios.addPerformanceRun(Scen);

            if (UI::selectableDelete())
                m_Action = [&] {
                clearSelection();
                Application::get().queueAsyncTask([&] { study.Scenarios.erase(Scen); }, std::format("Deleting scenario '{}'", Scen.Name));
                };

            ImGui::EndPopup();
        }

        // Tree Node
        const bool open = UI::treeNodeEmpty(Scen.PerformanceRuns.empty());
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
            select(Scen);

        // Name
        ImGui::BeginDisabled(study.Blocks.notEditable(Scen));
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (UI::inputText("Name", Scen.Name, Scen.Name != ScenarioId && study.Scenarios().contains(Scen.Name), "Scenario name", std::format("Scenario '{}' already exists in this study.", Scen.Name)))
            if (Scen.Name != ScenarioId)
                m_Action = [&] { study.Scenarios.updateKey(Scen, ScenarioId); };
        if (UI::isItemClicked())
            select(Scen);
        ImGui::EndDisabled();

        // Type
        UI::tableNextColumn(false);
        UI::textInfo("Scenario");

        // Run


        if (open)
        {
            for (auto& [ScenRunId, ScenRun] : Scen.PerformanceRuns)
                drawPerformanceRunNode(ScenRunId, ScenRun);

            ImGui::TreePop();
        }

        ImGui::PopID(); // Scenario ID
    }

    void ScenariosPanel::drawPerformanceRunNode(const std::string& PerfRunId, PerformanceRun& PerfRun) {
        auto& study = Application::study();
        auto& perfRunJob = PerfRun.job();

        ImGui::TableNextRow();
        ImGui::PushID(PerfRunId.c_str());

        UI::tableNextColumn(false);

        // Selectable Row
        if (UI::selectableRowEmpty(isSelected(PerfRun)))
            select(PerfRun);

        if (ImGui::BeginPopupContextItem())
        {
            if (UI::selectableNew("Noise Run"))
                study.Scenarios.addNoiseRun(PerfRun);

            if (UI::selectableNew("Emissions Run"))
                study.Scenarios.addEmissionsRun(PerfRun);

            if (!perfRunJob->ready() && UI::selectableWithIcon("Reset Run", ICON_FA_BACKWARD_STEP))
            {
                clearOutputSelection();
                Application::get().queueAsyncTask([&] {
                    for (const auto& nsRun : PerfRun.NoiseRuns | std::views::values)
                        study.Jobs.resetJob(nsRun.job());
                    for (const auto& emiRun : PerfRun.EmissionsRuns | std::views::values)
                        study.Jobs.resetJob(emiRun.job());

                    study.Jobs.resetJob(perfRunJob);
                    }, std::format("Resetting performance run '{}'", PerfRun.Name));
            }

            if (UI::selectableDelete())
                m_Action = [&] {
                clearSelection();
                Application::get().queueAsyncTask([&] { study.Scenarios.erase(PerfRun); }, std::format("Deleting performance run '{}'", PerfRun.Name));
                };
            ImGui::EndPopup();
        }

        // Tree Node
        const bool open = UI::treeNodeEmpty(PerfRun.NoiseRuns.empty());
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
            select(PerfRun);

        // Name
        ImGui::BeginDisabled(!PerfRun.job()->ready());
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (UI::inputText("Performance Run Name", PerfRun.Name, PerfRun.Name != PerfRunId && PerfRun.parentScenario().PerformanceRuns.contains(PerfRun.Name), "Performance run name", std::format("Performance run '{}' already exists in scenario '{}'", PerfRun.Name, PerfRun.parentScenario().Name)))
            if (PerfRun.Name != PerfRunId)
                m_Action = [&] { study.Scenarios.updateKey(PerfRun, PerfRunId); };
        if (UI::isItemClicked())
            select(PerfRun);
        ImGui::EndDisabled();

        // Type
        UI::tableNextColumn(false);
        UI::textInfo("Performance Run");

        // Run
        UI::tableNextColumn();
        if (UI::progressBar(*perfRunJob))
        {
            Application::get().panelStackOnPerformanceRunStart();
            study.Jobs.queueJob(perfRunJob);
        }

        if (open)
        {
            ImGui::PushID("Noise Runs");
            for (auto& [noiseRunId, nsRun] : PerfRun.NoiseRuns)
                drawNoiseRunNode(noiseRunId, nsRun);
            ImGui::PopID();

            ImGui::PushID("Emissions Runs");
            for (auto& [emiRunId, emiRun] : PerfRun.EmissionsRuns)
                drawEmissionsRunNode(emiRunId, emiRun);
            ImGui::PopID();

            ImGui::TreePop();
        }
        ImGui::PopID(); // Performance Run ID
    }

    void ScenariosPanel::drawNoiseRunNode(const std::string& NoiseRunId, NoiseRun& NsRun) {
        auto& study = Application::study();
        auto& nsRunJob = NsRun.job();

        ImGui::TableNextRow();
        ImGui::PushID(NoiseRunId.c_str());

        UI::tableNextColumn(false);

        // Selectable Row
        if (UI::selectableRowEmpty(isSelected(NsRun)))
            select(NsRun);

        if (ImGui::BeginPopupContextItem())
        {
            if (!nsRunJob->ready() && UI::selectableWithIcon("Reset Run", ICON_FA_BACKWARD_STEP))
            {
                clearOutputSelection();
                Application::get().queueAsyncTask([&] { study.Jobs.resetJob(nsRunJob); }, std::format("Resetting noise run '{}'", NsRun.Name));
            }

            if (UI::selectableDelete())
                m_Action = [&] {
                clearSelection();
                Application::get().queueAsyncTask([&] { study.Scenarios.erase(NsRun); }, std::format("Deleting noise run '{}'", NsRun.Name));
                };

            ImGui::EndPopup();
        }

        // Tree Node
        const bool open = UI::treeNodeEmpty(true);
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
            select(NsRun);

        // Name
        ImGui::BeginDisabled(!NsRun.job()->ready());
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (UI::inputText("Noise Run Name", NsRun.Name, NsRun.Name != NoiseRunId && NsRun.parentPerformanceRun().NoiseRuns.contains(NsRun.Name), "Noise run name", std::format("Noise run '{}' already exists in performance run '{}' of scenario '{}'", NsRun.Name, NsRun.parentPerformanceRun().Name, NsRun.parentScenario().Name)))
            if (NsRun.Name != NoiseRunId)
                m_Action = [&] { study.Scenarios.updateKey(NsRun, NoiseRunId); };
        if (UI::isItemClicked())
            select(NsRun);
        ImGui::EndDisabled();

        // Type
        UI::tableNextColumn(false);
        UI::textInfo("Noise Run");

        // Run
        UI::tableNextColumn();
        if (NsRun.parentPerformanceRun().job()->finished())
            if (UI::progressBar(*nsRunJob))
            {
                Application::get().panelStackOnNoiseRunStart();
                study.Jobs.queueJob(nsRunJob);
            }

        if (open)
            ImGui::TreePop();

        ImGui::PopID(); // Noise Run ID
    }

    void ScenariosPanel::drawEmissionsRunNode(const std::string& EmiRunId, EmissionsRun& EmiRun) {
        auto& study = Application::study();
        auto& emiRunJob = EmiRun.job();

        ImGui::TableNextRow();
        ImGui::PushID(EmiRunId.c_str());

        UI::tableNextColumn(false);

        // Selectable Row
        if (UI::selectableRowEmpty(isSelected(EmiRun)))
            select(EmiRun);

        if (ImGui::BeginPopupContextItem())
        {
            if (!emiRunJob->ready() && UI::selectableWithIcon("Reset Run", ICON_FA_BACKWARD_STEP))
            {
                clearSelection();
                Application::get().queueAsyncTask([&] { study.Jobs.resetJob(emiRunJob); }, std::format("Resetting emissions run '{}'", EmiRun.Name));
            }

            if (UI::selectableDelete())
                m_Action = [&] {
                clearSelection();
                Application::get().queueAsyncTask([&] { study.Scenarios.erase(EmiRun); }, std::format("Deleting emissions run '{}'", EmiRun.Name));
                };

            ImGui::EndPopup();
        }

        // Tree Node
        const bool open = UI::treeNodeEmpty(true);
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
            select(EmiRun);

        // Name
        ImGui::BeginDisabled(!EmiRun.job()->ready());
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (UI::inputText("Emissions Run Name", EmiRun.Name, EmiRun.Name != EmiRunId && EmiRun.parentPerformanceRun().EmissionsRuns.contains(EmiRun.Name), "Emissions run name", std::format("Emissions run '{}' already exists in performance run '{}' of scenario '{}'", EmiRun.Name, EmiRun.parentPerformanceRun().Name, EmiRun.parentScenario().Name)))
            if (EmiRun.Name != EmiRunId)
                m_Action = [&] { study.Scenarios.updateKey(EmiRun, EmiRunId); };
        if (UI::isItemClicked())
            select(EmiRun);
        ImGui::EndDisabled();

        // Type
        UI::tableNextColumn(false);
        UI::textInfo("Emissions Run");

        // Run
        UI::tableNextColumn();
        if (EmiRun.parentPerformanceRun().job()->finished())
            if (UI::progressBar(*emiRunJob))
                study.Jobs.queueJob(emiRunJob);

        if (open)
            ImGui::TreePop();

        ImGui::PopID(); // Emissions Run ID
    }

    namespace {
        void operationFlightRow(const Flight& Op);
        void operationTrack4dRow(const Track4d& Op);
    }

    void ScenariosPanel::drawSelectedScenario() {
        GRAPE_ASSERT(m_SelectedScenario);
        auto& scen = *m_SelectedScenario;

        auto& study = Application::study();

        const ImGuiStyle& style = ImGui::GetStyle();

        if (UI::buttonNew("Performance Run"))
            study.Scenarios.addPerformanceRun(scen);

        ImGui::SameLine();

        if (UI::buttonDelete("Scenario"))
            m_Action = [&] { clearSelection(); study.Scenarios.erase(scen); };

        ImGui::Separator();

        // Infos
        if (ImGui::CollapsingHeader("Infos"))
        {
            const float charCount = static_cast<float>(std::to_string(std::max(scen.arrivalsSize(), scen.departuresSize())).length());
            const float offset1 = ImGui::GetCursorPosX() + ImGui::CalcTextSize("Arrivals:").x;
            const float offset2 = offset1 + style.ItemInnerSpacing.x + ImGui::CalcTextSize("1").x * charCount + style.ItemSpacing.x;
            const float offset3 = offset2 + ImGui::CalcTextSize("Tracks 4D:").x;

            ImGui::TextDisabled("Arrivals:");
            ImGui::SameLine(offset1, style.ItemInnerSpacing.x);
            UI::textInfo(std::format("{}", scen.arrivalsSize()));

            ImGui::SameLine(offset2);
            ImGui::TextDisabled("Departures:");
            ImGui::SameLine(offset3, style.ItemInnerSpacing.x);
            UI::textInfo(std::format("{}", scen.departuresSize()));

            ImGui::TextDisabled("Flights:");
            ImGui::SameLine(offset1, style.ItemInnerSpacing.x);
            UI::textInfo(std::format("{}", scen.flightsSize()));

            ImGui::SameLine(offset2);
            ImGui::TextDisabled("Tracks 4D:");
            ImGui::SameLine(offset3, style.ItemInnerSpacing.x);
            UI::textInfo(std::format("{}", scen.tracks4dSize()));

            ImGui::TextDisabled("Total Operations:");
            ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
            UI::textInfo(std::format("{}", scen.size()));
        }

        if (ImGui::CollapsingHeader("Flights"))
        {
            ImGui::BeginDisabled(study.Blocks.notEditable(scen));

            if (UI::beginTable("Flights", 8, ImGuiTableFlags_None, ImVec2(0.0f, UI::getTableHeight(study.Operations.flightsSize()))))
            {
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn("Operation", ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn("Airport", ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn("Runway", ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn("Route", ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn("Aircraft", ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn(std::format("Weight ({})", Application::settings().WeightUnits.shortName()).c_str(), ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn("Doc29 Profile", ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableHeadersRow();

                for (const auto& [arrId, arr] : study.Operations.flightArrivals())
                {
                    ImGui::TableNextRow();
                    ImGui::PushID(&arr);

                    UI::tableNextColumn(false);

                    // Selectable Row
                    const bool containsOp = scen.contains(arr);
                    if (UI::selectableRowEmpty(containsOp))
                    {
                        if (containsOp)
                        {
                            study.Scenarios.eraseFlightArrival(scen, arr);
                        }
                        else if (ImGui::GetIO().KeyCtrl)
                        {
                            study.Scenarios.addFlightArrival(scen, arr);
                        }
                        else
                        {
                            study.Scenarios.eraseFlights(scen);
                            study.Scenarios.addFlightArrival(scen, arr);
                        }
                    }

                    // Name
                    UI::textInfo(arrId);

                    operationFlightRow(arr);

                    // Doc29 Profile
                    UI::tableNextColumn(false);
                    if (arr.hasDoc29Profile())
                        UI::textInfo(arr.Doc29Prof->Name);

                    ImGui::PopID(); // Arrival ID
                }

                for (const auto& [depId, dep] : study.Operations.flightDepartures())
                {
                    ImGui::TableNextRow();
                    ImGui::PushID(&dep);

                    UI::tableNextColumn(false);

                    // Selectable Row
                    const bool containsOp = scen.contains(dep);
                    if (UI::selectableRowEmpty(containsOp))
                    {
                        if (containsOp)
                        {
                            study.Scenarios.eraseFlightDeparture(scen, dep);
                        }
                        else if (ImGui::GetIO().KeyCtrl)
                        {
                            study.Scenarios.addFlightDeparture(scen, dep);
                        }
                        else
                        {
                            study.Scenarios.eraseFlights(scen);
                            study.Scenarios.addFlightDeparture(scen, dep);
                        }
                    }

                    // Name
                    UI::textInfo(depId);

                    operationFlightRow(dep);

                    // Doc29 Profile
                    UI::tableNextColumn(false);
                    if (dep.hasDoc29Profile())
                        UI::textInfo(dep.Doc29Prof->Name);

                    ImGui::PopID(); // Departure ID
                }
                UI::endTable();
            }
            ImGui::EndDisabled(); // Scenario not editable
        }


        if (ImGui::CollapsingHeader("Tracks 4D"))
        {
            ImGui::BeginDisabled(study.Blocks.notEditable(scen));

            if (UI::beginTable("Tracks4D", 3))
            {
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn("Operation", ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn("Aircraft", ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableHeadersRow();

                for (const auto& [ArrId, Arr] : study.Operations.track4dArrivals())
                {
                    ImGui::TableNextRow();
                    ImGui::PushID(&Arr);

                    UI::tableNextColumn(false);

                    // Selectable Row
                    const bool containsOp = scen.contains(Arr);
                    if (UI::selectableRowEmpty(containsOp))
                    {
                        if (containsOp)
                        {
                            study.Scenarios.eraseTrack4dArrival(scen, Arr);
                        }
                        else if (ImGui::GetIO().KeyCtrl)
                        {
                            study.Scenarios.addTrack4dArrival(scen, Arr);
                        }
                        else
                        {
                            study.Scenarios.eraseTracks4d(scen);
                            study.Scenarios.addTrack4dArrival(scen, Arr);
                        }
                    }

                    // Name
                    UI::textInfo(ArrId);

                    operationTrack4dRow(Arr);

                    ImGui::PopID(); // Arrival ID
                }

                for (const auto& [DepId, Dep] : study.Operations.track4dDepartures())
                {
                    ImGui::TableNextRow();
                    ImGui::PushID(&Dep);

                    UI::tableNextColumn(false);

                    // Selectable Row
                    const bool containsOp = scen.contains(Dep);
                    if (UI::selectableRowEmpty(containsOp))
                    {
                        if (containsOp)
                        {
                            study.Scenarios.eraseTrack4dDeparture(scen, Dep);
                        }
                        else if (ImGui::GetIO().KeyCtrl)
                        {
                            study.Scenarios.addTrack4dDeparture(scen, Dep);
                        }
                        else
                        {
                            study.Scenarios.eraseTracks4d(scen);
                            study.Scenarios.addTrack4dDeparture(scen, Dep);
                        }
                    }

                    // Name
                    UI::textInfo(DepId);

                    operationTrack4dRow(Dep);

                    ImGui::PopID(); // Departure ID
                }
                UI::endTable();
            }
            ImGui::EndDisabled(); // Scenario not editable
        }
    }

    namespace {
        void operationFlightRow(const Flight& Op) {
            const Settings& set = Application::settings();

            // Operation Type
            UI::tableNextColumn(false);
            UI::textInfo(OperationTypes.toString(Op.operationType()));

            if (Op.hasRoute())
            {
                // Airport
                UI::tableNextColumn(false);
                UI::textInfo(Op.route().parentAirport().Name);

                // Runway
                UI::tableNextColumn(false);
                UI::textInfo(Op.route().parentRunway().Name);

                // Route
                UI::tableNextColumn(false);
                UI::textInfo(Op.route().Name);
            }
            else
            {
                UI::tableNextColumn(false);
                UI::tableNextColumn(false);
                UI::tableNextColumn(false);
            }

            // Fleet Aircraft
            UI::tableNextColumn(false);
            UI::textInfo(Op.aircraft().Name);

            // Weight
            UI::tableNextColumn(false);
            UI::textInfo(fmt::format("{0:.{1}f}", set.WeightUnits.fromSi(Op.Weight), set.WeightUnits.decimals()));
        }

        void operationTrack4dRow(const Track4d& Op) {
            // Operation Type
            UI::tableNextColumn(false);
            UI::textInfo(OperationTypes.toString(Op.operationType()));

            // Fleet Aircraft
            UI::tableNextColumn(false);
            UI::textInfo(Op.aircraft().Name);
        }
    }

    namespace {
        struct CoordinateSystemDrawer : CoordinateSystemVisitor {
            bool visitCoordinateSystem(CoordinateSystem& Cs) { Cs.accept(*this); return Updated; }
            void visitLocalCartesian(LocalCartesian& Cs) override;

            bool Updated = false;
        };
    }

    void ScenariosPanel::drawSelectedPerformanceRun() {
        GRAPE_ASSERT(m_SelectedPerformanceRun);
        PerformanceRun& perfRun = *m_SelectedPerformanceRun;

        const Settings& set = Application::settings();
        const ImGuiStyle& style = ImGui::GetStyle();

        std::function<void()> action = nullptr; // Outside of loop edits
        bool updated = false;

        if (UI::buttonNew("Noise Run"))
            Application::study().Scenarios.addNoiseRun(perfRun);
        ImGui::SameLine();
        if (UI::buttonNew("Emissions Run"))
            Application::study().Scenarios.addEmissionsRun(perfRun);
        ImGui::SameLine();
        if (UI::buttonDelete("Performance Run"))
            m_Action = [&] { clearSelection(); Application::study().Scenarios.erase(perfRun); };

        ImGui::Separator();

        auto& perfRunSpec = perfRun.PerfRunSpec;
        if (ImGui::CollapsingHeader("Coordinate System"))
        {
            ImGui::BeginDisabled(!perfRun.job()->ready());

            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Type:");
            ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
            ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
            const std::string currTypeStr = CoordinateSystem::Types.toString(perfRunSpec.CoordSys->type());
            if (ImGui::BeginCombo("##CoordinateSystem", currTypeStr.c_str()))
            {
                for (const auto& typeStr : CoordinateSystem::Types)
                {
                    const bool selected = typeStr == currTypeStr;
                    if (ImGui::Selectable(typeStr, selected) && !selected)
                        switch (CoordinateSystem::Types.fromString(typeStr))
                        {
                        case CoordinateSystem::Type::LocalCartesian:
                            {
                                perfRunSpec.CoordSys = std::make_unique<LocalCartesian>(0.0f, 0.0f);
                                updated = true;
                                break;
                            }
                        case CoordinateSystem::Type::Geodesic:
                            {
                                perfRunSpec.CoordSys = std::make_unique<Geodesic>();
                                updated = true;
                                break;
                            }
                        default: GRAPE_ASSERT(false); break;
                        }
                }
                ImGui::EndCombo();
            }

            CoordinateSystemDrawer drawer;
            if (drawer.visitCoordinateSystem(*perfRunSpec.CoordSys))
                updated = true;

            ImGui::EndDisabled(); // Performance run job past ready
        }

        if (ImGui::CollapsingHeader("Atmosphere"))
        {
            static int opt = 0;
            ImGui::SetNextItemWidth(UI::g_StandardItemWidth * 0.6f);
            ImGui::Combo("##Options", &opt, "Deltas\0Absolute\0");

            ImGui::BeginDisabled(!perfRun.job()->ready());

            if (UI::buttonNew("Add Atmosphere"))
            {
                perfRunSpec.Atmospheres.addAtmosphere();
                updated = true;
            }
            ImGui::SameLine();
            if (UI::buttonDelete("Clear"))
            {
                perfRunSpec.Atmospheres.clear();
                updated = true;
            }

            const float tableHeight = UI::getTableHeight(perfRunSpec.Atmospheres.size(), true, ImGui::GetContentRegionAvail().y) * 1.3f;
            if (UI::beginTable("Atmospheres", 7, ImGuiTableFlags_None, ImVec2(0.0f, tableHeight)))
            {
                ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_NoHide);
                if (opt == 0)
                {
                    ImGui::TableSetupColumn(std::format("Temperature Delta ({})", set.TemperatureUnits.shortName()).c_str(), ImGuiTableColumnFlags_NoHide);
                    ImGui::TableSetupColumn(std::format("Pressure Delta ({})", set.PressureUnits.shortName()).c_str(), ImGuiTableColumnFlags_NoHide);
                }
                else
                {
                    ImGui::TableSetupColumn(std::format("Temperature ({})", set.TemperatureUnits.shortName()).c_str(), ImGuiTableColumnFlags_NoHide);
                    ImGui::TableSetupColumn(std::format("Pressure ({})", set.PressureUnits.shortName()).c_str(), ImGuiTableColumnFlags_NoHide);
                }
                ImGui::TableSetupColumn("Headwind Flag", ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn(std::format("Wind Speed ({})", set.SpeedUnits.shortName()).c_str(), ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn("Wind Direction", ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn("Relative Humidity (%)", ImGuiTableColumnFlags_NoHide);

                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableHeadersRow();

                ImGuiListClipper clipper;
                clipper.Begin(static_cast<int>(perfRunSpec.Atmospheres.size()));
                while (clipper.Step())
                {

                    for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
                    {
                        auto& [time, atm] = *std::next(perfRunSpec.Atmospheres.begin(), row);

                        ImGui::TableNextRow();
                        ImGui::PushID(&time);

                        // Selectable Row
                        UI::tableNextColumn();
                        UI::selectableRowEmpty();
                        if (ImGui::BeginPopupContextItem())
                        {
                            if (UI::selectableDelete())
                                action = [&, time] { perfRunSpec.Atmospheres.deleteAtmosphere(time); };
                            ImGui::EndPopup();
                        }

                        // Time
                        TimePoint newTime = time;
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                        if (UI::inputDateTime("Time of Observation", newTime))
                            action = [&, time, newTime] { perfRunSpec.Atmospheres.updateTime(time, newTime); };

                        if (opt == 0)
                        {
                            // Temperature Delta
                            UI::tableNextColumn();
                            double tempDelta = atm.temperatureDelta();
                            if (UI::inputDoubleDelta("Temperature delta", tempDelta, -100.0, 100.0, set.TemperatureUnits, false))
                            {
                                atm.setTemperatureDelta(tempDelta);
                                updated = true;
                            }

                            // Pressure Delta
                            UI::tableNextColumn();
                            double pressDelta = atm.pressureDelta();
                            if (UI::inputDoubleDelta("Pressure delta", pressDelta, -15000.0, 15000.0, set.PressureUnits, false))
                            {
                                atm.setPressureDelta(pressDelta);
                                updated = true;
                            }
                        }
                        else
                        {
                            // Temperature
                            UI::tableNextColumn();
                            double temp = atm.temperature(0.0);
                            if (UI::inputDouble("Temperature", temp, set.TemperatureUnits, false))
                            {
                                try { atm.setTemperatureDeltaE(temperatureDelta(0.0, temp)); }
                                catch (const GrapeException& err) { Log::io()->error("{}", err.what()); }
                                updated = true;
                            }

                            // Pressure
                            UI::tableNextColumn();
                            double press = atm.pressure(0.0);
                            if (UI::inputDouble("Pressure", press, set.PressureUnits, false))
                            {
                                try { atm.setPressureDeltaE(press - Constants::p0); }
                                catch (const GrapeException& err) { Log::io()->error("{}", err.what()); }
                                updated = true;
                            }
                        }

                        // Headwind Flag
                        UI::tableNextColumn(false);
                        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5.0f, 4.0f)); // Repush frame style
                        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f); // Repush frame style
                        bool unused = atm.isHeadwind(); // Only need to know if changed
                        if (ImGui::Checkbox("##FlightsDoc29Segmentation", &unused))
                        {
                            if (atm.isHeadwind())
                                atm.setWindDirection(0.0);
                            else
                                atm.setConstantHeadwind(atm.windSpeed());
                            updated = true;
                        }
                        ImGui::PopStyleVar(2); // Pop frame style

                        // Wind Speed
                        UI::tableNextColumn();
                        double windSpeed = atm.windSpeed();
                        if (UI::inputDouble("Wind Speed", windSpeed, set.SpeedUnits, false))
                        {
                            atm.setWindSpeed(windSpeed);
                            updated = true;
                        }

                        // Wind Direction
                        UI::tableNextColumn();
                        if (!atm.isHeadwind())
                        {
                            double windDirection = atm.windDirection();
                            if (UI::inputDouble("Wind Direction", windDirection, 0.0, 360.0, 2))
                            {
                                atm.setWindDirection(windDirection);
                                updated = true;
                            }
                        }

                        // Relative Humidity
                        UI::tableNextColumn();
                        double relativeHumidity = atm.relativeHumidity();
                        if (UI::inputPercentage("Relative Humidity", relativeHumidity, 0.0, 1.0, 0, false))
                        {
                            atm.setRelativeHumidity(relativeHumidity);
                            updated = true;
                        }

                        ImGui::PopID(); //Atmosphere Time
                    }
                }
                UI::endTable();
            }
            ImGui::EndDisabled(); // Performance run job past ready
        }

        if (ImGui::CollapsingHeader("Filters & Segmentation"))
        {
            ImGui::BeginDisabled(!perfRun.job()->ready());

            // Altitude
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Altitude Filter");
            ImGui::SameLine(0.0f, style.ItemSpacing.x * 2.0f);
            if (ImGui::Button("Reset##Altitude"))
            {
                perfRun.PerfRunSpec.FilterMinimumAltitude = -Constants::Inf;
                perfRun.PerfRunSpec.FilterMaximumAltitude = Constants::Inf;
                updated = true;
            }

            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Minimum:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
            if (UI::inputDouble("Minimum altitude", perfRun.PerfRunSpec.FilterMinimumAltitude, Constants::NaN, perfRun.PerfRunSpec.FilterMaximumAltitude, set.AltitudeUnits))
                updated = true;

            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Maximum:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
            if (UI::inputDouble("Maximum altitude", perfRun.PerfRunSpec.FilterMaximumAltitude, perfRun.PerfRunSpec.FilterMinimumAltitude, Constants::NaN, set.AltitudeUnits))
                updated = true;

            // Cumulative ground distance
            ImGui::Separator();
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Cumulative Ground Distance Filter");
            ImGui::SameLine(0.0f, style.ItemSpacing.x * 2.0f);
            if (ImGui::Button("Reset##Distance"))
            {
                perfRun.PerfRunSpec.FilterMinimumCumulativeGroundDistance = -Constants::Inf;
                perfRun.PerfRunSpec.FilterMaximumCumulativeGroundDistance = Constants::Inf;
                updated = true;
            }

            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Minimum:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
            if (UI::inputDouble("Minimum cumulative ground distance", perfRun.PerfRunSpec.FilterMinimumCumulativeGroundDistance, Constants::NaN, perfRun.PerfRunSpec.FilterMaximumCumulativeGroundDistance, set.DistanceUnits))
                updated = true;

            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Maximum:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
            if (UI::inputDouble("Maximum cumulative ground distance", perfRun.PerfRunSpec.FilterMaximumCumulativeGroundDistance, perfRun.PerfRunSpec.FilterMinimumCumulativeGroundDistance, Constants::NaN, set.DistanceUnits))
                updated = true;

            // Ground distance
            ImGui::Separator();
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Ground distance filter threshold:");
            ImGui::SameLine(0.0f, style.ItemSpacing.x);
            ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
            if (UI::inputDouble("Ground distance threshold", perfRun.PerfRunSpec.FilterGroundDistanceThreshold, 0.0, Constants::NaN, set.DistanceUnits))
                updated = true;
            ImGui::SameLine(0.0f, style.ItemSpacing.x * 2.0f);
            if (ImGui::Button("Reset##GroundDistance"))
            {
                perfRun.PerfRunSpec.FilterGroundDistanceThreshold = Constants::NaN;
                updated = true;
            }

            // Speed segmentation
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Speed delta segmentation threshold:");
            ImGui::SameLine(0.0f, style.ItemSpacing.x);
            ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
            if (UI::inputDouble("Speed delta segmentation threshold", perfRun.PerfRunSpec.SpeedDeltaSegmentationThreshold, Constants::Precision, Constants::NaN, set.SpeedUnits))
                updated = true;
            ImGui::SameLine(0.0f, style.ItemSpacing.x * 2.0f);
            if (ImGui::Button("Reset##SpeedSegmentation"))
            {
                perfRun.PerfRunSpec.SpeedDeltaSegmentationThreshold = Constants::NaN;
                updated = true;
            }

            ImGui::EndDisabled(); // Performance run job past ready
        }

        if (ImGui::CollapsingHeader("Flights"))
        {
            ImGui::BeginDisabled(!perfRun.job()->ready());

            // Performance Model
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Performance Model:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
            if (ImGui::BeginCombo("##PerformanceModel", PerformanceModelTypes.toString(perfRun.PerfRunSpec.FlightsPerformanceMdl).c_str()))
            {
                for (const auto& perfMdlStr : PerformanceModelTypes)
                {
                    const bool selected = perfRun.PerfRunSpec.FlightsPerformanceMdl == PerformanceModelTypes.fromString(perfMdlStr);
                    if (ImGui::Selectable(perfMdlStr, selected) && !selected)
                    {
                        perfRun.PerfRunSpec.FlightsPerformanceMdl = PerformanceModelTypes.fromString(perfMdlStr);
                        updated = true;
                    }
                }
                ImGui::EndCombo();
            }

            // Doc29 segmentation
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Doc29 segmentation:");
            ImGui::SameLine();
            if (ImGui::Checkbox("##Doc29Segmentation", &perfRun.PerfRunSpec.FlightsDoc29Segmentation))
                updated = true;

            ImGui::EndDisabled(); // Performance run job past ready
        }

        if (ImGui::CollapsingHeader("Tracks 4D"))
        {
            ImGui::BeginDisabled(!perfRun.job()->ready());

            // Calculate Tracks 4D
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Run performance:");
            ImGui::SameLine();
            if (ImGui::Checkbox("##RunPerformance", &perfRun.PerfRunSpec.Tracks4dCalculatePerformance))
                updated = true;

            if (perfRun.PerfRunSpec.Tracks4dCalculatePerformance)
            {
                ImGui::Separator();

                // Minimum point count
                ImGui::AlignTextToFramePadding();
                ImGui::TextDisabled("Minimum number of points:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
                if (UI::inputInt("Minimum tracks 4d points", perfRun.PerfRunSpec.Tracks4dMinimumPoints, 1, std::numeric_limits<int>::max()))
                    updated = true;

                // Recalculations
                ImGui::TextDisabled("Recalculate time:");
                ImGui::SameLine();
                if (ImGui::Checkbox("##RecalculateTime", &perfRun.PerfRunSpec.Tracks4dRecalculateTime))
                    updated = true;

                ImGui::TextDisabled("Recalculate cumulative ground distance:");
                ImGui::SameLine();
                if (ImGui::Checkbox("##RecalculateCumulativeGroundDistance", &perfRun.PerfRunSpec.Tracks4dRecalculateCumulativeGroundDistance))
                    updated = true;

                ImGui::TextDisabled("Recalculate ground speed:");
                ImGui::SameLine();
                if (ImGui::Checkbox("##RecalculateGroundspeed", &perfRun.PerfRunSpec.Tracks4dRecalculateGroundspeed))
                    updated = true;

                ImGui::TextDisabled("Recalculate fuel flow:");
                ImGui::SameLine();
                if (ImGui::Checkbox("##RecalculateFuelFlow", &perfRun.PerfRunSpec.Tracks4dRecalculateFuelFlow))
                    updated = true;
            }
            ImGui::EndDisabled(); // Performance run job past ready
        }

        if (ImGui::CollapsingHeader("Fuel Flow"))
        {
            ImGui::BeginDisabled(!perfRun.job()->ready());

            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Model:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
            if (ImGui::BeginCombo("##FuelFlowModel", FuelFlowModelTypes.toString(perfRun.PerfRunSpec.FuelFlowMdl).c_str()))
            {
                for (const auto& fuelFlowMdlStr : FuelFlowModelTypes)
                {
                    const bool selected = perfRun.PerfRunSpec.FuelFlowMdl == FuelFlowModelTypes.fromString(fuelFlowMdlStr);
                    if (ImGui::Selectable(fuelFlowMdlStr, selected) && !selected)
                    {
                        perfRun.PerfRunSpec.FuelFlowMdl = FuelFlowModelTypes.fromString(fuelFlowMdlStr);
                        updated = true;
                    }
                }
                ImGui::EndCombo();
            }

            if (perfRun.PerfRunSpec.FuelFlowMdl == FuelFlowModel::LTO || perfRun.PerfRunSpec.FuelFlowMdl == FuelFlowModel::LTODoc9889)
            {
                ImGui::AlignTextToFramePadding();
                ImGui::TextDisabled("Altitude Correction:");
                ImGui::SameLine();
                if (ImGui::Checkbox("##FuelFlowLTOAltitudeCorrection", &perfRun.PerfRunSpec.FuelFlowLTOAltitudeCorrection))
                    updated = true;
            }
            ImGui::EndDisabled(); // Performance run job past ready
        }

        // Output
        if (perfRun.job()->finished())
        {
            ImGui::Separator();
            const auto& perfRunOut = perfRun.output();

            if (ImGui::CollapsingHeader("Output"))
            {
                ImGui::BeginChild("Operations", ImVec2(-ImGui::GetContentRegionAvail().x * UI::g_StandardLeftAlignFraction, 0.0f));

                // Filter
                static UI::TextFilter filter;
                filter.draw();

                // Export
                UI::buttonEditRight(" " ICON_FA_FILE_ARROW_DOWN " ");
                if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonLeft))
                {
                    if (UI::selectableWithIcon("Export as .csv", ICON_FA_FILE_CSV))
                    {
                        auto [path, open] = UI::saveCsvFile(std::format("{} Performance Output", perfRun.Name).c_str());
                        if (open)
                            Application::get().queueAsyncTask([&, path] { IO::CSV::exportPerformanceRunOutput(perfRun.output(), path); }, std::format("Exporting performance run output to '{}'", path));
                    }

                    if (UI::selectableWithIcon("Export as .gpkg", ICON_FA_GLOBE))
                    {
                        auto [path, open] = UI::saveGpkgFile(std::format("{} Performance Output", perfRun.Name).c_str());
                        if (open)
                            Application::get().queueAsyncTask([&, path] { IO::GPKG::exportPerformanceRunOutput(perfRun, path); }, std::format("Exporting performance run output to '{}'", path));
                    }

                    ImGui::EndPopup();
                }

                if (UI::beginTable("Operations Table", 3))
                {
                    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
                    ImGui::TableSetupColumn("Operation", ImGuiTableColumnFlags_NoHide);
                    ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_NoHide);
                    ImGui::TableSetupScrollFreeze(0, 1);
                    ImGui::TableHeadersRow();

                    for (const auto& op : perfRunOut.arrivalOutputs())
                    {
                        auto& arrOp = op.get();
                        if (!filter.passesFilter(arrOp.Name))
                            continue;

                        ImGui::TableNextRow();
                        ImGui::PushID(arrOp.Name.c_str());

                        UI::tableNextColumn(false);

                        // Selectable Row
                        if (UI::selectableRowEmpty(isOutputSelected(arrOp)))
                            selectPerformanceOutput(arrOp);
                        if (ImGui::BeginPopupContextItem())
                        {
                            selectPerformanceOutput(arrOp);
                            if (ImGui::Selectable(ICON_FA_FILE_CSV " export"))
                            {
                                auto [path, open] = UI::saveCsvFile(std::format("{} Performance", arrOp.Name).c_str());
                                if (open)
                                    Application::get().queueAsyncTask([&, path] { IO::CSV::exportPerformanceOutput(*m_SelectedPerformanceOutput, path); }, std::format("Exporting performance run output to '{}'", path));
                            }
                            ImGui::EndPopup();
                        }

                        // Name
                        UI::textInfo(arrOp.Name);

                        // Operation
                        UI::tableNextColumn(false);
                        UI::textInfo(OperationTypes.toString(arrOp.operationType()));

                        // Type
                        UI::tableNextColumn(false);
                        UI::textInfo(Operation::Types.toString(arrOp.type()));

                        ImGui::PopID(); // Arrival ID
                    }

                    for (const auto& op : perfRunOut.departureOutputs())
                    {
                        auto& depOp = op.get();
                        if (!filter.passesFilter(depOp.Name))
                            continue;

                        ImGui::TableNextRow();
                        ImGui::PushID(depOp.Name.c_str());

                        UI::tableNextColumn(false);

                        // Selectable Row
                        if (UI::selectableRowEmpty(isOutputSelected(depOp)))
                            selectPerformanceOutput(depOp);
                        if (ImGui::BeginPopupContextItem())
                        {
                            selectPerformanceOutput(depOp);
                            if (ImGui::Selectable(ICON_FA_FILE_CSV " export"))
                            {
                                auto [path, open] = UI::saveCsvFile(std::format("{} Performance", depOp.Name).c_str());
                                if (open)
                                    Application::get().queueAsyncTask([&, path] { IO::CSV::exportPerformanceOutput(*m_SelectedPerformanceOutput, path); }, std::format("Exporting operation performance output to '{}'", path));
                            }
                            ImGui::EndPopup();
                        }

                        // Name
                        UI::textInfo(depOp.Name);

                        // Operation
                        UI::tableNextColumn(false);
                        UI::textInfo(OperationTypes.toString(depOp.operationType()));

                        // Type
                        UI::tableNextColumn(false);
                        UI::textInfo(Operation::Types.toString(depOp.type()));

                        ImGui::PopID(); // Departure ID
                    }
                    UI::endTable();
                }
                ImGui::EndChild();

                ImGui::SameLine();

                ImGui::BeginChild("PerformanceOutput");
                if (m_SelectedPerformanceOutput)
                    drawSelectedPerformanceOutput();
                ImGui::EndChild();
            }
        }

        if (action)
        {
            action();
            updated = true;
        }

        if (updated)
            Application::study().Scenarios.update(perfRun);
    }

    namespace {
        void CoordinateSystemDrawer::visitLocalCartesian(LocalCartesian& Cs) {
            auto& study = Application::study();
            const auto& style = ImGui::GetStyle();

            ImGui::Spacing();

            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Origin");

            // Set to...
            if (!study.Airports().empty())
            {
                ImGui::SameLine();
                ImGui::Button("Set to...");
                if (ImGui::BeginPopupContextItem(nullptr, ImGuiMouseButton_Left))
                {
                    for (const auto& [AptId, Apt] : study.Airports())
                    {
                        if (ImGui::BeginMenu(AptId.c_str()))
                        {
                            bool setApt = false;

                            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                                setApt = true;

                            for (const auto& [RwyId, Rwy] : Apt.Runways)
                            {
                                if (ImGui::Selectable(RwyId.c_str()))
                                {
                                    Cs.reset(Rwy.Longitude, Rwy.Latitude);
                                    Updated = true;
                                    setApt = false;
                                }
                            }

                            if (setApt)
                            {
                                Cs.reset(Apt.Longitude, Apt.Latitude);
                                Updated = true;
                                ImGui::CloseCurrentPopup();
                            }
                            ImGui::EndMenu();
                        }
                    }
                    ImGui::EndPopup();
                }
            }

            auto [lon0, lat0] = Cs.origin();

            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Longitude:");
            ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
            ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
            if (UI::inputDouble("Longitude", lon0, -180.0, 180.0))
            {
                Cs.reset(lon0, lat0);
                Updated = true;
            }

            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Latitude:");
            ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
            ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
            if (UI::inputDouble("Latitude origin", lat0, -90.0, 90.0))
            {
                Cs.reset(lon0, lat0);
                Updated = true;
            }
        }
    }

    namespace {
        struct ReceptorSetDrawer : ReceptorSetVisitor {
            bool visitReceptorSet(ReceptorSet& ReceptSet) { ReceptSet.accept(*this); return Updated; }
            void visitGrid(ReceptorGrid& ReceptSet) override;
            void visitPoints(ReceptorPoints& ReceptSet) override;

            bool Updated = false;
        };
    }

    void ScenariosPanel::drawSelectedNoiseRun() {
        GRAPE_ASSERT(m_SelectedNoiseRun);
        NoiseRun& nsRun = *m_SelectedNoiseRun;

        const auto& study = Application::study();

        const Settings& set = Application::settings();
        const auto& style = ImGui::GetStyle();

        bool updateNoiseRun = false;

        if (UI::buttonDelete("Noise Run"))
            m_Action = [&] { clearSelection(); study.Scenarios.erase(nsRun); };

        ImGui::Separator();

        if (ImGui::CollapsingHeader("Models"))
        {
            ImGui::BeginDisabled(!nsRun.job()->ready());
            const float offset = ImGui::GetCursorPosX() + ImGui::CalcTextSize("Atmospheric Absorption:").x;
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Noise:");
            ImGui::SameLine(offset, style.ItemInnerSpacing.x);
            ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
            if (ImGui::BeginCombo("##NoiseModel", NoiseModelTypes.toString(nsRun.NsRunSpec.NoiseMdl).c_str()))
            {
                for (const auto& nsMdlStr : NoiseModelTypes)
                {
                    const bool selected = nsRun.NsRunSpec.NoiseMdl == NoiseModelTypes.fromString(nsMdlStr);
                    if (ImGui::Selectable(nsMdlStr, selected) && !selected)
                    {
                        nsRun.NsRunSpec.NoiseMdl = NoiseModelTypes.fromString(nsMdlStr);
                        updateNoiseRun = true;
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Atmospheric Absorption:");
            ImGui::SameLine(offset, style.ItemInnerSpacing.x);
            ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
            if (ImGui::BeginCombo("##AtmosphericAbsorption", AtmosphericAbsorption::Types.toString(nsRun.NsRunSpec.AtmAbsorptionType).c_str()))
            {
                for (const auto& atmAbsStr : AtmosphericAbsorption::Types)
                {
                    const bool selected = nsRun.NsRunSpec.AtmAbsorptionType == AtmosphericAbsorption::Types.fromString(atmAbsStr);
                    if (ImGui::Selectable(atmAbsStr, selected) && !selected)
                    {
                        nsRun.NsRunSpec.AtmAbsorptionType = AtmosphericAbsorption::Types.fromString(atmAbsStr);
                        updateNoiseRun = true;
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::EndDisabled(); // Noise run job past ready
        }

        if (ImGui::CollapsingHeader("Receptor Set"))
        {
            ImGui::BeginDisabled(!nsRun.job()->ready());

            // Type
            ImGui::TextDisabled("Type:");
            ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
            UI::textInfo(ReceptorSet::Types.toString(nsRun.NsRunSpec.ReceptSet->type()));

            // Reset to...
            ImGui::Button("Reset to...");
            if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonLeft))
            {
                if (ImGui::Selectable("Points"))
                {
                    nsRun.NsRunSpec.ReceptSet = std::make_unique<ReceptorPoints>();
                    updateNoiseRun = true;
                }

                if (ImGui::Selectable("Grid"))
                {
                    nsRun.NsRunSpec.ReceptSet = std::make_unique<ReceptorGrid>();
                    updateNoiseRun = true;
                }
                ImGui::EndMenu();
            }

            ImGui::Separator();

            ReceptorSetDrawer receptSet;
            if (receptSet.visitReceptorSet(*nsRun.NsRunSpec.ReceptSet))
                updateNoiseRun = true;

            ImGui::EndDisabled(); // Noise run job past ready
        }

        if (ImGui::CollapsingHeader("Metrics"))
        {
            ImGui::BeginChild("Cumulative Metrics", ImVec2(-ImGui::GetContentRegionAvail().x * UI::g_StandardLeftAlignFraction, ImGui::GetContentRegionAvail().y / 1.5f));

            ImGui::AlignTextToFramePadding();
            UI::textInfo("Cumulative Metrics");

            // Edit Button
            ImGui::BeginDisabled(!nsRun.job()->ready());
            UI::buttonEditRight();
            if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonLeft))
            {
                if (UI::selectableNew("Cumulative Metric"))
                    study.Scenarios.addNoiseCumulativeMetric(nsRun);

                if (UI::selectableDelete("All"))
                {
                    m_SelectedNoiseCumulativeMetric = nullptr;
                    study.Scenarios.eraseNoiseCumulativeMetrics(nsRun);
                }

                ImGui::EndPopup();
            }
            ImGui::EndDisabled(); // Noise run job past ready

            ImGui::Separator();

            // Table
            if (UI::beginTable("Cumulative Metrics", 1, ImGuiTableFlags_None, ImVec2(0.0, UI::getTableHeight(nsRun.CumulativeMetrics.size(), false))))
            {
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);

                for (auto& [cumMetricId, cumMetric] : nsRun.CumulativeMetrics)
                {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();

                    ImGui::PushID(cumMetricId.c_str());

                    // Selectable Row
                    if (UI::selectableRowEmpty(isSelected(cumMetric)))
                        select(cumMetric);

                    ImGui::BeginDisabled(!nsRun.job()->ready());
                    if (ImGui::BeginPopupContextItem())
                    {
                        if (UI::selectableDelete())
                            m_Action = [&] { m_SelectedNoiseCumulativeMetric = nullptr; study.Scenarios.erase(cumMetric); };

                        ImGui::EndPopup();
                    }

                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                    if (UI::inputText(cumMetricId, cumMetric.Name, cumMetric.Name != cumMetricId && nsRun.CumulativeMetrics.contains(cumMetric.Name), "Cumulative Metric Name", std::format("Cumulative metric '{}' already exists in this noise run", cumMetric.Name)))
                        if (cumMetric.Name != cumMetricId)
                            m_Action = [&] { study.Scenarios.updateKey(cumMetric, cumMetricId); };
                    if (UI::isItemClicked())
                        select(cumMetric);

                    ImGui::EndDisabled(); // Noise run job past ready
                    ImGui::PopID(); // Cumulative Metric ID
                }

                UI::endTable();
            }

            ImGui::EndChild(); // Left Side

            if (m_SelectedNoiseCumulativeMetric)
            {
                auto& cumMetric = *m_SelectedNoiseCumulativeMetric;
                bool updateCumulativeMetric = false;

                ImGui::SameLine();

                ImGui::BeginChild("Selected cumulative metric", ImVec2(0.0f, ImGui::GetContentRegionAvail().y / 1.5f));

                ImGui::BeginDisabled(!nsRun.job()->ready());

                if (UI::buttonDelete("Cumulative Metric"))
                    m_Action = [&] { m_SelectedNoiseCumulativeMetric = nullptr; study.Scenarios.erase(cumMetric); };

                ImGui::Separator();

                ImGui::AlignTextToFramePadding();
                ImGui::TextDisabled("Start Time:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
                if (UI::inputDateTime("Start Time", cumMetric.StartTimePoint))
                    updateCumulativeMetric = true;
                ImGui::SameLine();
                ImGui::AlignTextToFramePadding();
                ImGui::TextDisabled("End Time:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
                if (UI::inputDateTime("End Time", cumMetric.EndTimePoint))
                    updateCumulativeMetric = true;
                ImGui::SameLine();
                ImGui::BeginDisabled(cumMetric.parentScenario().empty());
                if (ImGui::Button("Set to minimum & maximum scenario times"))
                {
                    cumMetric.setTimeSpanToScenarioSpan();
                    updateCumulativeMetric = true;
                }
                ImGui::EndDisabled(); // Parent scenario empty

                const float offset = ImGui::CalcTextSize("Cutoff Threshold (Maximum Level):").x;
                ImGui::AlignTextToFramePadding();
                ImGui::TextDisabled("Averaging Time Constant:");
                ImGui::SameLine(offset, style.ItemSpacing.x);
                ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
                if (UI::inputDouble("Averaging Time Constant", cumMetric.AveragingTimeConstant, 0.0, Constants::NaN, "dB"))
                    updateCumulativeMetric = true;
                ImGui::SameLine();
                if (ImGui::Button("Set to time span"))
                {
                    cumMetric.setAveragingTimeConstantToTimeSpan();
                    updateCumulativeMetric = true;
                }

                ImGui::AlignTextToFramePadding();
                ImGui::TextDisabled("Cutoff Threshold (Maximum Level):");
                ImGui::SameLine(offset, style.ItemSpacing.x);
                ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
                if (UI::inputDouble("Cutoff Threshold", cumMetric.Threshold, 0.0, Constants::NaN, "dB"))
                    updateCumulativeMetric = true;

                ImGui::EndDisabled(); // Noise run job past ready

                if (ImGui::CollapsingHeader("Weights"))
                {
                    ImGui::BeginDisabled(!nsRun.job()->ready());

                    const auto& highestTime = cumMetric.weights().rbegin()->first;
                    ImGui::BeginDisabled(highestTime >= std::chrono::hours(24) - std::chrono::seconds(1));
                    if (UI::buttonNew("Time"))
                    {
                        cumMetric.addWeight(highestTime + std::chrono::seconds(1), 1.0);
                        study.Scenarios.update(cumMetric);
                    }
                    ImGui::EndDisabled(); // Highest time lower than 23:59:59

                    ImGui::SameLine();

                    ImGui::BeginDisabled(cumMetric.weights().size() == 1);
                    if (UI::buttonDelete("Clear"))
                        cumMetric.clearWeights();
                    ImGui::EndDisabled(); // Only one weight

                    ImGui::SameLine();

                    ImGui::Button("Set to...");
                    if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonLeft))
                    {
                        for (const auto& metricStr : NoiseCumulativeMetric::StandardCumulativeMetrics)
                        {
                            if (ImGui::MenuItem(metricStr))
                            {
                                cumMetric.setStandard(NoiseCumulativeMetric::StandardCumulativeMetrics.fromString(metricStr));
                                updateCumulativeMetric = true;
                            }
                        }

                        ImGui::EndPopup();
                    }

                    const float offsetWeights = ImGui::CalcTextSize("00:00").x;
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x, 2.0));

                    for (auto& [time, weight] : cumMetric.weights())
                    {
                        const std::string timeStr = std::format("{:%T}", time);
                        ImGui::PushID(timeStr.c_str());

                        if (time == Duration(0))
                        {
                            ImGui::TextDisabled(timeStr.c_str());
                        }
                        else
                        {
                            Duration editTime = time;
                            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
                            ImGui::SetNextItemWidth(ImGui::CalcTextSize("00:00:00").x + style.FramePadding.x * 2.0f);
                            if (UI::inputTime(timeStr, editTime))
                                m_Action = [&, editTime] {
                                cumMetric.updateTime(time, editTime);
                                study.Scenarios.update(cumMetric);
                                };
                            ImGui::PopStyleVar();
                        }

                        ImGui::NewLine();
                        ImGui::SameLine(offsetWeights);
                        ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
                        if (UI::inputDouble("Weight", weight, 0.0, Constants::NaN, 3))
                            updateCumulativeMetric = true;

                        ImGui::PopID(); // Time String
                    }

                    ImGui::TextDisabled("24:00:00");

                    ImGui::PopStyleVar(); // Item vertical spacing
                    ImGui::EndDisabled(); // Noise run job past ready
                }

                if (ImGui::CollapsingHeader("Number Above Thresholds (Maximum Level)"))
                {
                    ImGui::BeginDisabled(!nsRun.job()->ready());

                    if (UI::buttonNew("Threshold"))
                    {
                        if (cumMetric.numberAboveThresholds().empty())
                            cumMetric.addNumberAboveThreshold(cumMetric.Threshold);
                        else
                            cumMetric.addNumberAboveThreshold(*cumMetric.numberAboveThresholds().rbegin() + 1.0);
                        updateCumulativeMetric = true;
                    }

                    ImGui::SameLine();

                    if (UI::buttonDelete("Clear"))
                    {
                        cumMetric.clearNumberAboveThresholds();
                        updateCumulativeMetric = true;
                    }

                    const float itemWidth = ImGui::CalcTextSize("XXX.XX dB").x + ImGui::GetStyle().FramePadding.x;
                    for (auto currThreshold : cumMetric.numberAboveThresholds())
                    {
                        ImGui::PushID(std::format("{}", currThreshold).c_str());

                        double newThreshold = currThreshold;
                        ImGui::SetNextItemWidth(itemWidth);
                        if (UI::inputDouble("Threshold", newThreshold, 0.0, Constants::NaN, 2, "dB"))
                            m_Action = [&, currThreshold, newThreshold] {
                            cumMetric.eraseNumberAboveThreshold(currThreshold);
                            cumMetric.addNumberAboveThreshold(newThreshold);
                            study.Scenarios.update(cumMetric);
                            };
                        ImGui::SameLine();

                        ImGui::PopID();
                    }
                    ImGui::NewLine();
                    ImGui::EndDisabled(); // Noise run job past ready
                }

                ImGui::EndChild(); // Right Side

                if (updateCumulativeMetric)
                    study.Scenarios.update(cumMetric);
            }

            ImGui::Separator();

            ImGui::BeginDisabled(!nsRun.job()->ready());
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Save single event metrics:");
            ImGui::SameLine();
            if (ImGui::Checkbox("##SaveSingleMetrics", &nsRun.NsRunSpec.SaveSingleMetrics))
                updateNoiseRun = true;
            ImGui::EndDisabled(); // Noise run job past ready
        }

        // Output
        if (nsRun.job()->finished())
        {
            ImGui::Separator();

            if (nsRun.NsRunSpec.SaveSingleMetrics)
                if (ImGui::CollapsingHeader("Output Single Event"))
                {
                    ImGui::BeginChild("Operations", ImVec2(-ImGui::GetContentRegionAvail().x * UI::g_StandardLeftAlignFraction, 0.0f));

                    // Filter
                    static UI::TextFilter filter;
                    filter.draw();

                    if (UI::beginTable("Operations Table", 3))
                    {
                        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
                        ImGui::TableSetupColumn("Operation", ImGuiTableColumnFlags_NoHide);
                        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_NoHide);
                        ImGui::TableSetupScrollFreeze(0, 1);
                        ImGui::TableHeadersRow();

                        for (const auto& op : nsRun.parentPerformanceRun().output().arrivalOutputs())
                        {
                            auto& arrOp = op.get();
                            if (!filter.passesFilter(arrOp.Name))
                                continue;

                            ImGui::TableNextRow();
                            ImGui::PushID(arrOp.Name.c_str());

                            UI::tableNextColumn(false);

                            // Selectable Row
                            if (UI::selectableRowEmpty(isOutputSelected(arrOp)))
                                selectNoiseSingleEventOutput(arrOp);
                            if (ImGui::BeginPopupContextItem())
                            {
                                selectNoiseSingleEventOutput(arrOp);
                                if (ImGui::Selectable(ICON_FA_FILE_CSV " export"))
                                {
                                    auto [path, open] = UI::saveCsvFile(std::format("{} Noise", arrOp.Name).c_str());
                                    if (open)
                                        Application::get().queueAsyncTask([&, path] { IO::CSV::exportNoiseSingleEventOutput(*m_SelectedNoiseSingleEventOutput, m_SelectedNoiseRun->output().receptors(), path); }, std::format("Exporting operation noise single event output to '{}'", path));
                                }
                                ImGui::EndPopup();
                            }

                            // Name
                            UI::textInfo(arrOp.Name);

                            // Operation
                            UI::tableNextColumn(false);
                            UI::textInfo(OperationTypes.toString(arrOp.operationType()));

                            // Type
                            UI::tableNextColumn(false);
                            UI::textInfo(Operation::Types.toString(arrOp.type()));

                            ImGui::PopID(); // Arrival ID
                        }

                        for (const auto& op : nsRun.parentPerformanceRun().output().departureOutputs())
                        {
                            auto& depOp = op.get();
                            if (!filter.passesFilter(depOp.Name))
                                continue;

                            ImGui::TableNextRow();
                            ImGui::PushID(depOp.Name.c_str());

                            UI::tableNextColumn(false);

                            // Selectable Row
                            if (UI::selectableRowEmpty(isOutputSelected(depOp)))
                                selectNoiseSingleEventOutput(depOp);
                            if (ImGui::BeginPopupContextItem())
                            {
                                selectNoiseSingleEventOutput(depOp);
                                if (ImGui::Selectable(ICON_FA_FILE_CSV " export"))
                                {
                                    auto [path, open] = UI::saveCsvFile(std::format("{} Noise", depOp.Name).c_str());
                                    if (open)
                                        Application::get().queueAsyncTask([&, path] { IO::CSV::exportNoiseSingleEventOutput(*m_SelectedNoiseSingleEventOutput, m_SelectedNoiseRun->output().receptors(), path); }, std::format("Exporting operation noise single event output to '{}'", path));
                                }
                                ImGui::EndPopup();
                            }

                            // Name
                            UI::textInfo(depOp.Name);

                            // Operation
                            UI::tableNextColumn(false);
                            UI::textInfo(OperationTypes.toString(depOp.operationType()));

                            // Type
                            UI::tableNextColumn(false);
                            UI::textInfo(Operation::Types.toString(depOp.type()));

                            ImGui::PopID(); // Departure ID
                        }
                        UI::endTable();
                    }
                    ImGui::EndChild();

                    ImGui::SameLine();

                    ImGui::BeginChild("Noise Single Event Output");
                    if (m_SelectedNoiseSingleEventOutput)
                        drawSelectedNoiseSingleEventOutput();
                    ImGui::EndChild();
                }

            if (!nsRun.output().cumulativeOutputs().empty())
                if (ImGui::CollapsingHeader("Output Cumulative Metrics"))
                {
                    const auto& cumOut = nsRun.output().cumulativeOutputs();

                    ImGui::BeginChild("Noise Output Cumulative Metrics", ImVec2(-ImGui::GetContentRegionAvail().x * UI::g_StandardLeftAlignFraction, 0.0f));

                    // Filter
                    static UI::TextFilter filter;
                    filter.draw();

                    // Export
                    UI::buttonEditRight(" " ICON_FA_FILE_ARROW_DOWN " ");
                    if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonLeft))
                    {
                        if (UI::selectableWithIcon("Export as .gpkg", ICON_FA_GLOBE))
                        {
                            auto [path, open] = UI::saveGpkgFile(std::format("{} Noise Cumulative Output", nsRun.Name).c_str());
                            if (open)
                                Application::get().queueAsyncTask([&, path] { IO::GPKG::exportNoiseRunOutput(nsRun, path); }, std::format("Exporting noise run cumulative output to '{}'", path));
                        }

                        ImGui::EndPopup();
                    }

                    // Table
                    if (UI::beginTable("Noise Output Cumulative Metrics", 1, ImGuiTableFlags_None))
                    {
                        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);

                        for (const auto cumMetric : cumOut | std::views::keys) // Key is pointer
                        {
                            if (!filter.passesFilter(cumMetric->Name))
                                continue;

                            ImGui::TableNextRow();
                            ImGui::PushID(cumMetric);

                            // Selectable Row
                            UI::tableNextColumn(false);
                            if (UI::selectableRowEmpty(isOutputSelected(*cumMetric)))
                                selectNoiseCumulativeOutput(*cumMetric);
                            if (ImGui::BeginPopupContextItem())
                            {
                                selectNoiseCumulativeOutput(*cumMetric);
                                if (ImGui::Selectable(ICON_FA_FILE_CSV " export"))
                                {
                                    auto [path, open] = UI::saveCsvFile(std::format("{} {} Output", nsRun.Name, cumMetric->Name).c_str());
                                    if (open)
                                        Application::get().queueAsyncTask([&, path] { IO::CSV::exportNoiseCumulativeMetricOutput(*m_SelectedNoiseCumulativeMetricOutput, *m_SelectedNoiseCumulativeOutput, m_SelectedNoiseRun->output().receptors(), path); }, std::format("Exporting noise cumulative metric output to '{}'", path));
                                }
                                ImGui::EndPopup();
                            }

                            // Name
                            UI::textInfo(cumMetric->Name);
                            if (ImGui::IsItemClicked())
                                selectNoiseCumulativeOutput(*cumMetric);

                            ImGui::PopID(); // Cumulative Metric ID
                        }

                        UI::endTable();
                    }

                    ImGui::EndChild(); // Left Side

                    ImGui::SameLine();

                    ImGui::BeginChild("Noise Output Selected Metric");
                    if (m_SelectedNoiseCumulativeMetricOutput)
                        drawSelectedNoiseCumulativeOutput();
                    ImGui::EndChild();
                }
        }

        if (updateNoiseRun)
            Application::study().Scenarios.update(nsRun);
    }

    void ScenariosPanel::drawSelectedEmissionsRun() {
        GRAPE_ASSERT(m_SelectedEmissionsRun);
        EmissionsRun& emiRun = *m_SelectedEmissionsRun;

        const Settings& set = Application::settings();
        const auto& style = ImGui::GetStyle();

        bool updated = false;

        if (UI::buttonDelete("Emissions Run"))
            m_Action = [&] { clearSelection(); Application::study().Scenarios.erase(emiRun); };

        ImGui::Separator();

        if (ImGui::CollapsingHeader("Models"))
        {
            ImGui::BeginDisabled(!emiRun.job()->ready());

            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Emissions:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
            if (ImGui::BeginCombo("##EmissionsModel", EmissionsModelTypes.toString(emiRun.EmissionsRunSpec.EmissionsMdl).c_str()))
            {
                for (const auto& emiMdlStr : EmissionsModelTypes)
                {
                    const bool selected = emiRun.EmissionsRunSpec.EmissionsMdl == EmissionsModelTypes.fromString(emiMdlStr);
                    if (ImGui::Selectable(emiMdlStr, selected) && !selected)
                    {
                        emiRun.EmissionsRunSpec.EmissionsMdl = EmissionsModelTypes.fromString(emiMdlStr);
                        updated = true;
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Calculate gas emissions:");
            ImGui::SameLine();
            if (ImGui::Checkbox("##CalculateGasEmissions", &emiRun.EmissionsRunSpec.CalculateGasEmissions))
                updated = true;

            if (emiRun.EmissionsRunSpec.CalculateGasEmissions && emiRun.EmissionsRunSpec.EmissionsMdl != EmissionsModel::LTOCycle)
            {
                ImGui::AlignTextToFramePadding();
                ImGui::TextDisabled("Use BFFM 2 to correct EIs:");
                ImGui::SameLine();
                if (ImGui::Checkbox("##UseBFFM2", &emiRun.EmissionsRunSpec.BFFM2Model))
                    updated = true;
            }

            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Calculate particle emissions:");
            ImGui::SameLine();
            if (ImGui::Checkbox("##CalculateParticleEmissions", &emiRun.EmissionsRunSpec.CalculateParticleEmissions))
                updated = true;

            if (emiRun.EmissionsRunSpec.CalculateParticleEmissions)
            {
                ImGui::AlignTextToFramePadding();
                ImGui::TextDisabled("Smoke Number to nvPM EI:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
                if (ImGui::BeginCombo("##SmokeNumbernvPMEI", EmissionsParticleSmokeNumberModelTypes.toString(emiRun.EmissionsRunSpec.ParticleSmokeNumberModel).c_str()))
                {
                    for (const auto& particleMdlStr : EmissionsParticleSmokeNumberModelTypes)
                    {
                        const bool selected = emiRun.EmissionsRunSpec.ParticleSmokeNumberModel == EmissionsParticleSmokeNumberModelTypes.fromString(particleMdlStr);
                        if (ImGui::Selectable(particleMdlStr, selected) && !selected)
                        {
                            emiRun.EmissionsRunSpec.ParticleSmokeNumberModel = EmissionsParticleSmokeNumberModelTypes.fromString(particleMdlStr);
                            updated = true;
                        }
                    }
                    ImGui::EndCombo();
                }
            }
            ImGui::Separator();

            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Save segment results:");
            ImGui::SameLine();
            if (ImGui::Checkbox("##SaveSegmentResults", &emiRun.EmissionsRunSpec.SaveSegmentResults))
                updated = true;

            ImGui::EndDisabled(); // Emissions job past ready
        }

        if (ImGui::CollapsingHeader("Settings"))
        {
            ImGui::BeginDisabled(!emiRun.job()->ready());

            if (emiRun.EmissionsRunSpec.EmissionsMdl == EmissionsModel::LTOCycle)
            {
                ImGui::PushID("LTOCycle");
                ImGui::TextDisabled("LTO Cycle Times");

                const float offset = ImGui::CalcTextSize("Approach:").x;

                ImGui::AlignTextToFramePadding();
                UI::textInfo("Idle:");
                ImGui::SameLine(offset, style.ItemSpacing.x);
                ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
                if (UI::inputDouble("Idle", emiRun.EmissionsRunSpec.LTOCycle.at(0), 0.0, Constants::NaN, 0, "s"))
                    updated = true;

                ImGui::AlignTextToFramePadding();
                UI::textInfo("Approach:");
                ImGui::SameLine(offset, style.ItemSpacing.x);
                ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
                if (UI::inputDouble("Approach", emiRun.EmissionsRunSpec.LTOCycle.at(1), 0.0, Constants::NaN, 0, "s"))
                    updated = true;

                ImGui::AlignTextToFramePadding();
                UI::textInfo("Climb Out:");
                ImGui::SameLine(offset, style.ItemSpacing.x);
                ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
                if (UI::inputDouble("ClimbOut", emiRun.EmissionsRunSpec.LTOCycle.at(2), 0.0, Constants::NaN, 0, "s"))
                    updated = true;

                ImGui::AlignTextToFramePadding();
                UI::textInfo("Takeoff:");
                ImGui::SameLine(offset, style.ItemSpacing.x);
                ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
                if (UI::inputDouble("Takeoff", emiRun.EmissionsRunSpec.LTOCycle.at(3), 0.0, Constants::NaN, 0, "s"))
                    updated = true;

                ImGui::PopID(); // LTOCycle
            }
            else
            {
                {
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextDisabled("Altitude Filter");
                    ImGui::SameLine(0.0f, style.ItemSpacing.x * 2.0f);
                    if (ImGui::Button("Reset##Altitude"))
                    {
                        emiRun.EmissionsRunSpec.FilterMinimumAltitude = -Constants::Inf;
                        emiRun.EmissionsRunSpec.FilterMaximumAltitude = Constants::Inf;
                        updated = true;
                    }

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextDisabled("Minimum:");
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
                    if (UI::inputDouble("Minimum altitude", emiRun.EmissionsRunSpec.FilterMinimumAltitude, Constants::NaN, emiRun.EmissionsRunSpec.FilterMaximumAltitude, set.AltitudeUnits))
                        updated = true;
                    ImGui::SameLine();
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextDisabled("Maximum:");
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
                    if (UI::inputDouble("Maximum altitude", emiRun.EmissionsRunSpec.FilterMaximumAltitude, emiRun.EmissionsRunSpec.FilterMinimumAltitude, Constants::NaN, set.AltitudeUnits))
                        updated = true;
                }

                {
                    ImGui::Separator();
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextDisabled("Cumulative Ground Distance Filter");
                    ImGui::SameLine(0.0f, style.ItemSpacing.x * 2.0f);
                    if (ImGui::Button("Reset##Distance"))
                    {
                        emiRun.EmissionsRunSpec.FilterMinimumCumulativeGroundDistance = -Constants::Inf;
                        emiRun.EmissionsRunSpec.FilterMaximumCumulativeGroundDistance = Constants::Inf;
                        updated = true;
                    }

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextDisabled("Minimum:");
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
                    if (UI::inputDouble("Minimum cumulative ground distance", emiRun.EmissionsRunSpec.FilterMinimumCumulativeGroundDistance, Constants::NaN, emiRun.EmissionsRunSpec.FilterMaximumCumulativeGroundDistance, set.DistanceUnits))
                        updated = true;
                    ImGui::SameLine();
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextDisabled("Maximum:");
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
                    if (UI::inputDouble("Maximum cumulative ground distance", emiRun.EmissionsRunSpec.FilterMaximumCumulativeGroundDistance, emiRun.EmissionsRunSpec.FilterMinimumCumulativeGroundDistance, Constants::NaN, set.DistanceUnits))
                        updated = true;
                }
            }

            if (emiRun.EmissionsRunSpec.CalculateParticleEmissions)
            {
                ImGui::PushID("Particles");
                ImGui::Separator();

                ImGui::AlignTextToFramePadding();
                ImGui::TextDisabled("Particle Effective Density:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
                if (UI::inputDouble("ParticleEffectiveDensity", emiRun.EmissionsRunSpec.ParticleEffectiveDensity, Constants::Precision, Constants::NaN, 0, "kg/m3"))
                    updated = true;

                ImGui::AlignTextToFramePadding();
                ImGui::TextDisabled("Particle Geometric Standard Deviation:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
                if (UI::inputDouble("ParticleGeometricStandardDeviation", emiRun.EmissionsRunSpec.ParticleGeometricStandardDeviation, Constants::Precision, Constants::NaN, 2))
                    updated = true;

                const float offset = ImGui::CalcTextSize("Approach:").x;
                ImGui::TextDisabled("Particle Geometric Mean Diameter");

                {
                    ImGui::AlignTextToFramePadding();
                    UI::textInfo("Idle:");
                    ImGui::SameLine(offset, style.ItemSpacing.x);
                    ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
                    double valNm = emiRun.EmissionsRunSpec.ParticleGeometricMeanDiameter.at(0) * 1e9;
                    if (UI::inputDouble("Idle", valNm, Constants::Precision, Constants::NaN, 0, "nm"))
                    {
                        emiRun.EmissionsRunSpec.ParticleGeometricMeanDiameter.at(0) = valNm * 1e-9;
                        updated = true;
                    }
                }

                {
                    ImGui::AlignTextToFramePadding();
                    UI::textInfo("Approach:");
                    ImGui::SameLine(offset, style.ItemSpacing.x);
                    ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
                    double valNm = emiRun.EmissionsRunSpec.ParticleGeometricMeanDiameter.at(1) * 1e9;
                    if (UI::inputDouble("Approach", valNm, Constants::Precision, Constants::NaN, 0, "nm"))
                    {
                        emiRun.EmissionsRunSpec.ParticleGeometricMeanDiameter.at(1) = valNm * 1e-9;
                        updated = true;
                    }
                }

                {
                    ImGui::AlignTextToFramePadding();
                    UI::textInfo("Climb Out:");
                    ImGui::SameLine(offset, style.ItemSpacing.x);
                    ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
                    double valNm = emiRun.EmissionsRunSpec.ParticleGeometricMeanDiameter.at(2) * 1e9;
                    if (UI::inputDouble("Climb Out", valNm, Constants::Precision, Constants::NaN, 0, "nm"))
                    {
                        emiRun.EmissionsRunSpec.ParticleGeometricMeanDiameter.at(2) = valNm * 1e-9;
                        updated = true;
                    }
                }

                {
                    ImGui::AlignTextToFramePadding();
                    UI::textInfo("Takeoff:");
                    ImGui::SameLine(offset, style.ItemSpacing.x);
                    ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
                    double valNm = emiRun.EmissionsRunSpec.ParticleGeometricMeanDiameter.at(3) * 1e9;
                    if (UI::inputDouble("Takeoff", valNm, Constants::Precision, Constants::NaN, 0, "nm"))
                    {
                        emiRun.EmissionsRunSpec.ParticleGeometricMeanDiameter.at(3) = valNm * 1e-9;
                        updated = true;
                    }
                }

                ImGui::PopID(); // Particles
            }

            ImGui::EndDisabled(); // Emissions job past ready
        }

        // Output
        if (emiRun.job()->finished())
        {
            ImGui::Separator();

            if (ImGui::CollapsingHeader("Output Totals"))
            {
                // Filter
                static UI::TextFilter filter;
                filter.draw();

                if (UI::buttonEditRight(ICON_FA_DOWNLOAD " .csv"))
                {
                    auto [path, open] = UI::saveCsvFile(std::format("{} Emissions Total Output", emiRun.Name).c_str());
                    if (open)
                        Application::get().queueAsyncTask([&, path] { IO::CSV::exportEmissionsRunOutput(emiRun.output(), path); }, std::format("Exporting emissions run output to '{}'", path));
                }


                if (UI::beginTable("EmissionsOutput", 9))
                {
                    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
                    ImGui::TableSetupColumn("Operation", ImGuiTableColumnFlags_NoHide);
                    ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_NoHide);
                    ImGui::TableSetupColumn(std::format("Fuel ({})", set.WeightUnits.shortName()).c_str());
                    ImGui::TableSetupColumn(std::format("HC ({})", set.EmissionsWeightUnits.shortName()).c_str());
                    ImGui::TableSetupColumn(std::format("CO ({})", set.EmissionsWeightUnits.shortName()).c_str());
                    ImGui::TableSetupColumn(std::format("NOx ({})", set.EmissionsWeightUnits.shortName()).c_str());
                    ImGui::TableSetupColumn(std::format("nvPM Mass ({})", set.EmissionsWeightUnits.shortName()).c_str());
                    ImGui::TableSetupColumn("nvPM Number (#)");

                    ImGui::TableSetupScrollFreeze(0, 1);
                    ImGui::TableHeadersRow();

                    // Totals
                    ImGui::TableNextRow();
                    const auto& emissionTotals = emiRun.output().totalEmissions();

                    UI::tableNextColumn(false);
                    UI::textInfo("Totals");

                    UI::tableNextColumn(false); // Operation
                    UI::tableNextColumn(false); // Type

                    // Fuel
                    UI::tableNextColumn(false);
                    UI::textInfo(std::format("{0:.{1}f}", set.WeightUnits.fromSi(emiRun.output().totalFuel()), set.WeightUnits.decimals()));

                    // HC
                    UI::tableNextColumn(false);
                    UI::textInfo(std::format("{0:.{1}f}", set.EmissionsWeightUnits.fromSi(emissionTotals.HC), set.EmissionsWeightUnits.decimals()));

                    // CO
                    UI::tableNextColumn(false);
                    UI::textInfo(std::format("{0:.{1}f}", set.EmissionsWeightUnits.fromSi(emissionTotals.CO), set.EmissionsWeightUnits.decimals()));

                    // NOx
                    UI::tableNextColumn(false);
                    UI::textInfo(std::format("{0:.{1}f}", set.EmissionsWeightUnits.fromSi(emissionTotals.NOx), set.EmissionsWeightUnits.decimals()));

                    // nvPM
                    UI::tableNextColumn(false);
                    UI::textInfo(std::format("{0:.{1}f}", set.EmissionsWeightUnits.fromSi(emissionTotals.nvPM), set.EmissionsWeightUnits.decimals()));

                    // nvPM number
                    UI::tableNextColumn(false);
                    UI::textInfo(std::format("{0:.3e}", emissionTotals.nvPMNumber));

                    // Arrival Operations
                    for (const auto& op : emiRun.parentPerformanceRun().output().arrivalOutputs())
                    {
                        auto& arrOp = op.get();
                        if (!filter.passesFilter(arrOp.Name))
                            continue;

                        ImGui::TableNextRow();
                        ImGui::PushID(arrOp.Name.c_str());

                        UI::tableNextColumn(false);

                        // Name
                        UI::textInfo(arrOp.Name);

                        // Operation
                        UI::tableNextColumn(false);
                        UI::textInfo(OperationTypes.toString(arrOp.operationType()));

                        // Type
                        UI::tableNextColumn(false);
                        UI::textInfo(Operation::Types.toString(arrOp.type()));

                        const auto& opOut = emiRun.output().operationOutput(arrOp);

                        // Fuel
                        UI::tableNextColumn(false);
                        UI::textInfo(std::format("{0:.{1}f}", set.WeightUnits.fromSi(opOut.totalFuel()), set.WeightUnits.decimals()));

                        // HC
                        UI::tableNextColumn(false);
                        UI::textInfo(std::format("{0:.{1}f}", set.EmissionsWeightUnits.fromSi(opOut.totalEmissions().HC), set.EmissionsWeightUnits.decimals()));

                        // CO
                        UI::tableNextColumn(false);
                        UI::textInfo(std::format("{0:.{1}f}", set.EmissionsWeightUnits.fromSi(opOut.totalEmissions().CO), set.EmissionsWeightUnits.decimals()));

                        // NOx
                        UI::tableNextColumn(false);
                        UI::textInfo(std::format("{0:.{1}f}", set.EmissionsWeightUnits.fromSi(opOut.totalEmissions().NOx), set.EmissionsWeightUnits.decimals()));

                        // nvPM
                        UI::tableNextColumn(false);
                        UI::textInfo(std::format("{0:.{1}f}", set.EmissionsWeightUnits.fromSi(opOut.totalEmissions().nvPM), set.EmissionsWeightUnits.decimals()));

                        // nvPM number
                        UI::tableNextColumn(false);
                        UI::textInfo(std::format("{0:.3e}", opOut.totalEmissions().nvPMNumber));

                        ImGui::PopID(); // Arrival ID
                    }

                    // Departure Operations
                    for (const auto& op : emiRun.parentPerformanceRun().output().departureOutputs())
                    {
                        auto& depOp = op.get();
                        if (!filter.passesFilter(depOp.Name))
                            continue;

                        ImGui::TableNextRow();
                        ImGui::PushID(depOp.Name.c_str());

                        UI::tableNextColumn(false);

                        // Name
                        UI::textInfo(depOp.Name);

                        // Operation
                        UI::tableNextColumn(false);
                        UI::textInfo(OperationTypes.toString(depOp.operationType()));

                        // Type
                        UI::tableNextColumn(false);
                        UI::textInfo(Operation::Types.toString(depOp.type()));

                        const auto& opOut = emiRun.output().operationOutput(depOp);

                        // Fuel
                        UI::tableNextColumn(false);
                        UI::textInfo(std::format("{0:.{1}f}", set.WeightUnits.fromSi(opOut.totalFuel()), set.WeightUnits.decimals()));

                        // HC
                        UI::tableNextColumn(false);
                        UI::textInfo(std::format("{0:.{1}f}", set.EmissionsWeightUnits.fromSi(opOut.totalEmissions().HC), set.EmissionsWeightUnits.decimals()));

                        // CO
                        UI::tableNextColumn(false);
                        UI::textInfo(std::format("{0:.{1}f}", set.EmissionsWeightUnits.fromSi(opOut.totalEmissions().CO), set.EmissionsWeightUnits.decimals()));

                        // NOx
                        UI::tableNextColumn(false);
                        UI::textInfo(std::format("{0:.{1}f}", set.EmissionsWeightUnits.fromSi(opOut.totalEmissions().NOx), set.EmissionsWeightUnits.decimals()));

                        // nvPM
                        UI::tableNextColumn(false);
                        UI::textInfo(std::format("{0:.{1}f}", set.EmissionsWeightUnits.fromSi(opOut.totalEmissions().nvPM), set.EmissionsWeightUnits.decimals()));

                        // nvPM number
                        UI::tableNextColumn(false);
                        UI::textInfo(std::format("{0:.3e}", opOut.totalEmissions().nvPMNumber));

                        ImGui::PopID(); // Departure ID
                    }
                    UI::endTable();
                }
            }

            if (emiRun.EmissionsRunSpec.SaveSegmentResults)
            {
                if (ImGui::CollapsingHeader("Output Segments"))
                {
                    ImGui::BeginChild("Operations", ImVec2(-ImGui::GetContentRegionAvail().x * UI::g_StandardLeftAlignFraction, 0.0f));

                    // Filter
                    static UI::TextFilter filter;
                    filter.draw();

                    if (UI::beginTable("Operations Table", 3))
                    {
                        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
                        ImGui::TableSetupColumn("Operation", ImGuiTableColumnFlags_NoHide);
                        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_NoHide);
                        ImGui::TableSetupScrollFreeze(0, 1);
                        ImGui::TableHeadersRow();

                        for (const auto& op : emiRun.parentPerformanceRun().output().arrivalOutputs())
                        {
                            auto& arrOp = op.get();
                            if (!filter.passesFilter(arrOp.Name))
                                continue;

                            ImGui::TableNextRow();
                            ImGui::PushID(arrOp.Name.c_str());

                            UI::tableNextColumn(false);

                            // Selectable Row
                            if (UI::selectableRowEmpty(isOutputSelected(arrOp)))
                                selectEmissionsSegmentOutput(arrOp);

                            if (ImGui::BeginPopupContextItem())
                            {
                                selectEmissionsSegmentOutput(arrOp);
                                if (ImGui::Selectable(ICON_FA_FILE_CSV " export"))
                                {
                                    auto [path, open] = UI::saveCsvFile(std::format("{} Emissions", arrOp.Name).c_str());
                                    if (open)
                                        Application::get().queueAsyncTask([&, path] { IO::CSV::exportEmissionsSegmentOutput(*m_SelectedEmissionsSegmentOutput, path); }, std::format("Exporting operation emissions segment output to '{}'", path));
                                }
                                ImGui::EndPopup();
                            }

                            // Name
                            UI::textInfo(arrOp.Name);

                            // Operation
                            UI::tableNextColumn(false);
                            UI::textInfo(OperationTypes.toString(arrOp.operationType()));

                            // Type
                            UI::tableNextColumn(false);
                            UI::textInfo(Operation::Types.toString(arrOp.type()));

                            ImGui::PopID(); // Arrival ID
                        }

                        for (const auto& op : emiRun.parentPerformanceRun().output().departureOutputs())
                        {
                            auto& depOp = op.get();
                            if (!filter.passesFilter(depOp.Name))
                                continue;

                            ImGui::TableNextRow();
                            ImGui::PushID(depOp.Name.c_str());

                            UI::tableNextColumn(false);

                            // Selectable Row
                            if (UI::selectableRowEmpty(isOutputSelected(depOp)))
                                selectEmissionsSegmentOutput(depOp);

                            if (ImGui::BeginPopupContextItem())
                            {
                                selectEmissionsSegmentOutput(depOp);
                                if (ImGui::Selectable(ICON_FA_FILE_CSV " export"))
                                {
                                    auto [path, open] = UI::saveCsvFile(std::format("{} Emissions", depOp.Name).c_str());
                                    if (open)
                                        Application::get().queueAsyncTask([&, path] { IO::CSV::exportEmissionsSegmentOutput(*m_SelectedEmissionsSegmentOutput, path); }, std::format("Exporting operation emissions segment output to '{}'", path));
                                }
                                ImGui::EndPopup();
                            }

                            // Name
                            UI::textInfo(depOp.Name);

                            // Operation
                            UI::tableNextColumn(false);
                            UI::textInfo(OperationTypes.toString(depOp.operationType()));

                            // Type
                            UI::tableNextColumn(false);
                            UI::textInfo(Operation::Types.toString(depOp.type()));

                            ImGui::PopID(); // Departure ID
                        }
                        UI::endTable();
                    }
                    ImGui::EndChild();

                    ImGui::SameLine();

                    ImGui::BeginChild("Emissions Segment Output");
                    if (m_SelectedEmissionsSegmentOutput)
                        drawSelectedEmissionsSegmentOutput();
                    ImGui::EndChild();
                }
            }
        }

        if (updated)
            Application::study().Scenarios.update(emiRun);
    }

    namespace {
        void ReceptorSetDrawer::visitGrid(ReceptorGrid& ReceptSet) {
            const auto& set = Application::settings();
            const auto& style = ImGui::GetStyle();

            {
                // Reference Point
                ImGui::AlignTextToFramePadding();
                ImGui::TextDisabled("Reference Point");
                ImGui::SameLine();
                ImGui::BeginDisabled(Application::study().Airports().empty());
                ImGui::Button("Set to...");
                if (ImGui::BeginPopupContextItem(nullptr, ImGuiMouseButton_Left))
                {
                    for (const auto& [aptId, apt] : Application::study().Airports())
                    {
                        if (ImGui::Selectable(aptId.c_str()))
                        {
                            ReceptSet.RefLongitude = apt.Longitude;
                            ReceptSet.RefLatitude = apt.Latitude;
                            ReceptSet.RefAltitudeMsl = apt.Elevation;
                            Updated = true;
                        }
                    }
                    ImGui::EndPopup();
                }
                ImGui::EndDisabled(); // Empty airports

                const float offset = ImGui::GetCursorPosX() + ImGui::CalcTextSize("Longitude:").x;

                ImGui::AlignTextToFramePadding();
                ImGui::TextDisabled("Location:");
                ImGui::SameLine(offset);
                const std::string currLocationStr = ReceptorGrid::Locations.toString(ReceptSet.RefLocation);
                ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
                if (ImGui::BeginCombo("##PerformanceModel", currLocationStr.c_str()))
                {
                    for (const auto& locationStr : ReceptorGrid::Locations)
                    {
                        const bool selected = locationStr == currLocationStr;
                        if (ImGui::Selectable(locationStr, selected) && !selected)
                        {
                            ReceptSet.RefLocation = ReceptorGrid::Locations.fromString(locationStr);
                            Updated = true;
                        }
                    }
                    ImGui::EndCombo();
                }

                ImGui::AlignTextToFramePadding();
                ImGui::TextDisabled("Longitude:");
                ImGui::SameLine(offset, style.ItemInnerSpacing.x);
                ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
                if (UI::inputDouble("Longitude", ReceptSet.RefLongitude, -180.0, 180.0))
                    Updated = true;

                ImGui::SameLine();
                ImGui::AlignTextToFramePadding();
                ImGui::TextDisabled("Latitude:");
                ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
                ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
                if (UI::inputDouble("Latitude", ReceptSet.RefLatitude, -90.0, 90.0))
                    Updated = true;

                ImGui::SameLine();
                ImGui::AlignTextToFramePadding();
                ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
                ImGui::TextDisabled("Altitude MSL:");
                ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
                ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
                if (UI::inputDouble("AltitudeMsl", ReceptSet.RefAltitudeMsl, 0.0, Constants::NaN, set.AltitudeUnits))
                    Updated = true;
            }

            { // Spacing
                ImGui::Separator();
                ImGui::TextDisabled("Spacing");

                ImGui::AlignTextToFramePadding();
                ImGui::TextDisabled("Horizontal:");
                ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
                ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
                if (UI::inputDouble("Horizontal Spacing", ReceptSet.HorizontalSpacing, 0.0, Constants::NaN, set.DistanceUnits))
                    Updated = true;

                ImGui::SameLine();
                ImGui::AlignTextToFramePadding();
                ImGui::TextDisabled("Vertical:");
                ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
                ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
                if (UI::inputDouble("Vertical Spacing", ReceptSet.VerticalSpacing, 0.0, Constants::NaN, set.DistanceUnits))
                    Updated = true;
            }

            { // # Points
                ImGui::Separator();
                ImGui::TextDisabled("# Points");

                int hCount = static_cast<int>(ReceptSet.HorizontalCount);
                ImGui::AlignTextToFramePadding();
                ImGui::TextDisabled("Horizontal:");
                ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
                ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
                if (UI::inputInt("Horizontal Count", hCount, 0))
                {
                    ReceptSet.HorizontalCount = hCount;
                    Updated = true;
                }

                ImGui::SameLine();
                int vCount = static_cast<int>(ReceptSet.VerticalCount);
                ImGui::AlignTextToFramePadding();
                ImGui::TextDisabled("Vertical:");
                ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
                ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
                if (UI::inputInt("Vertical Count", vCount, 0))
                {
                    ReceptSet.VerticalCount = vCount;
                    Updated = true;
                }
            }

            { // Grid Rotation
                ImGui::Separator();
                ImGui::AlignTextToFramePadding();
                ImGui::TextDisabled("Grid Rotation:");
                ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
                ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
                if (UI::inputDouble("Grid Rotation", ReceptSet.GridRotation, -360.0, 360.0, 0))
                    Updated = true;
            }
        }

        void ReceptorSetDrawer::visitPoints(ReceptorPoints& ReceptSet) {
            const Settings& set = Application::settings();

            std::function<bool()> action = nullptr; // Outside of loop

            // Filter
            static UI::TextFilter textFilter;
            textFilter.draw();

            // Edit button
            UI::buttonEditRight();
            if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonLeft))
            {
                if (UI::selectableNew("Point"))
                    if (ReceptSet.addPoint())
                        Updated = true;

                if (UI::selectableDelete("Clear"))
                {
                    ReceptSet.clear();
                    Updated = true;
                }

                ImGui::EndPopup();
            }

            if (UI::beginTable("Receptor Points", 4, ImGuiTableFlags_None, ImVec2(0.0f, UI::getTableHeight(ReceptSet.size()))))
            {
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn("Longitude", ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn("Latitude", ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn(std::format("Elevation ({})", set.AltitudeUnits.shortName()).c_str(), ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableHeadersRow();

                for (auto& [receptId, recept] : ReceptSet)
                {
                    if (!textFilter.passesFilter(receptId))
                        continue;

                    ImGui::TableNextRow();
                    ImGui::PushID(receptId.c_str());

                    UI::tableNextColumn(false);

                    // Selectable Row
                    UI::selectableRowEmpty();
                    if (ImGui::BeginPopupContextItem())
                    {
                        if (UI::selectableDelete())
                            action = [&] { return ReceptSet.deletePoint(receptId); };

                        ImGui::EndPopup();
                    }

                    // Name
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                    if (UI::inputText("Name", recept.Name, recept.Name != receptId && ReceptSet.contains(recept.Name), "Point name", std::format("Point '{}' already exists in this receptor set.", recept.Name)))
                        if (recept.Name != receptId)
                            action = [&] { return ReceptSet.updateName(receptId); };

                    UI::tableNextColumn();
                    if (UI::inputDouble("Longitude", recept.Longitude, -180.0, 180.0))
                        Updated = true;

                    UI::tableNextColumn();
                    if (UI::inputDouble("Latitude", recept.Latitude, -90.0, 90.0))
                        Updated = true;

                    UI::tableNextColumn();
                    if (UI::inputDouble("Altitude Msl", recept.Elevation, set.AltitudeUnits, false))
                        Updated = true;

                    ImGui::PopID(); // Receptor ID
                }
                UI::endTable();
            }

            if (action)
                if (action())
                    Updated = true;
        }
    }

    void ScenariosPanel::drawSelectedPerformanceOutput() const {
        GRAPE_ASSERT(m_SelectedPerformanceOutput);
        const PerformanceOutput& perfOut = *m_SelectedPerformanceOutput;

        const auto& set = Application::settings();

        if (UI::beginTable("PerformanceOutput", 13))
        {
            ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Origin", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Flight Phase", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn(std::format("Cumulative Ground Distance ({})", set.DistanceUnits.shortName()).c_str(), ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Longitude", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Latitude", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn(std::format("Altitude MSL ({})", set.AltitudeUnits.shortName()).c_str(), ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn(std::format("True Airspeed ({})", set.SpeedUnits.shortName()).c_str(), ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn(std::format("Ground Speed ({})", set.SpeedUnits.shortName()).c_str(), ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn(std::format("Corrected Net Thrust / Engine ({})", set.ThrustUnits.shortName()).c_str(), ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Bank Angle", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn(std::format("Fuel Flow ({})", set.FuelFlowUnits.shortName()).c_str(), ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            for (auto it = perfOut.begin(); it != perfOut.end(); ++it)
            {
                const auto& [CumGroundDist, Pt] = *it;
                const std::size_t i = std::distance(perfOut.begin(), it);

                ImGui::TableNextRow();

                // # Point
                UI::tableNextColumn(false);
                UI::textInfo(std::format("{}", i + 1));

                // Origin
                UI::tableNextColumn(false);
                UI::textInfo(PerformanceOutput::Origins.toString(Pt.PtOrigin));

                // Time
                UI::tableNextColumn(false);
                UI::textInfo(timeToUtcString(Pt.Time));

                // Flight Phase
                UI::tableNextColumn(false);
                UI::textInfo(FlightPhases.toString(Pt.FlPhase));

                // Cumulative GroundDistance
                UI::tableNextColumn(false);
                UI::textInfo(std::format("{0:.{1}f}", set.DistanceUnits.fromSi(CumGroundDist), set.DistanceUnits.decimals()));

                // Longitude
                UI::tableNextColumn(false);
                UI::textInfo(std::format("{:.6f}", Pt.Longitude));

                // Latitude
                UI::tableNextColumn(false);
                UI::textInfo(std::format("{:.6f}", Pt.Latitude));

                // Altitude MSL
                UI::tableNextColumn(false);
                UI::textInfo(std::format("{0:.{1}f}", set.AltitudeUnits.fromSi(Pt.AltitudeMsl), set.AltitudeUnits.decimals()));

                // True Airspeed
                UI::tableNextColumn(false);
                UI::textInfo(std::format("{0:.{1}f}", set.SpeedUnits.fromSi(Pt.TrueAirspeed), set.SpeedUnits.decimals()));

                // Ground Speed
                UI::tableNextColumn(false);
                UI::textInfo(std::format("{0:.{1}f}", set.SpeedUnits.fromSi(Pt.Groundspeed), set.SpeedUnits.decimals()));

                // Corrected Net Thrust Per Engine
                UI::tableNextColumn(false);
                UI::textInfo(std::format("{0:.{1}f}", set.ThrustUnits.fromSi(Pt.CorrNetThrustPerEng), set.ThrustUnits.decimals()));

                // Bank Angle
                UI::tableNextColumn(false);
                UI::textInfo(std::format("{:.0f}", Pt.BankAngle));

                // Fuel Flow Per Engine
                UI::tableNextColumn(false);
                UI::textInfo(std::format("{0:.{1}f}", set.FuelFlowUnits.fromSi(Pt.FuelFlowPerEng), set.FuelFlowUnits.decimals()));
            }
            UI::endTable();
        }
    }

    void ScenariosPanel::drawSelectedNoiseSingleEventOutput() const {
        GRAPE_ASSERT(m_SelectedNoiseRun);
        GRAPE_ASSERT(m_SelectedNoiseSingleEventOutput);

        const NoiseRunOutput& nsOut = m_SelectedNoiseRun->output();
        const ReceptorOutput& receptOutput = nsOut.receptors();
        const NoiseSingleEventOutput& nsSingleOut = *m_SelectedNoiseSingleEventOutput;
        GRAPE_ASSERT(receptOutput.size() == nsSingleOut.size());

        const Settings& set = Application::settings();

        if (UI::beginTable("Noise Single Event Output", 6))
        {
            ImGui::TableSetupColumn("Receptor", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Longitude", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Latitude", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn(std::format("Elevation ({})", set.AltitudeUnits.shortName()).c_str(), ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("LaMax (dB)", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("SEL (dB)", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            ImGuiListClipper clipper;
            clipper.Begin(static_cast<int>(receptOutput.size()));
            while (clipper.Step())
            {
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
                {
                    const auto& recept = receptOutput(row);
                    const auto& [lamax, sel] = nsSingleOut.values(row);

                    ImGui::TableNextRow();

                    // Receptor
                    UI::tableNextColumn(false);
                    UI::textInfo(recept.Name);

                    // Longitude
                    UI::tableNextColumn(false);
                    UI::textInfo(std::format("{:.6f}", recept.Longitude));

                    // Latitude
                    UI::tableNextColumn(false);
                    UI::textInfo(std::format("{:.6f}", recept.Latitude));

                    // Elevation
                    UI::tableNextColumn(false);
                    UI::textInfo(std::format("{0:.{1}f}", set.AltitudeUnits.fromSi(recept.Elevation), set.AltitudeUnits.decimals()));

                    // Lamax
                    UI::tableNextColumn(false);
                    UI::textInfo(std::format("{:.2f}", lamax));

                    // SEL
                    UI::tableNextColumn(false);
                    UI::textInfo(std::format("{:.2f}", sel));
                }
            }
            UI::endTable();
        }
    }

    void ScenariosPanel::drawSelectedNoiseCumulativeOutput() const {
        GRAPE_ASSERT(m_SelectedNoiseRun);
        GRAPE_ASSERT(m_SelectedNoiseCumulativeMetricOutput);
        GRAPE_ASSERT(m_SelectedNoiseCumulativeOutput);

        const NoiseRunOutput& nsOut = m_SelectedNoiseRun->output();
        const ReceptorOutput& receptOutput = nsOut.receptors();
        const NoiseCumulativeMetric& nsCumMetric = *m_SelectedNoiseCumulativeMetricOutput;
        const NoiseCumulativeOutput& nsCumOut = *m_SelectedNoiseCumulativeOutput;

        GRAPE_ASSERT(receptOutput.size() == nsCumOut.MaximumAbsolute.size());
        GRAPE_ASSERT(receptOutput.size() == nsCumOut.MaximumAverage.size());
        GRAPE_ASSERT(receptOutput.size() == nsCumOut.Exposure.size());
        for (const auto& nat : nsCumOut.NumberAboveThresholds)
            GRAPE_ASSERT(receptOutput.size() == nat.size());

        const Settings& set = Application::settings();

        const std::size_t tableSize = 9 + nsCumOut.NumberAboveThresholds.size();
        if (UI::beginTable("Noise Cumulative Output", static_cast<int>(tableSize)))
        {
            ImGui::TableSetupColumn("Receptor", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Longitude", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Latitude", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn(std::format("Elevation ({})", set.AltitudeUnits.shortName()).c_str(), ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Count", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Weighted Count", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Maximum Absolute (dB)", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Maximum Average (dB)", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Exposure (dB)", ImGuiTableColumnFlags_NoHide);
            for (const auto threshold : nsCumMetric.numberAboveThresholds())
                ImGui::TableSetupColumn(std::format("NA {:.2f} dB", threshold).c_str(), ImGuiTableColumnFlags_NoHide);

            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            ImGuiListClipper clipper;
            clipper.Begin(static_cast<int>(receptOutput.size()));
            while (clipper.Step())
            {
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
                {
                    const auto& recept = receptOutput(row);
                    const auto count = nsCumOut.Count.at(row);
                    const auto countWeighted = nsCumOut.CountWeighted.at(row);
                    const auto maxAbsolute = nsCumOut.MaximumAbsolute.at(row);
                    const auto maxAverage = nsCumOut.MaximumAverage.at(row);
                    const auto exposure = nsCumOut.Exposure.at(row);

                    ImGui::TableNextRow();

                    // Receptor
                    UI::tableNextColumn(false);
                    UI::textInfo(recept.Name);

                    // Longitude
                    UI::tableNextColumn(false);
                    UI::textInfo(std::format("{:.6f}", recept.Longitude));

                    // Latitude
                    UI::tableNextColumn(false);
                    UI::textInfo(std::format("{:.6f}", recept.Latitude));

                    // Elevation
                    UI::tableNextColumn(false);
                    UI::textInfo(std::format("{0:.{1}f}", set.AltitudeUnits.fromSi(recept.Elevation), set.AltitudeUnits.decimals()));

                    // Count
                    UI::tableNextColumn(false);
                    UI::textInfo(std::format("{0:2f}", count));

                    // Count Weighted
                    UI::tableNextColumn(false);
                    UI::textInfo(std::format("{0:2f}", countWeighted));

                    // Maximum Absolute
                    UI::tableNextColumn(false);
                    UI::textInfo(std::format("{:.2f}", maxAbsolute));

                    // Maximum Average
                    UI::tableNextColumn(false);
                    UI::textInfo(std::format("{:.2f}", maxAverage));

                    // Exposure
                    UI::tableNextColumn(false);
                    UI::textInfo(std::format("{:.2f}", exposure));

                    // Number Above
                    for (const auto& nat : nsCumOut.NumberAboveThresholds)
                    {
                        UI::tableNextColumn(false);
                        UI::textInfo(std::format("{:.1f}", nat.at(row)));
                    }
                }
            }
            UI::endTable();
        }
    }

    void ScenariosPanel::drawSelectedEmissionsSegmentOutput() const {
        GRAPE_ASSERT(m_SelectedEmissionsRun);
        GRAPE_ASSERT(m_SelectedEmissionsSegmentOutput);

        const auto& emiRun = *m_SelectedEmissionsRun;
        const EmissionsOperationOutput& emiOut = *m_SelectedEmissionsSegmentOutput;
        const auto& emiSegOutVec = emiOut.segmentOutput();

        const Settings& set = Application::settings();

        if (UI::beginTable("Emissions Output", 7))
        {
            if (emiRun.EmissionsRunSpec.EmissionsMdl == EmissionsModel::LTOCycle)
                ImGui::TableSetupColumn("LTO Phase", ImGuiTableColumnFlags_NoHide);
            else
                ImGui::TableSetupColumn("Segment Number", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn(std::format("Fuel ({})", set.WeightUnits.shortName()).c_str());
            ImGui::TableSetupColumn(std::format("HC ({})", set.EmissionsWeightUnits.shortName()).c_str());
            ImGui::TableSetupColumn(std::format("CO ({})", set.EmissionsWeightUnits.shortName()).c_str());
            ImGui::TableSetupColumn(std::format("NOx ({})", set.EmissionsWeightUnits.shortName()).c_str());
            ImGui::TableSetupColumn(std::format("nvPM Mass ({})", set.EmissionsWeightUnits.shortName()).c_str());
            ImGui::TableSetupColumn("nvPM Number (#)");
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            ImGuiListClipper clipper;
            clipper.Begin(static_cast<int>(emiSegOutVec.size()));
            while (clipper.Step())
            {
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
                {
                    const auto& segOut = emiSegOutVec.at(row);

                    ImGui::TableNextRow();

                    // LTO Phase / Segment Number
                    UI::tableNextColumn(false);
                    if (emiRun.EmissionsRunSpec.EmissionsMdl == EmissionsModel::LTOCycle)
                        UI::textInfo(LTOPhases.Strings.at(segOut.Index));
                    else
                        UI::textInfo(std::format("{}", segOut.Index));

                    // Fuel
                    UI::tableNextColumn(false);
                    UI::textInfo(std::format("{0:.{1}f}", set.WeightUnits.fromSi(segOut.Fuel), set.WeightUnits.decimals()));

                    // HC
                    UI::tableNextColumn(false);
                    UI::textInfo(std::format("{0:.{1}f}", set.EmissionsWeightUnits.fromSi(segOut.Emissions.HC), set.EmissionsWeightUnits.decimals()));

                    // CO
                    UI::tableNextColumn(false);
                    UI::textInfo(std::format("{0:.{1}f}", set.EmissionsWeightUnits.fromSi(segOut.Emissions.CO), set.EmissionsWeightUnits.decimals()));

                    // NOx
                    UI::tableNextColumn(false);
                    UI::textInfo(std::format("{0:.{1}f}", set.EmissionsWeightUnits.fromSi(segOut.Emissions.NOx), set.EmissionsWeightUnits.decimals()));

                    // nvPM
                    UI::tableNextColumn(false);
                    UI::textInfo(std::format("{0:.{1}f}", set.EmissionsWeightUnits.fromSi(segOut.Emissions.nvPM), set.EmissionsWeightUnits.decimals()));

                    // nvPM number
                    UI::tableNextColumn(false);
                    UI::textInfo(std::format("{0:.3e}", segOut.Emissions.nvPMNumber));
                }
            }
            UI::endTable();
        }
    }
}
