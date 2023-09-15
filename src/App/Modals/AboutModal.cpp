// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "AboutModal.h"

#include "Application.h"
#include "UI.h"

#include <fstream>

namespace GRAPE {
    void AboutModal::imGuiDraw() {
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        bool unused = true;
        if (ImGui::BeginPopupModal("About", &unused))
        {
            const auto& icon = Application::get().icon();

            UI::alignForWidth(static_cast<float>(icon.width()));
            ImGui::Image(icon.getDescriptorSet(), ImVec2(static_cast<float>(icon.width()), static_cast<float>(icon.height())), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
            UI::textCentered("GRAPE");
            UI::textCentered("Version " GRAPE_VERSION_STRING);
            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_Text, UI::g_ExtraColors.at(UI::ExtraColors::GrapeColInfoText));
            const float widthURL = ImGui::CalcTextSize(GRAPE_URL).x + ImGui::GetStyle().FramePadding.x * 2.0f;
            UI::alignForWidth(widthURL);
            ImGui::SetNextItemWidth(widthURL);
            if (ImGui::Selectable(GRAPE_URL, false, ImGuiSelectableFlags_DontClosePopups, ImVec2(widthURL, 0.0f)))
                platformOpen(GRAPE_URL);
            ImGui::PopStyleColor();
            ImGui::Spacing();

            UI::textCentered("Developed by");
            UI::textCentered(R"(Goncalo Soares Roque)");
            ImGui::Spacing();

            const float widthLegal = ImGui::CalcTextSize("Legal").x + ImGui::GetStyle().FramePadding.x * 2.0f;
            UI::alignForWidth(widthLegal);
            if (ImGui::Selectable("Legal##Selectable", false, ImGuiSelectableFlags_DontClosePopups, ImVec2(widthLegal, 0.0f)))
                ImGui::OpenPopup("Legal");

            legalModal();

            ImGui::EndPopup();
        }

        ImGui::PopStyleVar();
    }

    void AboutModal::legalModal() {
        ImGui::SetNextWindowPos(ImGui::GetWindowViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        bool unused = true;

        if (ImGui::BeginPopupModal("Legal", &unused))
        {
            if (ImGui::IsWindowAppearing())
                loadLegalString();

            ImGui::TextUnformatted(m_LegalString.c_str());
            ImGui::EndPopup();
        }
        else { clearLegalString(); }
    }

    void AboutModal::loadLegalString() {
        std::ifstream fStream(getResolvedPath("res/Files/Legal.txt"), std::ios::in | std::ios::binary);

        if (fStream)
        {
            clearLegalString();

            fStream.seekg(0, std::ios::end);
            m_LegalString.resize(fStream.tellg());
            fStream.seekg(0, std::ios::beg);
            fStream.read(m_LegalString.data(), m_LegalString.size());
            fStream.close();
        }
        else
        {
            m_LegalString = "Failed to load legal information.";
        }
    }

    void AboutModal::clearLegalString() {
        m_LegalString.clear();
    }
}
