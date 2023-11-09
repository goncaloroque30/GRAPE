// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Manager.h"

#include "Aircraft/FuelEmissions/LTO.h"

namespace GRAPE {
    /**
    * @brief Stores instances of the LTO class into a GrapeMap and synchronizes with the Database.
    */
    class LTOManager : public Manager {
    public:
        LTOManager(const Database& Db, Constraints& Blocks);

        auto& ltoEngines() noexcept { return m_LTOEngines; }
        auto& operator()() noexcept { return m_LTOEngines; }
        const LTOEngine& operator()(const std::string& LTOId) noexcept { return m_LTOEngines(LTOId); }
        [[nodiscard]] auto begin() const noexcept { return std::views::values(m_LTOEngines).begin(); }
        [[nodiscard]] auto end() const noexcept { return std::views::values(m_LTOEngines).end(); }
        [[nodiscard]] auto begin() noexcept { return std::views::values(m_LTOEngines).begin(); }
        [[nodiscard]] auto end() noexcept { return std::views::values(m_LTOEngines).end(); }

        /**
        * @brief Create a new LTO entry with Name. If Name is empty a default name will be generated.
        * @return The newly constructed LTOEngine and true or the already existing LTOEngine and false.
        */
        std::pair<LTOEngine&, bool> addLTOEngine(const std::string& Name = "") noexcept;

        /**
        * @brief Throwing version of addLTOEngine.
        *
        * Throws if Name is empty.
        * Throws if Name already exists in the container.
        *
        * @return The newly constructed LTOEngine.
        */
        LTOEngine& addLTOEngineE(const std::string& Name);

        void eraseAll() noexcept;
        void erase(LTOEngine& LtoEng) noexcept;

        /**
        * @brief Updates the name of the LTOEngine.
        * @return True if the update was successful, false otherwise.
        */
        bool updateKey(LTOEngine& LtoEng, std::string Id) noexcept;

        /**
        * @brief Resets the values of LtoEng in the database.
        */
        void update(const LTOEngine& LtoEng) const noexcept;

        void loadFromFile();

    private:
        GrapeMap<std::string, LTOEngine> m_LTOEngines;
    };
}
