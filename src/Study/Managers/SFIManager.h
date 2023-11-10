// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Manager.h"

#include "Aircraft/FuelEmissions/SFI.h"

namespace GRAPE {
    /**
    * @brief Stores instances of the SFI class into a GrapeMap and synchronizes with the Database.
    */
    class SFIManager : public Manager {
    public:
        SFIManager(const Database& Db, Constraints& Blocks);

        auto& sfiFuels() noexcept { return m_SFIFuels; }
        auto& operator()() noexcept { return m_SFIFuels; }
        const SFI& operator()(const std::string& SFIId) noexcept { return m_SFIFuels(SFIId); }
        [[nodiscard]] auto begin() const noexcept { return std::views::values(m_SFIFuels).begin(); }
        [[nodiscard]] auto end() const noexcept { return std::views::values(m_SFIFuels).end(); }
        [[nodiscard]] auto begin() noexcept { return std::views::values(m_SFIFuels).begin(); }
        [[nodiscard]] auto end() noexcept { return std::views::values(m_SFIFuels).end(); }

        /**
        * @brief Create a new SFI entry with Name. If Name is empty a default name will be generated.
        * @return The newly constructed SFI and true or the already existing SFI and false.
        */
        std::pair<SFI&, bool> addSFI(const std::string& Name = "") noexcept;

        /**
        * @brief Throwing version of addSFI().
        *
        * Throws if Name is empty.
        * Throws if Name already exists in the container.
        *
        * @return The newly constructed SFI.
        */
        SFI& addSFIE(const std::string& Name);

        void eraseAll() noexcept;
        void erase(SFI& Sfi) noexcept;

        /**
        * @brief Updates the name of the SFI.
        * @return True if the update was successful, false otherwise.
        */
        bool updateKey(SFI& Sfi, std::string Id) noexcept;

        /**
        * @brief Resets the values of Sfi in the database.
        */
        void update(const SFI& Sfi) const noexcept;

        /**
        * @brief Fills #m_SFIFuels with the values found in the database.
        */
        void loadFromFile();

    private:
        GrapeMap<std::string, SFI> m_SFIFuels;
    };
}
