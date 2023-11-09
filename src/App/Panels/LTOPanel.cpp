// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "LTOPanel.h"

#include "Application.h"
#include "UI.h"

namespace GRAPE {
    LTOPanel::LTOPanel() : Panel("LTO Engines") {}

    void LTOPanel::select(LTOEngine& LTOEng) {
        if (isSelected(LTOEng))
            return;

        if (!ImGui::GetIO().KeyCtrl)
            m_SelectedLTOEngines.clear();

        m_SelectedLTOEngines.emplace_back(&LTOEng);
    }

    void LTOPanel::deselect(LTOEngine& LTOEng) {
        std::erase(m_SelectedLTOEngines, &LTOEng);
    }

    void LTOPanel::eraseSelected() {
        for (const auto ltoEng : m_SelectedLTOEngines)
            Application::study().LTOEngines.erase(*ltoEng);
        m_SelectedLTOEngines.clear();
    }

    bool LTOPanel::isSelected(LTOEngine& LTOEng) const {
        return std::ranges::find(m_SelectedLTOEngines, &LTOEng) != m_SelectedLTOEngines.end();
    }

    void LTOPanel::reset() {
        m_SelectedLTOEngines.clear();
    }

    void LTOPanel::imGuiDraw() {
        if (!isOpen())
            return;

        auto& study = Application::study();
        const Settings& set = Application::settings();

        std::function<void()> action = nullptr;

        // Initialize
        ImGui::Begin(m_Name.c_str(), &m_Open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse);

        // Left side
        ImGui::BeginChild("Left Side", ImVec2(-ImGui::GetContentRegionAvail().x * UI::g_StandardLeftAlignFraction, 0.0f));

        // Filter
        static UI::TextFilter filter;
        filter.draw();

        // Edit Popup
        UI::buttonEditRight();
        if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonLeft))
        {
            if (UI::selectableNew("LTO Engine"))
                study.LTOEngines.addLTOEngine();

            if (UI::selectableDelete("All"))
            {
                m_SelectedLTOEngines.clear();
                Application::get().queueAsyncTask([&] { study.LTOEngines.eraseAll(); }, "Deleting all LTO engines");
            }

            ImGui::EndPopup();
        }

        // LTO Engine Names
        if (UI::beginTable("LTO Engines", 1))
        {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            for (auto& [ltoEngineId, ltoEngine] : study.LTOEngines())
            {
                if (!filter.passesFilter(ltoEngineId))
                    continue;

                ImGui::TableNextRow();
                ImGui::PushID(ltoEngineId.c_str());

                UI::tableNextColumn(false);
                if (UI::selectableRowEmpty(isSelected(ltoEngine)))
                    select(ltoEngine);

                if (ImGui::BeginPopupContextItem())
                {
                    ImGui::BeginDisabled(study.Blocks.notRemovable(ltoEngine));
                    if (UI::selectableDelete())
                    {
                        action = [&] { eraseSelected(); };
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndDisabled(); // Not removable
                    ImGui::EndPopup();
                }

                // Name
                ImGui::BeginDisabled(study.Blocks.notEditable(ltoEngine));
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (UI::inputText("LTOEngineId", ltoEngine.Name, ltoEngine.Name != ltoEngineId && study.LTOEngines().contains(ltoEngine.Name), "LTO Engine Name", std::format("The LTO Engine '{}' already exists in this study", ltoEngine.Name)))
                    if (ltoEngine.Name != ltoEngineId)
                        action = [&] { study.LTOEngines.updateKey(ltoEngine, ltoEngineId); };
                if (UI::isItemClicked())
                    select(ltoEngine);

                ImGui::EndDisabled(); // Not editable
                ImGui::PopID(); // LTO Engine ID
            }
            UI::endTable();
        }
        ImGui::EndChild();

        ImGui::SameLine();

        // Selected Data
        ImGui::BeginChild("Right Side");

        if (!m_SelectedLTOEngines.empty())
        {
            auto& selectedLTOEngine = *m_SelectedLTOEngines.front();

            bool update = false;

            ImGui::BeginDisabled(study.Blocks.notEditable(selectedLTOEngine));

            // Maximum Sea Level Static Thrust
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Maximum sea level static thrust:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
            if (UI::inputDouble("Maximum sea level static thrust", selectedLTOEngine.MaximumSeaLevelStaticThrust, 1.0, Constants::NaN, set.ThrustUnits))
                update = true;

            // Mixed Nozzle Engine
            ImGui::SameLine(0.0, UI::g_StandardItemWidth);
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Mixed Nozzle:");
            ImGui::SameLine();
            if (ImGui::Checkbox("##MixedNozzle", &selectedLTOEngine.MixedNozzle))
                update = true;

            // Bypass Ratio
            if (selectedLTOEngine.MixedNozzle)
            {
                ImGui::SameLine(0.0, UI::g_StandardItemWidth);
                ImGui::AlignTextToFramePadding();
                ImGui::TextDisabled("Bypass Ratio:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
                if (UI::inputDouble("Bypass ratio", selectedLTOEngine.BypassRatio, 0.0, Constants::NaN, 2))
                    update = true;
            }

            if (UI::beginTable("LTO Engine Values", 10))
            {
                ImGui::TableSetupColumn("LTO Stage", ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn(std::format("Fuel Flow ({})", set.FuelFlowUnits.shortName()).c_str(), ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn("Fuel Flow Correction Factor", ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn(std::format("HC EI ({})", set.EmissionIndexUnits.shortName()).c_str(), ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn(std::format("CO EI ({})", set.EmissionIndexUnits.shortName()).c_str(), ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn(std::format("NOx EI ({})", set.EmissionIndexUnits.shortName()).c_str(), ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn("Air to Fuel Ratio", ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn("Smoke Number", ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn("nvPM EI (mg/kg)", ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn("nvPM Number EI (#/kg)", ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableHeadersRow();

                for (std::size_t i = 0; i < LTOPhases.size(); ++i)
                {
                    ImGui::TableNextRow();
                    ImGui::PushID(LTOPhases.Strings.at(i));

                    UI::tableNextColumn();
                    UI::textInfo(LTOPhases.Strings.at(i));

                    UI::tableNextColumn();
                    if (UI::inputDouble("Fuel flow", selectedLTOEngine.FuelFlows.at(i), set.FuelFlowUnits, false))
                        update = true;

                    UI::tableNextColumn();
                    if (UI::inputDouble("Fuel flow correction factor", selectedLTOEngine.FuelFlowCorrectionFactors.at(i)))
                        update = true;

                    UI::tableNextColumn();
                    if (UI::inputDouble("HC EI", selectedLTOEngine.EmissionIndexesHC.at(i), set.EmissionIndexUnits, false))
                        update = true;

                    UI::tableNextColumn();
                    if (UI::inputDouble("CO EI", selectedLTOEngine.EmissionIndexesCO.at(i), set.EmissionIndexUnits, false))
                        update = true;

                    UI::tableNextColumn();
                    if (UI::inputDouble("NOx EI", selectedLTOEngine.EmissionIndexesNOx.at(i), set.EmissionIndexUnits, false))
                        update = true;

                    UI::tableNextColumn();
                    if (UI::inputDouble("AFR", selectedLTOEngine.AirFuelRatios.at(i), 0.0, Constants::NaN, true, 1, ""))
                        update = true;

                    UI::tableNextColumn();
                    if (UI::inputDouble("Smoke Number", selectedLTOEngine.SmokeNumbers.at(i), 0.0, Constants::NaN, false, 1, ""))
                        update = true;

                    UI::tableNextColumn();
                    double eiNvpmConv = toMilligramsPerKilogram(selectedLTOEngine.EmissionIndexesNVPM.at(i));
                    if (UI::inputDouble("nvPM EI", eiNvpmConv, 0.0, Constants::NaN, false, 2, ""))
                    {
                        selectedLTOEngine.EmissionIndexesNVPM.at(i) = fromMilligramsPerKilogram(eiNvpmConv);
                        update = true;
                    }

                    UI::tableNextColumn();
                    if (UI::inputDoubleScientific("nvPM number EI", selectedLTOEngine.EmissionIndexesNVPMNumber.at(i), 0.0, Constants::NaN, false, 3, ""))
                        update = true;

                    ImGui::PopID(); // LTO Phase
                }
                UI::endTable();
            }

            ImGui::EndDisabled(); // Not editable

            if (update)
                study.LTOEngines.update(selectedLTOEngine);
        }
        ImGui::EndChild();

        ImGui::End();

        // Actions outside loops
        if (action)
            action();
    }
}
