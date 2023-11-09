// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Panel.h"

#include "Aircraft/FuelEmissions/LTO.h"

namespace GRAPE {
    class LTOPanel : public Panel {
    public:
        LTOPanel();

        // Selection
        void select(LTOEngine& LTOEng);
        void deselect(LTOEngine& LTOEng);
        void eraseSelected();

        // Status Checks
        [[nodiscard]] bool isSelected(LTOEngine& LTOEng) const;

        // Panel Interface
        void reset() override;
        void imGuiDraw() override;
    private:
        std::vector<LTOEngine*> m_SelectedLTOEngines;
    };
}
