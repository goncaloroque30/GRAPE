// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "AirportsPanel.h"

#include "Application.h"
#include "UI.h"

namespace GRAPE {
    AirportsPanel::AirportsPanel() : Panel("Airports") {}

    void AirportsPanel::select(Airport& Apt) {
        if (isSelected(Apt))
            return;

        clearSelection();
        m_SelectedType = Selected::Airport;

        m_SelectedAirport = &Apt;
    }

    void AirportsPanel::select(Runway& Rwy) {
        if (isSelected(Rwy))
            return;

        clearSelection();
        m_SelectedType = Selected::Runway;

        m_SelectedRunway = &Rwy;
    }

    void AirportsPanel::select(Route& Rte) {
        if (isSelected(Rte))
            return;

        clearSelection();
        m_SelectedType = Selected::Route;

        m_SelectedRoute = &Rte;
    }

    void AirportsPanel::clearSelection() {
        m_SelectedAirport = nullptr;
        m_SelectedRunway = nullptr;
        m_SelectedRoute = nullptr;
    }

    bool AirportsPanel::isSelected(const Airport& Apt) const { return m_SelectedAirport == &Apt; }

    bool AirportsPanel::isSelected(const Runway& Rwy) const { return m_SelectedRunway == &Rwy; }

    bool AirportsPanel::isSelected(const Route& Rte) const { return m_SelectedRoute == &Rte; }

    void AirportsPanel::reset() { clearSelection(); }

