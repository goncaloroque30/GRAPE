// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "Doc29PerformanceManager.h"

#include "Schema/Schema.h"

namespace GRAPE {
    namespace {
        struct ThrustCoefficientsInserter : Doc29ThrustVisitor {
            ThrustCoefficientsInserter(const Database& Db, const Doc29Performance& Doc29Acft) : m_Db(Db), m_Doc29Acft(Doc29Acft) { Doc29Acft.thrust().accept(*this); }
            void visitDoc29ThrustRating(Doc29ThrustRating& Thrust) override;
            void visitDoc29ThrustPropeller(Doc29ThrustRatingPropeller& Thrust) override;

        private:
            const Database& m_Db;
            const Doc29Performance& m_Doc29Acft;
        };

        struct ProfileInserter : Doc29ProfileVisitor {
            ProfileInserter(const Database& Db, const Doc29Profile& Profile) : m_Db(Db) { Profile.accept(*this); }
            void visitDoc29ProfileArrivalPoints(const Doc29ProfileArrivalPoints& Doc29Prof) override;
            void visitDoc29ProfileArrivalProcedural(const Doc29ProfileArrivalProcedural& Doc29Prof) override;
            void visitDoc29ProfileDeparturePoints(const Doc29ProfileDeparturePoints& Doc29Prof) override;
            void visitDoc29ProfileDepartureProcedural(const Doc29ProfileDepartureProcedural& Doc29Prof) override;

        private:
            const Database& m_Db;
        };

        struct ThrustCoefficientsLoader : Doc29ThrustVisitor {
            explicit ThrustCoefficientsLoader(const Database& Db, Doc29Performance& Doc29Acft) : m_Db(Db), m_Doc29Acft(Doc29Acft) { Doc29Acft.thrust().accept(*this); }
            void visitDoc29ThrustRating(Doc29ThrustRating& Thrust) override;
            void visitDoc29ThrustPropeller(Doc29ThrustRatingPropeller& Thrust) override;

        private:
            const Database& m_Db;
            Doc29Performance& m_Doc29Acft;
        };

        struct ProfileLoader : Doc29ProfileVisitor {
            ProfileLoader(const Database& Db, Doc29Profile& Doc29Prof) : m_Db(Db) { Doc29Prof.accept(*this); }
            void visitDoc29ProfileArrivalPoints(Doc29ProfileArrivalPoints& Doc29Prof) override;
            void visitDoc29ProfileArrivalProcedural(Doc29ProfileArrivalProcedural& Doc29Prof) override;
            void visitDoc29ProfileDeparturePoints(Doc29ProfileDeparturePoints& Doc29Prof) override;
            void visitDoc29ProfileDepartureProcedural(Doc29ProfileDepartureProcedural& Profile) override;

        private:
            const Database& m_Db;
        };
    }

    Doc29PerformanceManager::Doc29PerformanceManager(const Database& Db, Constraints& Blocks) : Manager(Db, Blocks) {}

    std::pair<Doc29Performance&, bool> Doc29PerformanceManager::addPerformance(Doc29Performance::Type AcftType, const std::string& Name) {
        const std::string newName = Name.empty() ? uniqueKeyGenerator(m_Doc29Performances, "New Doc29 Aircraft") : Name;

        std::unique_ptr<Doc29Performance> newAircraft;
        switch (AcftType)
        {
        case Doc29Performance::Type::Jet: newAircraft = std::make_unique<Doc29PerformanceJet>(newName);
            break;
        case Doc29Performance::Type::Turboprop: newAircraft = std::make_unique<Doc29PerformanceTurboprop>(newName);
            break;
        case Doc29Performance::Type::Piston: newAircraft = std::make_unique<Doc29PerformancePiston>(newName);
            break;
        default: GRAPE_ASSERT(false);
            break;
        }

        auto ret = m_Doc29Performances.add(newName, std::move(newAircraft));
        auto& [doc29Acft, added] = ret;

        if (added)
            m_Db.insert(Schema::doc29_performance, { 0, 1 }, std::make_tuple(doc29Acft->Name, Doc29Performance::Types.toString(doc29Acft->type())));
        else
            Log::dataLogic()->error("Adding Doc29 aircraft '{}'. Aircraft already exists in this study.", Name);

        return { *doc29Acft, added };
    }

    bool Doc29PerformanceManager::addProfileArrival(Doc29Performance& Acft, Doc29Profile::Type ProfileType, const std::string& Name) const {
        const std::string newName = Name.empty() ? uniqueKeyGenerator(Acft.ArrivalProfiles, "New Doc29 Arrival Profile") : Name;

        std::unique_ptr<Doc29ProfileArrival> newProfile;
        switch (ProfileType)
        {
        case Doc29ProfileArrival::Type::Points: newProfile = std::make_unique<Doc29ProfileArrivalPoints>(Acft, newName);
            break;
        case Doc29ProfileArrival::Type::Procedural:
            {
                if (!Acft.containsAerodynamicCoefficientsWithType(Doc29AerodynamicCoefficients::Type::Land))
                {
                    Log::dataLogic()->error("Adding arrival procedural profile '{}'. Doc29 Aircraft '{}' does not have aerodynamic coefficients for landing.", newName, Acft.Name);
                    return false;
                }

                newProfile = std::make_unique<Doc29ProfileArrivalProcedural>(Acft, newName);
                break;
            }
        default: GRAPE_ASSERT(false);
            break;
        }

        auto [doc29Prof, added] = Acft.ArrivalProfiles.add(newName, std::move(newProfile));
        if (added)
            m_Db.insert(Schema::doc29_performance_profiles, {}, std::make_tuple(doc29Prof->parentDoc29Performance().Name, OperationTypes.toString(doc29Prof->operationType()), doc29Prof->Name, Doc29Profile::Types.toString(doc29Prof->type())));
        else
            Log::dataLogic()->error("Adding arrival profile '{}'. Profile already exist in aircraft '{}'.", newName, Acft.Name);

        return added;
    }

