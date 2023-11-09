// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "UI.h"

namespace GRAPE {
    class Modal {
    public:
        explicit Modal(std::string_view Name) : m_Name(Name) {}
        virtual ~Modal() = default;

        void updateImGuiId() { m_Id = ImGui::GetID(m_Name.c_str()); }
        virtual void imGuiDraw() = 0;

        [[nodiscard]] const std::string& name() const { return m_Name; }
        [[nodiscard]] ImGuiID imGuiId() const { return m_Id; }
    protected:
        std::string m_Name;
    private:
        ImGuiID m_Id = 0;
    };
}
