// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "RouteOutput.h"
#include "Base/BaseModels.h"

namespace GRAPE {
    class Airport;
    class Runway;
    struct RouteVisitor;
    struct RouteTypeVisitor;

    /**
    * @file Route.h
    *
    * A route belongs to a runway. The class structure is as follows:
    *         Route
    *        /     \
    *		/       \
    *  RouteType  RouteOp  <- virtual inheritance
    *       \       /
    *		 \     /
    * 	   RouteOpType
    */

    /**
    * @brief Pure virtual class describing a route. The possible types are stores in the #Type enum.
    */
    class Route {
    public:
        enum class Type {
            Simple = 0,
            Vectors,
            Rnp,
        };
        static constexpr EnumStrings<Type> Types{ "Simple", "Vectors", "Rnp" };

        Route(const Runway& RunwayIn, std::string_view Name);
        Route(const Route&) = delete;
        Route(Route&&) = delete;
        Route& operator=(const Route&) = delete;
        Route& operator=(Route&&) = delete;
        virtual ~Route() = default;

        // Parameters
        std::string Name;

        /**
        * @brief The Route type.
        */
        [[nodiscard]] virtual Type type() const noexcept = 0;

        /**
        * @return #OperationType::Arrival for a RouteArrival and #OperationType::Departure for a RouteDeparture.
        */
        [[nodiscard]] virtual OperationType operationType() const noexcept = 0;

        /**
        * @return The Airport which owns the Runway which owns this Route.
        */
        [[nodiscard]] const Airport& parentAirport() const;

        /**
        * @return The Runway which owns this Route.
        */
        [[nodiscard]] const Runway& parentRunway() const;

        // Visitor pattern
        virtual void accept(RouteVisitor& Vis) = 0;
        virtual void accept(RouteVisitor& Vis) const = 0;

        virtual void accept(RouteTypeVisitor& Vis) = 0;
        virtual void accept(RouteTypeVisitor& Vis) const = 0;
    protected:
        std::reference_wrapper<const Runway> m_Runway; ///< Runway which owns this route.
    };

    /**
    * @brief A simple route is defined as a sequence of longitude latitude points.
    */
    class RouteTypeSimple : public virtual Route {
    public:
        /**
        * @brief Data structure for a single point.
        */
        struct Point {
            Point() = default;
            Point(double Lon, double Lat) noexcept : Longitude(Lon), Latitude(Lat) {}
            double Longitude = 0.0, Latitude = 0.0;
            bool operator==(const Point&) const noexcept = default;
        };

        RouteTypeSimple(const Runway&, std::string_view) {}
        virtual ~RouteTypeSimple() override = default;

        [[nodiscard]] Type type() const noexcept override { return Type::Simple; }

        [[nodiscard]] const auto& points() const noexcept { return m_Points; }
        [[nodiscard]] auto begin() const noexcept { return m_Points.begin(); }
        [[nodiscard]] auto end() const noexcept { return m_Points.end(); }

        [[nodiscard]] auto begin() noexcept { return m_Points.begin(); }
        [[nodiscard]] auto end() noexcept { return m_Points.end(); }

        /**
        * @brief Add a point to the end of the points container.
        *
        * If the route is empty, adds 0.0, 0.0.
        * Otherwise adds a copy of the last point.
        */
        void addPoint() noexcept;

        /**
        * @brief Add the point to the end of the container.
        *
        * ASSERT that Longitude in [-180.0, 180.0].
        * ASSERT that Latitude in [-90.0, 90.0].
        */
        void addPoint(double Longitude, double Latitude) noexcept;

        /**
        * @brief Inserts a copy of the point at index, before index. If Index = size(), insets a copy of the last point at the end.
        *
        * ASSERT Index <= size().
        */
        void insertPoint(std::size_t Index) noexcept;

        /**
        * @brief Deletes the point at position Index.
        *
        * ASSERT Index < size().
        */
        void deletePoint(std::size_t Index) noexcept;