    bool Doc29PerformanceManager::addProfileDeparture(Doc29Performance& Acft, Doc29Profile::Type ProfileType, const std::string& Name) const {
        const std::string newName = Name.empty() ? uniqueKeyGenerator(Acft.DepartureProfiles, "New Doc29 Departure Profile") : Name;

        std::unique_ptr<Doc29ProfileDeparture> newProfile;
        switch (ProfileType)
        {
        case Doc29ProfileDeparture::Type::Points: newProfile = std::make_unique<Doc29ProfileDeparturePoints>(Acft, newName);
            break;
        case Doc29ProfileDeparture::Type::Procedural:
            {
                if (!Acft.thrust().isRatingSet(Doc29Thrust::Rating::MaximumTakeoff))
                {
                    Log::dataLogic()->error("Adding departure procedural profile '{}'. Doc29 Aircraft '{}' does not have engine coefficients for thrust rating maximum takeoff.", newName, Acft.Name);
                    return false;
                }

                if (!Acft.thrust().isRatingSet(Doc29Thrust::Rating::MaximumClimb))
                {
                    Log::dataLogic()->error("Adding departure procedural profile '{}'. Doc29 Aircraft '{}' does not have engine coefficients for thrust rating maximum climb.", newName, Acft.Name);
                    return false;
                }

                if (!Acft.containsAerodynamicCoefficientsWithType(Doc29AerodynamicCoefficients::Type::Takeoff))
                {
                    Log::dataLogic()->error("Adding departure procedural profile '{}'. Doc29 Aircraft '{}' does not have aerodynamic coefficients for takeoff.", newName, Acft.Name);
                    return false;
                }

                newProfile = std::make_unique<Doc29ProfileDepartureProcedural>(Acft, newName);
                break;
            }
        default: GRAPE_ASSERT(false);
            break;
        }

        auto [doc29Prof, added] = Acft.DepartureProfiles.add(newName, std::move(newProfile));
        if (added)
            m_Db.insert(Schema::doc29_performance_profiles, {}, std::make_tuple(doc29Prof->parentDoc29Performance().Name, OperationTypes.toString(doc29Prof->operationType()), doc29Prof->Name, Doc29Profile::Types.toString(doc29Prof->type())));
        else
            Log::dataLogic()->error("Adding departure profile '{}'. Profile already exists in aircraft '{}'.", newName, Acft.Name);

        return added;
    }

    Doc29Performance& Doc29PerformanceManager::addPerformanceE(Doc29Performance::Type AcftType, const std::string& Name) {
        if (Name.empty())
            throw GrapeException("Empty name not allowed.");

        std::unique_ptr<Doc29Performance> newAircraft;
        switch (AcftType)
        {
        case Doc29Performance::Type::Jet: newAircraft = std::make_unique<Doc29PerformanceJet>(Name);
            break;
        case Doc29Performance::Type::Turboprop: newAircraft = std::make_unique<Doc29PerformanceTurboprop>(Name);
            break;
        case Doc29Performance::Type::Piston: newAircraft = std::make_unique<Doc29PerformancePiston>(Name);
            break;
        default: GRAPE_ASSERT(false);
            break;
        }

        auto [doc29Acft, added] = m_Doc29Performances.add(Name, std::move(newAircraft));

        if (added)
            m_Db.insert(Schema::doc29_performance, { 0, 1 }, std::make_tuple(doc29Acft->Name, Doc29Performance::Types.toString(doc29Acft->type())));
        else
            throw GrapeException(std::format("Aircraft '{}' already exists in this study.", Name));

        return *doc29Acft;
    }

    Doc29ProfileArrival& Doc29PerformanceManager::addProfileArrivalE(Doc29Performance& Perf, Doc29Profile::Type ProfileType, const std::string& Name) const {
        if (Name.empty())
            throw GrapeException("Empty Doc29 arrival profile name not allowed.");

        std::unique_ptr<Doc29ProfileArrival> newProfile;
        switch (ProfileType)
        {
        case Doc29ProfileArrival::Type::Points: newProfile = std::make_unique<Doc29ProfileArrivalPoints>(Perf, Name);
            break;
        case Doc29ProfileArrival::Type::Procedural:
            {
                if (!Perf.containsAerodynamicCoefficientsWithType(Doc29AerodynamicCoefficients::Type::Land))
                    throw GrapeException(std::format("Doc29 Aircraft '{}' does not have aerodynamic coefficients for landing.", Perf.Name));

                newProfile = std::make_unique<Doc29ProfileArrivalProcedural>(Perf, Name);
                break;
            }
        default: GRAPE_ASSERT(false);
            break;
        }

        auto [doc29Prof, added] = Perf.ArrivalProfiles.add(Name, std::move(newProfile));
        if (added)
            m_Db.insert(Schema::doc29_performance_profiles, {}, std::make_tuple(doc29Prof->parentDoc29Performance().Name, OperationTypes.toString(doc29Prof->operationType()), doc29Prof->Name, Doc29Profile::Types.toString(doc29Prof->type())));
        else
            throw GrapeException(std::format("Arrival profile '{}' already exist in aircraft '{}'.", Name, Perf.Name));

        return *doc29Prof;
    }

