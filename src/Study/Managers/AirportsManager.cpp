// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "AirportsManager.h"

#include "Schema/Schema.h"

namespace GRAPE {
    namespace {
        auto allValues(const Route& Rte) { return std::make_tuple(Rte.parentAirport().Name, Rte.parentRunway().Name, OperationTypes.toString(Rte.operationType()), Rte.Name, Route::Types.toString(Rte.type())); }

        auto primaryKey(const Route& Rte) { return std::make_tuple(Rte.parentAirport().Name, Rte.parentRunway().Name, OperationTypes.toString(Rte.operationType()), Rte.Name); }

        struct RouteInserter : RouteTypeVisitor {
            RouteInserter(const Database& Db, const Route& Rte) : m_Db(Db) { Rte.accept(*this); }
            void visitSimple(const RouteTypeSimple& Rte) override;
            void visitVectors(const RouteTypeVectors& Rte) override;
            void visitRnp(const RouteTypeRnp& Rte) override;

        private:
            const Database& m_Db;
        };

        struct RouteLoader : RouteTypeVisitor {
            explicit RouteLoader(const Database& Db, Route& Rte) : m_Db(Db) { Rte.accept(*this); }
            void visitSimple(RouteTypeSimple& Rte) override;
            void visitVectors(RouteTypeVectors& Rte) override;
            void visitRnp(RouteTypeRnp& Rte) override;

        private:
            const Database& m_Db;
        };
    }

    AirportsManager::AirportsManager(const Database& Db, Constraints& Blocks) : Manager(Db, Blocks) {}

    std::pair<Airport&, bool> AirportsManager::addAirport(const std::string& Name) {
        const std::string newName = Name.empty() ? uniqueKeyGenerator(m_Airports, "New Airport") : Name;

        auto ret = m_Airports.add(newName, newName);
        auto& [apt, added] = ret;

        if (added)
            m_Db.insert(Schema::airports, { 0 }, std::make_tuple(apt.Name));
        else
            Log::dataLogic()->error("Adding airport '{}'. Airport already exists in this study.", newName);

        return ret;
    }

    std::pair<Runway&, bool> AirportsManager::addRunway(Airport& Apt, const std::string& Name) const {
        const std::string newName = Name.empty() ? uniqueKeyGenerator(Apt.Runways, "New Runway") : Name;

        auto ret = Apt.Runways.add(newName, Apt, newName);
        auto& [rwy, added] = ret;

        if (added)
            m_Db.insert(Schema::airports_runways, { 0, 1, 2, 3, 4 }, std::make_tuple(rwy.parentAirport().Name, rwy.Name, rwy.Longitude, rwy.Latitude, rwy.Elevation));
        else
            Log::dataLogic()->error("Adding runway '{}'. Runway already exists in airport '{}'.", newName, Apt.Name);

        return ret;
    }

    std::pair<Route&, bool> AirportsManager::addRouteArrival(Runway& Rwy, Route::Type RteType, const std::string& Name) const {
        const std::string newName = Name.empty() ? uniqueKeyGenerator(Rwy.ArrivalRoutes, "New Arrival Route") : Name;

        auto ret = Rwy.addArrival(newName, RteType);
        auto& [rte, added] = ret;

        if (added)
            m_Db.insert(Schema::airports_routes, {}, allValues(rte));
        else
            Log::dataLogic()->error("Adding arrival route '{}'. Arrival route already exists in runway '{}' of airport '{}'.", newName, Rwy.Name, Rwy.parentAirport().Name);

        return ret;
    }

    std::pair<Route&, bool> AirportsManager::addRouteDeparture(Runway& Rwy, Route::Type RteType, const std::string& Name) const {
        const std::string newName = Name.empty() ? uniqueKeyGenerator(Rwy.DepartureRoutes, "New Departure Route") : Name;

        auto ret = Rwy.addDeparture(newName, RteType);
        auto& [rte, added] = ret;

        if (added)
            m_Db.insert(Schema::airports_routes, {}, allValues(rte));
        else
            Log::dataLogic()->error("Adding departure route '{}'. Departure route already exists in runway '{}' of airport '{}'.", newName, Rwy.Name, Rwy.parentAirport().Name);

        return ret;
    }

