// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "Route.h"

#include "Runway.h"

namespace GRAPE {
    Route::Route(const Runway& RunwayIn, std::string_view Name) : Name(Name), m_Runway(RunwayIn) {}

    const Airport& Route::parentAirport() const {
        return m_Runway.get().parentAirport();
    }

    const Runway& Route::parentRunway() const {
        return m_Runway;
    }

    // Route Type Simple 
    void RouteTypeSimple::addPoint() noexcept {
        if (empty())
        {
            m_Points.emplace_back(0.0, 0.0);
            return;
        }

        m_Points.push_back(m_Points.back());
    }

    void RouteTypeSimple::addPoint(double Longitude, double Latitude) noexcept {
        GRAPE_ASSERT(Longitude >= -180.0 && Longitude <= 180.0);
        GRAPE_ASSERT(Latitude >= -90.0 && Latitude <= 90.0);

        m_Points.emplace_back(Longitude, Latitude);
    }

    void RouteTypeSimple::insertPoint(std::size_t Index) noexcept {
        GRAPE_ASSERT(Index <= size());

        const auto it = begin() + Index;
        if (it == end())
            m_Points.insert(it, m_Points.back());
        else
            m_Points.insert(begin() + Index, *it);
    }

    void RouteTypeSimple::deletePoint(std::size_t Index) noexcept {
        GRAPE_ASSERT(Index < size());
        m_Points.erase(begin() + Index);
    }

    void RouteTypeSimple::deletePoint() noexcept {
        GRAPE_ASSERT(!empty());
        m_Points.pop_back();
    }

    void RouteTypeSimple::clear() noexcept {
        m_Points.clear();
    }

    void RouteTypeSimple::addPointE(double Longitude, double Latitude) {
        if (!(Longitude >= -180.0 && Longitude <= 180.0))
            throw GrapeException("Longitude must be between -180.0 and 180.0.");

        if (!(Latitude >= -90.0 && Latitude <= 90.0))
            throw GrapeException("Latitude must be between -90.0 and 90.0.");

        m_Points.emplace_back(Longitude, Latitude);
    }

    void RouteTypeSimple::accept(RouteTypeVisitor& Vis) {
        Vis.visitSimple(*this);
    }

    void RouteTypeSimple::accept(RouteTypeVisitor& Vis) const {
        Vis.visitSimple(*this);
    }

    template<>
    void RouteSimple<OperationType::Arrival>::accept(RouteVisitor& Vis) {
        Vis.visitArrivalSimple(*this);

    }

    template<>
    void RouteSimple<OperationType::Departure>::accept(RouteVisitor& Vis) {
        Vis.visitDepartureSimple(*this);

    }

    template<>
    void RouteSimple<OperationType::Arrival>::accept(RouteVisitor& Vis) const {
        Vis.visitArrivalSimple(*this);

    }

    template<>
    void RouteSimple<OperationType::Departure>::accept(RouteVisitor& Vis) const {
        Vis.visitDepartureSimple(*this);

    }

    // Route Vectors
    void RouteTypeVectors::addVector() noexcept {
        return addStraight();
    }

    void RouteTypeVectors::addStraight() noexcept {
        if (empty())
        {
            m_Vectors.emplace_back(Straight());
            return;
        }

        std::visit(Overload{
            [&](const Straight& Vec) { m_Vectors.emplace_back(Vec); },
            [&](const Turn&) { m_Vectors.emplace_back(Straight()); },
            [&](const auto&) { GRAPE_ASSERT(false); },
            }, m_Vectors.back());
    }

    void RouteTypeVectors::addTurn() noexcept {
        if (empty())
        {
            m_Vectors.emplace_back(Turn());
            return;
        }

        std::visit(Overload{
            [&](const Straight&) { m_Vectors.emplace_back(Turn()); },
            [&](const Turn& Vec) { m_Vectors.emplace_back(Vec); },
            [&](const auto&) { GRAPE_ASSERT(false); },
            }, m_Vectors.back());
    }

