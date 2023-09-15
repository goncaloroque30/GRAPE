// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Manager.h"

#include "Airport/Airport.h"

namespace GRAPE {
    class AirportsManager : public Manager {
    public:
        AirportsManager(const Database& Db, Constraints& Blocks);
        auto& airports() { return m_Airports; }
        auto& operator()() { return m_Airports; }
        const Airport& operator()(const std::string& AptId) { return m_Airports(AptId); }
        [[nodiscard]] auto begin() const { return std::views::values(m_Airports).begin(); }
        [[nodiscard]] auto end() const { return std::views::values(m_Airports).end(); }

        std::pair<Airport&, bool> addAirport(const std::string& Name = "");
        std::pair<Runway&, bool> addRunway(Airport& Apt, const std::string& Name = "") const;
        std::pair<Route&, bool> addRouteArrival(Runway& Rwy, Route::Type RteType, const std::string& Name = "") const;
        std::pair<Route&, bool> addRouteDeparture(Runway& Rwy, Route::Type RteType, const std::string& Name = "") const;

        Airport& addAirportE(const std::string& Name);
        Runway& addRunwayE(Airport& Apt, const std::string& Name) const;
        Route& addRouteArrivalE(Runway& Rwy, Route::Type RteType, const std::string& Name) const;
        Route& addRouteDepartureE(Runway& Rwy, Route::Type RteType, const std::string& Name) const;

        void eraseAirports();
        void erase(const Airport& Apt);
        void erase(const Runway& Rwy);
        void erase(const Route& Rte);

        bool updateKey(Airport& Apt, std::string Id);
        bool updateKey(Runway& Rwy, std::string Id);
        bool updateKey(Route& Rte, std::string Id);

        void update(const Airport& Apt) const;
        void update(const Runway& Rwy) const;
        void update(const Route& Rte) const;

        void loadFromFile();

    private:
        GrapeMap<std::string, Airport> m_Airports{};
    };
}