    Airport& AirportsManager::addAirportE(const std::string& Name) {
        if (Name.empty())
            throw GrapeException("Empty airport name not allowed.");

        auto [apt, added] = m_Airports.add(Name, Name);

        if (added)
            m_Db.insert(Schema::airports, { 0 }, std::make_tuple(apt.Name));
        else
            throw GrapeException(std::format("Airport '{}' already exists in this study.", Name));

        return apt;
    }

    Runway& AirportsManager::addRunwayE(Airport& Apt, const std::string& Name) const {
        if (Name.empty())
            throw GrapeException("Empty runway name not allowed.");

        auto [rwy, added] = Apt.Runways.add(Name, Apt, Name);

        if (added)
            m_Db.insert(Schema::airports_runways, { 0, 1, 2, 3, 4 }, std::make_tuple(rwy.parentAirport().Name, rwy.Name, rwy.Longitude, rwy.Latitude, rwy.Elevation));
        else
            throw GrapeException(std::format("Runway '{}' already exists in airport '{}'.", Name, Apt.Name));

        return rwy;
    }

    Route& AirportsManager::addRouteArrivalE(Runway& Rwy, Route::Type RteType, const std::string& Name) const {
        if (Name.empty())
            throw GrapeException("Empty arrival route name not allowed.");

        auto [rte, added] = Rwy.addArrival(Name, RteType);

        if (added)
            m_Db.insert(Schema::airports_routes, {}, allValues(rte));
        else
            throw GrapeException(std::format("Arrival route '{}' already exists in runway '{}' of airport '{}'", Name, Rwy.Name, Rwy.parentAirport().Name));

        return rte;
    }

    Route& AirportsManager::addRouteDepartureE(Runway& Rwy, Route::Type RteType, const std::string& Name) const {
        if (Name.empty())
            throw GrapeException("Empty departure route name not allowed.");

        auto [rte, added] = Rwy.addDeparture(Name, RteType);

        if (added)
            m_Db.insert(Schema::airports_routes, {}, allValues(rte));
        else
            throw GrapeException(std::format("Departure route '{}' already exists in runway '{}' of airport '{}'", Name, Rwy.Name, Rwy.parentAirport().Name));

        return rte;
    }

    void AirportsManager::eraseAirports() {
        m_Airports.eraseIf([&](const auto& Node) {
            const auto& [aptName, apt] = Node;
            if (m_Blocks.notRemovable(apt))
            {
                Log::dataLogic()->error("Removing airport '{}'. There are {} flights which use a route from this airport.", aptName, m_Blocks.blocking(apt).size());
                return false;
            }

            m_Db.deleteD(Schema::airports, { 0 }, std::make_tuple(aptName));
            return true;
            });
    }

    void AirportsManager::erase(const Airport& Apt) {
        if (m_Blocks.notRemovable(Apt))
        {
            Log::dataLogic()->error("Removing airport '{}'. There are {} flights which use a route from this airport.", Apt.Name, m_Blocks.blocking(Apt).size());
            return;
        }

        m_Db.deleteD(Schema::airports, { 0 }, std::make_tuple(Apt.Name));

        m_Airports.erase(Apt.Name);
    }

    void AirportsManager::erase(const Runway& Rwy) {
        if (m_Blocks.notRemovable(Rwy))
        {
            Log::dataLogic()->error("Removing runway '{}'. There are {} flights which use a route from this runway.", Rwy.Name, m_Blocks.blocking(Rwy).size());
            return;
        }

        m_Db.deleteD(Schema::airports_runways, { 0, 1 }, std::make_tuple(Rwy.parentAirport().Name, Rwy.Name));
        m_Airports(Rwy.parentAirport().Name).Runways.erase(Rwy.Name);
    }

    void AirportsManager::erase(const Route& Rte) {
        if (m_Blocks.notRemovable(Rte))
        {
            Log::dataLogic()->error("Removing route '{}'. There are {} flights which use this route.", Rte.Name, m_Blocks.blocking(Rte).size());
            return;
        }

        m_Db.deleteD(Schema::airports_routes, { 0, 1, 2, 3 }, primaryKey(Rte));

        switch (Rte.operationType())
        {
        case OperationType::Arrival: m_Airports(Rte.parentAirport().Name).Runways(Rte.parentRunway().Name).ArrivalRoutes.erase(Rte.Name);
            break;
        case OperationType::Departure: m_Airports(Rte.parentAirport().Name).Runways(Rte.parentRunway().Name).DepartureRoutes.erase(Rte.Name);
            break;
        default: GRAPE_ASSERT(false);
        }
    }

