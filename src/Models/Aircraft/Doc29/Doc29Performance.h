// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Doc29Profile.h"
#include "Doc29Thrust.h"

namespace GRAPE {
    /**
    * @brief A Doc29AerodynamicCoefficients belongs to a Doc29Performance. It contains the aerodynamic coefficients for a single performance state. The possible types are stores in the #Type enum.
    */
    struct Doc29AerodynamicCoefficients {
        /**
        * @brief Types of Doc29 aerodynamic coefficients, determines which coefficients are mandatory.
        *
        * - #Type::Takeoff: #R, #B and #C coefficients.
        * - #Type::Land: #R and #D coefficients.
        * - #Type::Cruise: #R coefficients.
        */
        enum class Type {
            Takeoff = 0, Land, Cruise,
        };

        static constexpr EnumStrings<Type> Types{ "Takeoff", "Land", "Cruise" };

        explicit Doc29AerodynamicCoefficients(std::string_view NameIn) noexcept : Name(NameIn) {}

        // Data
        std::string Name;
        Type CoefficientType = Type::Cruise;
        double R = 0.01;
        double B = 0.01;
        double C = 0.01;
        double D = 0.01;

        /**
        * @brief Throwing set method for the #R coefficient. Throws if RIn not in ]0.0, inf].
        */
        void setRCoeffE(double RIn);

        /**
        * @brief Throwing set method for the #B coefficient. Throws if BIn not in ]0.0, inf].
        */
        void setBCoeffE(double BIn);

        /**
        * @brief Throwing set method for the #C coefficient. Throws if CIn not in ]0.0, inf].
        */
        void setCCoeffE(double CIn);

        /**
        * @brief Throwing set method for the #D coefficient. Throws if DIn not in ]0.0, inf].
        */
        void setDCoeffE(double DIn);

        /**
        * @brief Throwing set method for all coefficients. Throws if RIn, BIn, CIn, or DIn not in ]0.0, inf].
        */
        void setCoeffsE(double RIn, double BIn, double CIn, double DIn);
    };

    /**
    * @brief Base class for the different performance types. The possible types are stores in the #Type enum.
    *
    * A Doc29Performance owns arrival and departure profiles, a #Doc29Thrust as well as aerodynamic coefficients.
    */
    class Doc29Performance {
    public:
        enum class Type {
            Jet = 0, Turboprop, Piston,
        };

        static constexpr EnumStrings<Type> Types{ "Jet", "Turboprop", "Piston" };

        // Constructors & Destructor
        explicit Doc29Performance(std::string_view Name);
        Doc29Performance(const Doc29Performance& Other) = delete;
        Doc29Performance(Doc29Performance& Other) = delete;
        Doc29Performance& operator=(const Doc29Performance&) = delete;
        Doc29Performance& operator=(Doc29Performance&&) = delete;
        virtual ~Doc29Performance() = default;

        // Name
        std::string Name;

        /**
        * @return The Doc29Performance::Type.
        */
        [[nodiscard]] virtual Type type() const = 0;

        /**
        * @return The Doc29Thrust (rating or propeller) used by this Doc29 performance.
        */
        [[nodiscard]] Doc29Thrust& thrust() const { return *m_Thrust; }

        /**
        * @return The list of allowed Doc29Thrust::Type for this #Type.
        */
        [[nodiscard]] virtual std::vector<Doc29Thrust::Type> allowedThrustTypes() const = 0;

        /**
        * @brief Implemented in the derived classes, which filter for Doc29Thrust::Type that are not allowed.
        */
        virtual void setThrustType(Doc29Thrust::Type ThrustType) = 0;

        // Aerodynamic Coefficients
        GrapeMap<std::string, Doc29AerodynamicCoefficients> AerodynamicCoefficients; // Key is flap name
        BlockMap<const Doc29AerodynamicCoefficients*, const Doc29Profile*> b_BlockedAerodynamicCoefficients; // Prevent deletion

        /**
        * @brief Check if any of the Doc29AerodynamicCoefficients of this Doc29Performance are of type CoeffType.
        */
        [[nodiscard]] bool containsAerodynamicCoefficientsWithType(Doc29AerodynamicCoefficients::Type CoeffType) const;

        /**
        * @return List of the names of the Doc29AerodynamicCoefficients of this Doc29Performance of type CoeffType.
        */
        [[nodiscard]] std::vector<std::string> aerodynamicCoefficientsWithType(Doc29AerodynamicCoefficients::Type CoeffType) const;

