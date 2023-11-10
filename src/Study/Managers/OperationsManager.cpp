// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "OperationsManager.h"

#include "AircraftsManager.h"
#include "AirportsManager.h"

#include "Schema/Schema.h"

namespace GRAPE {
    namespace {
        void bindFlight(Statement& Stmt, const Flight& Op) {
            Stmt.bind(0, Op.Name);
            Stmt.bind(1, OperationTypes.toString(Op.operationType()));
            if (Op.hasRoute())
            {
                Stmt.bind(2, Op.route().parentAirport().Name);
                Stmt.bind(3, Op.route().parentRunway().Name);
                Stmt.bind(4, Op.route().Name);
            }
            else
            {
                Stmt.bind(2, std::monostate());
                Stmt.bind(3, std::monostate());
                Stmt.bind(4, std::monostate());
            }
            Stmt.bind(5, timeToUtcString(Op.Time));
            Stmt.bind(6, Op.Count);
            Stmt.bind(7, Op.aircraft().Name);
            Stmt.bind(8, Op.Weight);
        }

        void insertFlight(const Database& Db, const Flight& Op) {
            Statement stmt(Db, Schema::operations_flights.queryInsert({}));
            bindFlight(stmt, Op);
            stmt.step();
        }

        void updateFlight(const Database& Db, const Flight& Op) {
            Statement stmt(Db, Schema::operations_flights.queryUpdate({}, { 0, 1 }));
            bindFlight(stmt, Op);
            stmt.bind(9, Op.Name);
            stmt.bind(10, OperationTypes.toString(Op.operationType()));
            stmt.step();
        }

        auto allValues(const Track4d& Op) { return std::make_tuple(Op.Name, OperationTypes.toString(Op.operationType()), timeToUtcString(Op.Time), Op.Count, Op.aircraft().Name); }

        auto primaryKey(const Operation& Op) { return std::make_tuple(Op.Name, OperationTypes.toString(Op.operationType())); }

        template <OperationType OpType>
        void tracks4dPointsUpdater(const Database& Db, const Track4dOp<OpType>& Op) {
            Db.deleteD(Schema::operations_tracks_4d_points, { 0, 1 }, std::make_tuple(Op.Name, OperationTypes.toString(Op.operationType())));

            Statement stmt(Db, Schema::operations_tracks_4d_points.queryInsert());
            stmt.bindValues(Op.Name, OperationTypes.toString(Op.operationType()));
            for (auto it = Op.begin(); it != Op.end(); ++it)
            {
                const auto i = it - Op.begin();
                auto& pt = *it;

                std::size_t col = 2;
                stmt.bind(col++, static_cast<int>(i + 1));
                stmt.bind(col++, timeToUtcString(pt.Time));
                stmt.bind(col++, FlightPhases.toString(pt.FlPhase));
                stmt.bind(col++, pt.CumulativeGroundDistance);
                stmt.bind(col++, pt.Longitude);
                stmt.bind(col++, pt.Latitude);
                stmt.bind(col++, pt.AltitudeMsl);
                stmt.bind(col++, pt.TrueAirspeed);
                stmt.bind(col++, pt.Groundspeed);
                stmt.bind(col++, pt.CorrNetThrustPerEng);
                stmt.bind(col++, pt.BankAngle);
                stmt.bind(col++, pt.FuelFlowPerEng);
                stmt.step();
                stmt.reset();
            }
        }

        struct OperationUpdater : OperationVisitor {
            OperationUpdater(const Database& Db, const Operation& Op) : m_Db(Db) { Op.accept(*this); }
            void visitFlightArrival(const FlightArrival& Op) override;
            void visitFlightDeparture(const FlightDeparture& Op) override;
            void visitTrack4dArrival(const Track4dArrival& Op) override;
            void visitTrack4dDeparture(const Track4dDeparture& Op) override;

        private:
            const Database& m_Db;
        };