    Doc29ProfileDeparture& Doc29PerformanceManager::addProfileDepartureE(Doc29Performance& Perf, Doc29Profile::Type ProfileType, const std::string& Name) const {
        std::unique_ptr<Doc29ProfileDeparture> newProfile;
        switch (ProfileType)
        {
        case Doc29ProfileDeparture::Type::Points: newProfile = std::make_unique<Doc29ProfileDeparturePoints>(Perf, Name);
            break;
        case Doc29ProfileDeparture::Type::Procedural:
            {
                if (!Perf.thrust().isRatingSet(Doc29Thrust::Rating::MaximumTakeoff))
                    throw GrapeException(std::format("Doc29 Aircraft '{}' does not have engine coefficients for thrust rating maximum takeoff.", Perf.Name));

                if (!Perf.thrust().isRatingSet(Doc29Thrust::Rating::MaximumClimb))
                    throw GrapeException(std::format("Doc29 Aircraft '{}' does not have engine coefficients for thrust rating maximum climb.", Perf.Name));

                if (!Perf.containsAerodynamicCoefficientsWithType(Doc29AerodynamicCoefficients::Type::Takeoff))
                    throw GrapeException(std::format("Doc29 Aircraft '{}' does not have aerodynamic coefficients for takeoff.", Perf.Name));

                newProfile = std::make_unique<Doc29ProfileDepartureProcedural>(Perf, Name);
                break;
            }
        default: GRAPE_ASSERT(false);
            break;
        }

        auto [doc29Prof, added] = Perf.DepartureProfiles.add(Name, std::move(newProfile));
        if (added)
            m_Db.insert(Schema::doc29_performance_profiles, {}, std::make_tuple(doc29Prof->parentDoc29Performance().Name, OperationTypes.toString(doc29Prof->operationType()), doc29Prof->Name, Doc29Profile::Types.toString(doc29Prof->type())));
        else
            throw GrapeException(std::format("Departure profile '{}' already exists in aircraft '{}'.", Name, Perf.Name));

        return *doc29Prof;
    }

    void Doc29PerformanceManager::erasePerformances() {
        m_Doc29Performances.eraseIf([&](const auto& Node) {
            const auto& [name, acft] = Node;
            if (m_Blocks.notRemovable(*acft))
            {
                Log::dataLogic()->error("Removing Doc29 aircraft '{}'. There are {} aircrafts which use this Doc29 aircraft.", name, m_Blocks.blocking(*acft).size());
                return false;
            }

            m_Db.deleteD(Schema::doc29_performance, { 0 }, std::make_tuple(name));
            return true;
            });
    }

    void Doc29PerformanceManager::erasePerformance(const Doc29Performance& Doc29Perf) {
        if (m_Blocks.notRemovable(Doc29Perf))
        {
            Log::dataLogic()->error("Removing Doc29 aircraft '{}'. There are {} aircrafts which use this Doc29 aircraft.", Doc29Perf.Name, m_Blocks.blocking(Doc29Perf).size());
            return;
        }

        m_Db.deleteD(Schema::doc29_performance, { 0 }, std::make_tuple(Doc29Perf.Name));

        m_Doc29Performances.erase(Doc29Perf.Name);
    }

    void Doc29PerformanceManager::eraseProfileArrivals(Doc29Performance& Doc29Perf) {
        Doc29Perf.ArrivalProfiles.eraseIf([&](const auto& Node) {
            const auto& [doc29ProfId, doc29ProfPtr] = Node;
            const auto& doc29Prof = *doc29ProfPtr;
            if (m_Blocks.notRemovable(doc29Prof))
            {
                Log::dataLogic()->error("Removing Doc29 arrival profile '{}'. There are {} flights which use this Doc29 profile.", doc29ProfId, m_Blocks.blocking(doc29Prof).size());
                return false;
            }

            m_Db.deleteD(Schema::doc29_performance_profiles, { 0, 1, 2 }, std::make_tuple(Doc29Perf.Name, OperationTypes.toString(OperationType::Arrival), doc29ProfId));
            return true;
            });
    }

    void Doc29PerformanceManager::eraseProfileDepartures(Doc29Performance& Doc29Perf) {
        Doc29Perf.DepartureProfiles.eraseIf([&](const auto& Node) {
            const auto& [doc29ProfId, doc29ProfPtr] = Node;
            const auto& doc29Prof = *doc29ProfPtr;
            if (m_Blocks.notRemovable(doc29Prof))
            {
                Log::dataLogic()->error("Removing Doc29 departure profile '{}'. There are {} flights which use this Doc29 profile.", doc29ProfId, m_Blocks.blocking(doc29Prof).size());
                return false;
            }

            m_Db.deleteD(Schema::doc29_performance_profiles, { 0, 1, 2 }, std::make_tuple(Doc29Perf.Name, OperationTypes.toString(OperationType::Departure), doc29ProfId));
            return true;
            });
    }