        // Profiles
        GrapeMap<std::string, std::unique_ptr<Doc29ProfileArrival>> ArrivalProfiles;     // Key is profile name
        GrapeMap<std::string, std::unique_ptr<Doc29ProfileDeparture>> DepartureProfiles; // Key is profile name

        /**
        * @brief Helper function to add an Doc29ProfileArrival of ArrivalType.
        * @return The added arrival profile and true or the already existing arrival profile and false.
        */
        std::pair<Doc29ProfileArrival&, bool> addArrivalProfile(const std::string& NameIn, Doc29ProfileArrival::Type ArrivalType);

        /**
        * @brief Helper function to add an Doc29ProfileDeparture of DepartureType.
        * @return The added departure profile and true or the already existing departure profile and false.
        */
        std::pair<Doc29ProfileDeparture&, bool> addDepartureProfile(const std::string& NameIn, Doc29ProfileDeparture::Type DepartureType);

        /**
        * @brief Checks if Doc29Performance contains neither arrival nor departure profiles.
        */
        [[nodiscard]] bool emptyProfiles() const { return ArrivalProfiles.empty() && DepartureProfiles.empty(); }

        /**
        * @brief Arrival procedural profiles only allowed if Doc29Performance contains a Doc29AerodynamicCoefficients of type Land
        */
        [[nodiscard]] bool arrivalProfileProceduralAllowed() const { return containsAerodynamicCoefficientsWithType(Doc29AerodynamicCoefficients::Type::Land); }

        /**
        * @brief Departure procedural profiles only allowed if Doc29Performance contains a Doc29AerodynamicCoefficients of type Takeoff, and the Doc29Thrust has both MaximumTakeoff and MaximumClimb thrust ratings defined.
        */
        [[nodiscard]] bool departureProfileProceduralAllowed() const { return m_Thrust->isRatingSet(Doc29Thrust::Rating::MaximumTakeoff) && m_Thrust->isRatingSet(Doc29Thrust::Rating::MaximumClimb) && containsAerodynamicCoefficientsWithType(Doc29AerodynamicCoefficients::Type::Takeoff); }

        /**
        * @brief Checks if any of the Doc29ProfileArrival is of type procedural.
        */
        [[nodiscard]] bool containsArrivalProceduralProfiles() const;

        /**
        * @brief Checks if any of the Doc29ProfileDeparture is of type procedural.
        */
        [[nodiscard]] bool containsDepartureProceduralProfiles() const;

    protected:
        std::unique_ptr<Doc29Thrust> m_Thrust;
    };

    /**
    * @brief Doc29Performance of Jet type, only thrust rating coefficients are allowed.
    */
    class Doc29PerformanceJet : public Doc29Performance {
    public:
        explicit Doc29PerformanceJet(std::string_view NameIn);
        virtual ~Doc29PerformanceJet() override = default;

        /**
        * @return The Doc29Performance::Type.
        */
        [[nodiscard]] Type type() const override { return Type::Jet; }

        [[nodiscard]] std::vector<Doc29Thrust::Type> allowedThrustTypes() const override { return { Doc29Thrust::Type::Rating, Doc29Thrust::Type::None }; }
        void setThrustType(Doc29Thrust::Type ThrustType) override;
    };

    /**
    * @brief Doc29Performance of Turboprop type, thrust rating and thrust rating propeller coefficients are allowed.
    */
    class Doc29PerformanceTurboprop : public Doc29Performance {
    public:
        explicit Doc29PerformanceTurboprop(std::string_view NameIn);
        virtual ~Doc29PerformanceTurboprop() override = default;

        /**
        * @return The Doc29Performance::Type.
        */
        [[nodiscard]] Type type() const override { return Type::Turboprop; }

        [[nodiscard]] std::vector<Doc29Thrust::Type> allowedThrustTypes() const override { return { Doc29Thrust::Type::Rating, Doc29Thrust::Type::RatingPropeller, Doc29Thrust::Type::None }; }
        void setThrustType(Doc29Thrust::Type ThrustType) override;
    };

    /**
    * @brief Doc29Performance of Turboprop type, Doc29Thrust must be of type none.
    */
    class Doc29PerformancePiston : public Doc29Performance {
    public:
        explicit Doc29PerformancePiston(std::string_view NameIn);
        virtual ~Doc29PerformancePiston() override = default;

        /**
        * @return The Doc29Performance::Type.
        */
        [[nodiscard]] Type type() const override { return Type::Piston; }

        [[nodiscard]] std::vector<Doc29Thrust::Type> allowedThrustTypes() const override { return { Doc29Thrust::Type::RatingPropeller, Doc29Thrust::Type::None }; }
        void setThrustType(Doc29Thrust::Type ThrustType) override;
    };
}
