// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "FleetPanel.h"

#include "Application.h"
#include "UI.h"

namespace GRAPE {
    FleetPanel::FleetPanel() : Panel("Fleet") {}

    void FleetPanel::select(Aircraft& Acft) {
        if (isSelected(Acft))
            return;

        if (!ImGui::GetIO().KeyCtrl)
            m_SelectedAircraft.clear();

        m_SelectedAircraft.emplace_back(&Acft);
    }

    void FleetPanel::deselect(Aircraft& Acft) { std::erase(m_SelectedAircraft, &Acft); }

    void FleetPanel::eraseSelected() {
        for (const auto acft : m_SelectedAircraft)
            Application::study().Aircrafts.erase(*acft);
        m_SelectedAircraft.clear();
    }

    bool FleetPanel::isSelected(Aircraft& Acft) const { return std::ranges::find(m_SelectedAircraft, &Acft) != m_SelectedAircraft.end(); }

    void FleetPanel::reset() { m_SelectedAircraft.clear(); }

    void FleetPanel::imGuiDraw() {
        if (!isOpen())
            return;

        auto& study = Application::study();
        const Settings& set = Application::settings();

        std::function<void()> action = nullptr;

        // Initialize
        ImGui::Begin(m_Name.c_str(), &m_Open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse);

        static UI::TextFilter filter;
        filter.draw();

        UI::buttonEditRight();
        if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonLeft))
        {
            if (UI::selectableNew("Fleet ID"))
                study.Aircrafts.addAircraft();

            if (UI::selectableDelete("All"))
            {
                m_SelectedAircraft.clear();
                Application::get().queueAsyncTask([&] { study.Aircrafts.eraseAircrafts(); }, "Deleting fleet");
            }

            ImGui::EndPopup();
        }

        if (UI::beginTable("Fleet", 8))
        {
            ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("# Engines", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Doc29 Aircraft", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("SFI", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("LTO Engine", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Doc29 Noise", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Doc29 Noise Delta Arrivals (dB)", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Doc29 Noise Delta Departures (dB)", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            for (auto& [fleetId, acft] : study.Aircrafts())
            {
                if (!filter.passesFilter(fleetId))
                    continue;

                ImGui::TableNextRow();
                ImGui::PushID(fleetId.c_str());
                ImGui::BeginDisabled(study.Blocks.notEditable(acft));

                UI::tableNextColumn(false);
                if (UI::selectableRowEmpty(isSelected(acft)))
                    select(acft);

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
                if (UI::inputText("FleetId", acft.Name, acft.Name != fleetId && study.Aircrafts().contains(acft.Name), "Fleet ID", std::format("Fleet ID '{}' already exists in this study", acft.Name)))
                    if (acft.Name != fleetId)
                        action = [&] { study.Aircrafts.updateKey(acft, fleetId); };
                if (ImGui::IsItemClicked())
                    select(acft);

                // # Engines
                UI::tableNextColumn();
                ImGui::SliderInt("##Engines", &acft.EngineCount, 1, 4, "%d", ImGuiSliderFlags_AlwaysClamp);
                if (ImGui::IsItemDeactivatedAfterEdit())
                    study.Aircrafts.update(acft);
                if (ImGui::IsItemClicked())
                    select(acft);

                // Doc29 Aircraft
                UI::tableNextColumn();
                const char* currDoc29AcftName = acft.validDoc29Performance() ? acft.Doc29Acft->Name.c_str() : "";
                if (ImGui::BeginCombo("##Doc29Aircraft", currDoc29AcftName))
                {
                    for (const auto& [doc29AcftId, doc29Acft] : study.Doc29Aircrafts())
                    {
                        const bool selected = acft.validDoc29Performance() && &doc29Acft == acft.Doc29Acft;
                        if (ImGui::Selectable(doc29AcftId.c_str(), selected))
                            study.Aircrafts.setDoc29Performance(acft, &doc29Acft);

                        if (selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    if (ImGui::Selectable("##None", !acft.validDoc29Performance()))
                        study.Aircrafts.setDoc29Performance(acft, nullptr);

                    ImGui::EndCombo();
                }
                if (ImGui::IsItemClicked())
                    select(acft);

                // SFI Fuel
                UI::tableNextColumn();
                const char* currSFI = acft.validSFI() ? acft.SFIFuel->Name.c_str() : "";
                if (ImGui::BeginCombo("##SFIFuel", currSFI))
                {
                    for (const auto& [sfiId, sfi] : study.SFIs())
                    {
                        const bool selected = acft.validSFI() && &sfi == acft.SFIFuel;
                        if (ImGui::Selectable(sfiId.c_str(), selected))
                            study.Aircrafts.setSFI(acft, &sfi);

                        if (selected)
                            ImGui::SetItemDefaultFocus();
                    }

                    if (ImGui::Selectable("##None", !acft.validSFI()))
                        study.Aircrafts.setSFI(acft, nullptr);

                    ImGui::EndCombo();
                }
                if (ImGui::IsItemClicked())
                    select(acft);

                // LTO Engine
                UI::tableNextColumn();
                const char* currLTOEng = acft.validLTOEngine() ? acft.LTOEng->Name.c_str() : "";
                if (ImGui::BeginCombo("##LTOEngine", currLTOEng))
                {
                    for (const auto& [ltoEngId, ltoEng] : study.LTOEngines())
                    {
                        const bool selected = acft.validLTOEngine() && &ltoEng == acft.LTOEng;
                        if (ImGui::Selectable(ltoEngId.c_str(), selected))
                            study.Aircrafts.setLTO(acft, &ltoEng);

                        if (selected)
                            ImGui::SetItemDefaultFocus();
                    }

                    if (ImGui::Selectable("##None", !acft.validLTOEngine()))
                        study.Aircrafts.setLTO(acft, nullptr);

                    ImGui::EndCombo();
                }
                if (ImGui::IsItemClicked())
                    select(acft);

                // Doc29 Noise
                UI::tableNextColumn();
                const char* currDoc29NsName = acft.validDoc29Noise() ? acft.Doc29Ns->Name.c_str() : "";
                if (ImGui::BeginCombo("##Doc29Noise", currDoc29NsName))
                {
                    for (const auto& [doc29NsId, doc29Ns] : study.Doc29Noises())
                    {
                        const bool selected = acft.validDoc29Noise() && &doc29Ns == acft.Doc29Ns;
                        if (ImGui::Selectable(doc29NsId.c_str(), selected))
                            study.Aircrafts.setDoc29Noise(acft, &doc29Ns);

                        if (selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    if (ImGui::Selectable("##None", !acft.validDoc29Noise()))
                        study.Aircrafts.setDoc29Noise(acft, nullptr);

                    ImGui::EndCombo();
                }
                if (ImGui::IsItemClicked())
                    select(acft);

                // Doc29 Noise Delta Arrivals
                UI::tableNextColumn();
                if (UI::inputDouble("Noise Delta Arrivals", acft.Doc29NoiseDeltaArrivals, static_cast<std::size_t>(2)))
                    study.Aircrafts.update(acft);
                if (ImGui::IsItemClicked())
                    select(acft);

                // Doc29 Noise Delta Departures
                UI::tableNextColumn();
                if (UI::inputDouble("Noise Delta Departures", acft.Doc29NoiseDeltaDepartures, static_cast<std::size_t>(2)))
                    study.Aircrafts.update(acft);
                if (ImGui::IsItemClicked())
                    select(acft);

                ImGui::EndDisabled(); // Not editable
                ImGui::PopID();       // Fleet ID
            }
            UI::endTable();
        }
        ImGui::End();

        if (action)
            action();
    }
}
