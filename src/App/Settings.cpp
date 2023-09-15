// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "Settings.h"

#include "Aircraft/Doc29/Doc29NoiseGenerator.h"
#include "Airport/RouteCalculator.h"
#include "IO/AnpImport.h"

#pragma warning ( push )
#pragma warning ( disable : GRAPE_VENDOR_WARNINGS )
#include "imgui_internal.h"
#pragma warning ( pop )

namespace GRAPE {
    namespace {
        void settingsWriteAll(ImGuiContext*, ImGuiSettingsHandler* Handler, ImGuiTextBuffer* Buf) {
            const auto set = static_cast<Settings*>(Handler->UserData);

            Buf->appendf("%s", std::format("[{}][{}]\n", Handler->TypeName, Handler->TypeName).c_str());

            // Globals
            Buf->appendf("%s", std::format("RouteArcInterval={}\n", RouteCalculator::s_ArcInterval).c_str());
            Buf->appendf("%s", std::format("RouteHeadingChangeWarning={}\n", RouteCalculator::s_WarnHeadingChange).c_str());
            Buf->appendf("%s", std::format("RouteRNPRadiusDeltaWarning={}\n", RouteCalculator::s_WarnRnpRadiusDifference).c_str());
            Buf->appendf("%s", std::format("Doc29NoiseMaximumDistance={}\n", Doc29NoiseGenerator::s_MaximumDistance).c_str());
            Buf->appendf("%s", std::format("AnpImportFleet={}\n", static_cast<int>(IO::AnpImport::s_ImportFleet)).c_str());
            Buf->appendf("%s", std::format("AnpImporterApproachDescendAsLandThreshold={}\n", IO::AnpImport::s_MaxThresholdCrossingAltitude).c_str());

            // Units
            Buf->appendf("%s", std::format("Distance={}\n", magic_enum::enum_integer<Units::Distance>(set->DistanceUnits.Selected)).c_str());
            Buf->appendf("%s", std::format("Distance_dec={}\n", set->DistanceUnits.decimals()).c_str());
            Buf->appendf("%s", std::format("Altitude={}\n", magic_enum::enum_integer<Units::Distance>(set->AltitudeUnits.Selected)).c_str());
            Buf->appendf("%s", std::format("Altitude_dec={}\n", set->AltitudeUnits.decimals()).c_str());
            Buf->appendf("%s", std::format("Speed={}\n", magic_enum::enum_integer<Units::Speed>(set->SpeedUnits.Selected)).c_str());
            Buf->appendf("%s", std::format("Speed_dec={}\n", set->SpeedUnits.decimals()).c_str());
            Buf->appendf("%s", std::format("VerticalSpeed={}\n", magic_enum::enum_integer<Units::Speed>(set->VerticalSpeedUnits.Selected)).c_str());
            Buf->appendf("%s", std::format("VerticalSpeed_dec={}\n", set->VerticalSpeedUnits.decimals()).c_str());
            Buf->appendf("%s", std::format("Weight={}\n", magic_enum::enum_integer<Units::Weight>(set->WeightUnits.Selected)).c_str());
            Buf->appendf("%s", std::format("Weight_dec={}\n", set->WeightUnits.decimals()).c_str());
            Buf->appendf("%s", std::format("Thrust={}\n", magic_enum::enum_integer<Units::Force>(set->ThrustUnits.Selected)).c_str());
            Buf->appendf("%s", std::format("Thrust_dec={}\n", set->ThrustUnits.decimals()).c_str());
            Buf->appendf("%s", std::format("Temperature={}\n", magic_enum::enum_integer<Units::Temperature>(set->TemperatureUnits.Selected)).c_str());
            Buf->appendf("%s", std::format("Temperature_dec={}\n", set->TemperatureUnits.decimals()).c_str());
            Buf->appendf("%s", std::format("Pressure={}\n", magic_enum::enum_integer<Units::Pressure>(set->PressureUnits.Selected)).c_str());
            Buf->appendf("%s", std::format("Pressure_dec={}\n", set->PressureUnits.decimals()).c_str());
            Buf->appendf("%s", std::format("Power={}\n", magic_enum::enum_integer<Units::Power>(set->PowerUnits.Selected)).c_str());
            Buf->appendf("%s", std::format("Power_dec={}\n", set->PowerUnits.decimals()).c_str());
            Buf->appendf("%s", std::format("FuelFlow={}\n", magic_enum::enum_integer<Units::WeightPerTime>(set->FuelFlowUnits.Selected)).c_str());
            Buf->appendf("%s", std::format("FuelFlow_dec={}\n", set->FuelFlowUnits.decimals()).c_str());
            Buf->appendf("%s", std::format("EmissionIndex={}\n", magic_enum::enum_integer<Units::WeightPerWeight>(set->EmissionIndexUnits.Selected)).c_str());
            Buf->appendf("%s", std::format("EmissionIndex_dec={}\n", set->EmissionIndexUnits.decimals()).c_str());
            Buf->appendf("%s", std::format("EmissionsWeight={}\n", magic_enum::enum_integer<Units::WeightSmall>(set->EmissionsWeightUnits.Selected)).c_str());

            Buf->appendf("%s", std::format("EmissionsWeight_dec={}\n", set->EmissionsWeightUnits.decimals()).c_str());
            Buf->appendf("\n");
        }

