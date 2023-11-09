// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

namespace GRAPE {
    struct Doc29ThrustVisitor;
    class Atmosphere;
    /**
    * @brief Base class for the different ways to calculate rated thrust. The possible types are stored in the #Type enum.
    */
    class Doc29Thrust {
    public:

        enum class Type {
            None = 0,
            Rating,
            RatingPropeller,
        };
        static constexpr EnumStrings<Type> Types{ "None", "Rating", "Rating Propeller" };

        /**
        * @brief The supported thrust ratings.
        */
        enum class Rating {
            MaximumTakeoff = 0,
            MaximumClimb,
            Idle,
            MaximumTakeoffHighTemperature,
            MaximumClimbHighTemperature,
            IdleHighTemperature,
        };
        static constexpr EnumStrings<Rating> Ratings{ "Maximum Takeoff", "Maximum Climb", "Idle", "Maximum Takeoff High Temperature", "Maximum Climb High Temperature", "Idle High Temperature" };

        Doc29Thrust() = default;
        virtual ~Doc29Thrust() = default;

        // Access Data
        [[nodiscard]] virtual Type type() const { return Type::None; }

        /**
        * @brief Calculates the corrected net thrust per engine, implemented by the derived classes.
        */
        [[nodiscard]] virtual double calculate(Rating ThrustRating, double CalibratedAirspeed, double Altitude, double EngineBreakpointTemperature, const Atmosphere& Atm) const { GRAPE_ASSERT(false); return Constants::NaN; }

        /**
        * @brief Calls calculate().
        */
        [[nodiscard]] double operator()(Rating ThrustRating, double CalibratedAirspeed, double Altitude, double EngineBreakpointTemperature, const Atmosphere& Atm) const { return calculate(ThrustRating, CalibratedAirspeed, Altitude, EngineBreakpointTemperature, Atm); }

        /**
        * @brief Check if coefficients for ThrustRating are defined and thrust can be calculated for that rating.
        */
        [[nodiscard]] virtual bool isRatingSet(Rating ThrustRating) const { return false; }

        /**
        * @return The list of thrust ratings used by a Departure operation.
        */
        static constexpr std::array<Rating, 2> departureRatings() { return { Rating::MaximumTakeoff, Rating::MaximumClimb }; }

        // Visitor Pattern
        virtual void accept(Doc29ThrustVisitor& Vis);
        virtual void accept(Doc29ThrustVisitor& Vis) const;
    };

    /**
    * @brief Contains the rating coefficients needed to use formulas B-1 and B-4 of Doc29.
    */
    class Doc29ThrustRating : public Doc29Thrust {
    public:
        /**
        * @brief Data struct for the coefficients
        */
        struct Coefficients {
            double E = 0.0;
            double F = 0.0;
            double Ga = 0.0;
            double Gb = 0.0;
            double H = 0.0;
        };

        Doc29ThrustRating() = default;

        GrapeMap<Rating, Coefficients> Coeffs;

        /**
        * @return Iterator to the beginning of the GrapeMap mapping a Rating to Coefficients.
        */
        [[nodiscard]] auto begin() { return Coeffs.begin(); }

        /**
        * @return Iterator to the end of the GrapeMap mapping a Rating to Coefficients.
        */
        [[nodiscard]] auto end() { return Coeffs.end(); }

        /**
        * @return Const iterator to the beginning of the GrapeMap mapping a Rating to Coefficients.
        */
        [[nodiscard]] auto begin() const { return Coeffs.begin(); }

        /**
        * @return Const iterator to the end of the GrapeMap mapping a Rating to Coefficients.
        */
        [[nodiscard]] auto end() const { return Coeffs.end(); }

        /**
        * @return The Doc29Thrust::Type.
        */
        [[nodiscard]] Type type() const override { return Type::Rating; }

        /**
        * @return The corrected net thrust per engine.
        */
        [[nodiscard]] double calculate(Rating ThrustRating, double CalibratedAirspeed, double Altitude, double EngineBreakpointTemperature, const Atmosphere& Atm) const override;

        /**
        * @brief Check if coefficients for ThrustRating are defined and thrust can be calculated for that rating.
        */
        [[nodiscard]] bool isRatingSet(Rating ThrustRating) const override { return Coeffs.contains(ThrustRating); }

