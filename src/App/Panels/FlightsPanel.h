// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Panel.h"

#include "Operation/Flight.h"

namespace GRAPE {
    class FlightsPanel : public Panel {
    public:
        // Constructors & Destructor
        FlightsPanel();

        // Selection
        void select(FlightArrival& FlightArr);
        void select(FlightDeparture& FlightDep);
        void deselect(FlightArrival& FlightArr);
        void deselect(FlightDeparture& FlightDep);
        void eraseSelectedArrivals();
        void eraseSelectedDepartures();

        // Status Checks
        [[nodiscard]] bool isSelected(FlightArrival& FlightArr) const;
        [[nodiscard]] bool isSelected(FlightDeparture& FlightDep) const;

        // Panel Interface
        void reset() override;
        void imGuiDraw() override;
    private:
        std::vector<FlightArrival*> m_SelectedArrivals;
        std::vector<FlightDeparture*> m_SelectedDepartures;
    };
}
