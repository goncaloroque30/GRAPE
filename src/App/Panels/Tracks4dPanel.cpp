// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "Tracks4dPanel.h"

#include "Application.h"
#include "UI.h"

namespace GRAPE {
    Tracks4dPanel::Tracks4dPanel() : Panel("Tracks 4D") {}

    void Tracks4dPanel::select(Track4dArrival& Track4dArr) {
        if (!ImGui::GetIO().KeyCtrl)
        {
            reset();
            m_SelectedType = OperationType::Arrival;
        }
        else if (isSelected(Track4dArr))
        {
            return;
        }

        Application::study().Operations.load(Track4dArr);
        m_SelectedArrivals.emplace_back(&Track4dArr);
    }


    void Tracks4dPanel::select(Track4dDeparture& Track4dDep) {
        if (!ImGui::GetIO().KeyCtrl)
        {
            reset();
            m_SelectedType = OperationType::Departure;
        }
        else if (isSelected(Track4dDep))
        {
            return;
        }

        Application::study().Operations.load(Track4dDep);
        m_SelectedDepartures.emplace_back(&Track4dDep);
    }

    void Tracks4dPanel::deselect(Track4dArrival& Track4dArr) {
        Track4dArr.clear();
        std::erase(m_SelectedArrivals, &Track4dArr);
    }

    void Tracks4dPanel::deselect(Track4dDeparture& Track4dDep) {
        Track4dDep.clear();
        std::erase(m_SelectedDepartures, &Track4dDep);
    }

    void Tracks4dPanel::deselectArrivals() {
        for (const auto track4dArr : m_SelectedArrivals)
            track4dArr->clear();
        m_SelectedArrivals.clear();
    }
    void Tracks4dPanel::deselectDepartures() {
        for (const auto track4dDep : m_SelectedDepartures)
            track4dDep->clear();
        m_SelectedDepartures.clear();
    }

    void Tracks4dPanel::eraseSelectedArrivals() {
        for (const auto track4dArr : m_SelectedArrivals)
            Application::study().Operations.erase(*track4dArr);

        m_SelectedArrivals.clear();
    }

    void Tracks4dPanel::eraseSelectedDepartures() {
        for (const auto track4dDep : m_SelectedDepartures)
            Application::study().Operations.erase(*track4dDep);

        m_SelectedDepartures.clear();
    }

    bool Tracks4dPanel::isSelected(Track4dArrival& Track4dArr) const {
        return std::ranges::find(m_SelectedArrivals, &Track4dArr) != m_SelectedArrivals.end();
    }

    bool Tracks4dPanel::isSelected(Track4dDeparture& Track4dDep) const {
        return std::ranges::find(m_SelectedDepartures, &Track4dDep) != m_SelectedDepartures.end();
    }

    void Tracks4dPanel::reset() {
        deselectArrivals();
        deselectDepartures();
    }

    void Tracks4dPanel::onPerformanceRunStart() {
        reset();
    }

    void Tracks4dPanel::imGuiDraw() {
        if (!isOpen())
            return;

        auto& study = Application::study();
        auto& arrTracks4d = study.Operations.track4dArrivals();
        auto& depTracks4d = study.Operations.track4dDepartures();

        std::function<void()> action = nullptr;

        ImGui::Begin(m_Name.c_str(), &m_Open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse);

        ImGui::BeginChild("Left Side", ImVec2(-ImGui::GetContentRegionAvail().x * UI::g_StandardLeftAlignFraction * 0.9f, 0.0f));

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
            if (UI::selectableNew("Arrival"))
                study.Operations.addArrivalTrack4d();

            if (UI::selectableDelete("All"))
            {
                m_SelectedArrivals.clear();
                Application::get().queueAsyncTask([&] { study.Operations.eraseTrack4dArrivals(); }, "Deleting all arrival tracks 4D");
            }
            ImGui::EndPopup();
        }