        struct Track4dPointsUpdater : OperationVisitor {
            Track4dPointsUpdater(const Database& Db, const Track4d& Op) : m_Db(Db) { Op.accept(*this); }
            void visitTrack4dArrival(const Track4dArrival& Op) override { tracks4dPointsUpdater(m_Db, Op); }
            void visitTrack4dDeparture(const Track4dDeparture& Op) override { tracks4dPointsUpdater(m_Db, Op); }

        private:
            const Database& m_Db;
        };
    }

    OperationsManager::OperationsManager(const Database& Db, Constraints& Blocks, AircraftsManager& Aircrafts, AirportsManager& Airports) : Manager(Db, Blocks), Tracks4dLoader(Db), m_Aircrafts(Aircrafts), m_Airports(Airports) {}

    std::pair<FlightArrival&, bool> OperationsManager::addArrivalFlight(const std::string& Name, const Aircraft& AircraftIn) {
        auto ret = m_FlightArrivals.add(Name, Name, AircraftIn);
        auto& [op, added] = ret;

        if (added)
        {
            insertFlight(m_Db, op);
            OperationUpdater up(m_Db, op);
            m_Blocks.operationBlock(op);
        }
        else { Log::dataLogic()->error("Adding arrival flight '{}'. Arrival flight already exists in this study.", Name); }

        return ret;
    }

    bool OperationsManager::addArrivalFlight(const std::string& Name) {
        const std::string newName = Name.empty() ? uniqueKeyGenerator(m_FlightArrivals, "New Arrival Flight") : Name;

        const Aircraft* acft = m_Aircrafts().empty() ? nullptr : &m_Aircrafts.aircrafts().begin()->second;

        if (!acft)
        {
            Log::dataLogic()->error("Adding arrival flight '{}'. No aircraft were found in this study.", newName);
            return false;
        }

        auto [newArr, added] = addArrivalFlight(newName, *acft);
        return added;
    }

    std::pair<FlightDeparture&, bool> OperationsManager::addDepartureFlight(const std::string& Name, const Aircraft& AircraftIn) {
        auto ret = m_FlightDepartures.add(Name, Name, AircraftIn);
        auto& [op, added] = ret;

        if (added)
        {
            insertFlight(m_Db, op);
            OperationUpdater up(m_Db, op);
            m_Blocks.operationBlock(op);
        }
        else { Log::dataLogic()->error("Adding departure flight '{}'. Departure flight already exists in this study.", Name); }

        return ret;
    }

    bool OperationsManager::addDepartureFlight(const std::string& Name) {
        const std::string newName = Name.empty() ? uniqueKeyGenerator(m_FlightDepartures, "New Departure Flight") : Name;

        const Aircraft* acft = m_Aircrafts().empty() ? nullptr : &m_Aircrafts.aircrafts().begin()->second;

        if (!acft)
        {
            Log::dataLogic()->error("Adding departure flight '{}'. No aircraft were found in this study.", newName);
            return false;
        }

        auto [newDep, added] = addDepartureFlight(newName, *acft);
        return added;
    }

    std::pair<Track4dArrival&, bool> OperationsManager::addArrivalTrack4d(const std::string& Name, const Aircraft& AircraftIn) {
        auto ret = m_Track4dArrivals.add(Name, Name, AircraftIn);
        auto& [op, added] = ret;

        if (added)
        {
            m_Db.insert(Schema::operations_tracks_4d, {}, allValues(op));
            OperationUpdater up(m_Db, op);
            m_Blocks.operationBlock(op);
        }
        else { Log::dataLogic()->error("Adding arrival track 4D '{}'. Arrival track 4D already exists in this study.", Name); }

        return ret;
    }

    bool OperationsManager::addArrivalTrack4d(const std::string& Name) {
        const std::string newName = Name.empty() ? uniqueKeyGenerator(m_Track4dArrivals, "New Arrival Track4D") : Name;

        const Aircraft* acft = m_Aircrafts().empty() ? nullptr : &m_Aircrafts.aircrafts().begin()->second;

        if (!acft)
        {
            Log::dataLogic()->error("Adding arrival track 4D '{}'. No aircraft were found in this study.", newName);
            return false;
        }

        auto [newDep, added] = addArrivalTrack4d(newName, *acft);
        return added;
    }

