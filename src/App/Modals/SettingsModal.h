// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Modal.h"

namespace GRAPE {
    class SettingsModal : public Modal {
    public:
        SettingsModal() : Modal("Settings") {}

        void imGuiDraw() override;
    private:
        enum class Selected {
            Global = 0,
            Units = 1,
        } m_SelectedSettings = Selected::Global;
    };
}
