// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "FlightsPanel.h"

#include "Application.h"
#include "UI.h"

namespace GRAPE {
    FlightsPanel::FlightsPanel() : Panel("Flights") {}

    void FlightsPanel::select(FlightArrival& FlightArr) {
        if (isSelected(FlightArr))
            return;

        if (!ImGui::GetIO().KeyCtrl)
            m_SelectedArrivals.clear();

        m_SelectedArrivals.emplace_back(&FlightArr);
    }

    void FlightsPanel::select(FlightDeparture& FlightDep) {
        if (isSelected(FlightDep))
            return;

        if (!ImGui::GetIO().KeyCtrl)
            m_SelectedDepartures.clear();

        m_SelectedDepartures.emplace_back(&FlightDep);
    }

    void FlightsPanel::deselect(FlightArrival& FlightArr) { std::erase(m_SelectedArrivals, &FlightArr); }
    void FlightsPanel::deselect(FlightDeparture& FlightDep) { std::erase(m_SelectedDepartures, &FlightDep); }

    void FlightsPanel::eraseSelectedArrivals() {
        for (const auto& flightArr : m_SelectedArrivals)
            Application::study().Operations.erase(*flightArr);

        m_SelectedArrivals.clear();
    }

    void FlightsPanel::eraseSelectedDepartures() {
        for (const auto& flightDep : m_SelectedDepartures)
            Application::study().Operations.erase(*flightDep);

        m_SelectedDepartures.clear();
    }

    bool FlightsPanel::isSelected(FlightArrival& FlightArr) const { return std::ranges::find(m_SelectedArrivals, &FlightArr) != m_SelectedArrivals.end(); }

    bool FlightsPanel::isSelected(FlightDeparture& FlightDep) const { return std::ranges::find(m_SelectedDepartures, &FlightDep) != m_SelectedDepartures.end(); }

    void FlightsPanel::reset() {
        m_SelectedArrivals.clear();
        m_SelectedDepartures.clear();
    }

