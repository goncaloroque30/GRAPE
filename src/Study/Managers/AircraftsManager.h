// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Manager.h"

#include "Aircraft/Aircraft.h"

namespace GRAPE {
    class Doc29PerformanceManager;
    class Doc29NoiseManager;
    class LTOManager;
    class SFIManager;
    class OperationsManager;

    /**
    * @brief Stores instances of Aircraft class into a GrapeMap and synchronizes with the Database.
    */
    class AircraftsManager : public Manager {
    public:
        AircraftsManager(const Database& Db, Constraints& Blocks, Doc29PerformanceManager& Doc29Acft, Doc29NoiseManager& Doc29Ns, SFIManager& SFIs, LTOManager& LTOs, OperationsManager& Ops);

        auto& aircrafts() noexcept { return m_Aircrafts; }
        auto& operator()() noexcept { return m_Aircrafts; }
        Aircraft& operator()(const std::string& AcftId) noexcept { return m_Aircrafts(AcftId); }
        [[nodiscard]] auto begin() const noexcept { return std::views::values(m_Aircrafts).begin(); }
        [[nodiscard]] auto end() const noexcept { return std::views::values(m_Aircrafts).end(); }

        /**
        * @brief Create a new Aircraft with Name. If Name is empty a default name will be generated.
        * @return The newly constructed Aircraft and true or the already existing Aircraft and false.
        */
        std::pair<Aircraft&, bool> addAircraft(const std::string& Name = "");

        /**
        * @brief Throwing version of addAircraft().
        *
        * Throws if Name is empty.
        * Throws if Name already exists in the container.
        *
        * @return The newly constructed Aircraft.
        */
        Aircraft& addAircraftE(const std::string& Name);

        void setDoc29Performance(Aircraft& Acft, const Doc29Aircraft* Doc29Acft);
        void setDoc29Noise(Aircraft& Acft, const Doc29Noise* Doc29Ns);
        void setSFI(Aircraft& Acft, const SFI* Sfi);
        void setLTO(Aircraft& Acft, const LTOEngine* LTOEng);

        void eraseAircrafts();
        void erase(Aircraft& Acft);

        /**
        * @brief Updates the name of the Aircraft.
        * @return True if the update was successful, false otherwise.
        */
        bool updateKey(Aircraft& Acft, std::string Id);

        /**
        * @brief Resets the values of Aircraft in the database.
        */
        void update(const Aircraft& Acft) const;

        void loadFromFile();

    private:
        Doc29PerformanceManager& m_Doc29Aircrafts;
        Doc29NoiseManager& m_Doc29Noises;
        SFIManager& m_SFIFuels;
        LTOManager& m_LTOEngines;

        OperationsManager& m_Operations;

        GrapeMap<std::string, Aircraft> m_Aircrafts{};
    };
}