        /**
        * @brief Delete the last point.
        *
        * ASSERT !empty().
        */
        void deletePoint() noexcept;

        /**
        * @brief Delete all points.
        */
        void clear() noexcept;

        /**
        * @brief Throwing version of addPoint(double, double).
        *
        * Throws is Longitude not in [-180.0, 180.0].
        * Throws is Latitude not in [-90.0, 90.0].
        */
        void addPointE(double Longitude, double Latitude);

        /**
        * @return True if the simple route has no points.
        */
        [[nodiscard]] bool empty() const noexcept { return m_Points.empty(); }

        /**
        * @return The number of points in the route.
        */
        [[nodiscard]] std::size_t size() const noexcept { return m_Points.size(); }

        // Visitor pattern
        void accept(RouteTypeVisitor& Vis) override;
        void accept(RouteTypeVisitor& Vis) const override;
    protected:
        /**
        * Arrivals: Point 0, 1, 2, 3, 4, ... Threshold -> RouteOutput created at threshold and container reverse iterated
        * Departures: Threshold, Point 0, 1, 2, 3, 4, ... -> RouteOutput created at threshold and container forward iterated
        */
        std::vector<Point> m_Points;
    };

    /**
    * @brief A vector route is defined as a sequence of vectors.
    */
    class RouteTypeVectors : public virtual Route {
    public:
        /**
        * @brief A straight vector is simply a distance.
        */
        struct Straight {
            Straight() = default;
            explicit Straight(double Dist) noexcept : Distance(Dist) {}
            double Distance = 10.0;
        };

        /**
        * @brief A turn vector is defined by a turn radius and the final heading after the turn.
        */
        struct Turn {
            enum class Direction {
                Left = 0,
                Right,
            };
            static constexpr EnumStrings<Direction> Directions{ "Left", "Right" };

            Turn() = default;
            Turn(double TurnRad, double HeadingChangeIn, Direction TurnDir) noexcept : TurnRadius(TurnRad), HeadingChange(HeadingChangeIn), TurnDirection(TurnDir) {}
            double TurnRadius = 10.0;
            double HeadingChange = 0.0;
            Direction TurnDirection = Direction::Left;
        };

        typedef std::variant<Straight, Turn> Vector;

        enum class VectorType {
            Straight = 0,
            Turn
        };
        static constexpr EnumStrings<VectorType> VectorTypes{ "Straight", "Turn" };

        /**
        * @brief Helper struct to pe passed to std::visit to obtain the string representation.
        */
        struct VisitorVectorTypeString {
            std::string operator()(const Straight&) const noexcept { return VectorTypes.toString(VectorType::Straight); }
            std::string operator()(const Turn&) const noexcept { return VectorTypes.toString(VectorType::Turn); }
        };

        RouteTypeVectors(const Runway&, std::string_view) {}
        virtual ~RouteTypeVectors() override = default;

        [[nodiscard]] Type type() const noexcept override { return Type::Vectors; }

        [[nodiscard]] const auto& vectors() const noexcept { return m_Vectors; }
        [[nodiscard]] auto begin() const noexcept { return m_Vectors.begin(); }
        [[nodiscard]] auto end() const noexcept { return m_Vectors.end(); }

        auto begin() noexcept { return m_Vectors.begin(); }
        auto end() noexcept { return m_Vectors.end(); }

        /**
        * @brief Calls addStraight().
        */
        void addVector() noexcept;

        /**
        * @brief Add a Straight vector to the end of this route.
        *
        * If empty(), adds a default constructed Straight vector.
        * If the last vector is of type Straight, copy it.
        * If the last vector if of type Turn, adds a default constructed Straight vector.
        */
        void addStraight() noexcept;

        /**
        * @brief Add a Turn vector to the end of this route.
        *
        * If empty(), adds a default constructed Turn vector.
        * If the last vector is of type Straight, adds a default constructed Turn vector.
        * If the last vector if of type Turn, copy it.
        */
        void addTurn() noexcept;

