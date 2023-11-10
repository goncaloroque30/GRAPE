// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Doc29Profile.h"
#include "Doc29Thrust.h"

namespace GRAPE {
    /**
    * @brief A Doc29AerodynamicCoefficients belongs to a Doc29Aircraft. It contains the aerodynamic coefficients for a single performance state. The possible types are stores in the #Type enum.
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
    * @brief A Doc29Aircraft owns arrival and departure profiles, a #Doc29Thrust as well as aerodynamic coefficients.
    */
    class Doc29Aircraft {
    public:
        // Constructors & Destructor
        explicit Doc29Aircraft(std::string_view Name);
        Doc29Aircraft(const Doc29Aircraft& Other) = delete;
        Doc29Aircraft(Doc29Aircraft& Other) = delete;
        Doc29Aircraft& operator=(const Doc29Aircraft&) = delete;
        Doc29Aircraft& operator=(Doc29Aircraft&&) = delete;
        virtual ~Doc29Aircraft() = default;

        // Name
        std::string Name;

        double MaximumSeaLevelStaticThrust = 100000.0;

        double EngineBreakpointTemperature = 303.15; // 30 C.

        /**
        * @return The Doc29Thrust (rating or propeller) used by this Doc29 performance.
        */
        [[nodiscard]] Doc29Thrust& thrust() const { return *m_Thrust; }

        /**
        * @brief Throwing set method for #MaximumSeaLevelStaticThrust.
        *
        * Throw if not in range [1, inf].
        */
        void setMaximumSeaLevelStaticThrust(double MaximumSeaLevelStaticThrustIn);

        /**
        * @brief Throwing set method for #EngineBreakpointTemperature.
        *
        * Throw if not in range [0, inf].
        */
        void setEngineBreakpointTemperature(double EngineBreakpointTemperatureIn);

        /**
        * @brief Implemented in the derived classes, which filter for Doc29Thrust::Type that are not allowed.
        */
        virtual void setThrustType(Doc29Thrust::Type ThrustType);

        // Aerodynamic Coefficients
        GrapeMap<std::string, Doc29AerodynamicCoefficients> AerodynamicCoefficients; // Key is flap name
        BlockMap<const Doc29AerodynamicCoefficients*, const Doc29Profile*> b_BlockedAerodynamicCoefficients; // Prevent deletion

        /**
        * @brief Check if any of the Doc29AerodynamicCoefficients of this Doc29Aircraft are of type CoeffType.
        */
        [[nodiscard]] bool containsAerodynamicCoefficientsWithType(Doc29AerodynamicCoefficients::Type CoeffType) const;

        /**
        * @return List of the names of the Doc29AerodynamicCoefficients of this Doc29Aircraft of type CoeffType.
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
        * @brief Checks if Doc29Aircraft contains neither arrival nor departure profiles.
        */
        [[nodiscard]] bool emptyProfiles() const { return ArrivalProfiles.empty() && DepartureProfiles.empty(); }

        /**
        * @brief Arrival procedural profiles only allowed if Doc29Aircraft contains a Doc29AerodynamicCoefficients of type Land
        */
        [[nodiscard]] bool arrivalProfileProceduralAllowed() const { return containsAerodynamicCoefficientsWithType(Doc29AerodynamicCoefficients::Type::Land); }

        /**
        * @brief Departure procedural profiles only allowed if Doc29Aircraft contains a Doc29AerodynamicCoefficients of type Takeoff, and the Doc29Thrust has both MaximumTakeoff and MaximumClimb thrust ratings defined.
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
}
