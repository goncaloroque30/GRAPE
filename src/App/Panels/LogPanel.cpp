// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "LogPanel.h"

#include "UI.h"

#include <IconsFontAwesome6.h>
#include <nfd.h>

#include <fstream>

namespace GRAPE {
    namespace {
        // TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL, OFF
        constexpr std::array<const char*, spdlog::level::n_levels> LevelIcons = { nullptr, nullptr, ICON_FA_INFO, ICON_FA_TRIANGLE_EXCLAMATION, ICON_FA_CIRCLE_XMARK, nullptr, nullptr };
        constexpr std::array<const char*, spdlog::level::n_levels> LevelTooltips = { nullptr, nullptr, "Infos", "Warns", "Errors", nullptr, nullptr };
        const std::array<ImVec4, spdlog::level::n_levels> LevelIconColors = { ImColor(), ImColor(), ImColor(0, 0, 255), ImColor(255, 255, 0), ImColor(255, 0, 0), ImColor(), ImColor() };
        constexpr std::array LevelEnabled = { spdlog::level::info, spdlog::level::warn, spdlog::level::err };
    }

    LogPanel::LogPanel() : Panel("Log") {
        m_Open = true;
    }

    void LogPanel::imGuiDraw() {
        if (!isOpen())
            return;

        ImGui::Begin(m_Name.c_str(), &m_Open, ImGuiWindowFlags_NoCollapse);

        const auto messages = Log::last();
        ImGui::BeginDisabled(messages.empty());
        if (ImGui::Button("Export"))
        {
            nfdchar_t* outPath;
            constexpr nfdfilteritem_t filter[1] = { { "Text file", "txt" } };
            const nfdresult_t res = NFD_SaveDialog(&outPath, filter, 1, nullptr, "Log");
            if (res == NFD_OKAY)
            {
                std::ofstream stream(outPath, std::ios::binary | std::ios::trunc);
                for (const auto& [lvl, fmtString] : Log::last(std::numeric_limits<int>::max()))
                    stream << fmtString << "\n";
                NFD_FreePath(outPath);
            }
        }
        ImGui::EndDisabled();

        constexpr float buttonsWidth = 20.0f;
        const float offset = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - buttonsWidth * LevelEnabled.size() - ImGui::GetStyle().ItemSpacing.x * LevelEnabled.size();
        ImGui::SameLine(offset);
        for (const auto lvl : LevelEnabled)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, LevelIconColors.at(lvl));
            ImGui::Selectable(LevelIcons.at(lvl), &LevelActive[lvl], ImGuiSelectableFlags_None, ImVec2(buttonsWidth, 0.0f));
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", LevelTooltips.at(lvl));
            ImGui::PopStyleColor();
            ImGui::SameLine();
        }
        ImGui::NewLine();

        ImGui::BeginChild("Messages");

        const float msgOffset = ImGui::GetCursorPosX() + 30.0f;
        ImGui::PushStyleColor(ImGuiCol_Text, UI::g_ExtraColors.at(UI::GrapeColInfoText));
        for (const auto& message : messages | std::views::reverse)
        {
            if (!LevelActive.at(message.Level))
                continue;

            ImGui::PushStyleColor(ImGuiCol_Text, LevelIconColors.at(message.Level));
            ImGui::TextUnformatted(LevelIcons.at(message.Level));
            ImGui::PopStyleColor();

            ImGui::SameLine(msgOffset);
            ImGui::TextUnformatted(message.FormattedString.c_str());
        }
        ImGui::PopStyleColor();
        ImGui::EndChild();
        ImGui::End();
    }
}