        /**
        * @brief Add a Straight vector to the end.
        *
        * ASSERT that distance is in ]0.0, inf].
        */
        void addStraight(double GroundDistance) noexcept;

        /**
        * @brief Add a Turn vector to the end.
        *
        * ASSERT that TurnRadius is in ]0.0, inf].
        * ASSERT that HeadingChange is in [0.0, inf].
        */
        void addTurn(double TurnRadius, double HeadingChange, Turn::Direction TurnDir) noexcept;

        /**
        * @brief Call insertStraight(std::size_t).
        */
        void insertVector(std::size_t Index) noexcept;

        /**
        * @brief Inserts a Straight vector before Index or at the end if Index = size().
        *
        * ASSERT Index <= size().
        *
        * If vector at position Index is of Straight type copy it, otherwise creates a default constructed Straight vector.
        */
        void insertStraight(std::size_t Index) noexcept;

        /**
        * @brief Inserts a Turn vector before Index or at the end if Index = size().
        *
        * ASSERT Index <= size().
        *
        * If vector at position Index is of Turn type copy it, otherwise creates a default constructed Turn vector.
        */
        void insertTurn(std::size_t Index) noexcept;

        /**
        * @brief Changes the vector at position Index to Straight.
        * @return True if a change was performed, false otherwise.
        *
        * ASSERT Index < size().
        *
        * If vector at position Index was already of type Straight, does nothing.
        * Otherwise, replaces the Turn vector at position Index with a default constructed Straight vector.
        */
        bool setStraight(std::size_t Index) noexcept;

        /**
        * @brief Changes the vector at position Index to Turn.
        * @return True if a change was performed, false otherwise.
        *
        * ASSERT Index < size().
        *
        * If vector at position Index was already of type Turn, does nothing.
        * Otherwise, replaces the Straight vector at position Index with a default constructed Turn vector.
        */
        bool setTurn(std::size_t Index) noexcept;

        /**
        * @brief Deletes the vector at position Index.
        *
        * ASSERT Index < size().
        */
        void deleteVector(std::size_t Index) noexcept;

        /**
        * @brief Deletes the last vector.
        *
        * ASSERT !empty().
        */
        void deleteVector() noexcept;

        /**
        * @brief Delete all vectors.
        */
        void clear() noexcept;

        /**
        * @brief Throwing version of addStraight(double).
        *
        * Throws if GroundDistance not in ]0.0, inf].
        */
        void addStraightE(double GroundDistance);

        /**
        * @brief Throwing version of addTurn(double, double).
        *
        * Throws if TurnRadius not in ]0.0, inf].
        * Throws if HeadingChange not in [0.0, inf].
        */
        void addTurnE(double TurnRadius, double HeadingChange, Turn::Direction TurnDir);

        /**
        * @return True if the vector route has no vectors.
        */
        [[nodiscard]] bool empty() const noexcept { return m_Vectors.empty(); }

        /**
        * @return The number of vectors in the vector route.
        */
        [[nodiscard]] std::size_t size() const noexcept { return m_Vectors.size(); }

        // Visitor pattern
        void accept(RouteTypeVisitor& Vis) override;
        void accept(RouteTypeVisitor& Vis) const override;
    protected:
        /**
        * Arrivals & Departures: Threshold, Vector 0, 1, 2, 3, 4, ... -> RouteOutput created at threshold and container forward iterated
        */
        std::vector<Vector> m_Vectors;
    };

    /**
    * @brief A RNP route is defined as a sequence of RNP steps.
    */
    class RouteTypeRnp : public virtual Route {
    public:
        /**
        * @brief A track to a fix step is simply defined by the location of the fix.
        */
        struct TrackToFix {
            TrackToFix() = default;
            TrackToFix(double LongitudeIn, double LatitudeIn) noexcept : Longitude(LongitudeIn), Latitude(LatitudeIn) {}
            double Longitude = 0.0;
            double Latitude = 0.0;
        };