    void RouteTypeVectors::addStraight(double GroundDistance) noexcept {
        GRAPE_ASSERT(GroundDistance > 0.0);

        m_Vectors.emplace_back(Straight(GroundDistance));
    }

    void RouteTypeVectors::addTurn(double TurnRadius, double HeadingChange, Turn::Direction TurnDir) noexcept {
        GRAPE_ASSERT(TurnRadius > 0.0);
        GRAPE_ASSERT(HeadingChange >= 0.0);

        m_Vectors.emplace_back(Turn(TurnRadius, HeadingChange, TurnDir));
    }

    void RouteTypeVectors::insertVector(std::size_t Index) noexcept {
        return insertStraight(Index);
    }

    void RouteTypeVectors::insertStraight(std::size_t Index) noexcept {
        GRAPE_ASSERT(Index <= size());

        const auto it = begin() + Index;
        const Vector& vec = it == end() ? m_Vectors.back() : *it;
        std::visit(Overload{
            [&](const Straight& Vec) { m_Vectors.insert(it, Vec); },
            [&](const Turn&) { m_Vectors.insert(it, Straight()); },
            [](const auto&) { GRAPE_ASSERT(false); },
            }, vec);
    }

    void RouteTypeVectors::insertTurn(std::size_t Index) noexcept {
        GRAPE_ASSERT(Index <= size());

        const auto it = begin() + Index;
        const Vector& vec = it == end() ? m_Vectors.back() : *it;
        std::visit(Overload{
            [&](const Straight&) { m_Vectors.insert(it, Turn()); },
            [&](const Turn& Vec) { m_Vectors.insert(it, Vec); },
            [](const auto&) { GRAPE_ASSERT(false); },
            }, vec);
    }

    bool RouteTypeVectors::setStraight(std::size_t Index) noexcept {
        GRAPE_ASSERT(Index < size());

        if (std::holds_alternative<Straight>(m_Vectors.at(Index)))
            return false;

        m_Vectors.at(Index).emplace<Straight>();
        return true;
    }

    bool RouteTypeVectors::setTurn(std::size_t Index) noexcept {
        GRAPE_ASSERT(Index < size());

        if (std::holds_alternative<Turn>(m_Vectors.at(Index)))
            return false;

        m_Vectors.at(Index).emplace<Turn>();
        return true;
    }

    void RouteTypeVectors::deleteVector(std::size_t Index) noexcept {
        GRAPE_ASSERT(Index < size());
        m_Vectors.erase(begin() + Index);
    }

    void RouteTypeVectors::deleteVector() noexcept {
        GRAPE_ASSERT(!empty());
        m_Vectors.pop_back();
    }

    void RouteTypeVectors::clear() noexcept {
        m_Vectors.clear();
    }

    void RouteTypeVectors::addStraightE(double GroundDistance) {
        if (!(GroundDistance > 0.0))
            throw GrapeException("Ground distance must be higher than 0 m.");

        m_Vectors.emplace_back(Straight(GroundDistance));
    }
    void RouteTypeVectors::addTurnE(double TurnRadius, double HeadingChange, Turn::Direction TurnDir) {
        if (!(TurnRadius > 0.0))
            throw GrapeException("Turn radius must be higher than 0 m.");

        if (!(HeadingChange >= 0.0))
            throw GrapeException("Heading change must be at least 0.");

        m_Vectors.emplace_back(Turn(TurnRadius, HeadingChange, TurnDir));
    }

    void RouteTypeVectors::accept(RouteTypeVisitor& Vis) {
        Vis.visitVectors(*this);
    }

    void RouteTypeVectors::accept(RouteTypeVisitor& Vis) const {
        Vis.visitVectors(*this);
    }

    template<>
    void RouteVectors<OperationType::Arrival>::accept(RouteVisitor& Vis) {
        Vis.visitArrivalVectors(*this);

    }

    template<>
    void RouteVectors<OperationType::Departure>::accept(RouteVisitor& Vis) {
        Vis.visitDepartureVectors(*this);

    }