        if (UI::beginTable("Arrivals Tracks 4D", 4, ImGuiTableFlags_None, ImVec2(0.0f, UI::getTableHeight(arrTracks4d.size(), true, ImGui::GetContentRegionAvail().y / 2.0f))))
        {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Count", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Fleet ID", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            for (auto& [arrId, arr] : arrTracks4d)
            {
                if (!arrivalsFilter.passesFilter(arrId))
                    continue;

                ImGui::TableNextRow();
                ImGui::PushID(arrId.c_str());

                UI::tableNextColumn(false);

                // Selectable Row
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
                ImGui::BeginDisabled(study.Blocks.notEditable(arr));
                if (UI::inputText("Arrival Name", arr.Name, arr.Name != arrId && arrTracks4d.contains(arr.Name), "Arrival Name", std::format("Arrival track 4D '{}' already exists in this study.", arr.Name)))
                    if (arr.Name != arrId)
                        action = [&] { study.Operations.updateKey(arr, arrId); };
                if (UI::isItemClicked())
                    select(arr);

                // Time
                UI::tableNextColumn();
                if (UI::inputDateTime("Time", arr.Time))
                    study.Operations.update(arr);
                if (UI::isItemClicked())
                    select(arr);

                // Count
                UI::tableNextColumn();
                if (UI::inputDouble("Count", arr.Count, 0.0, Constants::NaN, 1))
                    study.Operations.update(arr);
                if (UI::isItemClicked())
                    select(arr);

                // Fleet ID
                UI::tableNextColumn();
                const auto& currAcftName = arr.aircraft().Name;
                if (ImGui::BeginCombo("##FleetID", currAcftName.c_str()))
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
                if (UI::isItemClicked())
                    select(arr);

                ImGui::EndDisabled(); // Not Editable
                ImGui::PopID(); // Arrival ID
            }
            UI::endTable();
        }
        ImGui::PopID(); // Arrivals

        ImGui::Separator();

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
            if (UI::selectableNew("Departure"))
                study.Operations.addDepartureTrack4d();