        // Visitor Pattern
        void accept(Doc29ThrustVisitor& Vis) override;
        void accept(Doc29ThrustVisitor& Vis) const override;
    private:
        /**
        * @brief Formula B-1 of Doc29.
        */
        [[nodiscard]] double thrust(Rating ThrustRating, double CalibratedAirspeed, double Altitude, double Temperature) const;

        /**
        * @brief Formula B-4 of Doc29.
        */
        [[nodiscard]] double thrustHighTemperature(Rating ThrustRating, double CalibratedAirspeed, double Temperature, double EngineBreakpointTemperature) const;
    };

    /**
    * @brief Contains the rating coefficients needed to use formula B-5 of Doc29.
    */
    class Doc29ThrustRatingPropeller : public Doc29Thrust {
    public:
        /**
        * @brief Data struct for the coefficients
        */
        struct Coefficients {
            Coefficients() = default;
            Coefficients(double PeIn, double PpIn) : Pe(PeIn), Pp(PpIn) {}

            double Pe = 1.0;
            double Pp = 1000.0;

            /**
            * @brief Throwing set method for the #Pe. Throws if PeIn not in ]0.0, 1.0].
            */
            void setEfficiency(double PeIn);

            /**
            * @brief Throwing set method for the #Pp. Throws if PpIn not in ]0.0, inf].
            */
            void setPower(double PpIn);
        };

        Doc29ThrustRatingPropeller() = default;


        GrapeMap<Rating, Coefficients> Coeffs;

        /**
        * @return Iterator to the beginning of #Coeffs mapping a Rating to Coefficients.
        */
        [[nodiscard]] auto begin() { return Coeffs.begin(); }

        /**
        * @return Iterator to the end of #Coeffs mapping a Rating to Coefficients.
        */
        [[nodiscard]] auto end() { return Coeffs.end(); }

        /**
        * @return Const iterator to the beginning of #Coeffs mapping a Rating to Coefficients.
        */
        [[nodiscard]] auto begin() const { return Coeffs.begin(); }

        /**
        * @return Iterator to the end of #Coeffs mapping a Rating to Coefficients.
        */
        [[nodiscard]] auto end() const { return Coeffs.end(); }

        /**
        * @return The Doc29Thrust::Type.
        */
        [[nodiscard]] Type type() const override { return Type::RatingPropeller; }

        /**
        * @brief Throwing function to add coefficients to #Coeffs.
        * @return The added coefficients for ThrustRating and true or the already existing coefficients and false.
        *
        * Throws if ThrustRating is not of type MaximumTakeoff or MaximumClimb.
        */
        std::pair<Coefficients&, bool> addCoefficients(Rating ThrustRating, double Efficiency, double Power);

        /**
        * @brief Throwing function to add coefficients to #Coeffs.
        * @return The added coefficients for ThrustRating and true or the already existing coefficients and false.
        *
        * Throws if ThrustRating is not of type MaximumTakeoff or MaximumClimb.
        */
        std::pair<Coefficients&, bool> addCoefficients(Rating ThrustRating, const Coefficients& CoeffsIn);

        /**
        * @brief Calculate corrected net thrust per engine with formula B-5 of Doc29.
        */
        [[nodiscard]] double calculate(Rating ThrustRating, double CalibratedAirspeed, double Altitude, double EngineBreakpointTemperature, const Atmosphere& Atm) const override;

        /**
        * @brief Check if coefficients for ThrustRating are defined and thrust can be calculated for that rating.
        */
        [[nodiscard]] bool isRatingSet(Rating ThrustRating) const override { return Coeffs.contains(ThrustRating); }

        // Visitor Pattern
        void accept(Doc29ThrustVisitor& Vis) override;
        void accept(Doc29ThrustVisitor& Vis) const override;
    };

    struct Doc29ThrustVisitor {
        virtual void visitDoc29Thrust(Doc29Thrust& Thrust) {}
        virtual void visitDoc29ThrustRating(Doc29ThrustRating& Thrust) {}
        virtual void visitDoc29ThrustPropeller(Doc29ThrustRatingPropeller& Thrust) {}
        virtual void visitDoc29Thrust(const Doc29Thrust& Thrust) {}
        virtual void visitDoc29ThrustRating(const Doc29ThrustRating& Thrust) {}
        virtual void visitDoc29ThrustPropeller(const Doc29ThrustRatingPropeller& Thrust) {}
        virtual ~Doc29ThrustVisitor() = default;
    };
}
