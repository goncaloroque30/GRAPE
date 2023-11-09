// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "Doc29PerformanceManager.h"
#include "Doc29NoiseManager.h"
#include "LTOManager.h"
#include "OperationsManager.h"
#include "SFIManager.h"

#include "AircraftsManager.h"

#include "Schema/Schema.h"

namespace GRAPE {
    namespace {
        struct OperationAircraftUpdater : OperationVisitor {
            explicit OperationAircraftUpdater(OperationsManager& Ops) : m_Operations(Ops) {}
            void updateOperation(Operation& Op) { Op.accept(*this); }
            void visitFlightArrival(FlightArrival& Op) override { m_Operations.setDoc29Profile(Op, nullptr); }
            void visitFlightDeparture(FlightDeparture& Op) override { m_Operations.setDoc29Profile(Op, nullptr); }
            void visitTrack4dArrival(Track4dArrival& Op) override { m_Operations.update(Op); }
            void visitTrack4dDeparture(Track4dDeparture& Op) override { m_Operations.update(Op); }

        private:
            OperationsManager& m_Operations;
        };
    }

    AircraftsManager::AircraftsManager(const Database& Db, Constraints& Blocks, Doc29PerformanceManager& Doc29Acft, Doc29NoiseManager& Doc29Ns, SFIManager& SFIs, LTOManager& LTOs, OperationsManager& Ops) : Manager(Db, Blocks), m_Doc29Aircrafts(Doc29Acft), m_Doc29Noises(Doc29Ns), m_SFIFuels(SFIs), m_LTOEngines(LTOs), m_Operations(Ops) {}

    std::pair<Aircraft&, bool> AircraftsManager::addAircraft(const std::string& Name) {
        const std::string newName = Name.empty() ? uniqueKeyGenerator(m_Aircrafts, "New Aircraft") : Name;

        auto ret = m_Aircrafts.add(newName, newName);
        auto& [acft, added] = ret;

        if (added)
            m_Db.insert(Schema::fleet, { 0 }, std::make_tuple(acft.Name));
        else
            Log::dataLogic()->error("Adding aircraft '{}'. Aircraft already exists in this study.", newName);

        return ret;
    }

    Aircraft& AircraftsManager::addAircraftE(const std::string& Name) {
        if (Name.empty())
            throw GrapeException("Empty aircraft name not allowed.");

        auto [acft, added] = m_Aircrafts.add(Name, Name);

        if (added)
            m_Db.insert(Schema::fleet, { 0 }, std::make_tuple(acft.Name));
        else
            throw GrapeException(std::format("Aircraft '{}' already exists in this study.", Name));

        return acft;
    }

    void AircraftsManager::setDoc29Performance(Aircraft& Acft, const Doc29Aircraft* Doc29Acft) {
        m_Blocks.aircraftUnblockDoc29Acft(Acft);
        Acft.Doc29Acft = Doc29Acft;
        m_Blocks.aircraftBlockDoc29Acft(Acft);
        update(Acft);
        if (m_Blocks.notRemovable(Acft))
        {
            Log::dataLogic()->warn("Changing aircraft {}. There are {} operations which use this aircraft and will be changed as well.", Acft.Name, m_Blocks.blocking(Acft).size());
            OperationAircraftUpdater updater(m_Operations);
            for (const auto op : m_Blocks.blocking(Acft))
                updater.updateOperation(*op);
        }
    }

    void AircraftsManager::setDoc29Noise(Aircraft& Acft, const Doc29Noise* Doc29Ns) {
        m_Blocks.aircraftUnblockDoc29Noise(Acft);
        Acft.Doc29Ns = Doc29Ns;
        m_Blocks.aircraftBlockDoc29Noise(Acft);
        update(Acft);
    }

    void AircraftsManager::setSFI(Aircraft& Acft, const SFI* Sfi) {
        m_Blocks.aircraftUnblockSfi(Acft);
        Acft.SFIFuel = Sfi;
        m_Blocks.aircraftBlockSfi(Acft);
        update(Acft);
    }

    void AircraftsManager::setLTO(Aircraft& Acft, const LTOEngine* LTOEng) {
        m_Blocks.aircraftUnblockLTOEngine(Acft);
        Acft.LTOEng = LTOEng;
        m_Blocks.aircraftBlockLTOEngine(Acft);
        update(Acft);
    }

    void AircraftsManager::eraseAircrafts() {
        m_Aircrafts.eraseIf([&](const auto& Node) {
            const auto& [fleetID, acft] = Node;
            if (m_Blocks.notRemovable(acft))
            {
                Log::dataLogic()->error("Removing aircraft '{}'. There are {} operations which use this aircraft.", fleetID, m_Blocks.blocking(acft).size());
                return false;
            }

            m_Db.deleteD(Schema::fleet, { 0 }, std::make_tuple(acft.Name));
            m_Blocks.aircraftUnblock(acft);
            return true;
            });
    }