    void FlightsPanel::imGuiDraw() {
        if (!isOpen())
            return;

        auto& study = Application::study();
        std::function<void()> action = nullptr;

        const Settings& set = Application::settings();

        auto& arrFlights = study.Operations.flightArrivals();
        auto& depFlights = study.Operations.flightDepartures();

        ImGui::Begin(m_Name.c_str(), &m_Open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse);

        // Arrivals
        ImGui::PushID("Arrivals");
        ImGui::AlignTextToFramePadding();
        UI::textInfo("Arrivals");

        // Filter
        ImGui::SameLine();
        static UI::TextFilter arrivalsFilter;
        arrivalsFilter.draw();

        // Edit Button
        ImGui::SameLine();
        UI::buttonEditRight();
        if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonLeft))
        {
            if (UI::selectableNew("Arrival", true, ImGuiSelectableFlags_DontClosePopups))
                study.Operations.addArrivalFlight();

            if (UI::selectableDelete("All"))
            {
                m_SelectedArrivals.clear();
                Application::get().queueAsyncTask([&] { study.Operations.eraseFlightArrivals(); }, "Deleting all arrival flights");
            }

            ImGui::EndPopup();
        }

        const float tableHeight = UI::getTableHeight(arrFlights.size(), true, ImGui::GetContentRegionAvail().y / 2.0f);
        if (UI::beginTable("Arrivals", 9, ImGuiTableFlags_None, ImVec2(0.0f, tableHeight)))
        {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Airport", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Runway", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Route", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Count", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Fleet ID", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn(std::format("Weight ({})", Application::settings().WeightUnits.shortName()).c_str(), ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Doc29 Profile", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            for (auto& [arrId, arr] : arrFlights)
            {
                if (!arrivalsFilter.passesFilter(arrId))
                    continue;

                ImGui::TableNextRow();
                ImGui::PushID(arrId.c_str());
                ImGui::BeginDisabled(study.Blocks.notEditable(arr));

                // Selectable Row
                UI::tableNextColumn(false);
                if (UI::selectableRowEmpty(isSelected(arr)))
                    select(arr);
                if (ImGui::BeginPopupContextItem())
                {
                    if (UI::selectableDelete())
                    {
                        action = [&] { eraseSelectedArrivals(); };
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }

                // Name
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (UI::inputText("Arrival Name", arr.Name, arr.Name != arrId && arrFlights.contains(arr.Name), "Arrival Name", fmt::format("Arrival flight '{}' already exists in this study.", arr.Name)))
                    if (arr.Name != arrId)
                        action = [&] { study.Operations.updateKey(arr, arrId); };
                if (ImGui::IsItemClicked())
                    select(arr);

                if (arr.hasRoute())
                {
                    // Airport
                    UI::tableNextColumn(false);
                    UI::textInfo(arr.route().parentAirport().Name);

                    // Runway
                    UI::tableNextColumn(false);
                    UI::textInfo(arr.route().parentRunway().Name);
                }
                else
                {
                    UI::tableNextColumn(false);
                    UI::tableNextColumn(false);
                }

                // Route
                UI::tableNextColumn();
                const char* currRteName = arr.hasRoute() ? arr.route().Name.c_str() : "";
                if (ImGui::BeginCombo("##Route", currRteName))
                {
                    for (const auto& apt : study.Airports() | std::views::values)
                    {
                        if (ImGui::BeginMenu(apt.Name.c_str()))
                        {
                            for (const auto& rwy : apt.Runways | std::views::values)
                            {
                                if (ImGui::BeginMenu(rwy.Name.c_str()))
                                {
                                    for (const auto& arrRte : rwy.ArrivalRoutes | std::views::values)
                                    {
                                        if (ImGui::Selectable(arrRte->Name.c_str(), arrRte->Name.c_str() == currRteName))
                                            study.Operations.setRoute(arr, arrRte.get());
                                    }
                                    ImGui::EndMenu();
                                }
                            }
                            ImGui::EndMenu();
                        }
                    }
                    if (ImGui::Selectable("##None", !arr.hasRoute()))
                        study.Operations.setRoute(arr, nullptr);
                    ImGui::EndCombo();
                }
                if (ImGui::IsItemClicked())
                    select(arr);

                // Time
                UI::tableNextColumn();
                if (UI::inputDateTime("Time", arr.Time))
                    study.Operations.update(arr);
                if (ImGui::IsItemClicked())
                    select(arr);

                // Count
                UI::tableNextColumn();
                if (UI::inputDouble("Count", arr.Count, 0.0, Constants::NaN, 1))
                    study.Operations.update(arr);
                if (ImGui::IsItemClicked())
                    select(arr);

                // Fleet Id
                UI::tableNextColumn();
                const auto& currAcftName = arr.aircraft().Name;
                if (ImGui::BeginCombo("##FleetId", currAcftName.c_str()))
                {
                    for (const auto& [acftId, acft] : study.Aircrafts())
                    {
                        const bool selected = &arr.aircraft() == &acft;
                        if (ImGui::Selectable(acftId.c_str(), selected))
                            study.Operations.setAircraft(arr, acft);

                        if (selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                if (ImGui::IsItemClicked())
                    select(arr);

                // Weight
                UI::tableNextColumn();
                if (UI::inputDouble("Aircraft weight", arr.Weight, 0.0, Constants::NaN, set.WeightUnits, false))
                    study.Operations.update(arr);
                if (ImGui::IsItemClicked())
                    select(arr);

                // Doc29 Profile
                UI::tableNextColumn();
                const char* currProfName = arr.Doc29Prof ? arr.Doc29Prof->Name.c_str() : "";
                if (ImGui::BeginCombo("##Doc29Profile", currProfName))
                {
                    if (arr.aircraft().Doc29Acft)
                        for (const auto& [profId, prof] : arr.aircraft().Doc29Acft->ArrivalProfiles)
                        {
                            const bool selected = arr.Doc29Prof == prof.get();
                            if (ImGui::Selectable(profId.c_str(), selected))
                                study.Operations.setDoc29Profile(arr, prof.get());

                            if (selected)
                                ImGui::SetItemDefaultFocus();
                        }

                    if (ImGui::Selectable("##None", arr.Doc29Prof))
                        study.Operations.setDoc29Profile(arr, nullptr);
                    ImGui::EndCombo();
                }
                if (ImGui::IsItemClicked())
                    select(arr);

                ImGui::EndDisabled(); // Not editable
                ImGui::PopID();       // Arrival ID
            }
            UI::endTable();
        }
        ImGui::PopID(); // Arrivals

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Departures
        ImGui::PushID("Departures");
        ImGui::AlignTextToFramePadding();
        UI::textInfo("Departures");

        // Filter
        ImGui::SameLine();
        static UI::TextFilter departuresFilter;
        departuresFilter.draw();

        // Edit Button
        ImGui::SameLine();
        UI::buttonEditRight();
        if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonLeft))
        {
            if (UI::selectableNew("Departure", true, ImGuiSelectableFlags_DontClosePopups))
                study.Operations.addDepartureFlight();

            if (UI::selectableDelete("All"))
            {
                m_SelectedDepartures.clear();
                Application::get().queueAsyncTask([&] { study.Operations.eraseFlightDepartures(); }, "Deleting all departure flights");
            }

            ImGui::EndPopup();
        }

        if (UI::beginTable("Departures", 11))
        {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Airport", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Runway", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Route", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Count", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Fleet ID", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn(std::format("Weight ({})", Application::settings().WeightUnits.shortName()).c_str(), ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Doc29 Profile", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Takeoff Thrust (%)", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Climb Thrust (%)", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            for (auto& [depId, dep] : depFlights)
            {
                if (!departuresFilter.passesFilter(depId))
                    continue;

                ImGui::TableNextRow();
                ImGui::PushID(depId.c_str());
                ImGui::BeginDisabled(study.Blocks.notEditable(dep));

                // Selectable Row
                UI::tableNextColumn(false);
                if (UI::selectableRowEmpty(isSelected(dep)))
                    select(dep);
                if (ImGui::BeginPopupContextItem())
                {
                    if (UI::selectableDelete())
                    {
                        action = [&] { eraseSelectedDepartures(); };
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }

                // Name
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (UI::inputText("Arrival Name", dep.Name, dep.Name != depId && depFlights.contains(dep.Name), "Arrival Name", fmt::format("Departure operation '{}' already exists in this study.", dep.Name)))
                    if (dep.Name != depId)
                        action = [&] { study.Operations.updateKey(dep, depId); };
                if (ImGui::IsItemClicked())
                    select(dep);

                if (dep.hasRoute())
                {
                    // Airport
                    UI::tableNextColumn(false);
                    UI::textInfo(dep.route().parentAirport().Name);

                    // Runway
                    UI::tableNextColumn(false);
                    UI::textInfo(dep.route().parentRunway().Name);
                }
                else
                {
                    UI::tableNextColumn(false);
                    UI::tableNextColumn(false);
                }

                // Route
                UI::tableNextColumn();
                const char* currRteName = dep.hasRoute() ? dep.route().Name.c_str() : "";
                if (ImGui::BeginCombo("##Route", currRteName))
                {
                    for (const auto& apt : study.Airports() | std::views::values)
                    {
                        if (ImGui::BeginMenu(apt.Name.c_str()))
                        {
                            for (const auto& rwy : apt.Runways | std::views::values)
                            {
                                if (ImGui::BeginMenu(rwy.Name.c_str()))
                                {
                                    for (const auto& depRte : rwy.DepartureRoutes | std::views::values)
                                    {
                                        if (ImGui::Selectable(depRte->Name.c_str(), depRte->Name.c_str() == currRteName))
                                            study.Operations.setRoute(dep, depRte.get());
                                    }
                                    ImGui::EndMenu();
                                }
                            }
                            ImGui::EndMenu();
                        }
                    }
                    if (ImGui::Selectable("##None", !dep.hasRoute()))
                        study.Operations.setRoute(dep, nullptr);
                    ImGui::EndCombo();
                }
                if (ImGui::IsItemClicked())
                    select(dep);

                // Time
                UI::tableNextColumn();
                if (UI::inputDateTime("Time", dep.Time))
                    study.Operations.update(dep);
                if (ImGui::IsItemClicked())
                    select(dep);

                // Count
                UI::tableNextColumn();
                if (UI::inputDouble("Count", dep.Count, 0.0, Constants::NaN, 1))
                    study.Operations.update(dep);
                if (ImGui::IsItemClicked())
                    select(dep);

                // Fleet Id
                UI::tableNextColumn();
                const auto& currAcftName = dep.aircraft().Name;
                if (ImGui::BeginCombo("##FleetId", currAcftName.c_str()))
                {
                    for (const auto& [acftId, acft] : study.Aircrafts())
                    {
                        const bool selected = &dep.aircraft() == &acft;
                        if (ImGui::Selectable(acftId.c_str(), selected))
                            study.Operations.setAircraft(dep, acft);

                        if (selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                if (ImGui::IsItemClicked())
                    select(dep);

                // Weight
                UI::tableNextColumn();
                if (UI::inputDouble("Aircraft weight", dep.Weight, 0.0, Constants::NaN, set.WeightUnits, false))
                    study.Operations.update(dep);
                if (ImGui::IsItemClicked())
                    select(dep);

                // Doc29 Profile
                UI::tableNextColumn();
                const char* currProfName = dep.hasDoc29Profile() ? dep.Doc29Prof->Name.c_str() : "";
                if (ImGui::BeginCombo("##Doc29Profile", currProfName))
                {
                    if (dep.aircraft().Doc29Acft)
                        for (const auto& [profId, prof] : dep.aircraft().Doc29Acft->DepartureProfiles)
                        {
                            const bool selected = dep.Doc29Prof == prof.get();
                            if (ImGui::Selectable(profId.c_str(), selected))
                                study.Operations.setDoc29Profile(dep, prof.get());

                            if (selected)
                                ImGui::SetItemDefaultFocus();
                        }
                    if (ImGui::Selectable("##None", !dep.hasDoc29Profile()))
                        study.Operations.setDoc29Profile(dep, nullptr);
                    ImGui::EndCombo();
                }
                if (ImGui::IsItemClicked())
                    select(dep);

                // Takeoff Thrust %
                UI::tableNextColumn();
                if (UI::inputPercentage("Takeoff Thrust", dep.ThrustPercentageTakeoff, 0.5, 1.0, 0, false))
                    study.Operations.update(dep);
                if (ImGui::IsItemClicked())
                    select(dep);

                // Climb Thrust %
                UI::tableNextColumn();
                if (UI::inputPercentage("Climb Thrust", dep.ThrustPercentageClimb, 0.5, 1.0, 0, false))
                    study.Operations.update(dep);
                if (ImGui::IsItemClicked())
                    select(dep);

                ImGui::EndDisabled(); // Not editable
                ImGui::PopID();       // Departure ID
            }
            UI::endTable();
        }
        ImGui::PopID(); // Departures

        // Actions outside loops
        if (action)
            action();

        ImGui::End();
    }
}
