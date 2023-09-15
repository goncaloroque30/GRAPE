// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Manager.h"

#include "Aircraft/Doc29/Doc29Performance.h"

namespace GRAPE {
    class Doc29PerformanceManager : public Manager {
    public:
        Doc29PerformanceManager(const Database& Db, Constraints& Blocks);

        auto& performances() { return m_Doc29Performances; }
        auto& operator()() { return m_Doc29Performances; }
        auto& operator()(const std::string& Doc29PerfId) { return *m_Doc29Performances(Doc29PerfId); }
        [[nodiscard]] auto begin() const { return std::views::values(m_Doc29Performances).begin(); }
        [[nodiscard]] auto end() const { return std::views::values(m_Doc29Performances).end(); }

        std::pair<Doc29Performance&, bool> addPerformance(Doc29Performance::Type AcftType, const std::string& Name = "");
        bool addProfileArrival(Doc29Performance& Acft, Doc29Profile::Type ProfileType, const std::string& Name = "") const;
        bool addProfileDeparture(Doc29Performance& Acft, Doc29Profile::Type ProfileType, const std::string& Name = "") const;

        Doc29Performance& addPerformanceE(Doc29Performance::Type AcftType, const std::string& Name);
        Doc29ProfileArrival& addProfileArrivalE(Doc29Performance& Perf, Doc29Profile::Type ProfileType, const std::string& Name) const;
        Doc29ProfileDeparture& addProfileDepartureE(Doc29Performance& Perf, Doc29Profile::Type ProfileType, const std::string& Name) const;

        void erasePerformances();
        void erasePerformance(const Doc29Performance& Doc29Perf);
        void eraseProfileArrivals(Doc29Performance& Doc29Perf);
        void eraseProfileDepartures(Doc29Performance& Doc29Perf);
        void eraseProfile(const Doc29Profile& Doc29Prof);

        bool updateKeyPerformance(Doc29Performance& Doc29Perf, std::string Id);
        bool updateKeyAerodynamicCoefficients(Doc29Performance& Doc29Perf, std::string Id) const;
        bool updateKeyProfile(Doc29Profile& Doc29Prof, std::string Id);

        void updatePerformance(const Doc29Performance& Doc29Perf) const;
        void updateThrust(const Doc29Performance& Doc29Perf) const;
        void updateAerodynamicCoefficients(const Doc29Performance& Doc29Perf) const;
        void updateProfile(const Doc29Profile& Doc29Prof) const;

        void loadFromFile();

    private:
        GrapeMap<std::string, std::unique_ptr<Doc29Performance>> m_Doc29Performances{};
    };
}
