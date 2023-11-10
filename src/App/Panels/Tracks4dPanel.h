// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Panel.h"

#include "Operation/Track4d.h"

namespace GRAPE {
    class Tracks4dPanel : public Panel {
    public:
        // Constructors & Destructor
        Tracks4dPanel();

        // Selection
        void select(Track4dArrival& Track4dArr);
        void select(Track4dDeparture& Track4dDep);
        void deselect(Track4dArrival& Track4dArr);
        void deselect(Track4dDeparture& Track4dDep);
        void deselectArrivals();
        void deselectDepartures();

        void eraseSelectedArrivals();
        void eraseSelectedDepartures();

        // Status Checks
        [[nodiscard]] bool isSelected(Track4dArrival& Track4dArr) const;
        [[nodiscard]] bool isSelected(Track4dDeparture& Track4dDep) const;

        // Panel Interface
        void reset() override;
        void onPerformanceRunStart() override;
        void imGuiDraw() override;
    private:
        std::vector<Track4dArrival*> m_SelectedArrivals;
        std::vector<Track4dDeparture*> m_SelectedDepartures;

        OperationType m_SelectedType = OperationType::Arrival;
    private:
        void drawTrack4d(Track4d& Track4dOp) const;
    };
}
