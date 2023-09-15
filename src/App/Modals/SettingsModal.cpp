// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "SettingsModal.h"

#include "Application.h"
#include "Aircraft/Doc29/Doc29NoiseGenerator.h"
#include "Airport/RouteCalculator.h"
#include "IO/AnpImport.h"

#pragma warning ( push )
#pragma warning ( disable : GRAPE_VENDOR_WARNINGS )
#include "magic_enum_utility.hpp"
#pragma warning ( pop )

namespace GRAPE {
    namespace {
        void drawGlobal() {
            const auto& set = Application::settings();

            ImGui::BeginDisabled(Application::study().Jobs.isAnyRunning());

            UI::textInfo("Route Calculator");

            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Arc Interval:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
            UI::inputDouble("Arc Interval", RouteCalculator::s_ArcInterval, Constants::Precision, 360.0, 1);

            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Heading change warning:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
            UI::inputDouble("Heading change warning", RouteCalculator::s_WarnHeadingChange, 1.0, 360.0, 1);

            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("RNP RF leg radius delta warning:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
            UI::inputDouble("RNP RF leg radius delta", RouteCalculator::s_WarnRnpRadiusDifference, 0.0, Constants::Inf, set.DistanceUnits);

            ImGui::Separator();

            UI::textInfo("Doc29 Noise Calculator");

            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Discard segment if distance to receptor above:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
            UI::inputDouble("Maximum segment distance", Doc29NoiseGenerator::s_MaximumDistance, 0.0, Constants::Inf, set.DistanceUnits);
            ImGui::SameLine(0.0, ImGui::GetStyle().ItemSpacing.x * 2.0f);
            if (ImGui::Button("Reset"))
                Doc29NoiseGenerator::s_MaximumDistance = Constants::Inf;

            ImGui::Separator();

            UI::textInfo("ANP Importer");

            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Create fleet entries:");
            ImGui::SameLine();
            ImGui::Checkbox("##Import Fleet", &IO::AnpImport::s_ImportFleet);

            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("Approach 'Descend' as 'Land' step altitude threshold:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(UI::g_StandardItemWidth);
            UI::inputDouble("Approach Threshold", IO::AnpImport::s_MaxThresholdCrossingAltitude, set.AltitudeUnits);

            ImGui::Separator();

            ImGui::EndDisabled(); // Any job running
        }

        template <typename Enum>
        bool drawUnit(const std::string& Name, Unit<Enum>& Un) {
            const float offset = ImGui::CalcTextSize("Emissions Weight:").x;
            bool edited = false;

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(std::format("{}:", Name).c_str());
            ImGui::SameLine(offset, ImGui::GetStyle().ItemInnerSpacing.x);
            ImGui::SetNextItemWidth(UI::g_StandardItemWidth * 0.8f);
            if (ImGui::BeginCombo(std::string("##Units").append(Name).c_str(), Un.shortName(Un.Selected).c_str()))
            {
                magic_enum::enum_for_each<Enum>([&](auto Value) {
                    if (ImGui::Selectable(Un.shortName(Value).c_str(), Un.Selected == Value))
                    {
                        Un.Selected = Value;
                        edited = true;
                    }
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("%s", Un.name(Value).c_str());

                    if (Un.Selected == Value)
                        ImGui::SetItemDefaultFocus();
                    });
                ImGui::EndCombo();
            }

            ImGui::SameLine();
            ImGui::SetNextItemWidth(UI::g_StandardItemWidth * 0.5f);
            if (ImGui::BeginCombo(std::string("##UnitsDecimals").append(Name).c_str(), std::to_string(Un.decimals()).c_str()))
            {
                for (std::size_t i = 0; i <= 6; ++i)
                {
                    if (ImGui::Selectable(std::to_string(i).c_str(), i == Un.decimals()))
                    {
                        Un.setDecimals(i);
                        edited = true;
                    }

                    if (i == Un.decimals())
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            return edited;
        }

        void drawUnits() {
            auto& set = Application::settings();

            const float spacing = UI::g_StandardItemWidth * 0.8f / 2.0f - ImGui::CalcTextSize("Units").x / 2.0f;
            ImGui::SameLine(0.0f, ImGui::CalcTextSize("Emissions Weight").x + ImGui::GetStyle().ItemInnerSpacing.x + spacing);
            UI::textInfo("Units");
            ImGui::SameLine(0.0f, spacing + ImGui::GetStyle().ItemSpacing.x * 2.0f);
            UI::textInfo("Decimals");

            ImGui::Separator();

            drawUnit("Distance", set.DistanceUnits);
            drawUnit("Altitude", set.AltitudeUnits);
            drawUnit("Speed", set.SpeedUnits);
            drawUnit("Vertical Speed", set.VerticalSpeedUnits);
            drawUnit("Weight", set.WeightUnits);
            drawUnit("Thrust", set.ThrustUnits);
            drawUnit("Temperature", set.TemperatureUnits);
            drawUnit("Pressure", set.PressureUnits);
            drawUnit("Power", set.PowerUnits);
            drawUnit("Fuel Flow", set.FuelFlowUnits);
            drawUnit("Emission Index", set.EmissionIndexUnits);
            drawUnit("Emissions Weight", set.EmissionsWeightUnits);
        }
    }

    void SettingsModal::imGuiDraw() {
        // Minimum Size and Center
        ImGui::SetNextWindowSizeConstraints(ImVec2(800.0f, 500.0f), ImVec2(FLT_MAX, FLT_MAX));
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        bool unused = true;
        if (ImGui::BeginPopupModal("Settings", &unused))
        {
            ImGui::BeginChild("Left Side", ImVec2(-ImGui::GetContentRegionAvail().x * UI::g_StandardLeftAlignFraction, 0.0f));

            if (ImGui::Selectable("Global", m_SelectedSettings == Selected::Global))
                m_SelectedSettings = Selected::Global;

            if (ImGui::Selectable("Units", m_SelectedSettings == Selected::Units))
                m_SelectedSettings = Selected::Units;

            ImGui::EndChild();

            ImGui::SameLine();

            ImGui::BeginChild("Right Side");
            switch (m_SelectedSettings)
            {
            case Selected::Global: drawGlobal(); break;
            case Selected::Units: drawUnits(); break;
            default: GRAPE_ASSERT(false); break;
            }
            ImGui::EndChild();
            ImGui::EndPopup();
        }
    }
}
