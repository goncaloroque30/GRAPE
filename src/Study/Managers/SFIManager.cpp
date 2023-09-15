// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "SFIManager.h"

#include "Schema/Schema.h"

namespace GRAPE {
    SFIManager::SFIManager(const Database& Db, Constraints& Blocks) : Manager(Db, Blocks) {}

    std::pair<SFI&, bool> SFIManager::addSFI(const std::string& Name) noexcept {
        const std::string newName = Name.empty() ? uniqueKeyGenerator(m_SFIFuels, "New SFI Coefficients") : Name;

        auto ret = m_SFIFuels.add(newName, newName);
        auto& [sfi, added] = ret;

        if (added)
            m_Db.insert(Schema::sfi_fuel, { 0 }, std::make_tuple(sfi.Name));
        else
            Log::dataLogic()->error("Adding SFI fuel coefficients '{}'. Name already exists in this study.", newName);

        return ret;
    }

    SFI& SFIManager::addSFIE(const std::string& Name) {
        if (Name.empty())
            throw GrapeException("Empty SFI entry name not allowed.");

        auto [sfi, added] = m_SFIFuels.add(Name, Name);

        if (added)
            m_Db.insert(Schema::sfi_fuel, { 0 }, std::make_tuple(sfi.Name));
        else
            throw GrapeException(std::format("SFI entry '{}' already exists in this study.", Name));

        return sfi;
    }

    void SFIManager::eraseAll() noexcept {
        m_SFIFuels.eraseIf([&](const auto& Node) {
            const auto& [name, sfi] = Node;
            if (m_Blocks.notRemovable(sfi))
            {
                Log::dataLogic()->error("Removing SFI fuel coefficients '{}'. There are {} aircrafts which use this coefficients.", name, m_Blocks.blocking(sfi).size());
                return false;
            }

            m_Db.deleteD(Schema::sfi_fuel, { 0 }, std::make_tuple(sfi.Name));
            return true;
            });
    }

    void SFIManager::erase(SFI& Sfi) noexcept {
        if (m_Blocks.notRemovable(Sfi))
        {
            Log::dataLogic()->error("Removing SFI fuel coefficients '{}'. There are {} aircrafts which use this coefficients.", Sfi.Name, m_Blocks.blocking(Sfi).size());
            return;
        }

        m_Db.deleteD(Schema::sfi_fuel, { 0 }, std::make_tuple(Sfi.Name));
        m_SFIFuels.erase(Sfi.Name);
    }

    bool SFIManager::updateKey(SFI& Sfi, std::string Id) noexcept {
        if (Sfi.Name.empty())
        {
            Log::dataLogic()->error("Updating SFI fuel coefficients '{}'. Empty name not allowed.", Id);
            Sfi.Name = Id;
            return false;
        }

        const bool updated = m_SFIFuels.update(Id, Sfi.Name);

        if (updated) { m_Db.update(Schema::sfi_fuel, { 0 }, std::make_tuple(Sfi.Name), { 0 }, std::make_tuple(Id)); }
        else
        {
            Log::dataLogic()->error("Updating SFI fuel coefficients '{}'. New name '{}' already exists in this study.", Id, Sfi.Name);
            Sfi.Name = Id;
        }

        return updated;
    }

    void SFIManager::update(const SFI& Sfi) const noexcept { m_Db.update(Schema::sfi_fuel, std::make_tuple(Sfi.Name, Sfi.A, Sfi.B1, Sfi.B2, Sfi.B3, Sfi.K1, Sfi.K2, Sfi.K3, Sfi.K4), { 0 }, std::make_tuple(Sfi.Name)); }

    void SFIManager::loadFromFile() {
        Statement stmt(m_Db, Schema::sfi_fuel.querySelect());
        stmt.step();
        while (stmt.hasRow())
        {
            const std::string name = stmt.getColumn(0);
            auto [sfi, added] = m_SFIFuels.add(name, name);
            GRAPE_ASSERT(added);
            sfi.A = stmt.getColumn(1);
            sfi.B1 = stmt.getColumn(2);
            sfi.B2 = stmt.getColumn(3);
            sfi.B3 = stmt.getColumn(4);
            sfi.K1 = stmt.getColumn(5);
            sfi.K2 = stmt.getColumn(6);
            sfi.K3 = stmt.getColumn(7);
            sfi.K4 = stmt.getColumn(8);

            stmt.step();
        }
    }
}