        /**
        * @brief A radius to fix step is defined by the location of the turn center and the location of the final point. The initial fix is defined by the previous step. This step type can therefore not be the first step.
        */
        struct RadiusToFix {
            RadiusToFix() = default;
            RadiusToFix(double LongitudeIn, double LatitudeIn, double CenterLongitudeIn, double CenterLatitudeIn) noexcept : Longitude(LongitudeIn), Latitude(LatitudeIn), CenterLongitude(CenterLongitudeIn), CenterLatitude(CenterLatitudeIn) {}
            double Longitude = 0.0;
            double Latitude = 0.0;
            double CenterLongitude = 0.0;
            double CenterLatitude = 0.0;
        };
        typedef std::variant<TrackToFix, RadiusToFix> RnpStep;

        enum class StepType {
            TrackToFix,
            RadiusToFix,
        };
        static constexpr EnumStrings<StepType> StepTypes{ "Track to Fix", "Radius to Fix" };

        /**
        * @brief Helper struct to pe passed to std::visit to obtain the string representation.
        */
        struct VisitorRnpStepTypeString {
            std::string operator()(const TrackToFix&) const { return StepTypes.toString(StepType::TrackToFix); }
            std::string operator()(const RadiusToFix&) const { return StepTypes.toString(StepType::RadiusToFix); }
        };

        RouteTypeRnp(const Runway&, std::string_view) noexcept {}
        virtual ~RouteTypeRnp() override = default;

        [[nodiscard]] Type type() const noexcept override { return Type::Rnp; }

        [[nodiscard]] const auto& rnpSteps() const noexcept { return m_RnpSteps; }
        [[nodiscard]] auto begin() const noexcept { return m_RnpSteps.begin(); }
        [[nodiscard]] auto end() const noexcept { return m_RnpSteps.end(); }

        auto begin() noexcept { return m_RnpSteps.begin(); }
        auto end() noexcept { return m_RnpSteps.end(); }

        /**
        * @brief Calls addTrackToFix().
        */
        void addStep() noexcept;

        /**
        * @brief Add a TrackToFix step to the end.
        *
        * If empty, adds a default constructed TrackToFix step.
        * If the last step is of TrackToFix type, copy it.
        * If the last step is not of TrackToFix type, adds a default constructed TrackToFix step.
        */
        void addTrackToFix() noexcept;

        /**
        * @brief Add a RadiusToFix step to the end.
        *
        * ASSERT !empty().
        *
        * If empty, adds a default constructed RadiusToFix step.
        * If the last step is of RadiusToFix type, copy it.
        * If the last step is not of RadiusToFix type, adds a default constructed RadiusToFix step.
        */
        void addRadiusToFix() noexcept;

        /**
        * @brief Add a TrackToFix step to the end.
        *
        * ASSERT Longitude in [-180.0, 180.0].
        * ASSERT Latitude in [-90.0, 90.0].
        */
        void addTrackToFix(double Longitude, double Latitude) noexcept;

        /**
        * @brief Add a RadiusToFix step to the end.
        *
        * ASSERT !empty().
        * ASSERT Longitude in [-180.0, 180.0].
        * ASSERT Latitude in [-90.0, 90.0].
        * ASSERT CenterLongitude in [-180.0, 180.0].
        * ASSERT CenterLatitude in [-90.0, 90.0].
        */
        void addRadiusToFix(double Longitude, double Latitude, double CenterLongitude, double CenterLatitude) noexcept;

        /**
        * @brief Calls insertTrackToFix(std::size_t).
        */
        void insertStep(std::size_t Index) noexcept;

        /**
        * @brief Inserts a TrackToFix step before Index or at the end if Index = size().
        *
        * ASSERT Index <= size().
        *
        * If step at Index is of type TrackToFix, copy it.
        * If step at Index is not of type TrackToFix, create a default constructed TrackToFix step.
        * If Index = size(), create a default constructed TrackToFix step.
        */
        void insertTrackToFix(std::size_t Index) noexcept;

