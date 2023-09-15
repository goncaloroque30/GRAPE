// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "Doc29Panel.h"

#include "Application.h"
#include "UI.h"

#include "IO/AnpImport.h"

namespace GRAPE {
    // Visitors not part of panel interface
    namespace {
        struct Doc29ThrustDrawer : Doc29ThrustVisitor {
            explicit Doc29ThrustDrawer(const Doc29Performance& Doc29Acft) : m_Doc29Acft(Doc29Acft) { Doc29Acft.thrust().accept(*this); }
            void visitDoc29ThrustRating(Doc29ThrustRating& Thrust) override;
            void visitDoc29ThrustPropeller(Doc29ThrustRatingPropeller& Thrust) override;

        private:
            const Doc29Performance& m_Doc29Acft;
        };

        struct Doc29ProfileDrawer : Doc29ProfileVisitor {
            explicit Doc29ProfileDrawer(Doc29Profile& Doc29Prof) { Doc29Prof.accept(*this); }
            void visitDoc29ProfileArrivalPoints(Doc29ProfileArrivalPoints& Doc29Prof) override;
            void visitDoc29ProfileDeparturePoints(Doc29ProfileDeparturePoints& Doc29Prof) override;
            void visitDoc29ProfileArrivalProcedural(Doc29ProfileArrivalProcedural& Doc29Prof) override;
            void visitDoc29ProfileDepartureProcedural(Doc29ProfileDepartureProcedural& Doc29Prof) override;
        };
    }

    void Doc29Panel::select(Doc29Performance& Doc29Acft) {
        if (isSelected(Doc29Acft))
            return;

        clearNoiseSelection();
        m_SelectedDoc29ProfileArrivals.clear();
        m_SelectedDoc29ProfileDepartures.clear();

        if (!ImGui::GetIO().KeyCtrl)
            m_SelectedDoc29Aircraft.clear();

        m_SelectedDoc29Aircraft.emplace_back(&Doc29Acft);
    }

    void Doc29Panel::select(Doc29ProfileArrival& Doc29Prof) {
        if (isSelected(Doc29Prof))
            return;

        if (!ImGui::GetIO().KeyCtrl)
            m_SelectedDoc29ProfileArrivals.clear();

        m_SelectedDoc29ProfileArrivals.emplace_back(&Doc29Prof);
    }

    void Doc29Panel::select(Doc29ProfileDeparture& Doc29Prof) {
        if (isSelected(Doc29Prof))
            return;

        if (!ImGui::GetIO().KeyCtrl)
            m_SelectedDoc29ProfileDepartures.clear();

        m_SelectedDoc29ProfileDepartures.emplace_back(&Doc29Prof);
    }

    void Doc29Panel::select(Doc29Noise& Doc29Ns) {
        if (isSelected(Doc29Ns))
            return;

        clearAircraftSelection();
        m_SelectedNpdData = nullptr;

        if (!ImGui::GetIO().KeyCtrl)
            m_SelectedDoc29Noises.clear();

        m_SelectedDoc29Noises.emplace_back(&Doc29Ns);
    }

    void Doc29Panel::deselect(Doc29Performance& Doc29Acft) {
        if (!m_SelectedDoc29Aircraft.empty() && m_SelectedDoc29Aircraft.front() == &Doc29Acft)
        {
            m_SelectedDoc29ProfileArrivals.clear();
            m_SelectedDoc29ProfileDepartures.clear();
        }

        std::erase(m_SelectedDoc29Aircraft, &Doc29Acft);
    }

    void Doc29Panel::deselect(Doc29ProfileArrival& Doc29Prof) { std::erase(m_SelectedDoc29ProfileArrivals, &Doc29Prof); }

    void Doc29Panel::deselect(Doc29ProfileDeparture& Doc29Prof) { std::erase(m_SelectedDoc29ProfileDepartures, &Doc29Prof); }

    void Doc29Panel::deselect(Doc29Noise& Doc29Ns) {
        if (!m_SelectedDoc29Noises.empty() && m_SelectedDoc29Noises.front() == &Doc29Ns)
            m_SelectedNpdData = nullptr;

        std::erase(m_SelectedDoc29Noises, &Doc29Ns);
    }

    void Doc29Panel::clearAircraftSelection() {
        m_SelectedDoc29Aircraft.clear();
        m_SelectedDoc29ProfileArrivals.clear();
        m_SelectedDoc29ProfileDepartures.clear();
    }

    void Doc29Panel::clearNoiseSelection() {
        m_SelectedDoc29Noises.clear();
        m_SelectedNpdData = nullptr;
    }

    void Doc29Panel::clearSelection() {
        clearAircraftSelection();
        clearNoiseSelection();
    }

    bool Doc29Panel::isSelected(Doc29Performance& Doc29Acft) const { return std::ranges::find(m_SelectedDoc29Aircraft, &Doc29Acft) != m_SelectedDoc29Aircraft.end(); }

    bool Doc29Panel::isSelected(Doc29ProfileArrival& Doc29Prof) const { return std::ranges::find(m_SelectedDoc29ProfileArrivals, &Doc29Prof) != m_SelectedDoc29ProfileArrivals.end(); }

    bool Doc29Panel::isSelected(Doc29ProfileDeparture& Doc29Prof) const { return std::ranges::find(m_SelectedDoc29ProfileDepartures, &Doc29Prof) != m_SelectedDoc29ProfileDepartures.end(); }

    bool Doc29Panel::isSelected(Doc29Noise& Doc29Ns) const { return std::ranges::find(m_SelectedDoc29Noises, &Doc29Ns) != m_SelectedDoc29Noises.end(); }

    void Doc29Panel::reset() { clearSelection(); }

    void Doc29Panel::onNoiseRunStart() { m_SelectedNpdData = nullptr; }