    std::pair<Track4dDeparture&, bool> OperationsManager::addDepartureTrack4d(const std::string& Name, const Aircraft& AircraftIn) {
        auto ret = m_Track4dDepartures.add(Name, Name, AircraftIn);
        auto& [op, added] = ret;

        if (added)
        {
            m_Db.insert(Schema::operations_tracks_4d, {}, allValues(op));
            OperationUpdater up(m_Db, op);
            m_Blocks.operationBlock(op);
        }
        else { Log::dataLogic()->error("Adding departure track 4D '{}'. Operation already exists in this study.", Name); }

        return ret;
    }

    bool OperationsManager::addDepartureTrack4d(const std::string& Name) {
        const std::string newName = Name.empty() ? uniqueKeyGenerator(m_Track4dDepartures, "New Departure Track4D") : Name;

        const Aircraft* acft = m_Aircrafts().empty() ? nullptr : &m_Aircrafts.aircrafts().begin()->second;

        if (!acft)
        {
            Log::dataLogic()->error("Adding departure track 4D '{}'. No aircraft were found in this study.", newName);
            return false;
        }

        auto [newDep, added] = addDepartureTrack4d(newName, *acft);
        return added;
    }

    FlightArrival& OperationsManager::addArrivalFlightE(const std::string& Name, const Aircraft& AircraftIn) {
        if (Name.empty())
            throw GrapeException("Empty flight name not allowed.");

        auto [op, added] = m_FlightArrivals.add(Name, Name, AircraftIn);

        if (added)
        {
            insertFlight(m_Db, op);
            OperationUpdater up(m_Db, op);
            m_Blocks.operationBlock(op);
        }
        else { throw GrapeException(std::format("Arrival flight '{}' already exists in this study.", Name)); }

        return op;
    }

    FlightDeparture& OperationsManager::addDepartureFlightE(const std::string& Name, const Aircraft& AircraftIn) {
        if (Name.empty())
            throw GrapeException("Empty flight name not allowed.");

        auto [op, added] = m_FlightDepartures.add(Name, Name, AircraftIn);

        if (added)
        {
            insertFlight(m_Db, op);
            OperationUpdater up(m_Db, op);
            m_Blocks.operationBlock(op);
        }
        else { throw GrapeException(std::format("Departure flight '{}' already exists in this study.", Name)); }

        return op;
    }

    Track4dArrival& OperationsManager::addArrivalTrack4dE(const std::string& Name, const Aircraft& AircraftIn) {
        if (Name.empty())
            throw GrapeException("Empty track 4D name not allowed.");

        auto [op, added] = m_Track4dArrivals.add(Name, Name, AircraftIn);

        if (added)
        {
            m_Db.insert(Schema::operations_tracks_4d, {}, allValues(op));
            OperationUpdater up(m_Db, op);
            m_Blocks.operationBlock(op);
        }
        else { throw GrapeException(std::format("Arrival track 4D '{}' already exists in this study.", Name)); }

        return op;
    }

    Track4dDeparture& OperationsManager::addDepartureTrack4dE(const std::string& Name, const Aircraft& AircraftIn) {
        if (Name.empty())
            throw GrapeException("Empty track 4D name not allowed.");

        auto [op, added] = m_Track4dDepartures.add(Name, Name, AircraftIn);

        if (added)
        {
            m_Db.insert(Schema::operations_tracks_4d, {}, allValues(op));
            OperationUpdater up(m_Db, op);
            m_Blocks.operationBlock(op);
        }
        else { throw GrapeException(std::format("Departure track 4D '{}' already exists in this study.", Name)); }

        return op;
    }

    void OperationsManager::setAircraft(FlightArrival& Op, const Aircraft& Acft) const {
        m_Blocks.operationUnblockAircraft(Op);
        Op.setAircraft(Acft);
        m_Blocks.operationBlockAircraft(Op);
        setDoc29Profile(Op, nullptr); // Will update database
    }