    bool AirportsManager::updateKey(Airport& Apt, const std::string Id) {
        if (Apt.Name.empty())
        {
            Log::dataLogic()->error("Updating airport '{}'. Empty name not allowed.", Id);
            Apt.Name = Id;
            return false;
        }

        const bool updated = m_Airports.update(Id, Apt.Name);

        if (updated) { m_Db.update(Schema::airports, { 0 }, std::make_tuple(Apt.Name), { 0 }, std::make_tuple(Id)); }
        else
        {
            Log::dataLogic()->error("Updating airport '{}'. Airport new name '{}' already exists in this study.", Id, Apt.Name);
            Apt.Name = Id;
        }

        return updated;
    }

    bool AirportsManager::updateKey(Runway& Rwy, const std::string Id) {
        if (Rwy.Name.empty())
        {
            Log::dataLogic()->error("Updating runway '{}'. Empty name not allowed.", Id);
            Rwy.Name = Id;
            return false;
        }

        const bool updated = m_Airports(Rwy.parentAirport().Name).Runways.update(Id, Rwy.Name);

        if (updated) { m_Db.update(Schema::airports_runways, { 1 }, std::make_tuple(Rwy.Name), { 0, 1 }, std::make_tuple(Rwy.parentAirport().Name, Id)); }
        else
        {
            Log::dataLogic()->error("Updating runway '{}'. Runway new name '{}' already exists in airport '{}'.", Id, Rwy.Name, Rwy.parentAirport().Name);
            Rwy.Name = Id;
        }

        return updated;
    }

    bool AirportsManager::updateKey(Route& Rte, const std::string Id) {
        if (Rte.Name.empty())
        {
            Log::dataLogic()->error("Updating route '{}'. Empty name not allowed.", Id);
            Rte.Name = Id;
            return false;
        }

        bool updated = false;

        switch (Rte.operationType())
        {
        case OperationType::Arrival: updated = m_Airports(Rte.parentAirport().Name).Runways(Rte.parentRunway().Name).ArrivalRoutes.update(Id, Rte.Name);
            break;
        case OperationType::Departure: updated = m_Airports(Rte.parentAirport().Name).Runways(Rte.parentRunway().Name).DepartureRoutes.update(Id, Rte.Name);
            break;
        default: GRAPE_ASSERT(false);
        }

        if (updated) { m_Db.update(Schema::airports_routes, { 3 }, std::make_tuple(Rte.Name), { 0, 1, 2, 3 }, std::make_tuple(Rte.parentAirport().Name, Rte.parentRunway().Name, OperationTypes.toString(Rte.operationType()), Id)); }
        else
        {
            Log::dataLogic()->error("Updating {} route '{}'. Route new name '{}' already exists in runway '{}' of airport '{}'.", OperationTypes.toString(Rte.operationType()), Id, Rte.Name, Rte.parentRunway().Name, Rte.parentAirport().Name);
            Rte.Name = Id;
        }

        return updated;
    }

    void AirportsManager::update(const Airport& Apt) const { m_Db.update(Schema::airports, std::make_tuple(Apt.Name, Apt.Longitude, Apt.Latitude, Apt.Elevation, Apt.ReferenceTemperature, Apt.ReferenceSeaLevelPressure), { 0 }, std::make_tuple(Apt.Name)); }

    void AirportsManager::update(const Runway& Rwy) const { m_Db.update(Schema::airports_runways, std::make_tuple(Rwy.parentAirport().Name, Rwy.Name, Rwy.Longitude, Rwy.Latitude, Rwy.Elevation, Rwy.Length, Rwy.Heading, Rwy.Gradient), { 0, 1 }, std::make_tuple(Rwy.parentAirport().Name, Rwy.Name)); }