        void* settingsReadOpen(ImGuiContext*, ImGuiSettingsHandler* Handler, const char*) { return Handler->UserData; }

        void settingsReadLine(ImGuiContext*, ImGuiSettingsHandler* Handler, void*, const char* Line) {
            const auto set = static_cast<Settings*>(Handler->UserData);

            double d1;
            int i1;

            // Globals
            if (sscanf_s(Line, "RouteArcInterval=%lf", &d1) == 1)
            {
                if (d1 >= Constants::Precision && d1 < 360.0)
                    RouteCalculator::s_ArcInterval = d1;
                return;
            }

            if (sscanf_s(Line, "RouteHeadingChangeWarning=%lf", &d1) == 1)
            {
                if (d1 >= 1.0 && d1 < 360.0)
                    RouteCalculator::s_WarnHeadingChange = d1;
                return;
            }

            if (sscanf_s(Line, "RouteRNPRadiusDeltaWarning=%lf", &d1) == 1)
            {
                if (d1 >= 0.0)
                    RouteCalculator::s_WarnRnpRadiusDifference = d1;
                return;
            }

            if (sscanf_s(Line, "Doc29NoiseMaximumDistance=%lf", &d1) == 1)
            {
                if (d1 >= 0.0)
                    Doc29NoiseGenerator::s_MaximumDistance = d1;
                return;
            }

            if (sscanf_s(Line, "AnpImportFleet=%i", &i1) == 1)
            {
                IO::AnpImport::s_ImportFleet = static_cast<bool>(i1);
                return;
            }

            if (sscanf_s(Line, "AnpImporterApproachDescendAsLandThreshold=%lf", &d1) == 1)
            {
                IO::AnpImport::s_MaxThresholdCrossingAltitude = d1;
                return;
            }

            // Units
            if (sscanf_s(Line, "Distance=%i", &i1) == 1)
                set->DistanceUnits.Selected = magic_enum::enum_cast<Units::Distance>(i1).value();
            else if (sscanf_s(Line, "Distance_dec=%i", &i1) == 1)
                set->DistanceUnits.setDecimals(static_cast<std::size_t>(i1));
            else if (sscanf_s(Line, "Altitude=%i", &i1) == 1)
                set->AltitudeUnits.Selected = magic_enum::enum_cast<Units::Distance>(i1).value();
            else if (sscanf_s(Line, "Altitude_dec=%i", &i1) == 1)
                set->AltitudeUnits.setDecimals(static_cast<std::size_t>(i1));
            else if (sscanf_s(Line, "Speed=%i", &i1) == 1)
                set->SpeedUnits.Selected = magic_enum::enum_cast<Units::Speed>(i1).value();
            else if (sscanf_s(Line, "Speed_dec=%i", &i1) == 1)
                set->SpeedUnits.setDecimals(static_cast<std::size_t>(i1));
            else if (sscanf_s(Line, "VerticalSpeed=%i", &i1) == 1)
                set->VerticalSpeedUnits.Selected = magic_enum::enum_cast<Units::Speed>(i1).value();
            else if (sscanf_s(Line, "VerticalSpeed_dec=%i", &i1) == 1)
                set->VerticalSpeedUnits.setDecimals(static_cast<std::size_t>(i1));
            else if (sscanf_s(Line, "Weight=%i", &i1) == 1)
                set->WeightUnits.Selected = magic_enum::enum_cast<Units::Weight>(i1).value();
            else if (sscanf_s(Line, "Weight_dec=%i", &i1) == 1)
                set->WeightUnits.setDecimals(static_cast<std::size_t>(i1));
            else if (sscanf_s(Line, "Thrust=%i", &i1) == 1)
                set->ThrustUnits.Selected = magic_enum::enum_cast<Units::Force>(i1).value();
            else if (sscanf_s(Line, "Thrust_dec=%i", &i1) == 1)
                set->ThrustUnits.setDecimals(static_cast<std::size_t>(i1));
            else if (sscanf_s(Line, "Temperature=%i", &i1) == 1)
                set->TemperatureUnits.Selected = magic_enum::enum_cast<Units::Temperature>(i1).value();
            else if (sscanf_s(Line, "Temperature_dec=%i", &i1) == 1)
                set->TemperatureUnits.setDecimals(static_cast<std::size_t>(i1));
            else if (sscanf_s(Line, "Pressure=%i", &i1) == 1)
                set->PressureUnits.Selected = magic_enum::enum_cast<Units::Pressure>(i1).value();
            else if (sscanf_s(Line, "Pressure_dec=%i", &i1) == 1)
                set->PressureUnits.setDecimals(static_cast<std::size_t>(i1));
            else if (sscanf_s(Line, "Power=%i", &i1) == 1)
                set->PowerUnits.Selected = magic_enum::enum_cast<Units::Power>(i1).value();
            else if (sscanf_s(Line, "Power_dec=%i", &i1) == 1)
                set->PowerUnits.setDecimals(static_cast<std::size_t>(i1));
            else if (sscanf_s(Line, "FuelFlow=%i", &i1) == 1)
                set->FuelFlowUnits.Selected = magic_enum::enum_cast<Units::WeightPerTime>(i1).value();
            else if (sscanf_s(Line, "FuelFlow_dec=%i", &i1) == 1)
                set->FuelFlowUnits.setDecimals(static_cast<std::size_t>(i1));
            else if (sscanf_s(Line, "EmissionIndex=%i", &i1) == 1)
                set->EmissionIndexUnits.Selected = magic_enum::enum_cast<Units::WeightPerWeight>(i1).value();
            else if (sscanf_s(Line, "EmissionIndex_dec=%i", &i1) == 1)
                set->EmissionIndexUnits.setDecimals(static_cast<std::size_t>(i1));
            else if (sscanf_s(Line, "EmissionsWeight=%i", &i1) == 1)
                set->EmissionsWeightUnits.Selected = magic_enum::enum_cast<Units::WeightSmall>(i1).value();
            else if (sscanf_s(Line, "EmissionsWeight_dec=%i", &i1) == 1)
                set->EmissionsWeightUnits.setDecimals(static_cast<std::size_t>(i1));
        }
    }

    void Settings::initDefineHandler() {
        ImGuiContext* context = ImGui::GetCurrentContext();

        ImGuiSettingsHandler setHandler;
        setHandler.TypeName = "Grape Settings";
        setHandler.TypeHash = ImHashStr("Grape Settings");
        setHandler.ReadOpenFn = settingsReadOpen;
        setHandler.ReadLineFn = settingsReadLine;
        setHandler.WriteAllFn = settingsWriteAll;
        setHandler.UserData = this;
        context->SettingsHandlers.push_back(setHandler);
    }
}
