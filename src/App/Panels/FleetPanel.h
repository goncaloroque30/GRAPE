// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Panel.h"

#include "Aircraft/Aircraft.h"

namespace GRAPE {
    class FleetPanel : public Panel {
    public:
        // Constructors & Destructor
        FleetPanel();

        // Selection
        void select(Aircraft& Acft);
        void deselect(Aircraft& Acft);
        void eraseSelected();

        // Status Checks
        [[nodiscard]] bool isSelected(Aircraft& Acft) const;

        // Panel Interface
        void reset() override;
        void imGuiDraw() override;
    private:
        std::vector<Aircraft*> m_SelectedAircraft;
    };
}