    void Doc29PerformanceManager::eraseProfile(const Doc29Profile& Doc29Prof) {
        if (m_Blocks.notRemovable(Doc29Prof))
        {
            Log::dataLogic()->error("Removing Doc29 arrival profile '{}'. There are {} flights which use this Doc29 profile.", Doc29Prof.Name, m_Blocks.blocking(Doc29Prof).size());
            return;
        }

        m_Db.deleteD(Schema::doc29_performance_profiles, { 0, 1, 2 }, std::make_tuple(Doc29Prof.parentDoc29Performance().Name, OperationTypes.toString(Doc29Prof.operationType()), Doc29Prof.Name));

        switch (Doc29Prof.operationType())
        {
        case OperationType::Arrival: m_Doc29Performances(Doc29Prof.parentDoc29Performance().Name)->ArrivalProfiles.erase(Doc29Prof.Name);
            break;
        case OperationType::Departure: m_Doc29Performances(Doc29Prof.parentDoc29Performance().Name)->DepartureProfiles.erase(Doc29Prof.Name);
            break;
        default: GRAPE_ASSERT(false);
            break;
        }
    }

    bool Doc29PerformanceManager::updateKeyPerformance(Doc29Performance& Doc29Perf, const std::string Id) {
        if (Doc29Perf.Name.empty())
        {
            Log::dataLogic()->error("Updating Doc29 aircraft '{}'. Empty name not allowed.", Id);
            Doc29Perf.Name = Id;
            return false;
        }

        const bool updated = m_Doc29Performances.update(Id, Doc29Perf.Name);

        if (updated) { m_Db.update(Schema::doc29_performance, { 0 }, std::make_tuple(Doc29Perf.Name), { 0 }, std::make_tuple(Id)); }
        else
        {
            Log::dataLogic()->error("Updating Doc29 aircraft '{}'. Aircraft new name '{}' already exists in this study.", Id, Doc29Perf.Name);
            Doc29Perf.Name = Id;
        }

        return updated;
    }

    bool Doc29PerformanceManager::updateKeyAerodynamicCoefficients(Doc29Performance& Doc29Perf, const std::string Id) const {
        auto& coeffs = Doc29Perf.AerodynamicCoefficients(Id);
        if (coeffs.Name.empty())
        {
            Log::dataLogic()->error("Updating aerodynamic coefficients '{}' in Doc29 aircraft '{}'. Empty name not allowed.", Id, Doc29Perf.Name);
            coeffs.Name = Id;
            return false;
        }

        const bool updated = Doc29Perf.AerodynamicCoefficients.update(Id, coeffs.Name);

        if (updated) { m_Db.update(Schema::doc29_performance_aerodynamic_coefficients, { 1 }, std::make_tuple(coeffs.Name), { 0, 1 }, std::make_tuple(Doc29Perf.Name, Id)); }
        else
        {
            Log::dataLogic()->error("Updating aerodynamic coefficients '{}' in Doc29 aircraft '{}'. Coefficients new name '{}' already exists in this aircraft.", Id, Doc29Perf.Name, coeffs.Name);
            coeffs.Name = Id;
        }

        return updated;
    }

    bool Doc29PerformanceManager::updateKeyProfile(Doc29Profile& Doc29Prof, const std::string Id) {
        if (Doc29Prof.Name.empty())
        {
            Log::dataLogic()->error("Updating Doc29 {} profile '{}'. Empty name not allowed.", OperationTypes.toString(Doc29Prof.operationType()), Id);
            Doc29Prof.Name = Id;
            return false;
        }

        bool updated = false;

        switch (Doc29Prof.operationType())
        {
        case OperationType::Arrival: updated = m_Doc29Performances(Doc29Prof.parentDoc29Performance().Name)->ArrivalProfiles.update(Id, Doc29Prof.Name);
            break;
        case OperationType::Departure: updated = m_Doc29Performances(Doc29Prof.parentDoc29Performance().Name)->DepartureProfiles.update(Id, Doc29Prof.Name);
            break;
        default: GRAPE_ASSERT(false);
        }

        if (updated) { m_Db.update(Schema::doc29_performance_profiles, { 2 }, std::make_tuple(Doc29Prof.Name), { 0, 1, 2 }, std::make_tuple(Doc29Prof.parentDoc29Performance().Name, OperationTypes.toString(Doc29Prof.operationType()), Id)); }
        else
        {
            Log::dataLogic()->error("Updating Doc29 {} profile '{}'. Profile new name '{}' already exists in Doc29 aircraft '{}'.", OperationTypes.toString(Doc29Prof.operationType()), Id, Doc29Prof.Name, Doc29Prof.parentDoc29Performance().Name);
            Doc29Prof.Name = Id;
        }

        return updated;
    }

    void Doc29PerformanceManager::updatePerformance(const Doc29Performance& Doc29Perf) const { m_Db.update(Schema::doc29_performance, std::make_tuple(Doc29Perf.Name, Doc29Performance::Types.toString(Doc29Perf.type())), { 0 }, std::make_tuple(Doc29Perf.Name)); }

    void Doc29PerformanceManager::updateThrust(const Doc29Performance& Doc29Perf) const {
        m_Db.deleteD(Schema::doc29_performance_thrust, { 0 }, std::make_tuple(Doc29Perf.Name));
        m_Db.insert(Schema::doc29_performance_thrust, {}, std::make_tuple(Doc29Perf.Name, Doc29Thrust::Types.toString(Doc29Perf.thrust().type())));
        ThrustCoefficientsInserter insertEngineCoeffs(m_Db, Doc29Perf);
    }

    void ThrustCoefficientsInserter::visitDoc29ThrustRating(Doc29ThrustRating& Thrust) {
        for (const auto& [thrustRating, engineCoeffs] : Thrust)
        {
            m_Db.insert(Schema::doc29_performance_thrust_ratings, {}, std::make_tuple(m_Doc29Acft.Name, Doc29Thrust::Ratings.toString(thrustRating)));
            m_Db.insert(Schema::doc29_performance_thrust_rating_coefficients, {}, std::make_tuple(m_Doc29Acft.Name, Doc29Thrust::Ratings.toString(thrustRating), engineCoeffs.E, engineCoeffs.F, engineCoeffs.Ga, engineCoeffs.Gb, engineCoeffs.H));
        }
    }

