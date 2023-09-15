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

    void LTOManager::update(const LTOEngine& LtoEng) const noexcept { m_Db.update(Schema::lto_fuel_emissions, std::make_tuple(LtoEng.Name, LtoEng.FuelFlows.at(0), LtoEng.FuelFlows.at(1), LtoEng.FuelFlows.at(2), LtoEng.FuelFlows.at(3), LtoEng.FuelFlowCorrectionFactors.at(0), LtoEng.FuelFlowCorrectionFactors.at(1), LtoEng.FuelFlowCorrectionFactors.at(2), LtoEng.FuelFlowCorrectionFactors.at(3), LtoEng.EmissionIndexesHC.at(0), LtoEng.EmissionIndexesHC.at(1), LtoEng.EmissionIndexesHC.at(2), LtoEng.EmissionIndexesHC.at(3), LtoEng.EmissionIndexesCO.at(0), LtoEng.EmissionIndexesCO.at(1), LtoEng.EmissionIndexesCO.at(2), LtoEng.EmissionIndexesCO.at(3), LtoEng.EmissionIndexesNOx.at(0), LtoEng.EmissionIndexesNOx.at(1), LtoEng.EmissionIndexesNOx.at(2), LtoEng.EmissionIndexesNOx.at(3)), { 0 }, std::make_tuple(LtoEng.Name)); }

    void LTOManager::loadFromFile() {
        Statement stmt(m_Db, Schema::lto_fuel_emissions.querySelect());
        stmt.step();
        while (!stmt.done())
        {
            const std::string name = stmt.getColumn(0);
            auto [lto, added] = m_LTOEngines.add(name, name);
            GRAPE_ASSERT(added);
            lto.FuelFlows.at(0) = stmt.getColumn(1);
            lto.FuelFlows.at(1) = stmt.getColumn(2);
            lto.FuelFlows.at(2) = stmt.getColumn(3);
            lto.FuelFlows.at(3) = stmt.getColumn(4);
            lto.FuelFlowCorrectionFactors.at(0) = stmt.getColumn(5);
            lto.FuelFlowCorrectionFactors.at(1) = stmt.getColumn(6);
            lto.FuelFlowCorrectionFactors.at(2) = stmt.getColumn(7);
            lto.FuelFlowCorrectionFactors.at(3) = stmt.getColumn(8);
            lto.EmissionIndexesHC.at(0) = stmt.getColumn(9);
            lto.EmissionIndexesHC.at(1) = stmt.getColumn(10);
            lto.EmissionIndexesHC.at(2) = stmt.getColumn(11);
            lto.EmissionIndexesHC.at(3) = stmt.getColumn(12);
            lto.EmissionIndexesCO.at(0) = stmt.getColumn(13);
            lto.EmissionIndexesCO.at(1) = stmt.getColumn(14);
            lto.EmissionIndexesCO.at(2) = stmt.getColumn(15);
            lto.EmissionIndexesCO.at(3) = stmt.getColumn(16);
            lto.EmissionIndexesNOx.at(0) = stmt.getColumn(17);
            lto.EmissionIndexesNOx.at(1) = stmt.getColumn(18);
            lto.EmissionIndexesNOx.at(2) = stmt.getColumn(19);
            lto.EmissionIndexesNOx.at(3) = stmt.getColumn(20);

            stmt.step();
        }
    }
}