        /**
        * @brief Inserts a RadiusToFix step before Index or at the end if Index = size().
        *
        * ASSERT Index <= size().
        * ASSERT Index != 0.
        *
        * If step at Index is of type RadiusToFix, copy it.
        * If step at Index is not of type RadiusToFix, create a default constructed RadiusToFix step.
        * If Index = size(), create a default constructed RadiusToFix step.
        */
        void insertRadiusToFix(std::size_t Index) noexcept;

        /**
        * @brief Changes the step at position Index to TrackToFix.
        * @return True if a change was performed, false otherwise.
        *
        * ASSERT Index < size().
        *
        * If step at Index was already of type TrackToFix, does nothing.
        * If step at Index was of type RadiusToFix, copy the step Longitude and Latitude.
        */
        bool setTrackToFix(std::size_t Index) noexcept;

        /**
        * @brief Changes the step at position Index to RadiusToFix.
        * @return True if a change was performed, false otherwise.
        *
        * ASSERT Index < size().
        * ASSERT Index != 0.
        *
        * If step at Index was already of type RadiusToFix, does nothing.
        * If step at Index was of type TrackToFix, copy the step Longitude and Latitude to both the fix and center points.
        */
        bool setRadiusToFix(std::size_t Index) noexcept;

        /**
        * @brief Deletes the step at position Index.
        *
        * ASSERT Index < size().
        */
        void deleteStep(std::size_t Index) noexcept;

        /**
        * @brief Delete the last step, calls deleteStep(size() - 1).
        *
        * ASSERT !empty().
        */
        void deleteStep() noexcept;

        /**
        * @brief Delete all steps.
        */
        void clear() noexcept;

        /**
        * @brief Throwing version of addTrackToFix(double, double).
        *
        * Throws if Longitude not in [-180.0, 180.0].
        * Throws if Latitude not in [-90.0, 90.0].
        */
        void addTrackToFixE(double Longitude, double Latitude);

        /**
        * @brief Throwing version of addRadiusToFix(double, double, double, double).
        *
        * Throws if route is empty.
        * Throws if Longitude not in [-180.0, 180.0].
        * Throws if Latitude not in [-90.0, 90.0].
        * Throws if CenterLongitude not in [-180.0, 180.0].
        * Throws if CenterLatitude not in [-90.0, 90.0].
        */
        void addRadiusToFixE(double Longitude, double Latitude, double CenterLongitude, double CenterLatitude);

        /**
        * @return True if the RNP route has no steps.
        */
        [[nodiscard]] bool empty() const noexcept { return m_RnpSteps.empty(); }

        /**
        * @return The number of steps in the RNP route.
        */
        [[nodiscard]] std::size_t size() const noexcept { return m_RnpSteps.size(); }

        // Visitor pattern
        void accept(RouteTypeVisitor& Vis) override;
        void accept(RouteTypeVisitor& Vis) const override;
    protected:
        /**
        * Arrivals: Step 0, 1, 2, 3, 4, ... Threshold -> RouteOutput created at threshold and container reverse iterated
        * Departures: Threshold, Step 0, 1, 2, 3, 4, ... -> RouteOutput created at threshold and container forward iterated
        */
        std::vector<RnpStep> m_RnpSteps;
    };

    template <OperationType OpType>
    class RouteOp : public virtual Route {
    public:
        // Constructors & Destructor (Copy and Move implicitly deleted)
        RouteOp(const Runway&, std::string_view) {}
        virtual ~RouteOp() override = default;

        // Access data
        [[nodiscard]] OperationType operationType() const noexcept override { return OpType; }
    };
    typedef RouteOp<OperationType::Arrival> RouteArrival;
    typedef RouteOp<OperationType::Departure> RouteDeparture;