    void OperationsManager::setAircraft(FlightDeparture& Op, const Aircraft& Acft) const {
        m_Blocks.operationUnblockAircraft(Op);
        Op.setAircraft(Acft);
        m_Blocks.operationBlockAircraft(Op);
        setDoc29Profile(Op, nullptr); // Will update database
    }

    void OperationsManager::setAircraft(Track4d& Op, const Aircraft& Acft) const {
        m_Blocks.operationUnblockAircraft(Op);
        Op.setAircraft(Acft);
        m_Blocks.operationBlockAircraft(Op);
        OperationUpdater up(m_Db, Op);
    }

    void OperationsManager::setRoute(FlightArrival& Op, const RouteArrival* Rte) const {
        m_Blocks.operationUnblockRoute(Op);
        Op.setRoute(Rte);
        m_Blocks.operationBlockRoute(Op);
        OperationUpdater up(m_Db, Op);
    }

    void OperationsManager::setRoute(FlightDeparture& Op, const RouteDeparture* Rte) const {
        m_Blocks.operationUnblockRoute(Op);
        Op.setRoute(Rte);
        m_Blocks.operationBlockRoute(Op);
        OperationUpdater up(m_Db, Op);
    }

    void OperationsManager::setDoc29Profile(FlightArrival& Op, const Doc29ProfileArrival* Profile) const {
        m_Blocks.operationBlockDoc29Profile(Op);
        Op.Doc29Prof = Profile;
        m_Blocks.operationUnblockDoc29Profile(Op);
        OperationUpdater up(m_Db, Op);
    }

    void OperationsManager::setDoc29Profile(FlightDeparture& Op, const Doc29ProfileDeparture* Profile) const {
        m_Blocks.operationBlockDoc29Profile(Op);
        Op.Doc29Prof = Profile;
        m_Blocks.operationUnblockDoc29Profile(Op);
        OperationUpdater up(m_Db, Op);
    }

    void OperationsManager::eraseFlightArrivals() {
        m_FlightArrivals.eraseIf([&](auto& Node) {
            auto& [opName, op] = Node;
            if (m_Blocks.notRemovable(op))
            {
                Log::dataLogic()->error("Removing arrival flight '{}'. There are {} scenarios which contain this operation.", opName, m_Blocks.blocking(op).size());
                return false;
            }

            m_Db.deleteD(Schema::operations_flights, { 0, 1 }, std::make_tuple(op.Name, OperationTypes.toString(op.operationType())));
            m_Blocks.operationUnblock(op);

            return true;
            });
    }

    void OperationsManager::eraseFlightDepartures() {
        m_FlightDepartures.eraseIf([&](auto& Node) {
            auto& [opName, op] = Node;
            if (m_Blocks.notRemovable(op))
            {
                Log::dataLogic()->error("Removing arrival flight '{}'. There are {} scenarios which contain this operation.", opName, m_Blocks.blocking(op).size());
                return false;
            }

            m_Db.deleteD(Schema::operations_flights, { 0, 1 }, primaryKey(op));
            m_Blocks.operationUnblock(op);

            return true;
            });
    }

    void OperationsManager::eraseTrack4dArrivals() {
        m_Track4dArrivals.eraseIf([&](auto& Node) {
            auto& [opName, op] = Node;
            if (m_Blocks.notRemovable(op))
            {
                Log::dataLogic()->error("Removing arrival track 4D '{}'. There are {} scenarios which contain this operation.", opName, m_Blocks.blocking(op).size());
                return false;
            }

            m_Db.deleteD(Schema::operations_tracks_4d, { 0, 1 }, primaryKey(op));
            m_Blocks.operationUnblock(op);

            return true;
            });
    }

    void OperationsManager::eraseTrack4dDepartures() {
        m_Track4dDepartures.eraseIf([&](auto& Node) {
            auto& [opName, op] = Node;
            if (m_Blocks.notRemovable(op))
            {
                Log::dataLogic()->error("Removing departure track 4D '{}'. There are {} scenarios which contain this operation.", opName, m_Blocks.blocking(op).size());
                return false;
            }

            m_Db.deleteD(Schema::operations_tracks_4d, { 0, 1 }, primaryKey(op));
            m_Blocks.operationUnblock(op);

            return true;
            });
    }