            if (UI::selectableDelete("All"))
            {
                m_SelectedDepartures.clear();
                Application::get().queueAsyncTask([&] { study.Operations.eraseTrack4dDepartures(); }, "Deleting all departure tracks 4D");
            }
            ImGui::EndPopup();
        }

        if (UI::beginTable("Departure Tracks 4D", 4))
        {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Count", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Fleet ID", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            for (auto& [depId, dep] : depTracks4d)
            {
                if (!departuresFilter.passesFilter(depId))
                    continue;

                ImGui::TableNextRow();
                ImGui::PushID(depId.c_str());

                UI::tableNextColumn(false);

                // Selectable Row
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
                ImGui::BeginDisabled(study.Blocks.notEditable(dep));
                if (UI::inputText("Departure Name", dep.Name, dep.Name != depId && depTracks4d.contains(dep.Name), "Departure Name", std::format("Departure track 4D '{}' already exists in this study.", dep.Name)))
                    if (dep.Name != depId)
                        action = [&] { study.Operations.updateKey(dep, depId); };
                if (UI::isItemClicked())
                    select(dep);

                // Time
                UI::tableNextColumn();
                if (UI::inputDateTime("Time", dep.Time))
                    study.Operations.update(dep);
                if (UI::isItemClicked())
                    select(dep);

                // Count
                UI::tableNextColumn();
                if (UI::inputDouble("Count", dep.Count, 0.0, Constants::NaN, 1))
                    study.Operations.update(dep);
                if (UI::isItemClicked())
                    select(dep);

                // Fleet ID
                UI::tableNextColumn();
                const auto& currAcftName = dep.aircraft().Name;
                if (ImGui::BeginCombo("##FleetID", currAcftName.c_str()))
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
                if (UI::isItemClicked())
                    select(dep);

                ImGui::EndDisabled(); // Not Editable
                ImGui::PopID(); // Departure ID
            }
            UI::endTable();
        }
        ImGui::PopID(); // Departures

        ImGui::EndChild(); // Left Side

        ImGui::SameLine();

        // Draw selected Track4d
        ImGui::BeginChild("Track4d");
        switch (m_SelectedType)
        {
        case OperationType::Arrival:
            {
                if (!m_SelectedArrivals.empty())
                    drawTrack4d(*m_SelectedArrivals.front());
                break;
            }
        case OperationType::Departure:
            {
                if (!m_SelectedDepartures.empty())
                    drawTrack4d(*m_SelectedDepartures.front());
                break;
            }
        default: GRAPE_ASSERT(false);
        }
        ImGui::EndChild();

        // Actions outside loops
        if (action)
            action();

        ImGui::End();
    }

    void Tracks4dPanel::drawTrack4d(Track4d& Track4dOp) const {
        const Settings& set = Application::settings();

        bool update = false;
        std::function<void()> action = nullptr;

        ImGui::BeginDisabled(Application::study().Blocks.notEditable(Track4dOp));

        if (UI::buttonNew("Point"))
        {
            Track4dOp.addPoint();
            update = true;
        }

        if (!Track4dOp.empty())
        {
            ImGui::SameLine();
            if (UI::buttonDelete("Clear"))
            {
                Track4dOp.clear();
                update = true;
            }
        }

        if (UI::beginTable("Track 4D Points", 11))
        {
            ImGui::TableSetupColumn("#");
            ImGui::TableSetupColumn("Flight Phase");
            ImGui::TableSetupColumn(std::format("Cumulative Ground Distance ({})", set.DistanceUnits.shortName()).c_str());
            ImGui::TableSetupColumn("Longitude");
            ImGui::TableSetupColumn("Latitude");
            ImGui::TableSetupColumn(std::format("Altitude MSL ({})", set.AltitudeUnits.shortName()).c_str());
            ImGui::TableSetupColumn(std::format("True Airspeed ({})", set.SpeedUnits.shortName()).c_str());
            ImGui::TableSetupColumn(std::format("Groundspeed ({})", set.SpeedUnits.shortName()).c_str());
            ImGui::TableSetupColumn(std::format("Corrected Net Thrust / Engine ({})", set.ThrustUnits.shortName()).c_str());
            ImGui::TableSetupColumn("Bank Angle");
            ImGui::TableSetupColumn(std::format("Fuel Flow ({})", set.FuelFlowUnits.shortName()).c_str());
            ImGui::TableHeadersRow();

            for (auto it = Track4dOp.begin(); it != Track4dOp.end(); ++it)
            {
                auto i = it - Track4dOp.begin();
                auto& pt = *it;

                ImGui::TableNextRow();
                ImGui::PushID(&*it);

                ImGui::TableNextColumn();

                // Selectable Row
                UI::selectableRowEmpty(false);
                if (ImGui::BeginPopupContextItem())
                {
                    if (UI::selectableNew("Point"))
                        action = [&, i] { Track4dOp.insertPoint(i); };
                    if (UI::selectableDelete())
                    {
                        action = [&, i] { Track4dOp.deletePoint(i); };
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::EndPopup();
                }

                // # (Point Number)
                UI::textInfo(std::format("{}", i + 1));

                // Type
                UI::tableNextColumn();
                if (ImGui::BeginCombo("##Type", FlightPhases.toString(pt.FlPhase).c_str()))
                {
                    for (const auto& phase : Track4dOp.phases())
                    {
                        const bool selected = phase == pt.FlPhase;
                        if (ImGui::Selectable(FlightPhases.toString(phase).c_str(), selected))
                        {
                            pt.FlPhase = phase;
                            update = true;
                        }
                    }
                    ImGui::EndCombo();
                }

                UI::tableNextColumn();
                if (UI::inputDouble("Cumulative Ground Distance", pt.CumulativeGroundDistance, set.DistanceUnits, false))
                    update = true;

                UI::tableNextColumn();
                if (UI::inputDouble("Longitude", pt.Longitude, -180.0, 180.0))
                    update = true;

                UI::tableNextColumn();
                if (UI::inputDouble("Latitude", pt.Latitude, -90.0, 90.0))
                    update = true;

                UI::tableNextColumn();
                if (UI::inputDouble("Altitude MSL", pt.AltitudeMsl, set.AltitudeUnits, false))
                    update = true;

                UI::tableNextColumn();
                if (UI::inputDouble("True Airspeed", pt.TrueAirspeed, 0.0, Constants::NaN, set.SpeedUnits, false))
                    update = true;

                UI::tableNextColumn();
                if (UI::inputDouble("Groundspeed", pt.Groundspeed, 0.0, Constants::NaN, set.SpeedUnits, false))
                    update = true;

                UI::tableNextColumn();
                if (UI::inputDouble("Thrust", pt.CorrNetThrustPerEng, set.ThrustUnits, false))
                    update = true;

                UI::tableNextColumn();
                if (UI::inputDouble("Bank angle", pt.BankAngle, -90.0, 90.0, 0))
                    update = true;

                UI::tableNextColumn();
                if (UI::inputDouble("Fuel Flow", pt.FuelFlowPerEng, 0.0, Constants::NaN, set.FuelFlowUnits, false))
                    update = true;

                ImGui::PopID();
            }
            UI::endTable();
        }

        ImGui::EndDisabled(); // Not editable

        if (action)
            action();

        if (update)
            Application::study().Operations.update(Track4dOp);
    }
}
