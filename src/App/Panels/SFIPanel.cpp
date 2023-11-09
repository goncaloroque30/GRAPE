// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "SFIPanel.h"

#include "Application.h"
#include "UI.h"

namespace GRAPE {
    SFIPanel::SFIPanel() : Panel("SFI Fuel") {}

    void SFIPanel::select(SFI& Sfi) {
        if (isSelected(Sfi))
            return;

        if (!ImGui::GetIO().KeyCtrl)
            m_SelectedSFI.clear();

        m_SelectedSFI.emplace_back(&Sfi);
    }

    void SFIPanel::deselect(SFI& Sfi) {
        std::erase(m_SelectedSFI, &Sfi);
    }

    void SFIPanel::eraseSelected() {
        for (const auto sfi : m_SelectedSFI)
            Application::study().SFIs.erase(*sfi);
        m_SelectedSFI.clear();
    }

    bool SFIPanel::isSelected(SFI& Sfi) const {
        return std::ranges::find(m_SelectedSFI, &Sfi) != m_SelectedSFI.end();
    }

    void SFIPanel::reset() {
        m_SelectedSFI.clear();
    }

    void SFIPanel::imGuiDraw() {
        if (!isOpen())
            return;

        auto& study = Application::study();
        const auto& set = Application::settings();

        std::function<void()> action = nullptr;

        // Initialize
        ImGui::Begin(m_Name.c_str(), &m_Open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse);

        static UI::TextFilter filter;
        filter.draw();

        UI::buttonEditRight();
        if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonLeft))
        {
            if (UI::selectableNew("SFI"))
                study.SFIs.addSFI();

            if (UI::selectableDelete("All"))
            {
                m_SelectedSFI.clear();
                Application::get().queueAsyncTask([&] { study.SFIs.eraseAll(); }, "Deleting all SFI coefficients");
            }

            ImGui::EndPopup();
        }

        if (UI::beginTable("SFI", 10))
        {
            ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn(std::format("Maximum Sea Level Static Thrust ({})", set.ThrustUnits.shortName()).c_str());
            ImGui::TableSetupColumn("A (kg/min/kN)", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("B1 (kg/min/kN)", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("B2 (kg/min/kN)", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("B3", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("K1 (kg/min/kN)", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("K2 (kg/min/kN)", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("K3 (kg/min/kN/ft)", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("K4 (kg/min/kN/lbf)");
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            for (auto& [sfiId, sfi] : study.SFIs())
            {
                if (!filter.passesFilter(sfiId))
                    continue;

                bool update = false;

                ImGui::TableNextRow();
                ImGui::PushID(sfiId.c_str());
                ImGui::BeginDisabled(study.Blocks.notEditable(sfi));

                UI::tableNextColumn(false);
                if (UI::selectableRowEmpty(isSelected(sfi)))
                    select(sfi);

                if (ImGui::BeginPopupContextItem())
                {
                    if (UI::selectableDelete())
                    {
                        action = [&] { eraseSelected(); };
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }

                // Name
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (UI::inputText("SFIId", sfi.Name, sfi.Name != sfiId && study.SFIs().contains(sfi.Name), "SFI ID", std::format("SFI ID '{}' already exists in this study", sfi.Name)))
                    if (sfi.Name != sfiId)
                        action = [&] { study.SFIs.updateKey(sfi, sfiId); };
                if (UI::isItemClicked())
                    select(sfi);

                // Maximum Sea Level Static Thrust
                UI::tableNextColumn();
                if (UI::inputDouble("Maximum sea level static thrust", sfi.MaximumSeaLevelStaticThrust, 1.0, Constants::NaN, set.ThrustUnits, false))
                    update = true;
                if (UI::isItemClicked())
                    select(sfi);

                // A
                UI::tableNextColumn();
                double a = sfi.A / toMinutes(1.0) / 1000.0; // kN to N
                if (UI::inputDouble("A", a, 2))
                {
                    sfi.A = a / fromMinutes(1.0) / 0.001; // N to kN
                    update = true;
                }
                if (UI::isItemClicked())
                    select(sfi);

                // B1
                UI::tableNextColumn();
                double b1 = sfi.B1 / toMinutes(1.0) / 1000.0; // kN to N
                if (UI::inputDouble("B1", b1, 2))
                {
                    sfi.B1 = b1 / fromMinutes(1.0) / 0.001; // N to kN
                    update = true;
                }
                if (UI::isItemClicked())
                    select(sfi);

                // B2
                UI::tableNextColumn();
                double b2 = sfi.B2 / toMinutes(1.0) / 1000.0; // kN to N
                if (UI::inputDouble("B2", b2, 2))
                {
                    sfi.B2 = b2 / fromMinutes(1.0) / 0.001; // N to kN
                    update = true;
                }
                if (UI::isItemClicked())
                    select(sfi);

                // B3
                UI::tableNextColumn();
                if (UI::inputDouble("B3", sfi.B3, 2))
                    update = true;
                if (UI::isItemClicked())
                    select(sfi);

                // K1
                UI::tableNextColumn();
                double k1 = sfi.K1 / toMinutes(1.0) / 1000.0; // kN to N
                if (UI::inputDouble("K1", k1, 2))
                {
                    sfi.K1 = k1 / fromMinutes(1.0) / 0.001; // N to kN
                    update = true;
                }
                if (UI::isItemClicked())
                    select(sfi);

                // K2
                UI::tableNextColumn();
                double k2 = sfi.K2 / toMinutes(1.0) / 1000.0; // kN to N
                if (UI::inputDouble("K2", k2, 2))
                {
                    sfi.K2 = k2 / fromMinutes(1.0) / 0.001; // N to kN
                    update = true;
                }
                if (UI::isItemClicked())
                    select(sfi);

                // K3
                UI::tableNextColumn();
                double k3 = sfi.K3 / toMinutes(1.0) / 1000.0 / toFeet(1.0); // kN to N
                if (UI::inputDouble("K3", k3, 2))
                {
                    sfi.K3 = k3 / fromMinutes(1.0) / 0.001 / fromFeet(1.0); // N to kN
                    update = true;
                }
                if (UI::isItemClicked())
                    select(sfi);

                // K4
                UI::tableNextColumn();
                double k4 = sfi.K4 / toMinutes(1.0) / 1000.0 / toPoundsOfForce(1.0); // kN to N
                if (UI::inputDouble("K4", k4, 2))
                {
                    sfi.K4 = k4 / fromMinutes(1.0) / 0.001 / fromPoundsOfForce(1.0); // N to kN
                    update = true;
                }
                if (UI::isItemClicked())
                    select(sfi);

                if (update)
                    study.SFIs.update(sfi);

                ImGui::EndDisabled(); // Not editable
                ImGui::PopID(); // SFI ID
            }
            UI::endTable();
        }
        ImGui::End();

        // Actions outside loops
        if (action)
            action();
    }
}