    void Doc29Panel::imGuiDraw() {
        if (!isOpen())
            return;

        auto& study = Application::study();

        // Initialize
        ImGui::Begin(m_Name.c_str(), &m_Open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse);

        // Left side
        ImGui::BeginChild("Doc29", ImVec2(-ImGui::GetContentRegionAvail().x * UI::g_StandardLeftAlignFraction, 0.0f));

        // Aircrafts
        ImGui::PushID("Performance");
        ImGui::AlignTextToFramePadding();
        UI::textInfo("Performance");
        ImGui::SameLine();

        // Text Filter
        static UI::TextFilter filterAcft;
        filterAcft.draw();

        // Edit Popup
        UI::buttonEditRight();
        if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonLeft))
        {
            if (UI::selectableNew("Jet"))
            {
                auto [doc29Acft, added] = study.Doc29Performances.addPerformance(Doc29Performance::Type::Jet);
                if (added)
                {
                    select(doc29Acft);
                    ImGui::CloseCurrentPopup();
                }
            }
            if (UI::selectableNew("Turboprop"))
            {
                auto [doc29Acft, added] = study.Doc29Performances.addPerformance(Doc29Performance::Type::Turboprop);
                if (added)
                {
                    select(doc29Acft);
                    ImGui::CloseCurrentPopup();
                }
            }
            if (UI::selectableNew("Piston"))
            {
                auto [doc29Acft, added] = study.Doc29Performances.addPerformance(Doc29Performance::Type::Piston);
                if (added)
                {
                    select(doc29Acft);
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::Separator();

            if (UI::selectableDelete("All"))
            {
                clearAircraftSelection();
                Application::get().queueAsyncTask([&] { study.Doc29Performances.erasePerformances(); }, "Deleting all Doc29 Performance entries");
            }
            ImGui::EndPopup();
        }

        if (UI::beginTable("Doc29 Performance", 2, ImGuiTableFlags_None, ImVec2(0.0f, UI::getTableHeight(study.Doc29Performances().size(), true, ImGui::GetContentRegionAvail().y / 2.0f))))
        {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            for (const auto& [AcftId, Acft] : study.Doc29Performances())
            {
                if (filterAcft.passesFilter(AcftId))
                    drawDoc29AircraftNode(AcftId, *Acft);
            }

            UI::endTable();
        }

        ImGui::PopID(); // Aircrafts

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Noise Entries
        ImGui::PushID("Noise Entries");
        ImGui::AlignTextToFramePadding();
        UI::textInfo("Noise");
        ImGui::SameLine();

        // Text Filter
        static UI::TextFilter filterNs;
        filterNs.draw();

        // Edit Popup
        UI::buttonEditRight();
        if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonLeft))
        {
            if (UI::selectableNew())
            {
                auto [doc29Ns, added] = study.Doc29Noises.addNoise();
                if (added)
                {
                    select(doc29Ns);
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::Separator();

            if (UI::selectableDelete("All"))
            {
                clearNoiseSelection();
                Application::get().queueAsyncTask([&] { study.Doc29Noises.eraseNoises(); }, "Deleting all Doc29 Noise entries");
            }

            ImGui::EndPopup();
        }

        if (UI::beginTable("Doc29 Noises", 1))
        {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            for (auto& [doc29NsId, doc29Ns] : study.Doc29Noises())
            {
                if (filterNs.passesFilter(doc29NsId))
                    drawDoc29NoiseNode(doc29NsId, doc29Ns);
            }

            UI::endTable();
        }

        ImGui::PopID(); // Noise Entries

        ImGui::EndChild();

        ImGui::SameLine();

        // Draw selected data
        if (!m_SelectedDoc29Aircraft.empty())
        {
            ImGui::BeginChild("Selection Data"); // Uses the rest of the space
            drawSelectedDoc29Aircraft();
            ImGui::EndChild();
        }
        else if (!m_SelectedDoc29Noises.empty())
        {
            ImGui::BeginChild("Selection Data"); // Uses the rest of the space
            drawSelectedDoc29Noise();
            ImGui::EndChild();
        }

        // Actions outside loops
        if (m_Action)
        {
            m_Action();
            m_Action = nullptr;
        }

        ImGui::End();
    }

    void Doc29Panel::drawDoc29AircraftNode(const std::string& Doc29AcftId, Doc29Performance& Doc29Acft) {
        auto& study = Application::study();

        ImGui::TableNextRow();
        ImGui::PushID(Doc29AcftId.c_str());

        UI::tableNextColumn(false);

        // Selectable Row
        if (UI::selectableRowEmpty(isSelected(Doc29Acft)))
            select(Doc29Acft);

        if (ImGui::BeginPopupContextItem())
        {
            ImGui::BeginDisabled(study.Blocks.notRemovable(Doc29Acft));
            if (UI::selectableDelete())
            {
                deselect(Doc29Acft);
                m_Action = [&] { study.Doc29Performances.erasePerformance(Doc29Acft); };
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndDisabled();

            ImGui::EndPopup();
        }

        // Name
        ImGui::BeginDisabled(study.Blocks.notEditable(Doc29Acft));
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (UI::inputText("Name", Doc29Acft.Name, Doc29Acft.Name != Doc29AcftId && study.Doc29Performances().contains(Doc29Acft.Name), "Aircraft name", std::format("Aircraft '{}' already exists in this study.", Doc29Acft.Name)))
            if (Doc29Acft.Name != Doc29AcftId)
                m_Action = [&] { study.Doc29Performances.updateKeyPerformance(Doc29Acft, Doc29AcftId); };
        ImGui::EndDisabled(); // Not editable
        if (UI::isItemClicked())
            select(Doc29Acft);

        // Type
        UI::tableNextColumn(false);
        UI::textInfo(Doc29Performance::Types.toString(Doc29Acft.type()));

        ImGui::PopID();
    }

    void Doc29Panel::drawDoc29NoiseNode(const std::string& Doc29NsId, Doc29Noise& Doc29Ns) {
        auto& study = Application::study();

        ImGui::TableNextRow();
        ImGui::PushID(Doc29NsId.c_str());

        UI::tableNextColumn(false);

        // Selectable Row
        if (UI::selectableRowEmpty(isSelected(Doc29Ns)))
            select(Doc29Ns);

        if (ImGui::BeginPopupContextItem())
        {
            ImGui::BeginDisabled(study.Blocks.notRemovable(Doc29Ns));
            if (UI::selectableDelete())
            {
                deselect(Doc29Ns);
                m_Action = [&] { study.Doc29Noises.eraseNoise(Doc29Ns); };
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndDisabled(); // Not removable

            ImGui::EndPopup();
        }

        // Name
        ImGui::BeginDisabled(study.Blocks.notEditable(Doc29Ns));
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (UI::inputText("Name", Doc29Ns.Name, Doc29Ns.Name != Doc29NsId && study.Doc29Noises().contains(Doc29Ns.Name), "Noise entry name", std::format("Noise entry '{}' already exists in this study.", Doc29Ns.Name)))
            if (Doc29Ns.Name != Doc29NsId)
                m_Action = [&] { study.Doc29Noises.updateKeyNoise(Doc29Ns, Doc29NsId); };
        ImGui::EndDisabled(); // Not editable
        if (UI::isItemClicked())
            select(Doc29Ns);

        ImGui::PopID();
    }

    void Doc29Panel::drawSelectedDoc29Aircraft() {
        GRAPE_ASSERT(!m_SelectedDoc29Aircraft.empty() && m_SelectedDoc29Aircraft.front());
        Doc29Performance& doc29Acft = *m_SelectedDoc29Aircraft.front();

        auto& study = Application::study();

        // Thrust
        if (ImGui::CollapsingHeader("Thrust"))
        {
            ImGui::BeginDisabled(study.Blocks.notEditable(doc29Acft));
            drawSelectedDoc29AircraftThrust();
            ImGui::EndDisabled();
        }

        // Aerodynamic Coefficients
        if (ImGui::CollapsingHeader("Aerodynamic Coefficients"))
        {
            ImGui::BeginDisabled(study.Blocks.notEditable(doc29Acft));
            drawSelectedDoc29AircraftAerodynamicCoefficients();
            ImGui::EndDisabled();
        }

        // Arrival Profiles
        if (ImGui::CollapsingHeader("Arrival Profiles"))
        {
            ImGui::PushID("Arrival Profiles");
            ImGui::BeginDisabled(study.Blocks.notEditable(doc29Acft));

            // Left side
            ImGui::BeginChild("Left Side", ImVec2(-ImGui::GetContentRegionAvail().x * UI::g_StandardLeftAlignFraction, 0.0f));

            UI::buttonEditRight();
            if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonLeft))
            {
                if (UI::selectableNew("Points"))
                    study.Doc29Performances.addProfileArrival(doc29Acft, Doc29Profile::Type::Points);

                if (UI::selectableNew("Procedural"))
                    study.Doc29Performances.addProfileArrival(doc29Acft, Doc29Profile::Type::Procedural);

                if (UI::selectableDelete("All Arrival Profiles"))
                {
                    m_SelectedDoc29ProfileArrivals.clear();
                    study.Doc29Performances.eraseProfileArrivals(doc29Acft);
                }

                ImGui::EndPopup();
            }

            // Names
            if (UI::beginTable("Arrival Profiles", 2))
            {
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableHeadersRow();

                for (auto& [doc29ProfId, doc29ProfPtr] : doc29Acft.ArrivalProfiles)
                {
                    auto& doc29Prof = *doc29ProfPtr;
                    ImGui::TableNextRow();
                    ImGui::PushID(doc29ProfId.c_str());

                    UI::tableNextColumn(false);

                    // Selectable Row
                    if (UI::selectableRowEmpty(isSelected(doc29Prof)))
                        select(doc29Prof);

                    if (ImGui::BeginPopupContextItem())
                    {
                        ImGui::BeginDisabled(study.Blocks.notRemovable(doc29Prof));
                        if (UI::selectableDelete())
                        {
                            deselect(doc29Prof);
                            m_Action = [&] { study.Doc29Performances.eraseProfile(doc29Prof); };
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::EndDisabled();
                        ImGui::EndPopup();
                    }

                    // Name
                    ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
                    if (UI::inputText("Name", doc29Prof.Name, doc29Prof.Name != doc29ProfId && doc29Prof.parentDoc29Performance().ArrivalProfiles.contains(doc29Prof.Name), "Profile name", std::format("Arrival profile '{}' already exists in aircraft '{}'.", doc29Prof.Name, doc29Prof.parentDoc29Performance().Name)))
                        if (doc29Prof.Name != doc29ProfId)
                            m_Action = [&] { study.Doc29Performances.updateKeyProfile(doc29Prof, doc29ProfId); };
                    if (UI::isItemClicked())
                        select(doc29Prof);

                    // Type
                    UI::tableNextColumn(false);
                    UI::textInfo(Doc29Profile::Types.toString(doc29Prof.type()).append(" Profile"));
                    ImGui::PopID(); // Doc29 arrival profile
                }
                UI::endTable();
            }
            ImGui::EndChild(); // Left side

            // Right Side
            if (!m_SelectedDoc29ProfileArrivals.empty())
            {
                ImGui::SameLine();
                ImGui::BeginChild("Right Side"); // Use rest of the space
                auto& doc29Prof = *m_SelectedDoc29ProfileArrivals.front();
                Doc29ProfileDrawer drawer(doc29Prof);
                ImGui::EndChild();
            }

            ImGui::EndDisabled(); // Not editable Doc29 aircraft
            ImGui::PopID();       // Arrival profiles
        }

        // Departure Profiles
        if (ImGui::CollapsingHeader("Departure Profiles"))
        {
            ImGui::PushID("Departure Profiles");
            ImGui::BeginDisabled(study.Blocks.notEditable(doc29Acft));

            // Left side
            ImGui::BeginChild("Left Side", ImVec2(-ImGui::GetContentRegionAvail().x * UI::g_StandardLeftAlignFraction, 0.0f));

            UI::buttonEditRight();
            if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonLeft))
            {
                if (UI::selectableNew("Points"))
                    study.Doc29Performances.addProfileDeparture(doc29Acft, Doc29Profile::Type::Points);

                if (UI::selectableNew("Procedural"))
                    study.Doc29Performances.addProfileDeparture(doc29Acft, Doc29Profile::Type::Procedural);

                if (UI::selectableDelete("All Departure Profiles"))
                {
                    m_SelectedDoc29ProfileDepartures.clear();
                    study.Doc29Performances.eraseProfileDepartures(doc29Acft);
                }

                ImGui::EndPopup();
            }

            // Names
            if (UI::beginTable("Departure Profiles", 2))
            {
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableHeadersRow();

                for (auto& [doc29ProfId, doc29ProfPtr] : doc29Acft.DepartureProfiles)
                {
                    auto& doc29Prof = *doc29ProfPtr;
                    ImGui::TableNextRow();
                    ImGui::PushID(doc29ProfId.c_str());

                    UI::tableNextColumn(false);

                    // Selectable Row
                    if (UI::selectableRowEmpty(isSelected(doc29Prof)))
                        select(doc29Prof);

                    if (ImGui::BeginPopupContextItem())
                    {
                        ImGui::BeginDisabled(study.Blocks.notRemovable(doc29Prof));
                        if (UI::selectableDelete())
                        {
                            deselect(doc29Prof);
                            m_Action = [&] { study.Doc29Performances.eraseProfile(doc29Prof); };
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::EndDisabled();
                        ImGui::EndPopup();
                    }

                    // Name
                    ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
                    if (UI::inputText("Name", doc29Prof.Name, doc29Prof.Name != doc29ProfId && doc29Prof.parentDoc29Performance().DepartureProfiles.contains(doc29Prof.Name), "Profile name", std::format("Departure profile '{}' already exists in aircraft '{}'.", doc29Prof.Name, doc29Prof.parentDoc29Performance().Name)))
                        if (doc29Prof.Name != doc29ProfId)
                            m_Action = [&] { study.Doc29Performances.updateKeyProfile(doc29Prof, doc29ProfId); };
                    if (UI::isItemClicked())
                        select(doc29Prof);

                    // Type
                    UI::tableNextColumn(false);
                    UI::textInfo(Doc29Profile::Types.toString(doc29Prof.type()).append(" Profile"));
                    ImGui::PopID(); // Doc29 departure profile
                }
                UI::endTable();
            }
            ImGui::EndChild(); // Left side

            // Right Side
            if (!m_SelectedDoc29ProfileDepartures.empty())
            {
                ImGui::SameLine();
                ImGui::BeginChild("Right Side"); // Use rest of the space
                auto& doc29Prof = *m_SelectedDoc29ProfileDepartures.front();
                Doc29ProfileDrawer drawer(doc29Prof);
                ImGui::EndChild();
            }

            ImGui::EndDisabled(); // Not editable Doc29 aircraft
            ImGui::PopID();       // Departure Profiles
        }
    }

    void Doc29Panel::drawSelectedDoc29AircraftThrust() const {
        GRAPE_ASSERT(!m_SelectedDoc29Aircraft.empty() && m_SelectedDoc29Aircraft.front());
        Doc29Performance& doc29Acft = *m_SelectedDoc29Aircraft.front();

        const auto& study = Application::study();
        const auto& style = ImGui::GetStyle();

        const float thrustOffset = ImGui::GetCursorPosX() + ImGui::CalcTextSize("Engine Breakpoint Temperature:").x;

        // Thrust Type
        ImGui::BeginDisabled(doc29Acft.containsDepartureProceduralProfiles() || doc29Acft.containsArrivalProceduralProfiles());
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("Thrust Type:");
        ImGui::SameLine(thrustOffset, style.ItemInnerSpacing.x);
        ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
        const auto thrustTypeStr = Doc29Thrust::Types.toString(doc29Acft.thrust().type());
        if (ImGui::BeginCombo("##ThrustType", thrustTypeStr.c_str()))
        {
            for (const auto& type : doc29Acft.allowedThrustTypes())
            {
                const bool selected = type == doc29Acft.thrust().type();
                if (ImGui::Selectable(Doc29Thrust::Types.toString(type).c_str(), selected))
                {
                    doc29Acft.setThrustType(type);
                    study.Doc29Performances.updateThrust(doc29Acft);
                }
            }
            ImGui::EndCombo();
        }
        ImGui::EndDisabled(); // Thrust Type not editable

        Doc29ThrustDrawer engineCoeffsDrawer(doc29Acft);
    }

    void Doc29ThrustDrawer::visitDoc29ThrustRating(Doc29ThrustRating& Thrust) {
        const Settings& set = Application::settings();

        bool updated = false;
        std::function<void()> action = nullptr;

        const auto popupAdd = "addCoeffs";
        const ImGuiID popupAddId = ImGui::GetID(popupAdd);

        ImGui::Separator();

        if (UI::buttonNew("Thrust Rating"))
            ImGui::OpenPopup(popupAddId);

        if (UI::beginTable("Thrust Rating Coefficients", 6, ImGuiTableFlags_None, ImVec2(0.0f, UI::getTableHeight(Thrust.Coeffs.size()))))
        {
            ImGui::TableSetupColumn("Thrust Rating", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn(std::format("E ({})", set.ThrustUnits.shortName()).c_str(), ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn(std::format("F ({})", set.Doc29ThrustFUnits.shortName()).c_str(), ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn(std::format("Ga ({})", set.Doc29ThrustGaUnits.shortName()).c_str(), ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn(std::format("Gb ({})", set.Doc29ThrustGbUnits.shortName()).c_str(), ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn(std::format("H ({})", set.Doc29ThrustHUnits.shortName()).c_str(), ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            for (auto& [thrustRating, coeffs] : Thrust)
            {
                ImGui::PushID(magic_enum::enum_integer(thrustRating));
                ImGui::TableNextRow();

                UI::tableNextColumn(false);

                // Selectable Row
                UI::selectableRowEmpty();
                if (ImGui::BeginPopupContextItem())
                {
                    if (UI::selectableNew("Thrust Rating", true, ImGuiSelectableFlags_DontClosePopups))
                        ImGui::OpenPopup(popupAddId);

                    const bool block = (thrustRating == Doc29Thrust::Rating::MaximumTakeoff || thrustRating == Doc29Thrust::Rating::MaximumClimb) && m_Doc29Acft.containsDepartureProceduralProfiles() || thrustRating == Doc29Thrust::Rating::Idle && m_Doc29Acft.containsArrivalProceduralProfiles();
                    ImGui::BeginDisabled(block);
                    if (UI::selectableDelete())
                    {
                        action = [&] { Thrust.Coeffs.erase(thrustRating); };
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndDisabled();

                    ImGui::EndPopup();
                }

                UI::textInfo(Doc29Thrust::Ratings.toString(thrustRating));

                UI::tableNextColumn();
                if (UI::inputDouble("E", coeffs.E, set.ThrustUnits, false))
                    updated = true;

                UI::tableNextColumn();
                if (UI::inputDouble("F", coeffs.F, set.Doc29ThrustFUnits, false))
                    updated = true;

                UI::tableNextColumn();
                if (UI::inputDouble("Ga", coeffs.Ga, set.Doc29ThrustGaUnits, false))
                    updated = true;

                UI::tableNextColumn();
                if (UI::inputDouble("Gb", coeffs.Gb, set.Doc29ThrustGbUnits, false))
                    updated = true;

                UI::tableNextColumn();
                if (UI::inputDouble("H", coeffs.H, set.Doc29ThrustHUnits, false))
                    updated = true;

                ImGui::PopID(); // Thrust Rating
            }
            UI::endTable();
        }

        // Popups
        if (ImGui::BeginPopup(popupAdd))
        {
            for (const auto& ratingStr : Doc29Thrust::Ratings)
            {
                auto rating = Doc29Thrust::Ratings.fromString(ratingStr);
                if (Thrust.Coeffs.contains(rating))
                    continue;

                if (ImGui::Selectable(ratingStr, false))
                {
                    auto [EngineCoeffs, added] = Thrust.Coeffs.add(rating);
                    if (added)
                    {
                        ImGui::CloseCurrentPopup();
                        updated = true;
                    }
                }
            }
            ImGui::EndPopup();
        }

        if (action)
        {
            action();
            updated = true;
        }

        if (updated)
            Application::study().Doc29Performances.updateThrust(m_Doc29Acft);
    }

    void Doc29ThrustDrawer::visitDoc29ThrustPropeller(Doc29ThrustRatingPropeller& Thrust) {
        const Settings& set = Application::settings();

        bool updated = false;
        std::function<void()> action = nullptr;

        const auto popupAdd = "addCoeffs";
        const ImGuiID popupAddId = ImGui::GetID(popupAdd);

        ImGui::Separator();

        if (UI::buttonNew("Thrust Rating"))
            ImGui::OpenPopup(popupAddId);

        if (UI::beginTable("Propeller Engine Coefficients", 3, ImGuiTableFlags_None, ImVec2(0.0f, UI::getTableHeight(Thrust.Coeffs.size()))))
        {
            ImGui::TableSetupColumn("Thrust Rating", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Propeller Efficiency (%)", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn(std::format("Propeller Power ({})", set.PowerUnits.shortName()).c_str(), ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            for (auto& [thrustRating, engineCoeffs] : Thrust)
            {
                ImGui::PushID(magic_enum::enum_integer(thrustRating));
                ImGui::TableNextRow();

                UI::tableNextColumn(false);

                // Selectable Row
                UI::selectableRowEmpty();
                if (ImGui::BeginPopupContextItem())
                {
                    if (UI::selectableNew("Thrust Rating", true, ImGuiSelectableFlags_DontClosePopups))
                        ImGui::OpenPopup(popupAddId);

                    const bool block = (thrustRating == Doc29Thrust::Rating::MaximumTakeoff || thrustRating == Doc29Thrust::Rating::MaximumClimb) && m_Doc29Acft.containsDepartureProceduralProfiles() || thrustRating == Doc29Thrust::Rating::Idle && m_Doc29Acft.containsArrivalProceduralProfiles();
                    ImGui::BeginDisabled(block);
                    if (UI::selectableDelete())
                    {
                        action = [&] { Thrust.Coeffs.erase(thrustRating); };
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndDisabled();

                    ImGui::EndPopup();
                }

                UI::textInfo(Doc29Thrust::Ratings.toString(thrustRating));

                UI::tableNextColumn();
                if (UI::inputPercentage("Pe", engineCoeffs.Pe, Constants::Precision, 1.0, 0, false))
                    updated = true; // Updating aircraft resets coefficients

                UI::tableNextColumn();
                if (UI::inputDouble("Pp", engineCoeffs.Pp, Constants::Precision, Constants::NaN, set.PowerUnits, false))
                    updated = true; // Updating aircraft resets coefficients

                ImGui::PopID();
            }
            UI::endTable();
        }

        // Popups
        if (ImGui::BeginPopup(popupAdd))
        {
            for (const auto& ratingStr : Doc29Thrust::Ratings)
            {
                auto rating = Doc29Thrust::Ratings.fromString(ratingStr);
                if (Thrust.Coeffs.contains(rating))
                    continue;

                if (ImGui::Selectable(ratingStr, false))
                {
                    auto [EngineCoeffs, added] = Thrust.Coeffs.add(rating);
                    if (added)
                    {
                        ImGui::CloseCurrentPopup();
                        updated = true;
                    }
                }
            }
            ImGui::EndPopup();
        }

        if (action)
        {
            action();
            updated = true;
        }

        if (updated)
            Application::study().Doc29Performances.updateThrust(m_Doc29Acft);
    }

    void Doc29Panel::drawSelectedDoc29AircraftAerodynamicCoefficients() {
        GRAPE_ASSERT(!m_SelectedDoc29Aircraft.empty() && m_SelectedDoc29Aircraft.front());
        Doc29Performance& doc29Acft = *m_SelectedDoc29Aircraft.front();

        const auto& study = Application::study();
        const auto& set = Application::settings();

        if (UI::buttonNew("Aerodynamic Coefficients"))
        {
            const std::string newStr = uniqueKeyGenerator(doc29Acft.AerodynamicCoefficients, "New coefficients");
            auto [aeroCoeffs, added] = doc29Acft.AerodynamicCoefficients.add(newStr, newStr);
            if (added)
                study.Doc29Performances.updateAerodynamicCoefficients(doc29Acft);
        }

        if (UI::beginTable("Aerodynamic Coefficients", 6, ImGuiTableFlags_None, ImVec2(0.0f, UI::getTableHeight(doc29Acft.AerodynamicCoefficients.size()))))
        {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("R", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn(std::format("B ({})", set.Doc29AeroBUnits.shortName()).c_str(), ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn(std::format("C ({})", set.Doc29AeroCDUnits.shortName()).c_str(), ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn(std::format("D ({})", set.Doc29AeroCDUnits.shortName()).c_str(), ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            for (auto& [CoeffsName, Coeffs] : doc29Acft.AerodynamicCoefficients)
            {
                ImGui::PushID(CoeffsName.c_str());
                ImGui::TableNextRow();

                const bool coeffsBlocked = doc29Acft.b_BlockedAerodynamicCoefficients.contains(Coeffs);

                UI::tableNextColumn(false);

                // Selectable Row
                UI::selectableRowEmpty();
                if (ImGui::BeginPopupContextItem())
                {
                    if (UI::selectableDelete())
                    {
                        m_Action = [&] {
                            if (doc29Acft.AerodynamicCoefficients.erase(CoeffsName))
                                study.Doc29Performances.updateAerodynamicCoefficients(doc29Acft);
                            };
                        ImGui::CloseCurrentPopup();
                    }
                    if (coeffsBlocked && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                        ImGui::SetTooltip("%s", std::format("{} profiles use this coefficients.", doc29Acft.b_BlockedAerodynamicCoefficients.blockingCount(Coeffs)).c_str());

                    ImGui::EndPopup();
                }

                // Name
                ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
                UI::inputText("Name", Coeffs.Name, Coeffs.Name != CoeffsName && doc29Acft.AerodynamicCoefficients.contains(Coeffs.Name), "Aerodynamic coefficient name", std::format("Aerodynamic coefficient '{}' already exists in aircraft '{}'", Coeffs.Name, doc29Acft.Name));
                if (ImGui::IsItemDeactivatedAfterEdit() && Coeffs.Name != CoeffsName)
                    study.Doc29Performances.updateKeyAerodynamicCoefficients(doc29Acft, CoeffsName);

                UI::tableNextColumn();
                ImGui::BeginDisabled(coeffsBlocked);
                if (ImGui::BeginCombo("##Type", Doc29AerodynamicCoefficients::Types.toString(Coeffs.CoefficientType).c_str()))
                {
                    for (const auto& typeStr : Doc29AerodynamicCoefficients::Types)
                    {
                        auto typ = Doc29AerodynamicCoefficients::Types.fromString(typeStr);
                        const bool selected = typ == Coeffs.CoefficientType;
                        if (ImGui::Selectable(typeStr, selected))
                        {
                            Coeffs.CoefficientType = typ;
                            study.Doc29Performances.updateAerodynamicCoefficients(doc29Acft);
                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::EndDisabled();
                if (coeffsBlocked && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                    ImGui::SetTooltip("%s", std::format("{} profiles use this coefficients.", doc29Acft.b_BlockedAerodynamicCoefficients.blockingCount(Coeffs)).c_str());

                // R
                UI::tableNextColumn();
                if (UI::inputDouble("R", Coeffs.R, Constants::Precision, Constants::NaN))
                    study.Doc29Performances.updateAerodynamicCoefficients(doc29Acft);

                // B (ft/lbf)
                UI::tableNextColumn();
                if (Coeffs.CoefficientType == Doc29AerodynamicCoefficients::Type::Takeoff)
                    if (UI::inputDouble("B", Coeffs.B, Constants::Precision, Constants::NaN, set.Doc29AeroBUnits, false))
                        study.Doc29Performances.updateAerodynamicCoefficients(doc29Acft);

                // C (kts/sqrt(lbf))
                UI::tableNextColumn();
                if (Coeffs.CoefficientType == Doc29AerodynamicCoefficients::Type::Takeoff)
                    if (UI::inputDouble("C", Coeffs.C, Constants::Precision, Constants::NaN, set.Doc29AeroCDUnits, false))
                        study.Doc29Performances.updateAerodynamicCoefficients(doc29Acft);

                // D (kts/sqrt(lbf))
                UI::tableNextColumn();
                if (Coeffs.CoefficientType == Doc29AerodynamicCoefficients::Type::Land)
                    if (UI::inputDouble("D", Coeffs.D, Constants::Precision, Constants::NaN, set.Doc29AeroCDUnits, false))
                        study.Doc29Performances.updateAerodynamicCoefficients(doc29Acft);

                ImGui::PopID();
            }
            UI::endTable();
        }
    }

    void Doc29ProfileDrawer::visitDoc29ProfileArrivalPoints(Doc29ProfileArrivalPoints& Doc29Prof) {
        const Settings& set = Application::settings();
        bool updated = false;

        if (UI::buttonNew())
        {
            Doc29Prof.addPoint();
            updated = true;
        }
        if (!Doc29Prof.empty())
        {
            ImGui::SameLine();
            if (UI::buttonDelete("Clear"))
            {
                Doc29Prof.clear();
                updated = true;
            }
        }

        std::function<void()> action = nullptr;

        if (UI::beginTable("Profile Arrival Points", 4))
        {
            ImGui::TableSetupColumn(std::format("Cumulative Ground Distance ({})", set.DistanceUnits.shortName()).c_str());
            ImGui::TableSetupColumn(std::format("Altitude AFE ({})", set.AltitudeUnits.shortName()).c_str());
            ImGui::TableSetupColumn(std::format("True Airspeed ({})", set.SpeedUnits.shortName()).c_str());
            ImGui::TableSetupColumn(std::format("Thrust ({})", set.ThrustUnits.shortName()).c_str());
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            for (auto it = Doc29Prof.begin(); it != Doc29Prof.end(); ++it)
            {
                const std::size_t i = std::distance(Doc29Prof.begin(), it);
                auto& [cumGroundDist, pt] = *it;

                ImGui::PushID(&pt);
                ImGui::TableNextRow();

                // Selectable Row
                UI::tableNextColumn(false);
                UI::selectableRowEmpty();
                if (ImGui::BeginPopupContextItem())
                {
                    if (UI::selectableNew("Above"))
                        action = [&, i] { Doc29Prof.insertPoint(i); };

                    if (UI::selectableNew("Below"))
                        action = [&, i] { Doc29Prof.insertPoint(i + 1); };

                    if (UI::selectableDelete())
                        action = [&, i] { Doc29Prof.deletePoint(i); };
                    ImGui::EndPopup();
                }

                double newCumGroundDist = cumGroundDist;
                ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
                if (UI::inputDouble("Cumulative Ground Distance", newCumGroundDist, set.DistanceUnits, false))
                    action = [&, i, newCumGroundDist] { Doc29Prof.updateCumulativeGroundDistance(i, newCumGroundDist); };

                UI::tableNextColumn();
                if (UI::inputDouble("Altitude AFE", pt.AltitudeAfe, set.AltitudeUnits, false))
                    updated = true;

                UI::tableNextColumn();
                if (UI::inputDouble("True Airspeed", pt.TrueAirspeed, 0.0, Constants::NaN, set.SpeedUnits, false))
                    updated = true;

                UI::tableNextColumn();
                if (UI::inputDouble("Thrust", pt.CorrNetThrustPerEng, Constants::Precision, Constants::NaN, set.ThrustUnits, false))
                    updated = true;

                ImGui::PopID();
            }
            UI::endTable();
        }

        if (action)
        {
            action();
            updated = true;
        }

        if (updated)
            Application::study().Doc29Performances.updateProfile(Doc29Prof);
    }

    void Doc29ProfileDrawer::visitDoc29ProfileDeparturePoints(Doc29ProfileDeparturePoints& Doc29Prof) {
        const Settings& set = Application::settings();

        bool updated = false;

        if (UI::buttonNew())
        {
            Doc29Prof.addPoint();
            updated = true;
        }

        if (!Doc29Prof.empty())
        {
            ImGui::SameLine();
            if (UI::buttonDelete("Clear"))
            {
                Doc29Prof.clear();
                updated = true;
            }
        }

        std::function<void()> action = nullptr;

        if (UI::beginTable("Profile Departure Points", 4))
        {
            ImGui::TableSetupColumn(std::format("Cumulative Ground Distance ({})", set.DistanceUnits.shortName()).c_str());
            ImGui::TableSetupColumn(std::format("Altitude AFE ({})", set.AltitudeUnits.shortName()).c_str());
            ImGui::TableSetupColumn(std::format("True Airspeed ({})", set.SpeedUnits.shortName()).c_str());
            ImGui::TableSetupColumn(std::format("Thrust ({})", set.ThrustUnits.shortName()).c_str());
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            for (auto it = Doc29Prof.begin(); it != Doc29Prof.end(); ++it)
            {
                const std::size_t i = std::distance(Doc29Prof.begin(), it);
                auto& [cumGroundDistance, pt] = *it;

                ImGui::PushID(&pt);
                ImGui::TableNextRow();

                // Selectable Row
                UI::tableNextColumn(false);
                UI::selectableRowEmpty();
                if (ImGui::BeginPopupContextItem())
                {
                    ImGui::BeginDisabled(cumGroundDistance < Constants::Precision);
                    if (UI::selectableNew("Above"))
                        action = [&, i] { Doc29Prof.insertPoint(i); };
                    ImGui::EndDisabled(); // Inserting above 0 m cumulative ground distance

                    if (UI::selectableNew("Below"))
                        action = [&, i] { Doc29Prof.insertPoint(i + 1); };

                    if (i != 0)
                        if (UI::selectableDelete())
                            action = [&, i] { Doc29Prof.deletePoint(i); };
                    ImGui::EndPopup();
                }

                double newCumGroundDistance = cumGroundDistance;
                ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
                if (UI::inputDouble("Cumulative Ground Distance", newCumGroundDistance, 0.0, Constants::NaN, set.DistanceUnits, false))
                    action = [&, i, newCumGroundDistance] { Doc29Prof.updateCumulativeGroundDistance(i, newCumGroundDistance); };

                UI::tableNextColumn();
                if (UI::inputDouble("Altitude AFE", pt.AltitudeAfe, set.AltitudeUnits, false))
                    updated = true;

                UI::tableNextColumn();
                if (UI::inputDouble("True Airspeed", pt.TrueAirspeed, 0.0, Constants::NaN, set.SpeedUnits, false))
                    updated = true;

                UI::tableNextColumn();
                if (UI::inputDouble("Thrust", pt.CorrNetThrustPerEng, Constants::Precision, Constants::NaN, set.ThrustUnits, false))
                    updated = true;

                ImGui::PopID();
            }
            UI::endTable();
        }

        if (action)
        {
            action();
            updated = true;
        }

        if (updated)
            Application::study().Doc29Performances.updateProfile(Doc29Prof);
    }

    void Doc29ProfileDrawer::visitDoc29ProfileArrivalProcedural(Doc29ProfileArrivalProcedural& Doc29Prof) {
        const Settings& set = Application::settings();
        const ImGuiStyle& style = ImGui::GetStyle();

        bool updated = false;

        std::function<void()> action = nullptr;
        // Air Steps
        {
            ImGui::PushID("Air Steps");
            UI::textInfo("Air Steps");

            UI::buttonNew();
            if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonLeft))
            {
                if (UI::selectableNew("Descend Decelerate"))
                {
                    Doc29Prof.addDescendDecelerate();
                    updated = true; // Updating profile resets steps
                }

                if (Doc29Prof.parentDoc29Performance().thrust().isRatingSet(Doc29Thrust::Rating::Idle))
                    if (UI::selectableNew("Descend Idle"))
                    {
                        Doc29Prof.addDescendIdle();
                        updated = true; // Updating profile resets steps
                    }

                if (UI::selectableNew("Level"))
                {
                    Doc29Prof.addLevel();
                    updated = true; // Updating profile resets steps
                }

                if (UI::selectableNew("Level Decelerate"))
                {
                    Doc29Prof.addLevelDecelerate();
                    updated = true; // Updating profile resets steps
                }

                if (Doc29Prof.parentDoc29Performance().thrust().isRatingSet(Doc29Thrust::Rating::Idle))
                    if (UI::selectableNew("Level Idle"))
                    {
                        Doc29Prof.addLevelIdle();
                        updated = true; // Updating profile resets steps
                    }

                ImGui::EndPopup();
            }

            if (!Doc29Prof.airStepsEmpty())
            {
                ImGui::SameLine();
                if (UI::buttonDelete("Clear"))
                {
                    Doc29Prof.clearAirSteps();
                    updated = true; // Updating profile resets steps
                }
            }
            ImGui::PopID();
        }

        if (UI::beginTable("Arrival Steps", 6, ImGuiTableFlags_None, ImVec2(0.0f, -ImGui::GetContentRegionAvail().y * 0.5f)))
        {
            ImGui::TableSetupColumn("Step Type");
            ImGui::TableSetupColumn("Aerodynamic Coefficients");
            ImGui::TableSetupColumn(std::format("Start Altitude AFE ({})", set.AltitudeUnits.shortName()).c_str());
            ImGui::TableSetupColumn("Descent Angle");
            ImGui::TableSetupColumn(std::format("Start Calibrated Airspeed ({})", set.SpeedUnits.shortName()).c_str());
            ImGui::TableSetupColumn(std::format("Ground Distance ({})", set.DistanceUnits.shortName()).c_str());
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            for (auto it = Doc29Prof.airStepsBegin(); it != Doc29Prof.airStepsEnd(); ++it)
            {
                const auto i = std::distance(Doc29Prof.airStepsBegin(), it);
                auto& step = *it;

                ImGui::PushID(&step);
                ImGui::TableNextRow();

                // Selectable Row
                UI::tableNextColumn(false);
                UI::selectableRowEmpty();
                if (ImGui::BeginPopupContextItem())
                {
                    if (UI::selectableDelete())
                        action = [&, i] { Doc29Prof.deleteStep(i); };
                    ImGui::EndPopup();
                }

                // Step Type
                UI::textInfo(std::visit(Doc29ProfileArrivalProcedural::VisitorStepTypeString(), step));

                // Aerodynamic Coefficients
                UI::tableNextColumn();
                std::visit(Overload{
                    [&](Doc29ProfileArrivalProcedural::DescendLand&) { GRAPE_ASSERT(false); },
                    [&](Doc29ProfileArrivalProcedural::GroundDecelerate&) { GRAPE_ASSERT(false); },
                    [&](Doc29ProfileArrivalProcedural::DescendIdle&) {},
                    [&](Doc29ProfileArrivalProcedural::LevelIdle&) {},
                    [&](auto& CoefficientsStep) {
                        auto& currAeroCoeffsName = CoefficientsStep.Doc29AerodynamicCoefficients->Name;
                        if (ImGui::BeginCombo("##AerodynamicCoefficients", currAeroCoeffsName.c_str()))
                        {
                            for (const auto& [aeroCoeffsId, aeroCoeffs] : Doc29Prof.parentDoc29Performance().AerodynamicCoefficients)
                            {
                                if (ImGui::Selectable(aeroCoeffsId.c_str(), aeroCoeffsId == currAeroCoeffsName))
                                {
                                    Doc29Prof.parentDoc29Performance().b_BlockedAerodynamicCoefficients.unblock(*CoefficientsStep.Doc29AerodynamicCoefficients, Doc29Prof);
                                    Doc29Prof.parentDoc29Performance().b_BlockedAerodynamicCoefficients.block(aeroCoeffs, Doc29Prof);
                                    CoefficientsStep.Doc29AerodynamicCoefficients = &aeroCoeffs;
                                    updated = true;
                                }
                            }
                            ImGui::EndCombo();
                        }
                    },
                    }, step);

                // Start Altitude AFE
                UI::tableNextColumn();
                std::visit(Overload{
                    [&](Doc29ProfileArrivalProcedural::DescendLand&) { GRAPE_ASSERT(false); },
                    [&](Doc29ProfileArrivalProcedural::GroundDecelerate&) { GRAPE_ASSERT(false); },
                    [&](Doc29ProfileArrivalProcedural::Level&) {},
                    [&](Doc29ProfileArrivalProcedural::LevelDecelerate&) {},
                    [&](Doc29ProfileArrivalProcedural::LevelIdle&) {},
                    [&](auto& StartAltitudeStep) {
                        if (UI::inputDouble("Start altitude AFE", StartAltitudeStep.StartAltitudeAfe, set.AltitudeUnits, false))
                            updated = true;
                    },
                    }, step);

                // Descent Angle
                UI::tableNextColumn();
                std::visit(Overload{
                    [&](Doc29ProfileArrivalProcedural::DescendLand&) { GRAPE_ASSERT(false); },
                    [&](Doc29ProfileArrivalProcedural::GroundDecelerate&) { GRAPE_ASSERT(false); },
                    [&](Doc29ProfileArrivalProcedural::Level&) {},
                    [&](Doc29ProfileArrivalProcedural::LevelDecelerate&) {},
                    [&](Doc29ProfileArrivalProcedural::LevelIdle&) {},
                    [&](auto& DescentAngleStep) {
                        if (UI::inputDouble("Descent angle", DescentAngleStep.DescentAngle, -90.0, -Constants::Precision))
                            updated = true;
                    },
                    }, step);

                // Start Calibrated Airspeed
                UI::tableNextColumn();
                std::visit(Overload{
                    [&](Doc29ProfileArrivalProcedural::DescendLand&) { GRAPE_ASSERT(false); },
                    [&](Doc29ProfileArrivalProcedural::GroundDecelerate&) { GRAPE_ASSERT(false); },
                    [&](Doc29ProfileArrivalProcedural::Level&) {},
                    [&](auto& StartCalibratedAirspeedStep) {
                        if (UI::inputDouble("Start calibrated airspeed", StartCalibratedAirspeedStep.StartCalibratedAirspeed, 0.0, Constants::NaN, set.SpeedUnits, false))
                            updated = true;
                    },
                    }, step);

                // Ground Distance
                UI::tableNextColumn();
                std::visit(Overload{
                    [&](Doc29ProfileArrivalProcedural::DescendLand&) { GRAPE_ASSERT(false); },
                    [&](Doc29ProfileArrivalProcedural::GroundDecelerate&) { GRAPE_ASSERT(false); },
                    [&](Doc29ProfileArrivalProcedural::DescendDecelerate&) {},
                    [&](Doc29ProfileArrivalProcedural::DescendIdle&) {},
                    [&](auto& GroundDistanceStep) {
                        if (UI::inputDouble("Ground distance", GroundDistanceStep.GroundDistance, Constants::Precision, Constants::NaN, set.DistanceUnits, false))
                            updated = true;
                    },
                    }, step);

                ImGui::PopID();
            }
            UI::endTable();
        }
        ImGui::Separator();

        // Descend land
        {
            auto& landStep = Doc29Prof.descendLandStep();
            UI::textInfo("Descend Land");

            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Aerodynamic Coefficients:");
            ImGui::SameLine(0.0, style.ItemInnerSpacing.x);
            ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
            auto& currAeroCoeffsName = landStep.Doc29AerodynamicCoefficients->Name;
            if (ImGui::BeginCombo("##AerodynamicCoefficientsDescendLand", currAeroCoeffsName.c_str()))
            {
                for (const auto& aeroCoeffsId : Doc29Prof.parentDoc29Performance().aerodynamicCoefficientsWithType(Doc29AerodynamicCoefficients::Type::Land))
                {
                    if (ImGui::Selectable(aeroCoeffsId.c_str(), aeroCoeffsId == currAeroCoeffsName))
                    {
                        auto& aeroCoeffs = Doc29Prof.parentDoc29Performance().AerodynamicCoefficients(aeroCoeffsId);
                        Doc29Prof.parentDoc29Performance().b_BlockedAerodynamicCoefficients.unblock(*landStep.Doc29AerodynamicCoefficients, Doc29Prof);
                        Doc29Prof.parentDoc29Performance().b_BlockedAerodynamicCoefficients.block(aeroCoeffs, Doc29Prof);
                        landStep.Doc29AerodynamicCoefficients = &aeroCoeffs;
                        updated = true;
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Descent Angle:");
            ImGui::SameLine(0.0, style.ItemInnerSpacing.x);
            ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
            if (UI::inputDouble("Descent angle", landStep.DescentAngle, -90.0, -Constants::Precision))
                updated = true;

            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Threshold Crossing Altitude AFE:");
            ImGui::SameLine(0.0, style.ItemInnerSpacing.x);
            ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
            if (UI::inputDouble("Threshold crossing altitude AFE", landStep.ThresholdCrossingAltitudeAfe, set.AltitudeUnits))
                updated = true;

            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Touchdown Roll:");
            ImGui::SameLine(0.0, style.ItemInnerSpacing.x);
            ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
            if (UI::inputDouble("Touchdown Roll", landStep.TouchdownRoll, set.DistanceUnits))
                updated = true;

            ImGui::Separator();
        }

        {
            ImGui::PushID("Ground Steps");
            UI::textInfo("Ground Steps");

            if (UI::buttonNew())
            {
                Doc29Prof.addGroundDecelerate();
                updated = true;
            }

            if (!Doc29Prof.groundStepsEmpty())
            {
                ImGui::SameLine();
                if (UI::buttonDelete("Clear"))
                {
                    Doc29Prof.clearGroundSteps();
                    updated = true;
                }
            }
            ImGui::PopID();
        }

        if (UI::beginTable("Arrival Ground Steps", 4))
        {
            ImGui::TableSetupColumn("Step Type");
            ImGui::TableSetupColumn(std::format("Ground Distance ({})", set.DistanceUnits.shortName()).c_str());
            ImGui::TableSetupColumn(std::format("Start Calibrated Airspeed ({})", set.SpeedUnits.shortName()).c_str());
            ImGui::TableSetupColumn("Start Thrust %");
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            for (auto it = Doc29Prof.groundStepsBegin(); it != Doc29Prof.groundStepsEnd(); ++it)
            {
                const auto i = std::distance(Doc29Prof.begin(), it);
                auto& groundStep = std::get<Doc29ProfileArrivalProcedural::GroundDecelerate>(*it);

                ImGui::PushID(&groundStep);
                ImGui::TableNextRow();

                // Selectable Row
                UI::tableNextColumn(false);
                UI::selectableRowEmpty();
                if (ImGui::BeginPopupContextItem())
                {
                    if (UI::selectableDelete())
                        action = [&, i] { Doc29Prof.deleteStep(i); };
                    ImGui::EndPopup();
                }

                // Step Type
                UI::textInfo(std::visit(Doc29ProfileArrivalProcedural::VisitorStepTypeString(), *it));

                // Ground Distance
                UI::tableNextColumn();
                if (UI::inputDouble("Ground distance", groundStep.GroundDistance, Constants::Precision, Constants::NaN, set.DistanceUnits, false))
                    updated = true;

                // Start Calibrated Airspeed
                UI::tableNextColumn();
                if (UI::inputDouble("Start calibrated airspeed", groundStep.StartCalibratedAirspeed, 0.0, Constants::NaN, set.SpeedUnits, false))
                    updated = true;

                // Thrust Percentage
                UI::tableNextColumn();
                if (UI::inputPercentage("Thrust %", groundStep.StartThrustPercentage, 0.0, 1.0, 0, false))
                    updated = true;

                ImGui::PopID();
            }
            UI::endTable();
        }

        if (action)
        {
            action();
            updated = true;
        }

        if (updated)
            Application::study().Doc29Performances.updateProfile(Doc29Prof);
    }

    void Doc29ProfileDrawer::visitDoc29ProfileDepartureProcedural(Doc29ProfileDepartureProcedural& Doc29Prof) {
        const Settings& set = Application::settings();
        const ImGuiStyle& style = ImGui::GetStyle();

        bool updated = false;

        UI::buttonNew();
        if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonLeft))
        {
            if (UI::selectableNew("Climb", false))
            {
                Doc29Prof.addClimb();
                updated = true; // Updating profile resets steps
            }

            if (UI::selectableNew("Climb and Accelerate (Climb Rating)", false))
            {
                Doc29Prof.addClimbAccelerate();
                updated = true; // Updating profile resets steps
            }

            if (UI::selectableNew("Climb and Accelerate (%)", false))
            {
                Doc29Prof.addClimbAcceleratePercentage();
                updated = true; // Updating profile resets steps
            }

            ImGui::EndPopup();
        }
        if (!Doc29Prof.empty())
        {
            ImGui::SameLine();
            if (UI::buttonDelete("Clear"))
            {
                Doc29Prof.clear();
                updated = true; // Updating profile resets steps
            }
        }

        // Takeoff Step
        {
            ImGui::Spacing();
            const float offset = ImGui::GetCursorPosX() + ImGui::CalcTextSize("Aerodynamic Coefficients:").x;
            auto& takeoffStep = std::get<Doc29ProfileDepartureProcedural::Takeoff>(*Doc29Prof.begin());
            UI::textInfo("Takeoff");
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Aerodynamic Coefficients:");
            ImGui::SameLine(offset, style.ItemInnerSpacing.x);
            auto& currAeroCoeffsName = takeoffStep.Doc29AerodynamicCoefficients->Name;
            ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
            if (ImGui::BeginCombo("##AerodynamicCoefficients", currAeroCoeffsName.c_str()))
            {
                for (const auto& aeroCoeffsId : Doc29Prof.parentDoc29Performance().aerodynamicCoefficientsWithType(Doc29AerodynamicCoefficients::Type::Takeoff))
                {
                    if (ImGui::Selectable(aeroCoeffsId.c_str(), aeroCoeffsId == currAeroCoeffsName))
                    {
                        const auto& aeroCoeffs = Doc29Prof.parentDoc29Performance().AerodynamicCoefficients(aeroCoeffsId);
                        Doc29Prof.parentDoc29Performance().b_BlockedAerodynamicCoefficients.unblock(*takeoffStep.Doc29AerodynamicCoefficients, Doc29Prof);
                        Doc29Prof.parentDoc29Performance().b_BlockedAerodynamicCoefficients.block(aeroCoeffs, Doc29Prof);
                        takeoffStep.Doc29AerodynamicCoefficients = &aeroCoeffs;
                        updated = true;
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Initial calibrated airspeed:");
            ImGui::SameLine(offset, style.ItemInnerSpacing.x);
            ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
            if (UI::inputDouble("Initial calibrated airspeed", takeoffStep.InitialCalibratedAirspeed, 0.0, Constants::NaN, set.SpeedUnits))
                updated = true;
        }

        if (Doc29Prof.empty())
            return;

        // Steps
        if (UI::beginTable("Departure Steps", 7))
        {
            ImGui::TableSetupColumn("Step Type");
            ImGui::TableSetupColumn("Aerodynamic Coefficients");
            ImGui::TableSetupColumn(std::format("End Altitude AFE ({})", set.AltitudeUnits.shortName()).c_str());
            ImGui::TableSetupColumn(std::format("End Calibrated Airspeed ({})", set.SpeedUnits.shortName()).c_str());
            ImGui::TableSetupColumn(std::format("Climb Rate ({})", set.VerticalSpeedUnits.shortName()).c_str());
            ImGui::TableSetupColumn("Acceleration Percentage (%)");
            ImGui::TableSetupColumn("Thrust Cutback");
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            for (auto it = Doc29Prof.begin() + 1; it != Doc29Prof.end(); ++it)
            {
                auto& step = *it;
                const std::size_t i = std::distance(Doc29Prof.begin(), it);

                ImGui::PushID(&step);
                ImGui::TableNextRow();

                // Step Type
                UI::tableNextColumn(false);
                UI::textInfo(std::visit(Doc29ProfileDepartureProcedural::VisitorStepTypeString(), step));

                // Aerodynamic coefficients
                UI::tableNextColumn();
                std::visit(Overload{
                    [&](Doc29ProfileDepartureProcedural::Takeoff&) { GRAPE_ASSERT(false); }, [&](auto& AllSteps) {
                        auto& currAeroCoeffs = AllSteps.Doc29AerodynamicCoefficients->Name;
                        if (ImGui::BeginCombo("##AerodynamicCoefficients", currAeroCoeffs.c_str()))
                        {
                            for (const auto& [aeroCoeffsId, aeroCoeffs] : Doc29Prof.parentDoc29Performance().AerodynamicCoefficients)
                            {
                                if (ImGui::Selectable(aeroCoeffsId.c_str(), aeroCoeffsId == currAeroCoeffs))
                                {
                                    Doc29Prof.parentDoc29Performance().b_BlockedAerodynamicCoefficients.unblock(*AllSteps.Doc29AerodynamicCoefficients, Doc29Prof);
                                    Doc29Prof.parentDoc29Performance().b_BlockedAerodynamicCoefficients.block(aeroCoeffs, Doc29Prof);
                                    AllSteps.Doc29AerodynamicCoefficients = &aeroCoeffs;
                                    updated = true;
                                }
                            }
                            ImGui::EndCombo();
                        }
                    },
                    }, step);

                // End altitude AFE
                UI::tableNextColumn();
                std::visit(Overload{
                    [&](Doc29ProfileDepartureProcedural::Takeoff&) { GRAPE_ASSERT(false); }, [&](Doc29ProfileDepartureProcedural::Climb& ClimbStep) {
                        if (UI::inputDouble("End altitude AFE", ClimbStep.EndAltitudeAfe, set.AltitudeUnits, false))
                            updated = true;
                    },
                    [&](auto&) {},
                    }, step);

                // End calibrated airspeed
                UI::tableNextColumn();
                std::visit(Overload{
                    [&](Doc29ProfileDepartureProcedural::Takeoff&) { GRAPE_ASSERT(false); }, [&](Doc29ProfileDepartureProcedural::Climb&) {}, [&](auto& AccelerateSteps) {
                        if (UI::inputDouble("End calibrated airspeed", AccelerateSteps.EndCalibratedAirspeed, 0.0, Constants::NaN, set.SpeedUnits, false))
                            updated = true;
                    },
                    }, step);

                // Acceleration Columns
                std::visit(Overload{
                    [&](Doc29ProfileDepartureProcedural::Takeoff&) { GRAPE_ASSERT(false); }, [&](Doc29ProfileDepartureProcedural::ClimbAccelerate& ClimbAccelerateStep) {
                        UI::tableNextColumn(); // Climb Rate
                        if (UI::inputDouble("Climb rate", ClimbAccelerateStep.ClimbParameter, 0.0, Constants::NaN, set.VerticalSpeedUnits, false))
                            updated = true;
                        UI::tableNextColumn(false); // Acceleration Percentage
                    },
                    [&](Doc29ProfileDepartureProcedural::ClimbAcceleratePercentage& ClimbAcceleratePercentage) {
                        UI::tableNextColumn(false); // Climb Rate
                        UI::tableNextColumn();      // Acceleration Percentage
                        if (UI::inputPercentage("Acceleration %%", ClimbAcceleratePercentage.ClimbParameter, Constants::Precision, 1.0))
                            updated = true;
                    },
                    [&](auto&) {
                        UI::tableNextColumn(false); // Climb Rate
                        UI::tableNextColumn(false); // Acceleration Percentage
                    },
                    }, step);

                // All steps can be thrust cutback step
                UI::tableNextColumn(false);
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
                if (ImGui::RadioButton("##ThrustCutback", i == Doc29Prof.thrustCutback()))
                {
                    if (i == Doc29Prof.thrustCutback())
                        Doc29Prof.setThrustCutback(0);
                    else
                        Doc29Prof.setThrustCutback(i);
                    updated = true;
                }
                ImGui::PopStyleVar();
                ImGui::PopID();
            }
            UI::endTable();
        }

        if (updated)
            Application::study().Doc29Performances.updateProfile(Doc29Prof);
    }

    void Doc29Panel::drawSelectedDoc29Noise() {
        GRAPE_ASSERT(!m_SelectedDoc29Noises.empty() && m_SelectedDoc29Noises.front());
        Doc29Noise& doc29Ns = *m_SelectedDoc29Noises.front();

        const auto& study = Application::study();
        const auto& style = ImGui::GetStyle();

        ImGui::PushID("Noise");
        ImGui::BeginDisabled(study.Blocks.notEditable(doc29Ns));
        const float offset = ImGui::GetCursorPosX() + ImGui::CalcTextSize("Lateral Directivity:").x;

        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("Lateral Directivity:");
        ImGui::SameLine(offset, style.ItemInnerSpacing.x);
        ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
        if (ImGui::BeginCombo("##LateralDirectivity", Doc29Noise::LateralDirectivities.toString(doc29Ns.LateralDir).c_str()))
        {
            for (const auto& lateralDirStr : Doc29Noise::LateralDirectivities)
            {
                auto lateralDir = Doc29Noise::LateralDirectivities.fromString(lateralDirStr);
                const bool selected = lateralDir == doc29Ns.LateralDir;
                if (ImGui::Selectable(lateralDirStr, selected))
                {
                    doc29Ns.LateralDir = lateralDir;
                    study.Doc29Noises.updateNoise(doc29Ns);
                }
            }
            ImGui::EndCombo();
        }

        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("SOR Correction:");
        ImGui::SameLine(offset, style.ItemInnerSpacing.x);
        ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
        if (ImGui::BeginCombo("##SOR Correction", Doc29Noise::SORCorrections.toString(doc29Ns.SOR).c_str()))
        {
            for (const auto& sorCorrStr : Doc29Noise::SORCorrections)
            {
                auto sorCorr = Doc29Noise::SORCorrections.fromString(sorCorrStr);
                const bool selected = sorCorr == doc29Ns.SOR;
                if (ImGui::Selectable(sorCorrStr, selected))
                {
                    doc29Ns.SOR = sorCorr;
                    study.Doc29Noises.updateNoise(doc29Ns);
                }
            }
            ImGui::EndCombo();
        }
        ImGui::Separator();

        // Spectrum
        UI::textInfo("Spectrum");
        if (UI::beginTable("Spectrum", 25, ImGuiTableFlags_None, ImVec2(0.0f, UI::getTableHeight(2))))
        {
            ImGui::TableSetupColumn("Operation Type", ImGuiTableColumnFlags_NoHide);
            for (const auto frequency : OneThirdOctaveCenterFrequencies)
                ImGui::TableSetupColumn(std::format("{:.0f} Hz", frequency).c_str(), ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            // Arrival Spectrum
            ImGui::TableNextRow();
            ImGui::PushID("Arrival Spectrum");
            UI::tableNextColumn(false);
            UI::textInfo("Arrival");
            for (std::size_t i = 0; i < OneThirdOctaveBandsSize; ++i)
            {
                UI::tableNextColumn();
                if (UI::inputDouble(std::format("{:.0f} Hz noise level", OneThirdOctaveCenterFrequencies.at(i)), doc29Ns.ArrivalSpectrum(i), 0.0, Constants::NaN, 1))
                    study.Doc29Noises.updateNoise(doc29Ns);
            }
            ImGui::PopID();

            // Departure Spectrum
            ImGui::TableNextRow();
            ImGui::PushID("Departure Spectrum");
            UI::tableNextColumn(false);
            UI::textInfo("Departure");
            for (std::size_t i = 0; i < OneThirdOctaveBandsSize; ++i)
            {
                UI::tableNextColumn();
                if (UI::inputDouble(std::format("{:.0f} Hz noise level", OneThirdOctaveCenterFrequencies.at(i)), doc29Ns.DepartureSpectrum(i), 0.0, Constants::NaN, 1))
                    study.Doc29Noises.updateNoise(doc29Ns);
            }
            ImGui::PopID();

            UI::endTable();
        }

        ImGui::Separator();

        // Left side
        ImGui::BeginChild("Metrics", ImVec2(-ImGui::GetContentRegionAvail().x * UI::g_StandardLeftAlignFraction, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, UI::g_ExtraColors[UI::GrapeColInfoText]);
        const ImGuiHoveredFlags hoveredFlags = Application::study().Jobs.isAnyRunning() ? ImGuiHoveredFlags_None : ImGuiHoveredFlags_AllowWhenDisabled;

        ImGui::Selectable("Arrival LaMax");
        if (UI::isItemClicked(hoveredFlags))
        {
            m_SelectedNpdData = &doc29Ns.ArrivalLamax;
            m_SelectedNpdOp = OperationType::Arrival;
            m_SelectedMetric = NoiseSingleMetric::Lamax;
        }

        ImGui::Selectable("Arrival SEL");
        if (UI::isItemClicked(hoveredFlags))
        {
            m_SelectedNpdData = &doc29Ns.ArrivalSel;
            m_SelectedNpdOp = OperationType::Arrival;
            m_SelectedMetric = NoiseSingleMetric::Sel;
        }

        ImGui::Selectable("Departure LaMax");
        if (UI::isItemClicked(hoveredFlags))
        {
            m_SelectedNpdData = &doc29Ns.DepartureLamax;
            m_SelectedNpdOp = OperationType::Departure;
            m_SelectedMetric = NoiseSingleMetric::Lamax;
        }

        ImGui::Selectable("Departure SEL");
        if (UI::isItemClicked(hoveredFlags))
        {
            m_SelectedNpdData = &doc29Ns.DepartureSel;
            m_SelectedNpdOp = OperationType::Departure;
            m_SelectedMetric = NoiseSingleMetric::Sel;
        }

        ImGui::PopStyleColor();
        ImGui::EndChild();

        if (m_SelectedNpdData)
        {
            ImGui::SameLine();
            ImGui::BeginChild("NPD");
            if (drawSelectedDoc29NoiseMetric())
                study.Doc29Noises.updateMetric(doc29Ns, m_SelectedNpdOp, m_SelectedMetric);
            ImGui::EndChild();
        }

        ImGui::EndDisabled(); // Not editable Doc29 noise
        ImGui::PopID();       // Noise
    }

    bool Doc29Panel::drawSelectedDoc29NoiseMetric() const {
        GRAPE_ASSERT(m_SelectedNpdData);
        NpdData& npd = *m_SelectedNpdData;

        const Settings& set = Application::settings();

        bool updated = false;
        if (UI::buttonNew("Thrust"))
        {
            double newThrust = set.ThrustUnits.toSi(1000.0);
            while (npd.contains(newThrust))
                newThrust += set.ThrustUnits.toSi(1000.0);
            npd.addThrust(newThrust);
            updated = true;
        }
        ImGui::SameLine();
        if (UI::buttonDelete("Clear"))
        {
            npd.clear();
            updated = true;
        }

        if (UI::beginTable("NPD Data", 11))
        {
            ImGui::TableSetupColumn(std::format("Thrust ({})", set.ThrustUnits.shortName()).c_str(), ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("200 ft (dB)", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("400 ft (dB)", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("630 ft (dB)", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("1000 ft (dB)", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("2000 ft (dB)", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("4000 ft (dB)", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("6300 ft (dB)", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("10000 ft (dB)", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("16000 ft (dB)", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("25000 ft (dB)", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            for (auto& [thrust, nsValsArray] : npd)
            {
                ImGui::PushID(&thrust);

                ImGui::TableNextRow();

                // Thrust
                UI::tableNextColumn();
                double newThr = thrust;
                if (UI::inputDouble("Thrust", newThr, 0.0, Constants::NaN, set.ThrustUnits, false))
                {
                    npd.updateThrust(thrust, newThr);
                    updated = true;
                }

                // Noise Levels
                for (auto& nsVal : nsValsArray)
                {
                    ImGui::PushID(&nsVal);
                    UI::tableNextColumn();
                    if (UI::inputDouble("Noise Value", nsVal, 0.0, Constants::NaN, 1))
                        updated = true;

                    ImGui::PopID();
                }
                ImGui::PopID();
            }
            UI::endTable();
        }
        return updated;
    }
}