    void AirportsManager::update(const Route& Rte) const {
        // Updating reinserts route
        m_Db.deleteD(Schema::airports_routes_simple, { 0, 1, 2, 3 }, primaryKey(Rte));
        m_Db.deleteD(Schema::airports_routes_vectors, { 0, 1, 2, 3 }, primaryKey(Rte));
        m_Db.deleteD(Schema::airports_routes_rnp, { 0, 1, 2, 3 }, primaryKey(Rte));
        m_Db.update(Schema::airports_routes, allValues(Rte), { 0, 1, 2, 3 }, primaryKey(Rte));

        RouteInserter rteInserter(m_Db, Rte);
    }

    void RouteInserter::visitSimple(const RouteTypeSimple& Rte) {
        Statement stmt(m_Db, Schema::airports_routes_simple.queryInsert());
        stmt.bindValues(primaryKey(Rte));
        for (auto it = Rte.begin(); it != Rte.end(); ++it)
        {
            const auto i = it - Rte.begin();
            stmt.bind(4, static_cast<int>(i + 1));
            stmt.bind(5, it->Longitude);
            stmt.bind(6, it->Latitude);
            stmt.step();
            stmt.reset();
        }
    }

    void RouteInserter::visitVectors(const RouteTypeVectors& Rte) {
        Statement stmt(m_Db, Schema::airports_routes_vectors.queryInsert());
        stmt.bindValues(primaryKey(Rte));
        for (auto it = Rte.begin(); it != Rte.end(); ++it)
        {
            const auto i = it - Rte.begin();
            stmt.bind(4, static_cast<int>(i + 1));
            stmt.bind(5, std::visit(RouteTypeVectors::VisitorVectorTypeString(), *it));
            std::visit(Overload{
                           [&](const RouteTypeVectors::Straight& Vec) {
                               stmt.bind(6, Vec.Distance);
                               stmt.bind(7, std::monostate());
                               stmt.bind(8, std::monostate());
                           },
                           [&](const RouteTypeVectors::Turn& Vec) {
                               stmt.bind(6, std::monostate());
                               stmt.bind(7, Vec.TurnRadius);
                               stmt.bind(8, Vec.HeadingChange);
                               stmt.bind(9, RouteTypeVectors::Turn::Directions.toString(Vec.TurnDirection));
                           },
                }, *it);
            stmt.step();
            stmt.reset();
        }
    }

    void RouteInserter::visitRnp(const RouteTypeRnp& Rte) {
        Statement stmt(m_Db, Schema::airports_routes_rnp.queryInsert());
        stmt.bindValues(primaryKey(Rte));
        for (auto it = Rte.begin(); it != Rte.end(); ++it)
        {
            const auto i = it - Rte.begin();
            stmt.bind(4, static_cast<int>(i + 1));
            stmt.bind(5, std::visit(RouteTypeRnp::VisitorRnpStepTypeString(), *it));
            std::visit(Overload{
                           [&](const RouteTypeRnp::TrackToFix& Step) {
                               stmt.bind(6, Step.Longitude);
                               stmt.bind(7, Step.Latitude);
                               stmt.bind(8, std::monostate());
                               stmt.bind(9, std::monostate());
                           },
                           [&](const RouteTypeRnp::RadiusToFix& Step) {
                               stmt.bind(6, Step.Longitude);
                               stmt.bind(7, Step.Latitude);
                               stmt.bind(8, Step.CenterLongitude);
                               stmt.bind(9, Step.CenterLatitude);
                           },
                }, *it);
            stmt.step();
            stmt.reset();
        }
    }