    void AirportsPanel::imGuiDraw() {
        if (!isOpen())
            return;

        auto& study = Application::study();

        // Initialize
        ImGui::Begin(m_Name.c_str(), &m_Open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse);

        // Left side
        ImGui::BeginChild("Left Side", ImVec2(-ImGui::GetContentRegionAvail().x * UI::g_StandardLeftAlignFraction, 0.0f));

        // Edit Popup
        UI::buttonEditRight();
        if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonLeft))
        {
            if (UI::selectableNew("Airport"))
                study.Airports.addAirport();

            if (UI::selectableDelete("All"))
            {
                clearSelection();
                Application::get().queueAsyncTask([&] { study.Airports.eraseAirports(); }, "Deleting all airports");
            }
            ImGui::EndPopup();
        }

        // Airport Hierarchy
        if (UI::beginTable("Airport Hierarchy", 2))
        {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            // Draw each Airport node
            for (auto& [aptId, apt] : study.Airports())
                drawAirportNode(aptId, apt);

            UI::endTable();
        }
        ImGui::EndChild();

        ImGui::SameLine();

        // Selected Data
        ImGui::BeginChild("Right Side");
        switch (m_SelectedType)
        {
        case Selected::Airport: if (m_SelectedAirport)
            drawSelectedAirport();
            break;
        case Selected::Runway: if (m_SelectedRunway)
            drawSelectedRunway();
            break;
        case Selected::Route: if (m_SelectedRoute)
            drawSelectedRoute();
            break;
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

    void AirportsPanel::drawAirportNode(const std::string& AirportId, Airport& Apt) {
        auto& study = Application::study();

        ImGui::TableNextRow();
        ImGui::PushID(AirportId.c_str());

        UI::tableNextColumn(false);

        // Selectable Row
        if (UI::selectableRowEmpty(isSelected(Apt)))
            select(Apt);

        if (ImGui::BeginPopupContextItem())
        {
            if (UI::selectableNew("Runway"))
                study.Airports.addRunway(Apt);

            if (UI::selectableDelete())
                m_Action = [&] {
                if (isSelected(Apt))
                    clearSelection();
                study.Airports.erase(Apt);
                };

            ImGui::EndPopup();
        }

        // Tree Node
        const bool open = UI::treeNodeEmpty(Apt.Runways.empty());
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
            select(Apt);

        // Name
        ImGui::BeginDisabled(study.Blocks.notEditable(Apt));
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (UI::inputText("Name", Apt.Name, Apt.Name != AirportId && study.Airports().contains(Apt.Name), "Airport name", std::format("Airport '{}' already exists in this study.", Apt.Name)))
            if (Apt.Name != AirportId)
                m_Action = [&] { study.Airports.updateKey(Apt, AirportId); };
        ImGui::EndDisabled();
        if (UI::isItemClicked())
            select(Apt);

        // Type
        UI::tableNextColumn(false);
        UI::textInfo("Airport");

        if (open)
        {
            // Draw each child runway
            for (auto& [rwyId, rwy] : Apt.Runways)
                drawRunwayNode(rwyId, rwy);

            ImGui::TreePop();
        }

        ImGui::PopID(); // Airport ID
    }

    void AirportsPanel::drawRunwayNode(const std::string& RunwayId, Runway& Rwy) {
        auto& study = Application::study();

        ImGui::TableNextRow();
        ImGui::PushID(RunwayId.c_str());

        UI::tableNextColumn(false);

        // Selectable Row
        if (UI::selectableRowEmpty(isSelected(Rwy)))
            select(Rwy);

        if (ImGui::BeginPopupContextItem())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, UI::g_ExtraColors[UI::GrapeColNew]);
            if (ImGui::BeginMenu("+ " ICON_FA_PLANE_ARRIVAL " Arrival Route"))
            {
                if (ImGui::MenuItem("Simple"))
                    study.Airports.addRouteArrival(Rwy, Route::Type::Simple);

                if (ImGui::MenuItem("Vectors"))
                    study.Airports.addRouteArrival(Rwy, Route::Type::Vectors);

                if (ImGui::MenuItem("RNP"))
                    study.Airports.addRouteArrival(Rwy, Route::Type::Rnp);

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("+ " ICON_FA_PLANE_DEPARTURE " Departure Route"))
            {
                if (ImGui::MenuItem("Simple"))
                    study.Airports.addRouteDeparture(Rwy, Route::Type::Simple);

                if (ImGui::MenuItem("Vectors"))
                    study.Airports.addRouteDeparture(Rwy, Route::Type::Vectors);

                if (ImGui::MenuItem("RNP"))
                    study.Airports.addRouteDeparture(Rwy, Route::Type::Rnp);

                ImGui::EndMenu();
            }
            ImGui::PopStyleColor();

            if (UI::selectableDelete("Runway"))
                m_Action = [&] {
                if (isSelected(Rwy))
                    clearSelection();
                study.Airports.erase(Rwy);
                };

            ImGui::EndPopup();
        }

        // Tree Node
        const bool open = UI::treeNodeEmpty(Rwy.empty());
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
            select(Rwy);

        // Name
        ImGui::BeginDisabled(study.Blocks.notEditable(Rwy));
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (UI::inputText("Name", Rwy.Name, Rwy.Name != RunwayId && Rwy.parentAirport().Runways.contains(Rwy.Name), "Runway name", std::format("Runway '{}' already exists in airport '{}'.", Rwy.Name, Rwy.parentAirport().Name)))
            if (Rwy.Name != RunwayId)
                m_Action = [&] { study.Airports.updateKey(Rwy, RunwayId); };
        ImGui::EndDisabled();
        if (UI::isItemClicked())
            select(Rwy);

        // Type
        UI::tableNextColumn(false);
        UI::textInfo("Runway");

        if (open)
        {
            if (!Rwy.ArrivalRoutes.empty())
            {
                ImGui::TableNextRow();

                // Tree Node
                UI::tableNextColumn();
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
                const bool arrOpen = UI::treeNode("Arrival Routes", false);
                ImGui::PopStyleColor();

                // Type

                if (arrOpen)
                {
                    for (auto& [rteId, arrRte] : Rwy.ArrivalRoutes)
                        drawRouteNode(rteId, *arrRte);
                    ImGui::TreePop();
                }
            }

            if (!Rwy.DepartureRoutes.empty())
            {
                ImGui::TableNextRow();

                // Tree Node
                UI::tableNextColumn();
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
                const bool depOpen = UI::treeNode("Departure Routes", false);
                ImGui::PopStyleColor();

                // Type

                if (depOpen)
                {
                    for (auto& [rteId, depRte] : Rwy.DepartureRoutes)
                        drawRouteNode(rteId, *depRte);
                    ImGui::TreePop();
                }
            }
            ImGui::TreePop();
        }
        ImGui::PopID(); // Runway ID
    }

    void AirportsPanel::drawRouteNode(const std::string& RouteId, Route& Rte) {
        auto& study = Application::study();

        ImGui::TableNextRow();
        ImGui::PushID(RouteId.c_str());

        UI::tableNextColumn(false);

        // Selectable Row
        if (UI::selectableRowEmpty(isSelected(Rte)))
            select(Rte);

        if (ImGui::BeginPopupContextItem())
        {
            if (UI::selectableDelete("Route"))
                m_Action = [&] {
                if (isSelected(Rte))
                    clearSelection();
                study.Airports.erase(Rte);
                };

            ImGui::EndPopup();
        }

        // Name
        ImGui::BeginDisabled(study.Blocks.notEditable(Rte));
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        switch (Rte.operationType())
        {
        case OperationType::Arrival:
            {
                if (UI::inputText("Name", Rte.Name, Rte.Name != RouteId && Rte.parentRunway().ArrivalRoutes.contains(Rte.Name), "Route name", std::format("Arrival route '{}' already exists in runway '{}'.", Rte.Name, Rte.parentRunway().Name)))
                    if (Rte.Name != RouteId)
                        m_Action = [&] { study.Airports.updateKey(Rte, RouteId); };
                break;
            }
        case OperationType::Departure:
            {
                if (UI::inputText("Name", Rte.Name, Rte.Name != RouteId && Rte.parentRunway().DepartureRoutes.contains(Rte.Name), "Route name", std::format("Departure route '{}' already exists in runway '{}'.", Rte.Name, Rte.parentRunway().Name)))
                    if (Rte.Name != RouteId)
                        m_Action = [&] { study.Airports.updateKey(Rte, RouteId); };
                break;
            }
        default: GRAPE_ASSERT(false);
        }
        ImGui::EndDisabled();
        if (UI::isItemClicked())
            select(Rte);

        // Type
        UI::tableNextColumn(false);
        UI::textInfo(std::format("{} Route", Route::Types.toString(Rte.type())));

        ImGui::PopID(); // Route ID
    }

    void AirportsPanel::drawSelectedAirport() {
        GRAPE_ASSERT(m_SelectedAirport);
        Airport& apt = *m_SelectedAirport;

        auto& study = Application::study();

        const Settings& set = Application::settings();
        const ImGuiStyle& style = ImGui::GetStyle();

        bool updated = false;

        // New Runway
        if (UI::buttonNew("Runway"))
            study.Airports.addRunway(apt);

        ImGui::SameLine();

        // Delete Airport
        const bool nRemovable = study.Blocks.notRemovable(apt);
        ImGui::BeginDisabled(nRemovable);
        if (UI::buttonDelete("Airport"))
            m_Action = [&] {
            clearSelection();
            study.Airports.erase(apt);
            };
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && nRemovable)
            UI::setTooltipInvalid(std::format("There are {} flights which use a route from this airport.", study.Blocks.blocking(apt).size()));
        ImGui::EndDisabled(); // Not Removable

        ImGui::Separator();

        ImGui::BeginDisabled(study.Blocks.notEditable(apt));

        const float offset = ImGui::GetCursorPosX() + ImGui::CalcTextSize("Reference Sea Level Pressure:").x;

        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("Longitude:");
        ImGui::SameLine(offset, style.ItemInnerSpacing.x);
        ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
        if (UI::inputDouble("Longitude", apt.Longitude, -180.0, 180.0))
            updated = true;

        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("Latitude:");
        ImGui::SameLine(offset, style.ItemInnerSpacing.x);
        ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
        if (UI::inputDouble("Latitude", apt.Latitude, -90.0, 90.0))
            updated = true;

        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("Elevation:");
        ImGui::SameLine(offset, style.ItemInnerSpacing.x);
        ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
        if (UI::inputDouble("Elevation", apt.Elevation, set.AltitudeUnits))
            updated = true;

        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("Reference Temperature:");
        ImGui::SameLine(offset, style.ItemInnerSpacing.x);
        ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
        if (UI::inputDouble("ReferenceTemperature", apt.ReferenceTemperature, Constants::Precision, Constants::NaN, set.TemperatureUnits))
            updated = true;

        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("Reference Sea Level Pressure:");
        ImGui::SameLine(offset, style.ItemInnerSpacing.x);
        ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
        if (UI::inputDouble("ReferenceSeaLevelPressure:", apt.ReferenceSeaLevelPressure, Constants::Precision, Constants::NaN, set.PressureUnits))
            updated = true;

        ImGui::EndDisabled(); // Not Editable

        if (updated)
            study.Airports.update(apt);
    }

    void AirportsPanel::drawSelectedRunway() {
        GRAPE_ASSERT(m_SelectedRunway);
        Runway& rwy = *m_SelectedRunway;
        auto& study = Application::study();

        const ImGuiStyle& style = ImGui::GetStyle();
        const Settings& set = Application::settings();

        bool updated = false;

        // New Route
        UI::buttonNew("Route");
        if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonLeft))
        {
            ImGui::PushStyleColor(ImGuiCol_Text, UI::g_ExtraColors[UI::GrapeColNew]);
            if (ImGui::BeginMenu(ICON_FA_PLANE_ARRIVAL " Arrival"))
            {
                if (ImGui::Selectable("Simple"))
                    study.Airports.addRouteArrival(rwy, Route::Type::Simple);
                if (ImGui::Selectable("Vectors"))
                    study.Airports.addRouteArrival(rwy, Route::Type::Vectors);
                if (ImGui::Selectable("RNP"))
                    study.Airports.addRouteArrival(rwy, Route::Type::Rnp);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu(ICON_FA_PLANE_DEPARTURE " Departure"))
            {
                if (ImGui::Selectable("Simple"))
                    study.Airports.addRouteDeparture(rwy, Route::Type::Simple);
                if (ImGui::Selectable("Vectors"))
                    study.Airports.addRouteDeparture(rwy, Route::Type::Vectors);
                if (ImGui::Selectable("RNP"))
                    study.Airports.addRouteDeparture(rwy, Route::Type::Rnp);
                ImGui::EndMenu();
            }
            ImGui::PopStyleColor();

            ImGui::EndPopup();
        }

        ImGui::SameLine();

        // Delete Runway
        const bool nRemovable = study.Blocks.notRemovable(rwy);
        ImGui::BeginDisabled(nRemovable);
        if (UI::buttonDelete("Runway"))
            m_Action = [&] {
            clearSelection();
            study.Airports.erase(rwy);
            };
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && nRemovable)
            UI::setTooltipInvalid(std::format("There are {} flights which use a route from this runway.", study.Blocks.blocking(rwy).size()));
        ImGui::EndDisabled(); // Not Removable

        ImGui::Separator();

        ImGui::BeginDisabled(Application::study().Blocks.notEditable(rwy));

        const float offset = ImGui::GetCursorPosX() + ImGui::CalcTextSize("Longitude:").x;

        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("Longitude:");
        ImGui::SameLine(offset, style.ItemInnerSpacing.x);
        ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
        if (UI::inputDouble("Longitude", rwy.Longitude, -180.0, 180.0))
            updated = true;

        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("Latitude:");
        ImGui::SameLine(offset, style.ItemInnerSpacing.x);
        ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
        if (UI::inputDouble("Latitude", rwy.Latitude, -90.0, 90.0))
            updated = true;

        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("Elevation:");
        ImGui::SameLine(offset, style.ItemInnerSpacing.x);
        ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
        if (UI::inputDouble("Elevation", rwy.Elevation, set.AltitudeUnits))
            updated = true;

        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("Length:");
        ImGui::SameLine(offset, style.ItemInnerSpacing.x);
        ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
        if (UI::inputDouble("Length", rwy.Length, Constants::Precision, Constants::NaN, set.DistanceUnits))
            updated = true;

        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("Heading:");
        ImGui::SameLine(offset, style.ItemInnerSpacing.x);
        ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
        if (UI::inputDouble("Heading", rwy.Heading, 0.0, 360.0, 2))
            updated = true;

        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("Gradient:");
        ImGui::SameLine(offset, style.ItemInnerSpacing.x);
        ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
        if (UI::inputPercentage("Gradient", rwy.Gradient, -1.0, 1.0))
            updated = true;

        ImGui::EndDisabled(); // Not Editable

        if (updated)
            study.Airports.update(rwy);
    }

    namespace {
        struct RouteDrawer : RouteTypeVisitor {
            explicit RouteDrawer(Route& Rte) { Rte.accept(*this); }
            void visitSimple(RouteTypeSimple& Rte) override;
            void visitVectors(RouteTypeVectors& Rte) override;
            void visitRnp(RouteTypeRnp& Rte) override;
        };
    }

    void AirportsPanel::drawSelectedRoute() {
        GRAPE_ASSERT(m_SelectedRoute);
        Route& rte = *m_SelectedRoute;

        auto& study = Application::study();

        const ImGuiStyle& style = ImGui::GetStyle();

        const bool nRemovable = study.Blocks.notRemovable(rte);
        ImGui::BeginDisabled(nRemovable);
        if (UI::buttonDelete("Route"))
            m_Action = [&] {
            clearSelection();
            study.Airports.erase(rte);
            };
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && nRemovable)
            UI::setTooltipInvalid(std::format("There are {} flights which use this route.", study.Blocks.blocking(rte).size()));
        ImGui::EndDisabled(); // Not Removable

        ImGui::Separator();

        ImGui::BeginDisabled(Application::study().Blocks.notEditable(rte));

        ImGui::TextDisabled("Operation:");
        ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
        UI::textInfo(std::format("{}", OperationTypes.toString(rte.operationType())));
        ImGui::SameLine(0.0f, style.ItemSpacing.x);
        ImGui::TextDisabled("Route Type:");
        ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
        UI::textInfo(std::format("{}", Route::Types.toString(rte.type())));

        ImGui::TextDisabled("Runway Heading:");
        ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
        UI::textInfo(std::format("{:.2f}", rte.parentRunway().Heading));

        ImGui::Separator();

        RouteDrawer drawRte(rte);

        ImGui::EndDisabled(); // Not Editable
    }

    void RouteDrawer::visitSimple(RouteTypeSimple& Rte) {
        std::function<void()> action = nullptr; // Outside of loop edits
        bool updated = false;

        if (UI::buttonNew("+", ImVec2(0.0f, 0.0f), false))
        {
            Rte.addPoint();
            updated = true; // Updating route resets points
        }
        if (!Rte.empty())
        {
            ImGui::SameLine(0.0, ImGui::GetStyle().ItemInnerSpacing.x);
            if (UI::buttonDelete("-", ImVec2(0.0f, 0.0f), false))
            {
                Rte.deletePoint();
                updated = true; // Updating route resets points
            }
            ImGui::SameLine();
            if (UI::buttonDelete("Clear"))
            {
                Rte.clear();
                updated = true; // Updating route resets points
            }
        }

        if (UI::beginTable("Route Simple", 3))
        {
            ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Longitude", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Latitude", ImGuiTableColumnFlags_NoHide);
            ImGui::TableHeadersRow();

            if (Rte.operationType() == OperationType::Departure)
            {
                ImGui::TableNextRow();

                UI::tableNextColumn(false);
                UI::textInfo(std::format("{}", Rte.parentRunway().Name));

                UI::tableNextColumn(false);
                UI::textInfo(std::format("{:.6f}", Rte.parentRunway().Longitude));

                UI::tableNextColumn(false);
                UI::textInfo(std::format("{:.6f}", Rte.parentRunway().Latitude));
            }

            for (auto it = Rte.begin(); it != Rte.end(); ++it)
            {
                auto& [lon, lat] = *it;
                const auto i = it - Rte.begin();

                ImGui::TableNextRow();
                ImGui::PushID(&*it);

                // Selectable Row Point Number
                UI::tableNextColumn(false);
                UI::selectableRowInfo(std::format("{}", i + 1));
                if (ImGui::BeginPopupContextItem())
                {
                    if (UI::selectableNew())
                        action = [&, i] { Rte.insertPoint(i); };
                    if (UI::selectableDelete())
                        action = [&, i] { Rte.deletePoint(i); };
                    ImGui::EndPopup();
                }

                // Longitude
                UI::tableNextColumn();
                if (UI::inputDouble("Longitude", lon, -180.0, 180.0))
                    updated = true;

                // Latitude
                UI::tableNextColumn();
                if (UI::inputDouble("Latitude", lat, -90.0, 90.0))
                    updated = true;

                ImGui::PopID(); // Point
            }

            if (Rte.operationType() == OperationType::Arrival)
            {
                ImGui::TableNextRow();

                UI::tableNextColumn(false);
                UI::textInfo(std::format("{}", Rte.parentRunway().Name));

                UI::tableNextColumn(false);
                UI::textInfo(std::format("{:.6f}", Rte.parentRunway().Longitude));

                UI::tableNextColumn(false);
                UI::textInfo(std::format("{:.6f}", Rte.parentRunway().Latitude));
            }
            UI::endTable();
        }

        if (action)
        {
            action();
            updated = true;
        }

        if (updated)
            Application::study().Airports.update(Rte);
    }

    void RouteDrawer::visitVectors(RouteTypeVectors& Rte) {
        const Settings& set = Application::settings();

        std::function<void()> action = nullptr; // Outside of loop edits
        bool updated = false;

        if (UI::buttonNew("+", ImVec2(0.0f, 0.0f), false))
        {
            Rte.addVector();
            updated = true; // Updating route resets vectors
        }
        if (!Rte.empty())
        {
            ImGui::SameLine(0.0, ImGui::GetStyle().ItemInnerSpacing.x);
            if (UI::buttonDelete("-", ImVec2(0.0f, 0.0f), false))
            {
                Rte.deleteVector();
                updated = true; // Updating route resets vectors
            }
            ImGui::SameLine();
            if (UI::buttonDelete("Clear"))
            {
                Rte.clear();
                updated = true;
            }
        }

        if (UI::beginTable("Route Vectors", 6))
        {
            ImGui::TableSetupColumn("#");
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn(std::format("Distance ({})", set.DistanceUnits.shortName()).c_str(), ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn(std::format("Turn Radius ({})", set.DistanceUnits.shortName()).c_str(), ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Heading Change", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Turn Direction", ImGuiTableColumnFlags_NoHide);
            ImGui::TableHeadersRow();

            ImGui::TableNextRow();
            for (auto it = Rte.begin(); it != Rte.end(); ++it)
            {
                auto& vec = *it;
                const auto i = it - Rte.begin();

                ImGui::TableNextRow();
                ImGui::PushID(&vec);

                // Selectable Row Point Number
                UI::tableNextColumn(false);
                UI::selectableRowInfo(std::format("{}", i + 1));
                if (ImGui::BeginPopupContextItem())
                {
                    if (UI::selectableNew())
                        action = [&, i] { Rte.insertVector(i); };
                    if (UI::selectableDelete())
                        action = [&, i] { Rte.deleteVector(i); };
                    ImGui::EndPopup();
                }

                // Vector Type
                UI::tableNextColumn();
                const std::string currStr = std::visit(RouteTypeVectors::VisitorVectorTypeString(), vec);
                if (ImGui::BeginCombo("##Vector type", currStr.c_str()))
                {
                    for (const auto& vecStr : RouteTypeVectors::VectorTypes)
                    {
                        const bool isSelected = vecStr == currStr;
                        const auto type = RouteTypeVectors::VectorTypes.fromString(vecStr);
                        if (ImGui::Selectable(vecStr, isSelected))
                            switch (type)
                            {
                            case RouteTypeVectors::VectorType::Straight:
                                {
                                    if (Rte.setStraight(i))
                                        updated = true;
                                    break;
                                }
                            case RouteTypeVectors::VectorType::Turn:
                                {
                                    if (Rte.setTurn(i))
                                        updated = true;
                                    break;
                                }
                            default: GRAPE_ASSERT(false);
                                break;
                            }

                        if (isSelected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                std::visit(Overload{
                               [&](RouteTypeVectors::Straight& Vec) {
                                   UI::tableNextColumn();
                                   if (UI::inputDouble("Distance", Vec.Distance, Constants::Precision, Constants::NaN, set.DistanceUnits, false))
                                       updated = true;
                               },
                               [&](RouteTypeVectors::Turn& Vec) {
                                   UI::tableNextColumn(false);

                                   UI::tableNextColumn();
                                   if (UI::inputDouble("Turn radius", Vec.TurnRadius, Constants::Precision, Constants::NaN, set.DistanceUnits, false))
                                       updated = true;

                                   UI::tableNextColumn();
                                   if (UI::inputDouble("Heading change", Vec.HeadingChange, 0.0, 360.0, 2))
                                       updated = true;

                                   UI::tableNextColumn();
                                   const std::string currDir = RouteTypeVectors::Turn::Directions.toString(Vec.TurnDirection);
                                    if (ImGui::BeginCombo("##Turn direction", currDir.c_str()))
                                    {
                                        for (const auto& dirStr : RouteTypeVectors::Turn::Directions)
                                        {
                                            const bool isSelected = dirStr == currDir;
                                            if (ImGui::Selectable(dirStr, isSelected))
                                            {
                                                Vec.TurnDirection = RouteTypeVectors::Turn::Directions.fromString(dirStr);
                                                updated = true;
                                            }

                                            if (isSelected)
                                                ImGui::SetItemDefaultFocus();
                                        }
                                        ImGui::EndCombo();
                                    }
                               },
                    }, vec);
                ImGui::PopID(); // Vector
            }
            UI::endTable();
        }

        if (action)
        {
            action();
            updated = true;
        }

        if (updated)
            Application::study().Airports.update(Rte);
    }

    void RouteDrawer::visitRnp(RouteTypeRnp& Rte) {
        std::function<void()> action = nullptr; // Outside of loop edits
        bool updated = false;

        if (UI::buttonNew("+", ImVec2(0.0f, 0.0f), false))
        {
            Rte.addStep();
            updated = true; // Updating route resets vectors
        }
        if (!Rte.empty())
        {
            ImGui::SameLine(0.0, ImGui::GetStyle().ItemInnerSpacing.x);
            if (UI::buttonDelete("-", ImVec2(0.0f, 0.0f), false))
            {
                Rte.deleteStep();
                updated = true; // Updating route resets vectors
            }
            ImGui::SameLine();
            if (UI::buttonDelete("Clear"))
            {
                Rte.clear();
                updated = true; // Updating route resets vectors
            }
        }

        if (UI::beginTable("Route Rnp", 6))
        {
            ImGui::TableSetupColumn("#");
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableSetupColumn("Longitude", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableSetupColumn("Latitude", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableSetupColumn("Center Longitude", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableSetupColumn("Center Latitude", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableHeadersRow();

            if (Rte.operationType() == OperationType::Departure)
            {
                ImGui::TableNextRow();

                UI::tableNextColumn(false);
                UI::textInfo(std::format("{}", Rte.parentRunway().Name));

                UI::tableNextColumn(false);
                UI::textInfo("Runway Threshold");

                UI::tableNextColumn(false);
                UI::textInfo(std::format("{:.6f}", Rte.parentRunway().Longitude));

                UI::tableNextColumn(false);
                UI::textInfo(std::format("{:.6f}", Rte.parentRunway().Latitude));
            }

            for (auto it = Rte.begin(); it != Rte.end(); ++it)
            {
                auto& step = *it;
                const auto i = it - Rte.begin();

                ImGui::TableNextRow();
                ImGui::PushID(&step);

                UI::tableNextColumn(false);
                UI::selectableRowInfo(std::format("{}", i + 1));
                if (ImGui::BeginPopupContextItem())
                {
                    if (UI::selectableNew())
                        action = [&, i] { Rte.insertStep(i); };
                    if (UI::selectableDelete())
                        action = [&, i] { Rte.deleteStep(i); };

                    ImGui::EndPopup();
                }

                // Type
                UI::tableNextColumn();
                if (i == 0) { UI::textInfo(RouteTypeRnp::StepTypes.toString(RouteTypeRnp::StepType::TrackToFix)); }
                else
                {
                    const std::string currType = std::visit(RouteTypeRnp::VisitorRnpStepTypeString(), step);
                    if (ImGui::BeginCombo("##Vector type", currType.c_str()))
                    {
                        for (const auto& stepStr : RouteTypeRnp::StepTypes)
                        {
                            const bool isSelected = stepStr == currType;
                            if (ImGui::Selectable(stepStr, isSelected))
                                switch (RouteTypeRnp::StepTypes.fromString(stepStr))
                                {
                                case RouteTypeRnp::StepType::TrackToFix:
                                    {
                                        if (Rte.setTrackToFix(i))
                                            updated = true;
                                        break;
                                    }
                                case RouteTypeRnp::StepType::RadiusToFix:
                                    {
                                        if (Rte.setRadiusToFix(i))
                                            updated = true;
                                        break;
                                    }
                                default: GRAPE_ASSERT(false);
                                    break;
                                }

                            if (isSelected)
                                ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                }
                std::visit(Overload{
                               [&](RouteTypeRnp::TrackToFix& Step) {
                                   UI::tableNextColumn();
                                   if (UI::inputDouble("Longitude", Step.Longitude, -180.0, 180.0))
                                       updated = true;

                                   UI::tableNextColumn();
                                   if (UI::inputDouble("Latitude", Step.Latitude, -90.0, 90.0))
                                       updated = true;
                               },
                               [&](RouteTypeRnp::RadiusToFix& Step) {
                                   UI::tableNextColumn();
                                   if (UI::inputDouble("Longitude", Step.Longitude, -180.0, 180.0))
                                       updated = true;

                                   UI::tableNextColumn();
                                   if (UI::inputDouble("Latitude", Step.Latitude, -90.0, 90.0))
                                       updated = true;

                                   UI::tableNextColumn();
                                   if (UI::inputDouble("Center Longitude", Step.CenterLongitude, -180.0, 180.0))
                                       updated = true;

                                   UI::tableNextColumn();
                                   if (UI::inputDouble("Center Latitude", Step.CenterLatitude, -90.0, 90.0))
                                       updated = true;
                               },
                    }, step);
                ImGui::PopID(); // RNP Step
            }

            if (Rte.operationType() == OperationType::Arrival)
            {
                ImGui::TableNextRow();

                UI::tableNextColumn(false);
                UI::textInfo(std::format("{}", Rte.parentRunway().Name));

                UI::tableNextColumn(false);
                UI::textInfo("Runway Threshold");

                UI::tableNextColumn(false);
                UI::textInfo(std::format("{:.6f}", Rte.parentRunway().Longitude));

                UI::tableNextColumn(false);
                UI::textInfo(std::format("{:.6f}", Rte.parentRunway().Latitude));
            }
            UI::endTable();
        }

        if (action)
        {
            action();
            updated = true;
        }

        if (updated)
            Application::study().Airports.update(Rte);
    }
}
