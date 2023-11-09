// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "LTOManager.h"

#include "Schema/Schema.h"

namespace GRAPE {
    LTOManager::LTOManager(const Database& Db, Constraints& Blocks) : Manager(Db, Blocks) {}

    std::pair<LTOEngine&, bool> LTOManager::addLTOEngine(const std::string& Name) noexcept {
        const std::string newName = Name.empty() ? uniqueKeyGenerator(m_LTOEngines, "New LTO Engine") : Name;

        auto ret = m_LTOEngines.add(newName, newName);
        auto& [lto, added] = ret;

        if (added)
            m_Db.insert(Schema::lto_fuel_emissions, { 0 }, std::make_tuple(lto.Name));
        else
            Log::dataLogic()->error("Adding LTO engine '{}'. Name already exists in this study.", newName);

        return ret;
    }

    LTOEngine& LTOManager::addLTOEngineE(const std::string& Name) {
        if (Name.empty())
            throw GrapeException("Empty LTO engine name not allowed.");

        auto [lto, added] = m_LTOEngines.add(Name, Name);

        if (added)
            m_Db.insert(Schema::lto_fuel_emissions, { 0 }, std::make_tuple(lto.Name));
        else
            throw GrapeException(std::format("LTO engine '{}' already exists in this study.", Name));

        return lto;
    }

    void LTOManager::eraseAll() noexcept {
        m_LTOEngines.eraseIf([&](const auto& Node) {
            const auto& [name, lto] = Node;
            if (m_Blocks.notRemovable(lto))
            {
                Log::dataLogic()->error("Removing LTO engine '{}'. There are {} aircrafts which use this LTO engine.", name, m_Blocks.blocking(lto).size());
                return false;
            }

            m_Db.deleteD(Schema::lto_fuel_emissions, { 0 }, std::make_tuple(lto.Name));
            return true;
            });
    }

    void LTOManager::erase(LTOEngine& LtoEng) noexcept {
        if (m_Blocks.notRemovable(LtoEng))
        {
            Log::dataLogic()->error("Removing LTO engine '{}'. There are {} aircrafts which use this LTO engine.", LtoEng.Name, m_Blocks.blocking(LtoEng).size());
            return;
        }

        m_Db.deleteD(Schema::lto_fuel_emissions, { 0 }, std::make_tuple(LtoEng.Name));
        m_LTOEngines.erase(LtoEng.Name);
    }

    bool LTOManager::updateKey(LTOEngine& LtoEng, std::string Id) noexcept {
        if (LtoEng.Name.empty())
        {
            Log::dataLogic()->error("Updating LTO engine '{}'. Empty name not allowed.", Id);
            LtoEng.Name = Id;
            return false;
        }

        const bool updated = m_LTOEngines.update(Id, LtoEng.Name);

        if (updated) { m_Db.update(Schema::lto_fuel_emissions, { 0 }, std::make_tuple(LtoEng.Name), { 0 }, std::make_tuple(Id)); }
        else
        {
            Log::dataLogic()->error("Updating LTO engine '{}'. New name '{}' already exists in this study.", Id, LtoEng.Name);
            LtoEng.Name = Id;
        }

        return updated;
    }

    void LTOManager::update(const LTOEngine& LtoEng) const noexcept {
        Statement stmt(m_Db, Schema::lto_fuel_emissions.queryUpdate({}, { 0 }));

        stmt.bind(0, LtoEng.Name);
        std::size_t i = 0;

        stmt.bind(++i, LtoEng.MaximumSeaLevelStaticThrust);

        for (const auto fuelFlow : LtoEng.FuelFlows)
            stmt.bind(++i, fuelFlow);
        for (const auto fuelFlowCorr : LtoEng.FuelFlowCorrectionFactors)
            stmt.bind(++i, fuelFlowCorr);

        for (const auto hc : LtoEng.EmissionIndexesHC)
            stmt.bind(++i, hc);
        for (const auto co : LtoEng.EmissionIndexesCO)
            stmt.bind(++i, co);
        for (const auto nox : LtoEng.EmissionIndexesNOx)
            stmt.bind(++i, nox);

        stmt.bind(++i, static_cast<int>(LtoEng.MixedNozzle));
        stmt.bind(++i, LtoEng.BypassRatio);

        for (const auto afr : LtoEng.AirFuelRatios)
            stmt.bind(++i, afr);

        for (const auto sn : LtoEng.SmokeNumbers)
            std::isnan(sn) ? stmt.bind(++i, std::monostate()) : stmt.bind(++i, sn);
        for (const auto nvpm : LtoEng.EmissionIndexesNVPM)
            std::isnan(nvpm) ? stmt.bind(++i, std::monostate()) : stmt.bind(++i, nvpm);
        for (const auto nvpmNumber : LtoEng.EmissionIndexesNVPMNumber)
            std::isnan(nvpmNumber) ? stmt.bind(++i, std::monostate()) : stmt.bind(++i, nvpmNumber);

        stmt.bind(++i, LtoEng.Name);

        stmt.step();
    }

    void LTOManager::loadFromFile() {
        Statement stmt(m_Db, Schema::lto_fuel_emissions.querySelect());
        stmt.step();
        while (!stmt.done())
        {
            const std::string name = stmt.getColumn(0);
            auto [lto, added] = m_LTOEngines.add(name, name);
            GRAPE_ASSERT(added);

            std::size_t i = 0;

            lto.MaximumSeaLevelStaticThrust = stmt.getColumn(++i);

            for (auto& fuelFlow : lto.FuelFlows)
                fuelFlow = stmt.getColumn(++i);
            for (auto& fuelFlowCorr : lto.FuelFlowCorrectionFactors)
                fuelFlowCorr = stmt.getColumn(++i);

            for (auto& hc : lto.EmissionIndexesHC)
                hc = stmt.getColumn(++i);
            for (auto& co : lto.EmissionIndexesCO)
                co = stmt.getColumn(++i);
            for (auto& nox : lto.EmissionIndexesNOx)
                nox = stmt.getColumn(++i);

            lto.MixedNozzle = static_cast<bool>(stmt.getColumn(++i).getInt());
            lto.BypassRatio = stmt.getColumn(++i);

            for (auto& afr : lto.AirFuelRatios)
                afr = stmt.getColumn(++i);

            for (auto& sn : lto.SmokeNumbers)
            {
                ++i;
                if (!stmt.isColumnNull(i))
                    sn = stmt.getColumn(i);
            }
            for (auto& nvpm : lto.EmissionIndexesNVPM)
            {
                ++i;
                if (!stmt.isColumnNull(i))
                    nvpm = stmt.getColumn(i);
            }
            for (auto& nvpmNumber : lto.EmissionIndexesNVPMNumber)
            {
                ++i;
                if (!stmt.isColumnNull(i))
                    nvpmNumber = stmt.getColumn(i);
            }
            stmt.step();
        }
    }
}