    void ThrustCoefficientsInserter::visitDoc29ThrustPropeller(Doc29ThrustRatingPropeller& Thrust) {
        for (const auto& [thrustRating, engineCoeffs] : Thrust)
        {
            m_Db.insert(Schema::doc29_performance_thrust_ratings, {}, std::make_tuple(m_Doc29Acft.Name, Doc29Thrust::Ratings.toString(thrustRating)));
            m_Db.insert(Schema::doc29_performance_thrust_rating_coefficients_propeller, {}, std::make_tuple(m_Doc29Acft.Name, Doc29Thrust::Ratings.toString(thrustRating), engineCoeffs.Pe, engineCoeffs.Pp));
        }
    }

    void Doc29PerformanceManager::updateAerodynamicCoefficients(const Doc29Performance& Doc29Perf) const {
        m_Db.deleteD(Schema::doc29_performance_aerodynamic_coefficients, { 0 }, std::make_tuple(Doc29Perf.Name));
        for (const auto& aeroCoeffs : Doc29Perf.AerodynamicCoefficients | std::views::values)
            m_Db.insert(Schema::doc29_performance_aerodynamic_coefficients, {}, std::make_tuple(Doc29Perf.Name, aeroCoeffs.Name, Doc29AerodynamicCoefficients::Types.toString(aeroCoeffs.CoefficientType), aeroCoeffs.R, aeroCoeffs.B, aeroCoeffs.C, aeroCoeffs.D));
    }

    void Doc29PerformanceManager::updateProfile(const Doc29Profile& Doc29Prof) const {
        // Updating reinserts profile
        m_Db.deleteD(Schema::doc29_performance_profiles, { 0, 1, 2 }, std::make_tuple(Doc29Prof.parentDoc29Performance().Name, OperationTypes.toString(Doc29Prof.operationType()), Doc29Prof.Name));
        m_Db.insert(Schema::doc29_performance_profiles, {}, std::make_tuple(Doc29Prof.parentDoc29Performance().Name, OperationTypes.toString(Doc29Prof.operationType()), Doc29Prof.Name, Doc29Profile::Types.toString(Doc29Prof.type())));
        ProfileInserter insertProfile(m_Db, Doc29Prof);
    }

    void ProfileInserter::visitDoc29ProfileArrivalPoints(const Doc29ProfileArrivalPoints& Doc29Prof) {
        for (const auto& [cumGroundDistance, pt] : Doc29Prof)
            m_Db.insert(Schema::doc29_performance_profiles_points, {}, std::make_tuple(Doc29Prof.parentDoc29Performance().Name, OperationTypes.toString(Doc29Prof.operationType()), Doc29Prof.Name, cumGroundDistance, pt.AltitudeAfe, pt.TrueAirspeed, pt.CorrNetThrustPerEng));
    }

    void ProfileInserter::visitDoc29ProfileDeparturePoints(const Doc29ProfileDeparturePoints& Doc29Prof) {
        for (const auto& [cumGroundDistance, pt] : Doc29Prof)
            m_Db.insert(Schema::doc29_performance_profiles_points, {}, std::make_tuple(Doc29Prof.parentDoc29Performance().Name, OperationTypes.toString(Doc29Prof.operationType()), Doc29Prof.Name, cumGroundDistance, pt.AltitudeAfe, pt.TrueAirspeed, pt.CorrNetThrustPerEng));
    }

    void ProfileInserter::visitDoc29ProfileArrivalProcedural(const Doc29ProfileArrivalProcedural& Doc29Prof) {
        Statement stmt(m_Db, Schema::doc29_performance_profiles_arrival_procedural.queryInsert());
        stmt.bindValues(Doc29Prof.parentDoc29Performance().Name, OperationTypes.toString(Doc29Prof.operationType()), Doc29Prof.Name);

        for (auto it = Doc29Prof.begin(); it != Doc29Prof.end(); ++it)
        {
            const auto i = it - Doc29Prof.begin();
            const auto& step = *it;

            stmt.bind(3, static_cast<int>(i + 1));
            stmt.bind(4, std::visit(Doc29ProfileArrivalProcedural::VisitorStepTypeString(), step));
            std::visit(Overload{
                [&](const Doc29ProfileArrivalProcedural::DescendDecelerate& Step) {
                    stmt.bind(5, Step.Doc29AerodynamicCoefficients->Name);
                    stmt.bind(6, Step.StartAltitudeAfe);
                    stmt.bind(7, Step.DescentAngle);
                    stmt.bind(8, Step.StartCalibratedAirspeed);
                },
                [&](const Doc29ProfileArrivalProcedural::DescendIdle& Step) {
                    stmt.bind(5, std::monostate());
                    stmt.bind(6, Step.StartAltitudeAfe);
                    stmt.bind(7, Step.DescentAngle);
                    stmt.bind(8, Step.StartCalibratedAirspeed);
                },
                [&](const Doc29ProfileArrivalProcedural::Level& Step) {
                    stmt.bind(5, Step.Doc29AerodynamicCoefficients->Name);
                    stmt.bind(6, Step.GroundDistance);
                    stmt.bind(7, std::monostate());
                    stmt.bind(8, std::monostate());
                },
                [&](const Doc29ProfileArrivalProcedural::LevelDecelerate& Step) {
                    stmt.bind(5, Step.Doc29AerodynamicCoefficients->Name);
                    stmt.bind(6, Step.GroundDistance);
                    stmt.bind(7, Step.StartCalibratedAirspeed);
                    stmt.bind(8, std::monostate());
                },
                [&](const Doc29ProfileArrivalProcedural::LevelIdle& Step) {
                    stmt.bind(5, std::monostate());
                    stmt.bind(6, Step.GroundDistance);
                    stmt.bind(7, Step.StartCalibratedAirspeed);
                    stmt.bind(8, std::monostate());
                },
                [&](const Doc29ProfileArrivalProcedural::DescendLand& Step) {
                    stmt.bind(5, Step.Doc29AerodynamicCoefficients->Name);
                    stmt.bind(6, Step.DescentAngle);
                    stmt.bind(7, Step.ThresholdCrossingAltitudeAfe);
                    stmt.bind(8, Step.TouchdownRoll);
                },
                [&](const Doc29ProfileArrivalProcedural::GroundDecelerate& Step) {
                    stmt.bind(5, std::monostate());
                    stmt.bind(6, Step.GroundDistance);
                    stmt.bind(7, Step.StartCalibratedAirspeed);
                    stmt.bind(8, Step.StartThrustPercentage);
                },
                }, step);
            stmt.step();
            stmt.reset();
        }
    }

