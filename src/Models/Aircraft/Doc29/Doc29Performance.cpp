// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "Doc29Performance.h"

namespace GRAPE {
    void Doc29AerodynamicCoefficients::setRCoeffE(double RIn) {
        if (!(RIn > 0.0))
            throw GrapeException("Aerodynamic coefficient R must be higher than 0.");
        R = RIn;
    }

    void Doc29AerodynamicCoefficients::setBCoeffE(double BIn) {
        if (!(BIn > 0.0))
            throw GrapeException("Aerodynamic coefficient B must be higher than 0.");
        B = BIn;
    }

    void Doc29AerodynamicCoefficients::setCCoeffE(double CIn) {
        if (!(CIn > 0.0))
            throw GrapeException("Aerodynamic coefficient C must be higher than 0.");
        C = CIn;
    }

    void Doc29AerodynamicCoefficients::setDCoeffE(double DIn) {
        if (!(DIn > 0.0))
            throw GrapeException("Aerodynamic coefficient D must be higher than 0.");
        D = DIn;
    }

    void Doc29AerodynamicCoefficients::setCoeffsE(double RIn, double BIn, double CIn, double DIn) {
        if (!(RIn > 0.0))
            throw GrapeException("Aerodynamic coefficient R must be higher than 0.");
        if (!(BIn > 0.0))
            throw GrapeException("Aerodynamic coefficient B must be higher than 0.");
        if (!(CIn > 0.0))
            throw GrapeException("Aerodynamic coefficient C must be higher than 0.");
        if (!(DIn > 0.0))
            throw GrapeException("Aerodynamic coefficient D must be higher than 0.");

        R = RIn;
        B = BIn;
        C = CIn;
        D = DIn;
    }

    Doc29Performance::Doc29Performance(std::string_view Name) : Name(Name) { m_Thrust = std::make_unique<Doc29Thrust>(); }

    // Add helpers
    std::pair<Doc29ProfileArrival&, bool> Doc29Performance::addArrivalProfile(const std::string& NameIn, Doc29ProfileArrival::Type ArrivalType) {
        std::unique_ptr<Doc29ProfileArrival> newProf;
        switch (ArrivalType)
        {
        case Doc29ProfileArrival::Type::Points: newProf = std::make_unique<Doc29ProfileArrivalPoints>(*this, NameIn);
            break;
        case Doc29ProfileArrival::Type::Procedural: newProf = std::make_unique<Doc29ProfileArrivalProcedural>(*this, NameIn);
            break;
        default: GRAPE_ASSERT(false);
            break;
        }

        auto [Prof, added] = ArrivalProfiles.add(NameIn, std::move(newProf));
        return { *Prof, added };
    }

    std::pair<Doc29ProfileDeparture&, bool> Doc29Performance::addDepartureProfile(const std::string& NameIn, Doc29ProfileDeparture::Type DepartureType) {
        std::unique_ptr<Doc29ProfileDeparture> newProf;
        switch (DepartureType)
        {
        case Doc29ProfileDeparture::Type::Points: newProf = std::make_unique<Doc29ProfileDeparturePoints>(*this, NameIn);
            break;
        case Doc29ProfileDeparture::Type::Procedural: newProf = std::make_unique<Doc29ProfileDepartureProcedural>(*this, NameIn);
            break;
        default: GRAPE_ASSERT(false);
            break;
        }

        auto [Prof, added] = DepartureProfiles.add(NameIn, std::move(newProf));
        return { *Prof, added };
    }

    bool Doc29Performance::containsAerodynamicCoefficientsWithType(Doc29AerodynamicCoefficients::Type CoeffType) const {
        bool exists = false;
        for (const auto& coeffs : AerodynamicCoefficients | std::views::values)
        {
            if (coeffs.CoefficientType == CoeffType)
            {
                exists = true;
                break;
            }
        }
        return exists;
    }

    bool Doc29Performance::containsArrivalProceduralProfiles() const { return std::ranges::any_of(ArrivalProfiles | std::views::values, [](const auto& Prof) { return Prof->type() == Doc29Profile::Type::Procedural; }); }

    bool Doc29Performance::containsDepartureProceduralProfiles() const { return std::ranges::any_of(DepartureProfiles | std::views::values, [](const auto& Prof) { return Prof->type() == Doc29Profile::Type::Procedural; }); }

    Doc29PerformanceJet::Doc29PerformanceJet(std::string_view NameIn) : Doc29Performance(NameIn) { m_Thrust = std::make_unique<Doc29ThrustRating>(); }

    std::vector<std::string> Doc29Performance::aerodynamicCoefficientsWithType(Doc29AerodynamicCoefficients::Type CoeffType) const {
        std::vector<std::string> names;
        for (const auto& [name, coeffs] : AerodynamicCoefficients)
        {
            if (coeffs.CoefficientType == CoeffType)
                names.emplace_back(name);
        }
        return names;
    }

    void Doc29PerformanceJet::setThrustType(Doc29Thrust::Type ThrustType) {
        switch (ThrustType)
        {
        case Doc29Thrust::Type::None: m_Thrust = std::make_unique<Doc29Thrust>();
            break;
        case Doc29Thrust::Type::Rating: m_Thrust = std::make_unique<Doc29ThrustRating>();
            break;
        case Doc29Thrust::Type::RatingPropeller: GRAPE_ASSERT(false);
            break;
        default: GRAPE_ASSERT(false);
            break;
        }
    }

    Doc29PerformanceTurboprop::Doc29PerformanceTurboprop(std::string_view NameIn) : Doc29Performance(NameIn) { m_Thrust = std::make_unique<Doc29ThrustRatingPropeller>(); }

    void Doc29PerformanceTurboprop::setThrustType(Doc29Thrust::Type ThrustType) {
        switch (ThrustType)
        {
        case Doc29Thrust::Type::None: m_Thrust = std::make_unique<Doc29Thrust>();
            break;
        case Doc29Thrust::Type::Rating: m_Thrust = std::make_unique<Doc29ThrustRating>();
            break;
        case Doc29Thrust::Type::RatingPropeller: m_Thrust = std::make_unique<Doc29ThrustRatingPropeller>();
            break;
        default: GRAPE_ASSERT(false);
            break;
        }
    }

    Doc29PerformancePiston::Doc29PerformancePiston(std::string_view NameIn) : Doc29Performance(NameIn) { m_Thrust = std::make_unique<Doc29Thrust>(); }

    void Doc29PerformancePiston::setThrustType(Doc29Thrust::Type ThrustType) {
        switch (ThrustType)
        {
        case Doc29Thrust::Type::None: m_Thrust = std::make_unique<Doc29Thrust>(); break;
        case Doc29Thrust::Type::Rating: GRAPE_ASSERT(false); break;
        case Doc29Thrust::Type::RatingPropeller: GRAPE_ASSERT(false); break;
        default: GRAPE_ASSERT(false); break;
        }
    }
}