    void OperationsManager::erase(FlightArrival& Op) {
        if (m_Blocks.notRemovable(Op))
        {
            Log::dataLogic()->error("Removing departure flight '{}'. There are {} scenarios which contain this operation.", Op.Name, m_Blocks.blocking(Op).size());
            return;
        }

        m_Db.deleteD(Schema::operations_flights, { 0, 1 }, primaryKey(Op));
        m_Blocks.operationUnblock(Op);

        m_FlightArrivals.erase(Op.Name);
    }

    void OperationsManager::erase(FlightDeparture& Op) {
        if (m_Blocks.notRemovable(Op))
        {
            Log::dataLogic()->error("Removing departure flight '{}'. There are {} scenarios which contain this operation.", Op.Name, m_Blocks.blocking(Op).size());
            return;
        }

        m_Db.deleteD(Schema::operations_flights, { 0, 1 }, primaryKey(Op));
        m_Blocks.operationUnblock(Op);

        m_FlightDepartures.erase(Op.Name);
    }

    void OperationsManager::erase(Track4dArrival& Op) {
        if (m_Blocks.notRemovable(Op))
        {
            Log::dataLogic()->error("Removing arrival track 4D '{}'. There are {} scenarios which contain this operation.", Op.Name, m_Blocks.blocking(Op).size());
            return;
        }

        m_Db.deleteD(Schema::operations_tracks_4d, { 0, 1 }, primaryKey(Op));
        m_Blocks.operationUnblock(Op);

        m_Track4dArrivals.erase(Op.Name);
    }

    void OperationsManager::erase(Track4dDeparture& Op) {
        if (m_Blocks.notRemovable(Op))
        {
            Log::dataLogic()->error("Removing departure track 4D operation '{}'. There are {} scenarios which contain this operation.", Op.Name, m_Blocks.blocking(Op).size());
            return;
        }

        m_Db.deleteD(Schema::operations_tracks_4d, { 0, 1 }, primaryKey(Op));
        m_Blocks.operationUnblock(Op);

        m_Track4dDepartures.erase(Op.Name);
    }

    bool OperationsManager::updateKey(FlightArrival& Op, const std::string Id) {
        if (Op.Name.empty())
        {
            Log::dataLogic()->error("Updating arrival flight operation '{}'. Empty name not allowed.", Id);
            Op.Name = Id;
            return false;
        }

        const bool updated = m_FlightArrivals.update(Id, Op.Name);

        if (updated) { m_Db.update(Schema::operations_flights, { 0 }, std::make_tuple(Op.Name), { 0, 1 }, std::make_tuple(Id, OperationTypes.toString(Op.operationType()))); }
        else
        {
            Log::dataLogic()->error("Updating arrival flight operation '{}'. Operation new name '{}' already exists in this study.", Id, Op.Name);
            Op.Name = Id;
        }

        return updated;
    }

    bool OperationsManager::updateKey(FlightDeparture& Op, const std::string Id) {
        if (Op.Name.empty())
        {
            Log::dataLogic()->error("Updating departure flight operation '{}'. Empty name not allowed.", Id);
            Op.Name = Id;
            return false;
        }

        const bool updated = m_FlightDepartures.update(Id, Op.Name);

        if (updated) { m_Db.update(Schema::operations_flights, { 0 }, std::make_tuple(Op.Name), { 0, 1 }, std::make_tuple(Id, OperationTypes.toString(Op.operationType()))); }
        else
        {
            Log::dataLogic()->error("Updating departure flight operation '{}'. Operation new name '{}' already exists in this study.", Id, Op.Name);
            Op.Name = Id;
        }

        return updated;
    }