    void ProfileInserter::visitDoc29ProfileDepartureProcedural(const Doc29ProfileDepartureProcedural& Doc29Prof) {
        Statement stmt(m_Db, Schema::doc29_performance_profiles_departure_procedural.queryInsert());
        stmt.bindValues(Doc29Prof.parentDoc29Performance().Name, OperationTypes.toString(Doc29Prof.operationType()), Doc29Prof.Name);
        for (auto it = Doc29Prof.begin(); it != Doc29Prof.end(); ++it)
        {
            const auto i = it - Doc29Prof.begin();
            const auto& step = *it;

            stmt.bind(3, static_cast<int>(i + 1));
            stmt.bind(4, std::visit(Doc29ProfileDepartureProcedural::VisitorStepTypeString(), step));
            stmt.bind(5, static_cast<std::size_t>(i) == Doc29Prof.thrustCutback() ? 1 : 0);

            std::visit(Overload{
                [&](const Doc29ProfileDepartureProcedural::Takeoff& Step) {
                    stmt.bind(6, Step.Doc29AerodynamicCoefficients->Name);
                    stmt.bind(7, Step.InitialCalibratedAirspeed);
                    stmt.bind(8, std::monostate());
                },
                [&](const Doc29ProfileDepartureProcedural::Climb& Step) {
                    stmt.bind(6, Step.Doc29AerodynamicCoefficients->Name);
                    stmt.bind(7, Step.EndAltitudeAfe);
                    stmt.bind(8, std::monostate());
                },
                [&](const Doc29ProfileDepartureProcedural::ClimbAccelerate& Step) {
                    stmt.bind(6, Step.Doc29AerodynamicCoefficients->Name);
                    stmt.bind(7, Step.EndCalibratedAirspeed);
                    stmt.bind(8, Step.ClimbParameter);
                },
                [&](const Doc29ProfileDepartureProcedural::ClimbAcceleratePercentage& Step) {
                    stmt.bind(6, Step.Doc29AerodynamicCoefficients->Name);
                    stmt.bind(7, Step.EndCalibratedAirspeed);
                    stmt.bind(8, Step.ClimbParameter);
                },
                }, step);
            stmt.step();
            stmt.reset();
        }
    }

