// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Manager.h"

#include "Aircraft/Doc29/Doc29Aircraft.h"

namespace GRAPE {
    class Doc29PerformanceManager : public Manager {
    public:
        Doc29PerformanceManager(const Database& Db, Constraints& Blocks);

        auto& performances() { return m_Doc29Aircrafts; }
        auto& operator()() { return m_Doc29Aircrafts; }
        auto& operator()(const std::string& Doc29PerfId) { return m_Doc29Aircrafts(Doc29PerfId); }
        [[nodiscard]] auto begin() const { return std::views::values(m_Doc29Aircrafts).begin(); }
        [[nodiscard]] auto end() const { return std::views::values(m_Doc29Aircrafts).end(); }

        std::pair<Doc29Aircraft&, bool> addPerformance(const std::string& Name = "");
        bool addProfileArrival(Doc29Aircraft& Doc29Acft, Doc29Profile::Type ProfileType, const std::string& Name = "") const;
        bool addProfileDeparture(Doc29Aircraft& Doc29Acft, Doc29Profile::Type ProfileType, const std::string& Name = "") const;

        Doc29Aircraft& addPerformanceE(const std::string& Name);
        Doc29ProfileArrival& addProfileArrivalE(Doc29Aircraft& Doc29Acft, Doc29Profile::Type ProfileType, const std::string& Name) const;
        Doc29ProfileDeparture& addProfileDepartureE(Doc29Aircraft& Doc29Acft, Doc29Profile::Type ProfileType, const std::string& Name) const;

        void erasePerformances();
        void erasePerformance(const Doc29Aircraft& Doc29Acft);
        void eraseProfileArrivals(Doc29Aircraft& Doc29Acft);
        void eraseProfileDepartures(Doc29Aircraft& Doc29Acft);
        void eraseProfile(const Doc29Profile& Doc29Prof);

        bool updateKeyPerformance(Doc29Aircraft& Doc29Acft, std::string Id);
        bool updateKeyAerodynamicCoefficients(Doc29Aircraft& Doc29Acft, std::string Id) const;
        bool updateKeyProfile(Doc29Profile& Doc29Prof, std::string Id);

        void updatePerformance(const Doc29Aircraft& Doc29Acft) const;
        void updateThrust(const Doc29Aircraft& Doc29Acft) const;
        void updateAerodynamicCoefficients(const Doc29Aircraft& Doc29Acft) const;
        void updateProfile(const Doc29Profile& Doc29Prof) const;

        void loadFromFile();

    private:
        GrapeMap<std::string, Doc29Aircraft> m_Doc29Aircrafts{};
    };
}