    bool OperationsManager::updateKey(Track4dArrival& Op, const std::string Id) {
        if (Op.Name.empty())
        {
            Log::dataLogic()->error("Updating arrival track 4D operation '{}'. Empty name not allowed.", Id);
            Op.Name = Id;
            return false;
        }

        const bool updated = m_Track4dArrivals.update(Id, Op.Name);

        if (updated) { m_Db.update(Schema::operations_tracks_4d, { 0 }, std::make_tuple(Op.Name), { 0, 1 }, std::make_tuple(Id, OperationTypes.toString(Op.operationType()))); }
        else
        {
            Log::dataLogic()->error("Updating arrival track 4D operation '{}'. Operation new name '{}' already exists in this study.", Id, Op.Name);
            Op.Name = Id;
        }

        return updated;
    }

    bool OperationsManager::updateKey(Track4dDeparture& Op, const std::string Id) {
        if (Op.Name.empty())
        {
            Log::dataLogic()->error("Updating departure track 4D operation '{}'. Empty name not allowed.", Id);
            Op.Name = Id;
            return false;
        }

        const bool updated = m_Track4dDepartures.update(Id, Op.Name);

        if (updated) { m_Db.update(Schema::operations_tracks_4d, { 0 }, std::make_tuple(Op.Name), { 0, 1 }, std::make_tuple(Id, OperationTypes.toString(Op.operationType()))); }
        else
        {
            Log::dataLogic()->error("Updating departure track 4D operation '{}'. Operation new name '{}' already exists in this study.", Id, Op.Name);
            Op.Name = Id;
        }

        return updated;
    }

    void OperationsManager::update(const Flight& Op) const { OperationUpdater up(m_Db, Op); }

    void OperationsManager::update(const Track4d& Op) const {
        OperationUpdater up(m_Db, Op);
        Track4dPointsUpdater upPts(m_Db, Op);
    }

    void OperationUpdater::visitFlightArrival(const FlightArrival& Op) {
        m_Db.deleteD(Schema::operations_flights_arrival, { 0, 1 }, primaryKey(Op));
        m_Db.deleteD(Schema::operations_flights_departure, { 0, 1 }, primaryKey(Op));
        updateFlight(m_Db, Op);

        Statement stmt(m_Db, Schema::operations_flights_arrival.queryInsert());
        stmt.bindValues(primaryKey(Op));
        Op.hasDoc29Profile() ? stmt.bind(2, Op.Doc29Prof->Name) : stmt.bind(2, std::monostate());
        stmt.step();
    }

    void OperationUpdater::visitFlightDeparture(const FlightDeparture& Op) {
        m_Db.deleteD(Schema::operations_flights_arrival, { 0, 1 }, primaryKey(Op));
        m_Db.deleteD(Schema::operations_flights_departure, { 0, 1 }, primaryKey(Op));
        updateFlight(m_Db, Op);

        Statement stmt(m_Db, Schema::operations_flights_departure.queryInsert());
        stmt.bindValues(primaryKey(Op));
        Op.hasDoc29Profile() ? stmt.bind(2, Op.Doc29Prof->Name) : stmt.bind(2, std::monostate());
        stmt.bind(3, Op.ThrustPercentageTakeoff);
        stmt.bind(4, Op.ThrustPercentageClimb);
        stmt.step();
    }

    void OperationUpdater::visitTrack4dArrival(const Track4dArrival& Op) { m_Db.update(Schema::operations_tracks_4d, allValues(Op), { 0, 1 }, primaryKey(Op)); }

    void OperationUpdater::visitTrack4dDeparture(const Track4dDeparture& Op) { m_Db.update(Schema::operations_tracks_4d, allValues(Op), { 0, 1 }, primaryKey(Op)); }