    template<>
    void RouteVectors<OperationType::Arrival>::accept(RouteVisitor& Vis) const {
        Vis.visitArrivalVectors(*this);

    }

    template<>
    void RouteVectors<OperationType::Departure>::accept(RouteVisitor& Vis) const {
        Vis.visitDepartureVectors(*this);

    }

    // Route RNP
    void RouteTypeRnp::addStep() noexcept {
        addTrackToFix();
    }

    void RouteTypeRnp::addTrackToFix() noexcept {
        if (empty())
        {
            m_RnpSteps.emplace_back(TrackToFix());
            return;
        }

        std::visit(Overload{
            [&](const TrackToFix& Step) { m_RnpSteps.emplace_back(Step); },
            [&](const RadiusToFix& Step) { m_RnpSteps.emplace_back(TrackToFix(Step.Longitude, Step.Latitude)); },
            [&](const auto&) { GRAPE_ASSERT(false); },
            }, m_RnpSteps.back());
    }

    void RouteTypeRnp::addRadiusToFix() noexcept {
        GRAPE_ASSERT(!empty());

        // Visit last step
        std::visit(Overload{
            [&](const TrackToFix& Step) { m_RnpSteps.emplace_back(RadiusToFix(Step.Longitude, Step.Latitude, Step.Longitude, Step.Latitude)); },
            [&](const RadiusToFix& Step) { m_RnpSteps.emplace_back(Step); },
            [&](const auto&) { GRAPE_ASSERT(false); },
            }, m_RnpSteps.back());
    }

    void RouteTypeRnp::addTrackToFix(double Longitude, double Latitude) noexcept {
        GRAPE_ASSERT(Longitude >= -180.0 && Longitude <= 180.0);
        GRAPE_ASSERT(Latitude >= -90.0 && Latitude <= 90.0);

        m_RnpSteps.emplace_back(TrackToFix(Longitude, Latitude));
    }

    void RouteTypeRnp::addRadiusToFix(double Longitude, double Latitude, double CenterLongitude, double CenterLatitude) noexcept {
        GRAPE_ASSERT(!empty());
        GRAPE_ASSERT(Longitude >= -180.0 && Longitude <= 180.0);
        GRAPE_ASSERT(Latitude >= -90.0 && Latitude <= 90.0);
        GRAPE_ASSERT(CenterLongitude >= -180.0 && CenterLongitude <= 180.0);
        GRAPE_ASSERT(CenterLatitude >= -90.0 && CenterLatitude <= 90.0);

        m_RnpSteps.emplace_back(RadiusToFix(Longitude, Latitude, CenterLongitude, CenterLatitude));
    }

    void RouteTypeRnp::insertStep(std::size_t Index) noexcept {
        return insertTrackToFix(Index);
    }

    void RouteTypeRnp::insertTrackToFix(std::size_t Index) noexcept {
        GRAPE_ASSERT(Index <= size());

        const auto it = begin() + Index;
        const RnpStep& step = it == end() ? m_RnpSteps.back() : *it;
        return std::visit(Overload{
            [&](const TrackToFix& Step) { m_RnpSteps.insert(it, Step); },
            [&](const RadiusToFix& Step) { m_RnpSteps.insert(it, TrackToFix(Step.Longitude, Step.Latitude)); },
            [](const auto&) { GRAPE_ASSERT(false); },
            }, step);
    }

    void RouteTypeRnp::insertRadiusToFix(std::size_t Index) noexcept {
        GRAPE_ASSERT(Index <= size() && Index != 0);

        const auto it = begin() + Index;
        const RnpStep& step = it == end() ? m_RnpSteps.back() : *it;
        std::visit(Overload{
            [&](const TrackToFix& Step) { m_RnpSteps.insert(it, RadiusToFix(Step.Longitude, Step.Latitude, Step.Longitude, Step.Latitude)); },
            [&](const RadiusToFix& Step) { m_RnpSteps.insert(it, Step); },
            [](const auto&) { GRAPE_ASSERT(false); },
            }, step);
    }

