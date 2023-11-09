// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Manager.h"

#include "Aircraft/Aircraft.h"
#include "Database/Database.h"
#include "Operation/Operations.h"

namespace GRAPE {
    class AircraftsManager;
    class AirportsManager;

    class OperationsManager : public Manager {
    public:
        OperationsManager(const Database& Db, Constraints& Blocks, AircraftsManager& Aircrafts, AirportsManager& Airports);

        auto& flightArrivals() { return m_FlightArrivals; }
        auto& flightDepartures() { return m_FlightDepartures; }
        auto& track4dArrivals() { return m_Track4dArrivals; }
        auto& track4dDepartures() { return m_Track4dDepartures; }

        [[nodiscard]] const auto& flightArrivals() const { return m_FlightArrivals; }
        [[nodiscard]] const auto& flightDepartures() const { return m_FlightDepartures; }
        [[nodiscard]] const auto& track4dArrivals() const { return m_Track4dArrivals; }
        [[nodiscard]] const auto& track4dDepartures() const { return m_Track4dDepartures; }

        std::pair<FlightArrival&, bool> addArrivalFlight(const std::string& Name, const Aircraft& AircraftIn);
        bool addArrivalFlight(const std::string& Name = "");
        std::pair<FlightDeparture&, bool> addDepartureFlight(const std::string& Name, const Aircraft& AircraftIn);
        bool addDepartureFlight(const std::string& Name = "");
        std::pair<Track4dArrival&, bool> addArrivalTrack4d(const std::string& Name, const Aircraft& AircraftIn);
        bool addArrivalTrack4d(const std::string& Name = "");
        std::pair<Track4dDeparture&, bool> addDepartureTrack4d(const std::string& Name, const Aircraft& AircraftIn);
        bool addDepartureTrack4d(const std::string& Name = "");

        FlightArrival& addArrivalFlightE(const std::string& Name, const Aircraft& AircraftIn);
        FlightDeparture& addDepartureFlightE(const std::string& Name, const Aircraft& AircraftIn);
        Track4dArrival& addArrivalTrack4dE(const std::string& Name, const Aircraft& AircraftIn);
        Track4dDeparture& addDepartureTrack4dE(const std::string& Name, const Aircraft& AircraftIn);

        void setAircraft(FlightArrival& Op, const Aircraft& Acft) const;
        void setAircraft(FlightDeparture& Op, const Aircraft& Acft) const;
        void setAircraft(Track4d& Op, const Aircraft& Acft) const;
        void setRoute(FlightArrival& Op, const RouteArrival* Rte) const;
        void setRoute(FlightDeparture& Op, const RouteDeparture* Rte) const;
        void setDoc29Profile(FlightArrival& Op, const Doc29ProfileArrival* Profile) const;
        void setDoc29Profile(FlightDeparture& Op, const Doc29ProfileDeparture* Profile) const;

        void eraseFlightArrivals();
        void eraseFlightDepartures();
        void eraseTrack4dArrivals();
        void eraseTrack4dDepartures();
        void erase(FlightArrival& Op);
        void erase(FlightDeparture& Op);
        void erase(Track4dArrival& Op);
        void erase(Track4dDeparture& Op);

        bool updateKey(FlightArrival& Op, std::string Id);
        bool updateKey(FlightDeparture& Op, std::string Id);
        bool updateKey(Track4dArrival& Op, std::string Id);
        bool updateKey(Track4dDeparture& Op, std::string Id);

        void update(const Flight& Op) const;
        void update(const Track4d& Op) const;

        [[nodiscard]] bool emptyFlights() const { return m_FlightArrivals.empty() && m_FlightDepartures.empty(); }
        [[nodiscard]] bool emptyTracks4d() const { return m_Track4dArrivals.empty() && m_Track4dDepartures.empty(); }
        [[nodiscard]] bool emptyArrivals() const { return m_FlightArrivals.empty() && m_Track4dArrivals.empty(); }
        [[nodiscard]] bool emptyDepartures() const { return m_FlightDepartures.empty() && m_Track4dDepartures.empty(); }
        [[nodiscard]] bool empty() const { return emptyFlights() && emptyTracks4d(); }

        [[nodiscard]] std::size_t flightsSize() const { return m_FlightArrivals.size() + m_FlightDepartures.size(); }
        [[nodiscard]] std::size_t tracks4dSize() const { return m_Track4dArrivals.size() + m_Track4dDepartures.size(); }
        [[nodiscard]] std::size_t arrivalsSize() const { return m_FlightArrivals.size() + m_Track4dArrivals.size(); }
        [[nodiscard]] std::size_t departuresSize() const { return m_FlightDepartures.size() + m_Track4dDepartures.size(); }
        [[nodiscard]] std::size_t operationsSize() const { return flightsSize() + tracks4dSize(); }

        struct Tracks4dLoader {
            explicit Tracks4dLoader(const Database& DbIn) : Db(DbIn) {}
            Database Db;
            std::mutex Mutex;
        } Tracks4dLoader;

        void loadFromFile();
        void loadArr(const Track4dArrival& Op);
        void loadDep(const Track4dDeparture& Op);
        void unloadArr(const Track4dArrival& Op, bool Shrink = false);
        void unloadDep(const Track4dDeparture& Op, bool Shrink = false);
        void load(Track4d& Op);

    private:
        AircraftsManager& m_Aircrafts;
        AirportsManager& m_Airports;

        GrapeMap<std::string, FlightArrival> m_FlightArrivals{};     // Key is Operation name
        GrapeMap<std::string, FlightDeparture> m_FlightDepartures{}; // Key is Operation name

        GrapeMap<std::string, Track4dArrival> m_Track4dArrivals{};     // Key is Operation name
        GrapeMap<std::string, Track4dDeparture> m_Track4dDepartures{}; // Key is Operation name
    };
}