    void OperationsManager::loadFromFile() {
        // Flights
        {
            Statement stmtFl(m_Db, Schema::operations_flights.querySelect());
            stmtFl.step();
            while (stmtFl.hasRow())
            {
                const std::string name = stmtFl.getColumn(0);
                const std::string opName = stmtFl.getColumn(1);
                const OperationType op = OperationTypes.fromString(opName);
                TimePoint time = std::chrono::round<Duration>(std::chrono::tai_clock::now());
                const auto timeOpt = utcStringToTime(stmtFl.getColumn(5));
                if (timeOpt)
                    time = timeOpt.value();
                else
                    Log::database()->warn("Loading flight '{}'. Invalid time.", name);

                double count = stmtFl.getColumn(6);
                const Aircraft& acft = m_Aircrafts(stmtFl.getColumn(7));
                double weight = stmtFl.getColumn(8);
                switch (op)
                {
                case OperationType::Arrival:
                    {

                        auto [flightArr, added] = m_FlightArrivals.add(name, name, acft);
                        if (!stmtFl.isColumnNull(2) && !stmtFl.isColumnNull(3) && !stmtFl.isColumnNull(4))
                        {
                            const std::string aptName = stmtFl.getColumn(2);
                            const std::string rwyName = stmtFl.getColumn(3);
                            const std::string rteName = stmtFl.getColumn(4);
                            flightArr.setRoute(m_Airports(aptName).Runways(rwyName).ArrivalRoutes(rteName).get());
                        }
                        flightArr.Time = time;
                        flightArr.Count = count;
                        flightArr.Weight = weight;

                        Statement stmtArr(m_Db, Schema::operations_flights_arrival.querySelect({ 2 }, { 0, 1 }));
                        stmtArr.bindValues(primaryKey(flightArr));
                        stmtArr.step();
                        if (stmtArr.hasRow() && !stmtArr.isColumnNull(0))
                        {
                            const std::string doc29Prof = stmtArr.getColumn(0);
                            if (flightArr.aircraft().validDoc29Performance())
                            {
                                const auto& doc29Acft = *flightArr.aircraft().Doc29Acft;
                                if (doc29Acft.ArrivalProfiles.contains(doc29Prof))
                                    flightArr.Doc29Prof = &*doc29Acft.ArrivalProfiles(doc29Prof);
                                else
                                    Log::database()->warn("Loading arrival flight '{}'. Doc29 Performance ID '{}' selected for fleet ID '{}' does not contain arrival profile '{}'.", flightArr.Name, doc29Acft.Name, flightArr.aircraft().Name, doc29Prof);
                            }
                            else
                            {
                                Log::database()->warn("Loading arrival flight '{}'. Fleet ID '{}' has no valid Doc29 Performance ID but a Doc29 Profile was provided ({}). It will be ignored.", flightArr.Name, flightArr.aircraft().Name, doc29Prof);
                            }
                        }
                        m_Blocks.operationBlock(flightArr);
                        break;
                    }
                case OperationType::Departure:
                    {
                        auto [flightDep, added] = m_FlightDepartures.add(name, name, acft);
                        if (!stmtFl.isColumnNull(2) && !stmtFl.isColumnNull(3) && !stmtFl.isColumnNull(4))
                        {
                            const std::string aptName = stmtFl.getColumn(2);
                            const std::string rwyName = stmtFl.getColumn(3);
                            const std::string rteName = stmtFl.getColumn(4);
                            flightDep.setRoute(m_Airports(aptName).Runways(rwyName).DepartureRoutes(rteName).get());
                        }
                        flightDep.Time = time;
                        flightDep.Count = count;
                        flightDep.Weight = weight;

                        Statement stmtDep(m_Db, Schema::operations_flights_departure.querySelect({ 2, 3, 4 }, { 0, 1 }));
                        stmtDep.bindValues(primaryKey(flightDep));
                        stmtDep.step();
                        if (stmtDep.hasRow())
                        {
                            if (!stmtDep.isColumnNull(0))
                            {
                                const std::string doc29Prof = stmtDep.getColumn(0);
                                if (flightDep.aircraft().validDoc29Performance())
                                {
                                    const auto& doc29Acft = *flightDep.aircraft().Doc29Acft;
                                    if (doc29Acft.DepartureProfiles.contains(doc29Prof))
                                        flightDep.Doc29Prof = &*doc29Acft.DepartureProfiles(doc29Prof);
                                    else
                                        Log::database()->warn("Loading departure flight '{}'. Doc29 Performance ID '{}' selected for fleet ID '{}' does not contain departure profile '{}'.", flightDep.Name, doc29Acft.Name, flightDep.aircraft().Name, doc29Prof);
                                }
                                else
                                {
                                    Log::database()->warn("Loading departure flight '{}'. Fleet ID '{}' has no valid Doc29 Performance ID but a Doc29 Profile was provided ({}). It will be ignored.", flightDep.Name, flightDep.aircraft().Name, doc29Prof);
                                }
                            }
                            flightDep.ThrustPercentageTakeoff = stmtDep.getColumn(1);
                            flightDep.ThrustPercentageClimb = stmtDep.getColumn(2);
                        }
                        m_Blocks.operationBlock(flightDep);
                        break;
                    }
                default: GRAPE_ASSERT(false);
                    break;
                }
                stmtFl.step();
            }
        }

        // Tracks 4D
        {
            Statement stmt(m_Db, Schema::operations_tracks_4d.querySelect());
            stmt.step();
            while (stmt.hasRow())
            {
                const std::string name = stmt.getColumn(0);
                const OperationType op = OperationTypes.fromString(stmt.getColumn(1));

                TimePoint time = now();
                const auto timeOpt = utcStringToTime(stmt.getColumn(2).getString());
                if (timeOpt)
                    time = timeOpt.value();
                else
                    Log::database()->warn("Loading Track 4D '{}'. Invalid time.", name);

                double count = stmt.getColumn(3);
                const Aircraft& acft = m_Aircrafts(stmt.getColumn(4));

                switch (op)
                {
                case OperationType::Arrival:
                    {
                        auto [track4dArr, added] = m_Track4dArrivals.add(name, name, acft, time, count);
                        m_Blocks.operationBlock(track4dArr);
                        break;
                    }
                case OperationType::Departure:
                    {
                        auto [track4dDep, added] = m_Track4dDepartures.add(name, name, acft, time, count);
                        m_Blocks.operationBlock(track4dDep);
                        break;
                    }
                default: GRAPE_ASSERT(false);
                    break;
                }
                stmt.step();
            }
        }
    }