    template <OperationType OpType>
    class RouteSimple : public RouteOp<OpType>, public RouteTypeSimple {
    public:
        // Constructors & Destructor (Copy and Move implicitly deleted)
        RouteSimple(const Runway& RunwayIn, std::string_view Name) : Route(RunwayIn, Name), RouteOp<OpType>(RunwayIn, Name), RouteTypeSimple(RunwayIn, Name) {}
        virtual ~RouteSimple() override = default;

        // Visitor pattern
        void accept(RouteVisitor& Vis) override;
        void accept(RouteVisitor& Vis) const override;

        friend class RouteCalculator;
    private:
        using RouteTypeSimple::accept; // Hides overload warning of accept method
    };
    typedef RouteSimple<OperationType::Arrival> RouteArrivalSimple;
    typedef RouteSimple<OperationType::Departure> RouteDepartureSimple;

    template <OperationType OpType>
    class RouteVectors : public RouteOp<OpType>, public RouteTypeVectors {
    public:
        RouteVectors(const Runway& RunwayIn, const std::string& Name) : Route(RunwayIn, Name), RouteOp<OpType>(RunwayIn, Name), RouteTypeVectors(RunwayIn, Name) {}
        virtual ~RouteVectors() override = default;

        // Visitor pattern
        void accept(RouteVisitor& Vis) override;
        void accept(RouteVisitor& Vis) const override;

        friend class RouteCalculator;
    private:
        using RouteTypeVectors::accept;  // Hides overload warning of accept method
    };
    typedef RouteVectors<OperationType::Arrival> RouteArrivalVectors;
    typedef RouteVectors<OperationType::Departure> RouteDepartureVectors;

    template <OperationType OpType>
    class RouteRnp : public RouteOp<OpType>, public RouteTypeRnp {
    public:
        RouteRnp(const Runway& RunwayIn, std::string_view Name) : Route(RunwayIn, Name), RouteOp<OpType>(RunwayIn, Name), RouteTypeRnp(RunwayIn, Name) {}
        virtual ~RouteRnp() override = default;

        // Visitor pattern
        void accept(RouteVisitor& Vis) override;
        void accept(RouteVisitor& Vis) const override;

        friend class RouteCalculator;
    private:
        using RouteTypeRnp::accept;  // Hides overload warning of accept method
    };
    typedef RouteRnp<OperationType::Arrival> RouteArrivalRnp;
    typedef RouteRnp<OperationType::Departure> RouteDepartureRnp;

    struct RouteVisitor {
        virtual void visitArrivalSimple(RouteArrivalSimple& Rte) {}
        virtual void visitDepartureSimple(RouteDepartureSimple& Rte) {}
        virtual void visitArrivalVectors(RouteArrivalVectors& Rte) {}
        virtual void visitDepartureVectors(RouteDepartureVectors& Rte) {}
        virtual void visitArrivalRnp(RouteArrivalRnp& Rte) {}
        virtual void visitDepartureRnp(RouteDepartureRnp& Rte) {}
        virtual void visitArrivalSimple(const RouteArrivalSimple& Rte) {}
        virtual void visitDepartureSimple(const RouteDepartureSimple& Rte) {}
        virtual void visitArrivalVectors(const RouteArrivalVectors& Rte) {}
        virtual void visitDepartureVectors(const RouteDepartureVectors& Rte) {}
        virtual void visitArrivalRnp(const RouteArrivalRnp& Rte) {}
        virtual void visitDepartureRnp(const RouteDepartureRnp& Rte) {}
        virtual ~RouteVisitor() = default;
    };

    struct RouteTypeVisitor {
        virtual void visitSimple(RouteTypeSimple& Rte) {}
        virtual void visitVectors(RouteTypeVectors& Rte) {}
        virtual void visitRnp(RouteTypeRnp& Rte) {}
        virtual void visitSimple(const RouteTypeSimple& Rte) {}
        virtual void visitVectors(const RouteTypeVectors& Rte) {}
        virtual void visitRnp(const RouteTypeRnp& Rte) {}
        virtual ~RouteTypeVisitor() = default;
    };
}