    void Doc29PerformanceManager::loadFromFile() {
        Statement stmtAcft(m_Db, Schema::doc29_performance.querySelect());
        stmtAcft.step();
        while (stmtAcft.hasRow())
        {
            // Doc29 Aircraft
            const std::string doc29AcftName = stmtAcft.getColumn(0);
            switch (Doc29Performance::Types.fromString(stmtAcft.getColumn(1)))
            {
            case Doc29Performance::Type::Jet: m_Doc29Performances.add(doc29AcftName, std::make_unique<Doc29PerformanceJet>(doc29AcftName));
                break;
            case Doc29Performance::Type::Turboprop: m_Doc29Performances.add(doc29AcftName, std::make_unique<Doc29PerformanceTurboprop>(doc29AcftName));
                break;
            case Doc29Performance::Type::Piston: m_Doc29Performances.add(doc29AcftName, std::make_unique<Doc29PerformancePiston>(doc29AcftName));
                break;
            default: GRAPE_ASSERT(false);
            }
            auto& doc29Acft = *m_Doc29Performances(doc29AcftName);

            // Thrust
            Statement stmtThrust(m_Db, Schema::doc29_performance_thrust.querySelect({ 1 }, { 0 }));
            stmtThrust.bindValues(doc29AcftName);
            stmtThrust.step();
            if (stmtThrust.hasRow())
                doc29Acft.setThrustType(Doc29Thrust::Types.fromString(stmtThrust.getColumn(0)));
            else
                m_Db.insert(Schema::doc29_performance_thrust, {}, std::make_tuple(doc29AcftName, Doc29Thrust::Types.toString(doc29Acft.thrust().type())));

            ThrustCoefficientsLoader engineCoeffsLoader(m_Db, doc29Acft);

            // Aerodynamic Coefficients
            Statement stmtAeroCoeffs(m_Db, Schema::doc29_performance_aerodynamic_coefficients.querySelect({}, { 0 }));
            stmtAeroCoeffs.bindValues(doc29AcftName);
            stmtAeroCoeffs.step();
            while (stmtAeroCoeffs.hasRow())
            {
                const std::string aeroCoeffsName = stmtAeroCoeffs.getColumn(1);
                auto [aeroCoeffs, added] = doc29Acft.AerodynamicCoefficients.add(aeroCoeffsName, aeroCoeffsName);
                GRAPE_ASSERT(added);
                aeroCoeffs.CoefficientType = Doc29AerodynamicCoefficients::Types.fromString(stmtAeroCoeffs.getColumn(2));
                aeroCoeffs.R = stmtAeroCoeffs.getColumn(3);

                switch (aeroCoeffs.CoefficientType)
                {
                case Doc29AerodynamicCoefficients::Type::Takeoff:
                    {
                        aeroCoeffs.B = stmtAeroCoeffs.getColumn(4);
                        aeroCoeffs.C = stmtAeroCoeffs.getColumn(5);
                        break;
                    }
                case Doc29AerodynamicCoefficients::Type::Land:
                    {
                        aeroCoeffs.D = stmtAeroCoeffs.getColumn(6);
                        break;
                    }
                case Doc29AerodynamicCoefficients::Type::Cruise: break;
                default: GRAPE_ASSERT(false);
                    break;
                }
                stmtAeroCoeffs.step();
            }

            // Doc29 Profiles
            Statement stmtProf(m_Db, Schema::doc29_performance_profiles.querySelect({ 1, 2, 3 }, { 0 }));
            stmtProf.bindValues(doc29Acft.Name);
            stmtProf.step();
            while (stmtProf.hasRow())
            {
                const std::string doc29ProfName = stmtProf.getColumn(1);
                const Doc29Profile::Type profType = Doc29Profile::Types.fromString(stmtProf.getColumn(2));
                switch (OperationTypes.fromString(stmtProf.getColumn(0)))
                {
                case OperationType::Arrival:
                    {
                        auto [doc29Prof, added] = doc29Acft.addArrivalProfile(doc29ProfName, profType);
                        ProfileLoader profLoader(m_Db, doc29Prof);
                        break;
                    }
                case OperationType::Departure:
                    {
                        auto [doc29Prof, added] = doc29Acft.addDepartureProfile(doc29ProfName, profType);
                        ProfileLoader profLoader(m_Db, doc29Prof);
                        break;
                    }
                default: GRAPE_ASSERT(false);
                }
                stmtProf.step();
            }
            stmtAcft.step();
        }
    }

    void ThrustCoefficientsLoader::visitDoc29ThrustRating(Doc29ThrustRating& Thrust) {
        Statement stmt(m_Db, Schema::doc29_performance_thrust_ratings.querySelect({ 1 }, { 0 }));
        stmt.bindValues(m_Doc29Acft.Name);
        stmt.step();
        while (stmt.hasRow())
        {
            const std::string thrustRatingStr = stmt.getColumn(0);
            auto [engineCoeffs, added] = Thrust.Coeffs.add(Doc29Thrust::Ratings.fromString(thrustRatingStr));
            GRAPE_ASSERT(added);

            Statement stmtCoeffs(m_Db, Schema::doc29_performance_thrust_rating_coefficients.querySelect({ 2, 3, 4, 5, 6 }, { 0, 1 }));
            stmtCoeffs.bindValues(m_Doc29Acft.Name, thrustRatingStr);
            stmtCoeffs.step();
            if (stmtCoeffs.hasRow())
            {
                engineCoeffs.E = stmtCoeffs.getColumn(0);
                engineCoeffs.F = stmtCoeffs.getColumn(1);
                engineCoeffs.Ga = stmtCoeffs.getColumn(2);
                engineCoeffs.Gb = stmtCoeffs.getColumn(3);
                engineCoeffs.H = stmtCoeffs.getColumn(4);
            }
            stmt.step();
        }
    }

    void ThrustCoefficientsLoader::visitDoc29ThrustPropeller(Doc29ThrustRatingPropeller& Thrust) {
        Statement stmt(m_Db, Schema::doc29_performance_thrust_ratings.querySelect({ 1 }, { 0 }));
        stmt.bindValues(m_Doc29Acft.Name);
        stmt.step();
        while (stmt.hasRow())
        {
            const std::string thrustRatingStr = stmt.getColumn(0);
            auto [engineCoeffs, added] = Thrust.Coeffs.add(Doc29Thrust::Ratings.fromString(thrustRatingStr));
            GRAPE_ASSERT(added);

            Statement stmtCoeffs(m_Db, Schema::doc29_performance_thrust_rating_coefficients_propeller.querySelect({ 2, 3 }, { 0, 1 }));
            stmtCoeffs.bindValues(m_Doc29Acft.Name, thrustRatingStr);
            stmtCoeffs.step();
            if (stmtCoeffs.hasRow())
            {
                engineCoeffs.Pe = stmtCoeffs.getColumn(0);
                engineCoeffs.Pp = stmtCoeffs.getColumn(1);
            }
            stmt.step();
        }
    }

    void ProfileLoader::visitDoc29ProfileArrivalPoints(Doc29ProfileArrivalPoints& Doc29Prof) {
        Statement stmt(m_Db, Schema::doc29_performance_profiles_points.querySelect({ 3, 4, 5, 6 }, { 0, 1, 2 }, { 3 }));
        stmt.bindValues(Doc29Prof.parentDoc29Performance().Name, OperationTypes.toString(Doc29Prof.operationType()), Doc29Prof.Name);
        stmt.step();
        while (stmt.hasRow())
        {
            const double cumDist = stmt.getColumn(0);
            const double altAfe = stmt.getColumn(1);
            const double tas = stmt.getColumn(2);
            const double thrust = stmt.getColumn(3);
            Doc29Prof.addPoint(cumDist, altAfe, tas, thrust);
            stmt.step();
        }
    }