    void OperationsManager::loadArr(const Track4dArrival& Op) { load(m_Track4dArrivals(Op.Name)); }

    void OperationsManager::loadDep(const Track4dDeparture& Op) { load(m_Track4dDepartures(Op.Name)); }

    void OperationsManager::unloadArr(const Track4dArrival& Op, bool Shrink) { m_Track4dArrivals(Op.Name).clear(Shrink); }

    void OperationsManager::unloadDep(const Track4dDeparture& Op, bool Shrink) { m_Track4dDepartures(Op.Name).clear(Shrink); }

    void OperationsManager::load(Track4d& Op) {
        std::scoped_lock lck(Tracks4dLoader.Mutex);

        Tracks4dLoader.Db.beginTransaction();
        Statement stmt(Tracks4dLoader.Db, Schema::operations_tracks_4d_points.querySelect({}, { 0, 1 }, { 2 }));
        stmt.bindValues(primaryKey(Op));
        stmt.step();
        while (stmt.hasRow())
        {
            std::size_t col = 3;

            TimePoint time = now();
            const auto timeOpt = utcStringToTime(stmt.getColumn(col++).getString());
            if (timeOpt)
                time = timeOpt.value();
            else
                Log::database()->warn("Loading point {} of Track 4D '{}'. Invalid time.", stmt.getColumn(2).getInt(), Op.Name);

            const FlightPhase flPhase = FlightPhases.fromString(stmt.getColumn(col++));
            const double cumGroundDist = stmt.getColumn(col++);
            const double lon = stmt.getColumn(col++);
            const double lat = stmt.getColumn(col++);
            const double altMsl = stmt.getColumn(col++);
            const double trueAirspeed = stmt.getColumn(col++);
            const double groundspeed = stmt.getColumn(col++);
            const double corrNetThrustPerEng = stmt.getColumn(col++);
            const double bankAngle = stmt.getColumn(col++);
            const double fuelFlowPerEng = stmt.getColumn(col++);
            Op.addPoint(time, flPhase, cumGroundDist, lon, lat, altMsl, trueAirspeed, groundspeed, corrNetThrustPerEng, bankAngle, fuelFlowPerEng);

            stmt.step();
        }
        Tracks4dLoader.Db.commitTransaction();
    }
}
