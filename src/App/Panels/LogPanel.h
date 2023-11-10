// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Panel.h"

namespace GRAPE {
    struct LogPanel : Panel {
        LogPanel();

        void imGuiDraw() override;

        std::array<bool, spdlog::level::n_levels> LevelActive = { true, true, true, true, true, true, true };
    };
}