    bool RouteTypeRnp::setTrackToFix(std::size_t Index) noexcept {
        GRAPE_ASSERT(Index < size());

        auto& step = m_RnpSteps.at(Index);
        return std::visit(Overload{
        [&](const TrackToFix&) { return false;  },
        [&](const RadiusToFix& Step) { step.emplace<TrackToFix>(Step.Longitude, Step.Latitude); return true; },
        [&](const auto&) { GRAPE_ASSERT(false); return false;  },
            }, step);
    }

    bool RouteTypeRnp::setRadiusToFix(std::size_t Index) noexcept {
        GRAPE_ASSERT(Index < size() && Index != 0);

        auto& step = m_RnpSteps.at(Index);
        return std::visit(Overload{
        [&](const TrackToFix& Step) { step.emplace<RadiusToFix>(Step.Longitude, Step.Latitude, Step.Longitude, Step.Latitude); return true;  },
        [&](const RadiusToFix&) { return false; },
        [&](const auto&) { GRAPE_ASSERT(false); return false;  },
            }, step);
    }

    void RouteTypeRnp::deleteStep(std::size_t Index) noexcept {
        GRAPE_ASSERT(Index < size());
        if (Index == 0 && size() > 1)
            setTrackToFix(1);
        m_RnpSteps.erase(m_RnpSteps.begin() + Index);
    }

    void RouteTypeRnp::deleteStep() noexcept {
        GRAPE_ASSERT(!empty());
        deleteStep(size() - 1);
    }

    void RouteTypeRnp::clear() noexcept {
        m_RnpSteps.clear();
    }

    void RouteTypeRnp::addTrackToFixE(double Longitude, double Latitude) {
        if (!(Longitude >= -180.0 && Longitude <= 180.0))
            throw GrapeException("Longitude must be between -180.0 and 180.0.");

        if (!(Latitude >= -90.0 && Latitude <= 90.0))
            throw GrapeException("Latitude must be between -90.0 and 90.0.");

        m_RnpSteps.emplace_back(TrackToFix(Longitude, Latitude));
    }

    void RouteTypeRnp::addRadiusToFixE(double Longitude, double Latitude, double CenterLongitude, double CenterLatitude) {
        if (empty())
            throw GrapeException("The first RNP step can't be a radius to fix step.");

        if (!(Longitude >= -180.0 && Longitude <= 180.0))
            throw GrapeException("Longitude must be between -180.0 and 180.0.");

        if (!(Latitude >= -90.0 && Latitude <= 90.0))
            throw GrapeException("Latitude must be between -90.0 and 90.0.");

        if (!(CenterLongitude >= -180.0 && CenterLongitude <= 180.0))
            throw GrapeException("Center longitude must be between -180.0 and 180.0.");

        if (!(CenterLatitude >= -90.0 && CenterLatitude <= 90.0))
            throw GrapeException("Center latitude must be between -90.0 and 90.0.");

        m_RnpSteps.emplace_back(RadiusToFix(Longitude, Latitude, CenterLongitude, CenterLatitude));
    }

    void RouteTypeRnp::accept(RouteTypeVisitor& Vis) {
        Vis.visitRnp(*this);
    }

    void RouteTypeRnp::accept(RouteTypeVisitor& Vis) const {
        Vis.visitRnp(*this);
    }

    template<>
    void RouteRnp<OperationType::Arrival>::accept(RouteVisitor& Vis) {
        Vis.visitArrivalRnp(*this);

    }

    template<>
    void RouteRnp<OperationType::Departure>::accept(RouteVisitor& Vis) {
        Vis.visitDepartureRnp(*this);

    }

    template<>
    void RouteRnp<OperationType::Arrival>::accept(RouteVisitor& Vis) const {
        Vis.visitArrivalRnp(*this);

    }

    template<>
    void RouteRnp<OperationType::Departure>::accept(RouteVisitor& Vis) const {
        Vis.visitDepartureRnp(*this);

    }
}
