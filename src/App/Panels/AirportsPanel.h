// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Panel.h"

#include "Airport/Airport.h"

namespace GRAPE {
    class AirportsPanel : public Panel {
    public:
        // Constructors & Destructor
        AirportsPanel();

        // Selection
        void select(Airport& Apt);
        void select(Runway& Rwy);
        void select(Route& Rte);

        void clearSelection();

        // Status Checks
        [[nodiscard]] bool isSelected(const Airport& Apt) const;
        [[nodiscard]] bool isSelected(const Runway& Rwy) const;
        [[nodiscard]] bool isSelected(const Route& Rte) const;

        // Panel Interface
        void reset() override;
        void imGuiDraw() override;
    private:
        Airport* m_SelectedAirport = nullptr;
        Runway* m_SelectedRunway = nullptr;
        Route* m_SelectedRoute = nullptr;

        enum class Selected {
            Airport = 0,
            Runway,
            Route,
        } m_SelectedType = Selected::Airport;

        std::function<void()> m_Action;
    private:
        void drawAirportNode(const std::string& AirportId, Airport& Apt);
        void drawRunwayNode(const std::string& RunwayId, Runway& Rwy);
        void drawRouteNode(const std::string& RouteId, Route& Rte);
        void drawSelectedAirport();
        void drawSelectedRunway();
        void drawSelectedRoute();
    };
}