    void ProfileLoader::visitDoc29ProfileDeparturePoints(Doc29ProfileDeparturePoints& Doc29Prof) {
        Statement stmt(m_Db, Schema::doc29_performance_profiles_points.querySelect({ 3, 4, 5, 6 }, { 0, 1, 2 }, { 3 }));
        stmt.bindValues(Doc29Prof.parentDoc29Performance().Name, OperationTypes.toString(Doc29Prof.operationType()), Doc29Prof.Name);
        stmt.step();
        while (stmt.hasRow())
        {
            const double cumDist = stmt.getColumn(0);
            const double altAfe = stmt.getColumn(1);
            const double tas = stmt.getColumn(2);
            const double thrust = stmt.getColumn(3);
            Doc29Prof.addPoint(cumDist, altAfe, tas, thrust);

            stmt.step();
        }
    }

    void ProfileLoader::visitDoc29ProfileArrivalProcedural(Doc29ProfileArrivalProcedural& Doc29Prof) {
        // Descending order so that steps are introduced in the correct order.
        Statement stmt(m_Db, Schema::doc29_performance_profiles_arrival_procedural.querySelect({ 4, 5, 6, 7, 8 }, { 0, 1, 2 }, { 3 }));
        stmt.bindValues(Doc29Prof.parentDoc29Performance().Name, OperationTypes.toString(Doc29Prof.operationType()), Doc29Prof.Name);
        stmt.step();
        while (stmt.hasRow())
        {
            const std::string aeroCoeffId = stmt.getColumn(1);
            const double param1 = stmt.getColumn(2);
            const double param2 = stmt.getColumn(3);
            const double param3 = stmt.getColumn(4);
            switch (Doc29ProfileArrivalProcedural::StepTypes.fromString(stmt.getColumn(0)))
            {
            case Doc29ProfileArrivalProcedural::StepType::DescendDecelerate: Doc29Prof.addDescendDecelerate(aeroCoeffId, param1, param2, param3);
                break;
            case Doc29ProfileArrivalProcedural::StepType::DescendIdle: Doc29Prof.addDescendIdle(param1, param2, param3);
                break;
            case Doc29ProfileArrivalProcedural::StepType::Level: Doc29Prof.addLevel(aeroCoeffId, param1);
                break;
            case Doc29ProfileArrivalProcedural::StepType::LevelDecelerate: Doc29Prof.addLevelDecelerate(aeroCoeffId, param1, param2);
                break;
            case Doc29ProfileArrivalProcedural::StepType::LevelIdle: Doc29Prof.addLevelIdle(param1, param2);
                break;
            case Doc29ProfileArrivalProcedural::StepType::DescendLand: Doc29Prof.setDescendLandParameters(aeroCoeffId, param1, param2, param3);
                break;
            case Doc29ProfileArrivalProcedural::StepType::GroundDecelerate: Doc29Prof.addGroundDecelerate(param1, param2, param3);
                break;
            default: GRAPE_ASSERT(false);
                break;
            }

            stmt.step();
        }
    }

    void ProfileLoader::visitDoc29ProfileDepartureProcedural(Doc29ProfileDepartureProcedural& Profile) {
        std::size_t thrustCutbackIndex = 0;
        std::size_t stepCount = 1; // Takeoff always exists

        Statement stmt(m_Db, Schema::doc29_performance_profiles_departure_procedural.querySelect({ 4, 5, 6, 7, 8 }, { 0, 1, 2 }, { 3 }));
        stmt.bindValues(Profile.parentDoc29Performance().Name, OperationTypes.toString(Profile.operationType()), Profile.Name);
        stmt.step();
        while (stmt.hasRow())
        {
            const std::string aeroCoeffId = stmt.getColumn(2);
            const bool thrustCutback = stmt.getColumn(1).getInt(); // implicit conversion int to bool
            const double param1 = stmt.getColumn(3);
            const double param2 = stmt.getColumn(4);

            switch (Doc29ProfileDepartureProcedural::StepTypes.fromString(stmt.getColumn(0)))
            {
            case Doc29ProfileDepartureProcedural::StepType::Takeoff: Profile.setTakeoffParameters(aeroCoeffId, param1);
                break;
            case Doc29ProfileDepartureProcedural::StepType::Climb:
                {
                    if (thrustCutback)
                        thrustCutbackIndex = stepCount;
                    Profile.addClimb(aeroCoeffId, param1);
                    stepCount++;
                    break;
                }
            case Doc29ProfileDepartureProcedural::StepType::ClimbAccelerate:
                {
                    if (thrustCutback)
                        thrustCutbackIndex = stepCount;
                    Profile.addClimbAccelerate(aeroCoeffId, param1, param2);
                    stepCount++;
                    break;
                }
            case Doc29ProfileDepartureProcedural::StepType::ClimbAcceleratePercentage:
                {
                    if (thrustCutback)
                        thrustCutbackIndex = stepCount;
                    Profile.addClimbAcceleratePercentage(aeroCoeffId, param1, param2);
                    stepCount++;
                    break;
                }
            default: GRAPE_ASSERT(false);
                break;
            }
            stmt.step();
        }
        if (thrustCutbackIndex != 0)
            Profile.setThrustCutback(thrustCutbackIndex);
    }
}