    void AircraftsManager::erase(Aircraft& Acft) {
        if (m_Blocks.notRemovable(Acft))
        {
            Log::dataLogic()->error("Removing aircraft '{}'. There are {} operations which use this aircraft.", Acft.Name, m_Blocks.blocking(Acft).size());
            return;
        }

        m_Db.deleteD(Schema::fleet, { 0 }, std::make_tuple(Acft.Name));
        m_Blocks.aircraftUnblock(Acft);

        m_Aircrafts.erase(Acft.Name);
    }

    bool AircraftsManager::updateKey(Aircraft& Acft, const std::string Id) {
        if (Acft.Name.empty())
        {
            Log::dataLogic()->error("Updating aircraft '{}'. Empty name not allowed.", Id);
            Acft.Name = Id;
            return false;
        }

        const bool updated = m_Aircrafts.update(Id, Acft.Name);

        if (updated) { m_Db.update(Schema::fleet, { 0 }, std::make_tuple(Acft.Name), { 0 }, std::make_tuple(Id)); }
        else
        {
            Log::dataLogic()->error("Updating aircraft '{}'. Aircraft new name '{}' already exists in this study.", Id, Acft.Name);
            Acft.Name = Id;
        }

        return updated;
    }

    void AircraftsManager::update(const Aircraft& Acft) const {
        Statement stmt(m_Db, Schema::fleet.queryUpdate({}, { 0 }));
        stmt.bind(0, Acft.Name);
        stmt.bind(1, Acft.EngineCount);
        Acft.validDoc29Performance() ? stmt.bind(2, Acft.Doc29Acft->Name) : stmt.bind(2, std::monostate());
        Acft.validSFI() ? stmt.bind(3, Acft.SFIFuel->Name) : stmt.bind(3, std::monostate());
        Acft.validLTOEngine() ? stmt.bind(4, Acft.LTOEng->Name) : stmt.bind(4, std::monostate());
        Acft.validDoc29Noise() ? stmt.bind(5, Acft.Doc29Ns->Name) : stmt.bind(5, std::monostate());
        stmt.bind(6, Acft.Doc29NoiseDeltaArrivals);
        stmt.bind(7, Acft.Doc29NoiseDeltaDepartures);

        stmt.bind(8, Acft.Name);

        stmt.step();
    }

    void AircraftsManager::loadFromFile() {
        Statement stmt(m_Db, Schema::fleet.querySelect());
        stmt.step();
        while (stmt.hasRow())
        {
            const std::string name = stmt.getColumn(0);
            auto [acft, added] = m_Aircrafts.add(name, name);
            GRAPE_ASSERT(added);

            acft.EngineCount = stmt.getColumn(1);

            if (!stmt.isColumnNull(2))
            {
                const std::string doc29AcftStr = stmt.getColumn(2);
                if (m_Doc29Aircrafts.performances().contains(doc29AcftStr))
                    acft.Doc29Acft = &m_Doc29Aircrafts(doc29AcftStr);
                else
                    Log::io()->error("Loading Aircraft '{}'. Doc29 Aircraft '{}' does not exist in this study.", name, doc29AcftStr);
            }
            if (!stmt.isColumnNull(3))
            {
                const std::string sfiFuelStr = stmt.getColumn(3);
                if (m_SFIFuels.sfiFuels().contains(sfiFuelStr))
                    acft.SFIFuel = &m_SFIFuels(sfiFuelStr);
                else
                    Log::io()->error("Loading Aircraft '{}'. SFI '{}' does not exist in this study.", name, sfiFuelStr);
            }
            if (!stmt.isColumnNull(4))
            {
                const std::string ltoEngStr = stmt.getColumn(4);
                if (m_LTOEngines.ltoEngines().contains(ltoEngStr))
                    acft.LTOEng = &m_LTOEngines(ltoEngStr);
                else
                    Log::io()->error("Loading Aircraft '{}'. LTO Engine '{}' does not exist in this study.", name, ltoEngStr);
            }
            if (!stmt.isColumnNull(5))
            {
                const std::string doc29NsStr = stmt.getColumn(5);
                if (m_Doc29Noises.noises().contains(doc29NsStr))
                    acft.Doc29Ns = &m_Doc29Noises(doc29NsStr);
                else
                    Log::io()->error("Loading Aircraft '{}'. Doc29 Noise '{}' does not exist in this study.", name, doc29NsStr);
            }

            acft.Doc29NoiseDeltaArrivals = stmt.getColumn(6);
            acft.Doc29NoiseDeltaDepartures = stmt.getColumn(7);

            m_Blocks.aircraftBlock(acft);

            stmt.step();
        }
    }
}