    void AirportsManager::loadFromFile() {
        Statement stmtApt(m_Db, Schema::airports.querySelect());
        stmtApt.step();
        while (stmtApt.hasRow())
        {
            const std::string aptName = stmtApt.getColumn(0);
            auto [apt, addedApt] = m_Airports.add(aptName, aptName);
            apt.Longitude = stmtApt.getColumn(1);
            apt.Latitude = stmtApt.getColumn(2);
            apt.Elevation = stmtApt.getColumn(3);
            if (!stmtApt.isColumnNull(4))
                apt.ReferenceTemperature = stmtApt.getColumn(4);
            if (!stmtApt.isColumnNull(5))
                apt.ReferenceSeaLevelPressure = stmtApt.getColumn(5);

            // Runways
            Statement stmtRwy(m_Db, Schema::airports_runways.querySelect({}, { 0 }));
            stmtRwy.bindValues(aptName);
            stmtRwy.step();
            while (stmtRwy.hasRow())
            {
                const std::string rwyName = stmtRwy.getColumn(1);
                auto [rwy, addedRwy] = apt.Runways.add(rwyName, apt, rwyName);
                rwy.Longitude = stmtRwy.getColumn(2);
                rwy.Latitude = stmtRwy.getColumn(3);
                rwy.Elevation = stmtRwy.getColumn(4);
                rwy.Length = stmtRwy.getColumn(5);
                rwy.Heading = stmtRwy.getColumn(6);
                rwy.Gradient = stmtRwy.getColumn(7);

                // Routes
                Statement stmtRte(m_Db, Schema::airports_routes.querySelect({ 2, 3, 4 }, { 0, 1 }));
                stmtRte.bindValues(aptName, rwyName);
                stmtRte.step();
                while (stmtRte.hasRow())
                {
                    const OperationType op = OperationTypes.fromString(stmtRte.getColumn(0));
                    const std::string rteName = stmtRte.getColumn(1);
                    const Route::Type rteType = Route::Types.fromString(stmtRte.getColumn(2));
                    switch (op)
                    {
                    case OperationType::Arrival:
                        {
                            auto [newRte, added] = rwy.addArrival(rteName, rteType);
                            RouteLoader rteLoader(m_Db, newRte);
                            break;
                        }
                    case OperationType::Departure:
                        {
                            auto [newRte, added] = rwy.addDeparture(rteName, rteType);
                            RouteLoader rteLoader(m_Db, newRte);
                            break;
                        }
                    }
                    stmtRte.step();
                }
                stmtRwy.step();
            }
            stmtApt.step();
        }
    }

    void RouteLoader::visitSimple(RouteTypeSimple& Rte) {
        Statement stmt(m_Db, Schema::airports_routes_simple.querySelect({ 5, 6 }, { 0, 1, 2, 3 }, { 4 }));
        stmt.bindValues(primaryKey(Rte));
        stmt.step();
        while (stmt.hasRow())
        {
            Rte.addPoint(stmt.getColumn(0), stmt.getColumn(1));
            stmt.step();
        }
    }

    void RouteLoader::visitVectors(RouteTypeVectors& Rte) {
        Statement stmt(m_Db, Schema::airports_routes_vectors.querySelect({ 5, 6, 7, 8, 9 }, { 0, 1, 2, 3 }, { 4 }));
        stmt.bindValues(primaryKey(Rte));
        stmt.step();
        while (stmt.hasRow())
        {
            switch (RouteTypeVectors::VectorTypes.fromString(stmt.getColumn(0)))
            {
            case RouteTypeVectors::VectorType::Straight: Rte.addStraight(stmt.getColumn(1));
                break;
            case RouteTypeVectors::VectorType::Turn: Rte.addTurn(stmt.getColumn(2), stmt.getColumn(3), RouteTypeVectors::Turn::Directions.fromString(stmt.getColumn(4)));
                break;
            default: GRAPE_ASSERT(false);
                break;
            }
            stmt.step();
        }
    }

    void RouteLoader::visitRnp(RouteTypeRnp& Rte) {
        Statement stmt(m_Db, Schema::airports_routes_rnp.querySelect({ 5, 6, 7, 8, 9 }, { 0, 1, 2, 3 }, { 4 }));
        stmt.bindValues(primaryKey(Rte));
        stmt.step();
        while (stmt.hasRow())
        {
            switch (RouteTypeRnp::StepTypes.fromString(stmt.getColumn(0)))
            {
            case RouteTypeRnp::StepType::TrackToFix: Rte.addTrackToFix(stmt.getColumn(1), stmt.getColumn(2));
                break;
            case RouteTypeRnp::StepType::RadiusToFix: Rte.addRadiusToFix(stmt.getColumn(1), stmt.getColumn(2), stmt.getColumn(3), stmt.getColumn(4));
                break;
            default: GRAPE_ASSERT(false);
                break;
            }
            stmt.step();
        }
    }
}
