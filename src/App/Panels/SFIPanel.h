// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Panel.h"

#include "Aircraft/FuelEmissions/SFI.h"

namespace GRAPE {
    class SFIPanel : public Panel {
    public:
        // Constructors & Destructor
        SFIPanel();

        // Selection
        void select(SFI& Sfi);
        void deselect(SFI& Sfi);
        void eraseSelected();

        // Status Checks
        [[nodiscard]] bool isSelected(SFI& Sfi) const;

        // Panel Interface
        void reset() override;
        void imGuiDraw() override;
    private:
        std::vector<SFI*> m_SelectedSFI;
    };
}
