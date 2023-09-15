// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Base/BaseModels.h"

namespace GRAPE {
    struct Doc29AerodynamicCoefficients;
    class Doc29Performance;
    struct Doc29ProfileVisitor;
    struct Doc29ProfileArrivalVisitor;
    struct Doc29ProfileDepartureVisitor;

    /**
    * @brief Base class for all Doc29 profiles. A Doc29Profile belongs to a Doc29Performance. It can be an arrival or departure profile. The possible types are stores in the #Type enum.
    */
    class Doc29Profile {
    public:
        enum class Type {
            Points = 0, Procedural,
        };

        static constexpr EnumStrings<Type> Types{ "Points", "Procedural" };

        Doc29Profile(Doc29Performance& Doc29AircraftIn, std::string_view Name) : Name(Name), p_Doc29Performance(Doc29AircraftIn) {}
        Doc29Profile(const Doc29Profile&) = delete;
        Doc29Profile(Doc29Profile&&) = delete;
        Doc29Profile& operator=(const Doc29Profile&) = delete;
        Doc29Profile& operator=(Doc29Profile&&) = delete;

        virtual ~Doc29Profile() = default;

        // Data
        std::string Name;

        /**
        * @return The OperationType.
        */
        [[nodiscard]] virtual OperationType operationType() const = 0;

        /**
        * @return The profile type (points or procedural).
        */
        [[nodiscard]] virtual Type type() const = 0;

        /**
        * @return The Doc29Performance which owns this Doc29Profile
        */
        [[nodiscard]] Doc29Performance& parentDoc29Performance() const { return p_Doc29Performance; }

        // Visitor Pattern
        virtual void accept(Doc29ProfileVisitor& Vis) = 0;
        virtual void accept(Doc29ProfileVisitor& Vis) const = 0;

    private:
        std::reference_wrapper<Doc29Performance> p_Doc29Performance;
    };

    /**
    * @brief Base class for all Doc29 arrival profiles.
    */
    class Doc29ProfileArrival : public Doc29Profile {
    public:
        // Constructors & Destructor (Copy and Move implicitly deleted)
        Doc29ProfileArrival(Doc29Performance& Doc29AircraftIn, std::string_view NameIn) : Doc29Profile(Doc29AircraftIn, NameIn) {}
        virtual ~Doc29ProfileArrival() override = default;

        /**
        * @return The OperationType.
        */
        [[nodiscard]] OperationType operationType() const override { return OperationType::Arrival; }

        // Visitor Pattern
        using Doc29Profile::accept;
        virtual void accept(Doc29ProfileArrivalVisitor& Vis) = 0;
        virtual void accept(Doc29ProfileArrivalVisitor& Vis) const = 0;
    };

    /**
    * @brief Base class for all Doc29 departure profiles.
    */
    class Doc29ProfileDeparture : public Doc29Profile {
    public:
        // Constructors & Destructor (Copy and Move implicitly deleted)
        Doc29ProfileDeparture(Doc29Performance& Doc29AircraftIn, std::string_view NameIn) : Doc29Profile(Doc29AircraftIn, NameIn) {}
        virtual ~Doc29ProfileDeparture() override = default;

        /**
        * @return The OperationType.
        */
        [[nodiscard]] OperationType operationType() const override { return OperationType::Departure; }

        // Visitor Pattern
        using Doc29Profile::accept;
        virtual void accept(Doc29ProfileDepartureVisitor& Vis) = 0;
        virtual void accept(Doc29ProfileDepartureVisitor& Vis) const = 0;
    };

    /**
    * @brief An arrival profile defined by a sequence of points.
    */
    class Doc29ProfileArrivalPoints : public Doc29ProfileArrival {
    public:
        /**
        * @brief Data structure that stores all the variables needed at single profile point.
        */
        struct Point {
            double AltitudeAfe = Constants::NaN;
            double TrueAirspeed = Constants::NaN;
            double CorrNetThrustPerEng = Constants::NaN;
        };

        /**
        * @brief Construct an empty profile.
        * @param Doc29PerformanceIn The parent Doc29Performance.
        * @param NameIn The profile name.
        */
        Doc29ProfileArrivalPoints(Doc29Performance& Doc29PerformanceIn, std::string_view NameIn);
        virtual ~Doc29ProfileArrivalPoints() override = default;

        /**
        * @return The Doc29Profile::Type.
        */
        [[nodiscard]] Type type() const override { return Type::Points; }

        /**
        * @return The points container.
        */
        [[nodiscard]] const auto& points() const { return m_Points; }

        /**
        * @return Const iterator to the beginning of container.
        */
        [[nodiscard]] auto begin() const { return m_Points.begin(); }

        /**
        * @return Const iterator to the end of container.
        */
        [[nodiscard]] auto end() const { return m_Points.end(); }

        /**
        * @return Iterator to the beginning of the container.
        */
        auto begin() { return m_Points.begin(); }

        /**
        * @return Iterator to the end of the container.
        */
        auto end() { return m_Points.end(); }

        /**
        * @return Reverse const iterator to the beginning of the container.
        */
        [[nodiscard]] auto rbegin() const { return m_Points.rbegin(); }

        /**
        * @return Reverse const iterator to the end of the container.
        */
        [[nodiscard]] auto rend() const { return m_Points.rend(); }

        /**
        * @return Reverse iterator to the beginning of the container.
        */
        auto rbegin() { return m_Points.rbegin(); }

        /**
        * @return Reverse iterator to the end of the container.
        */
        auto rend() { return m_Points.rend(); }

        /**
        * @brief Add a default point if empty or a point to the beginning of the profile at the first cumulative ground distance - 1 meter.
        */
        void addPoint() noexcept;

        /**
        * @brief Add a point to the profile.
        *
        * ASSERT TrueAirspeed in [0, inf].
        * ASSERT CorrNetThrustPerEng in ]0, inf].
        */
        void addPoint(double CumulativeGroundDistance, double AltitudeAfe, double TrueAirspeed, double CorrNetThrustPerEng) noexcept;

        /**
        * @brief Update cumulative ground distance of the point at Index, eventually moving the points position.
        *
        * ASSERT Index < size()
        */
        void updateCumulativeGroundDistance(std::size_t Index, double NewCumulativeGroundDistance) noexcept;

        /**
        * @brief Insert a point in the profile at Index.
        *
        * ASSERT Index <= size().
        *
        * Depending on Index, inserts at the beginning, end, or the middle of the profile.
        */
        void insertPoint(std::size_t Index) noexcept;

        /**
        * @brief Delete point at Index.
        *
        * ASSERT Index < size().
        */
        void deletePoint(std::size_t Index) noexcept;

        /**
        * @brief Delete the last point.
        *
        * Does nothing if profile is empty.
        */
        void deletePoint() noexcept;

        /**
        * @brief Delete all points.
        */
        void clear() noexcept;

        /**
        * @brief Throwing version of addPoint(double, double, double, double).
        *
        * Throws if TrueAirspeed not in [0, inf].
        * Throws if CorrNetThrustPerEng not in ]0, inf].
        */
        void addPointE(double CumulativeGroundDistance, double AltitudeAfe, double TrueAirspeed, double CorrNetThrustPerEng);

        /**
        * @return True if only the landing threshold point exists.
        */
        [[nodiscard]] bool empty() const { return m_Points.empty(); }

        /**
        * @return The number of points in the profile.
        */
        [[nodiscard]] std::size_t size() const { return m_Points.size(); }

        // Visitor pattern
        void accept(Doc29ProfileVisitor& Vis) override;
        void accept(Doc29ProfileVisitor& Vis) const override;
        void accept(Doc29ProfileArrivalVisitor& Vis) override;
        void accept(Doc29ProfileArrivalVisitor& Vis) const override;

    private:
        std::map<double, Point> m_Points;
    };

    /**
    * @brief A departure profile defined by a sequence of points.
    */
    class Doc29ProfileDeparturePoints : public Doc29ProfileDeparture {
    public:
        /**
        * @brief Data structure that stores all the variables needed at single profile point.
        */
        struct Point {
            double AltitudeAfe = Constants::NaN;
            double TrueAirspeed = Constants::NaN;
            double CorrNetThrustPerEng = Constants::NaN;
        };

        /**
        * @brief Construct an empty profile.
        * @param Doc29PerformanceIn The parent Doc29Performance.
        * @param NameIn The profile name.
        */
        Doc29ProfileDeparturePoints(Doc29Performance& Doc29PerformanceIn, std::string_view NameIn);
        virtual ~Doc29ProfileDeparturePoints() override = default;

        /**
        * @return The Doc29Profile::Type.
        */
        [[nodiscard]] Type type() const override { return Type::Points; }

        /**
        * @return The GrapeMap mapping cumulative ground distances to a Point.
        */
        [[nodiscard]] const auto& points() const { return m_Points; }

        /**
        * @return Const iterator to the beginning of the GrapeMap cumulative ground distances to a Point.
        */
        [[nodiscard]] auto begin() const { return m_Points.begin(); }

        /**
        * @return Const iterator to the end of the GrapeMap cumulative ground distances to a Point.
        */
        [[nodiscard]] auto end() const { return m_Points.end(); }

        /**
        * @return Iterator to the beginning of the GrapeMap cumulative ground distances to a Point.
        */
        auto begin() { return m_Points.begin(); }

        /**
        * @return Iterator to the end of the GrapeMap cumulative ground distances to a Point.
        */
        auto end() { return m_Points.end(); }

        /**
        * @return Reverse const iterator to the beginning of the container.
        */
        [[nodiscard]] auto rbegin() const { return m_Points.rbegin(); }

        /**
        * @return Reverse const iterator to the end of the container.
        */
        [[nodiscard]] auto rend() const { return m_Points.rend(); }

        /**
        * @return Reverse iterator to the beginning of the container.
        */
        auto rbegin() { return m_Points.rbegin(); }

        /**
        * @return Reverse iterator to the end of the container.
        */
        auto rend() { return m_Points.rend(); }

        /**
        * @brief Add a default point if empty or a point to the end of the profile at the last cumulative ground distance + 1 meter.
        */
        void addPoint() noexcept;

        /**
        * @brief Add a point to the profile.
        *
        * ASSERT CumulativeGroundDistance in [0, inf].
        * ASSERT TrueAirspeed in [0, inf].
        * ASSERT CorrNetThrustPerEng in ]0, inf].
        */
        void addPoint(double CumulativeGroundDistance, double AltitudeAfe, double TrueAirspeed, double CorrNetThrustPerEng) noexcept;

        /**
        * @brief Update cumulative ground distance of the point at Index, eventually moving the points position.
        *
        * ASSERT Index < size().
        * ASSERT NewCumulativeGroundDistance in [0, inf].
        */
        void updateCumulativeGroundDistance(std::size_t Index, double NewCumulativeGroundDistance) noexcept;

        /**
        * @brief Insert a point in the profile at Index.
        *
        * ASSERT Index < size().
        *
        * Depending on Index, inserts at the begin, middle or end of the profile.
        */
        void insertPoint(std::size_t Index) noexcept;

        /**
        * @brief Delete point at Index.
        *
        * ASSERT Index < size().
        */
        void deletePoint(std::size_t Index) noexcept;

        /**
        * @brief Delete point.
        *
        * Delete the last point of the profile. Does nothing if profile is empty.
        */
        void deletePoint() noexcept;

        /**
        * @brief Delete all points.
        */
        void clear() noexcept;

        /**
        * @brief Throwing version of addPoint(double, double, double, double).
        *
        * Throws if CumulativeGroundDistance not in ]0.0, inf].
        * Throws if TrueAirspeed not in [0.0, inf].
        * Throws if CorrNetThrustPerEng not in ]0.0, inf].
        */
        void addPointE(double CumulativeGroundDistance, double AltitudeAfe, double TrueAirspeed, double CorrNetThrustPerEng);

        /**
        * @return True if the container is empty.
        */
        [[nodiscard]] bool empty() const { return m_Points.empty(); }

        /**
        * @return The number of points in the profile.
        */
        [[nodiscard]] std::size_t size() const { return m_Points.size(); }

        // Visitor pattern
        void accept(Doc29ProfileVisitor& Vis) override;
        void accept(Doc29ProfileVisitor& Vis) const override;
        void accept(Doc29ProfileDepartureVisitor& Vis) override;
        void accept(Doc29ProfileDepartureVisitor& Vis) const override;
    private:
        std::map<double, Point> m_Points;
    };

    /**
    * @brief An arrival profile defined by a sequence of procedural steps.
    */
    class Doc29ProfileArrivalProcedural : public Doc29ProfileArrival {
    public:
        /**
        * @brief Descend and decelerate. Values are user defined, unrealistic values can be given.
        */
        struct DescendDecelerate {
            const Doc29AerodynamicCoefficients* Doc29AerodynamicCoefficients = nullptr;
            double StartAltitudeAfe = Constants::NaN;
            double DescentAngle = Constants::NaN;
            double StartCalibratedAirspeed = Constants::NaN;
        };

        /**
        * @brief Descent at idle thrust with DescentAngle from StartAltitudeAfe and StartCalibratedAirspeed. Thrust calculated for Idle setting.
        */
        struct DescendIdle {
            double StartAltitudeAfe = Constants::NaN;
            double DescentAngle = Constants::NaN;
            double StartCalibratedAirspeed = Constants::NaN;
        };

        /**
        * @brief Maintain the previous altitude and calibrated airspeed for GroundDistance. Values are user defined, unrealistic values can be given.
        */
        struct Level {
            const Doc29AerodynamicCoefficients* Doc29AerodynamicCoefficients = nullptr;
            double GroundDistance = Constants::NaN;
        };

        /**
        * @brief Maintain the previous altitude and decelerate from StartCalibratedAirspeed over GroundDistance. Values are user defined, unrealistic values can be given.
        */
        struct LevelDecelerate {
            const Doc29AerodynamicCoefficients* Doc29AerodynamicCoefficients = nullptr;
            double GroundDistance = Constants::NaN;
            double StartCalibratedAirspeed = Constants::NaN;
        };

        /**
        * @brief Maintain StartAltitudeAfe and StartCalibratedAirspeed over GroundDistance.
        */
        struct LevelIdle {
            double GroundDistance = Constants::NaN;
            double StartCalibratedAirspeed = Constants::NaN;
        };

        /**
        * @brief Descend with DescentAngle to ThresholdCrossingAltitudeAfe. All Doc29ProfileArrivalProcedural have a single DescendLand step.
        */
        struct DescendLand {
            const Doc29AerodynamicCoefficients* Doc29AerodynamicCoefficients = nullptr;
            double DescentAngle = Constants::NaN;
            double ThresholdCrossingAltitudeAfe = Constants::NaN;
            double TouchdownRoll = Constants::NaN;
        };

        /**
        * @brief Decelerate over the landing roll. Step type only allowed after the DescendLand step.
        */
        struct GroundDecelerate {
            double GroundDistance = Constants::NaN;
            double StartCalibratedAirspeed = Constants::NaN;
            double StartThrustPercentage = Constants::NaN;
        };

        typedef std::variant<DescendDecelerate, DescendIdle, Level, LevelDecelerate, LevelIdle, DescendLand, GroundDecelerate> Step;

        enum class StepType {
            DescendDecelerate = 0, DescendIdle, Level, LevelDecelerate, LevelIdle, DescendLand, GroundDecelerate,
        };

        static constexpr EnumStrings<StepType> StepTypes{ "Descend Decelerate", "Descend Idle", "Level", "Level Decelerate", "Level Idle", "Descend Land", "Ground Decelerate" };

        struct VisitorStepTypeString {
            std::string operator()(const DescendDecelerate&) const { return StepTypes.toString(StepType::DescendDecelerate); }
            std::string operator()(const DescendIdle&) const { return StepTypes.toString(StepType::DescendIdle); }
            std::string operator()(const Level&) const { return StepTypes.toString(StepType::Level); }
            std::string operator()(const LevelDecelerate&) const { return StepTypes.toString(StepType::LevelDecelerate); }
            std::string operator()(const LevelIdle&) const { return StepTypes.toString(StepType::LevelIdle); }
            std::string operator()(const DescendLand&) const { return StepTypes.toString(StepType::DescendLand); }
            std::string operator()(const GroundDecelerate&) const { return StepTypes.toString(StepType::GroundDecelerate); }
        };

        /**
        * @brief Construct a profile with an ArrivalStart step and a DescendLand step.
        * @param Doc29PerformanceIn The parent Doc29Performance.
        * @param NameIn The profile name.
        *
        * ASSERT that Doc29PerformanceIn has Doc29AerodynamicCoefficients of type Land.
        */
        Doc29ProfileArrivalProcedural(Doc29Performance& Doc29PerformanceIn, std::string_view NameIn);
        virtual ~Doc29ProfileArrivalProcedural() override;

        /**
        * @return The Doc29Profile::Type.
        */
        [[nodiscard]] Type type() const override { return Type::Procedural; }

        /**
        * @return The single DescendLand step.
        */
        [[nodiscard]] DescendLand& descendLandStep() { return std::get<DescendLand>(m_Steps.at(m_LandIndex)); }

        /**
        * @return The single DescendLand step.
        */
        [[nodiscard]] const DescendLand& descendLandStep() const { return std::get<DescendLand>(m_Steps.at(m_LandIndex)); }

        /**
        * @return The vector containing all the steps.
        */
        [[nodiscard]] const auto& steps() const { return m_Steps; }

        /**
        * @return Const iterator to the beginning of the vector containing all the steps.
        */
        [[nodiscard]] auto begin() const { return m_Steps.begin(); }

        /**
        * @return Const iterator to the end of the vector containing all the steps.
        */
        [[nodiscard]] auto end() const { return m_Steps.end(); }

        /**
        * @return Iterator to the beginning of the vector containing all the steps.
        */
        [[nodiscard]] auto begin() { return m_Steps.begin(); }

        /**
        * @return Iterator to the end of the vector containing all the steps.
        */
        [[nodiscard]] auto end() { return m_Steps.end(); }

        /**
        * @return Const iterator to the beginning of the air steps within the vector.
        */
        [[nodiscard]] auto airStepsBegin() const { return m_Steps.begin(); }

        /**
        * @return Const iterator to the end of the air steps within the vector.
        */
        [[nodiscard]] auto airStepsEnd() const { return std::next(m_Steps.begin(), m_LandIndex); }

        /**
        * @return Iterator to the beginning of the air steps within the vector.
        */
        [[nodiscard]] auto airStepsBegin() { return m_Steps.begin(); }

        /**
        * @return Iterator to the end of the air steps within the vector.
        */
        [[nodiscard]] auto airStepsEnd() { return std::next(m_Steps.begin(), m_LandIndex); }

        /**
        * @return Reverse iterator to the start of the air steps within the vector.
        */
        [[nodiscard]] auto airStepsRBegin() const { return std::next(m_Steps.rbegin(), size() - m_LandIndex); }

        /**
        * @return Reverse iterator to the end of the air steps within the vector.
        */
        [[nodiscard]] auto airStepsREnd() const { return m_Steps.rend(); }

        /**
        * @return Reverse iterator to the start of the air steps within the vector.
        */
        [[nodiscard]] auto airStepsRBegin() { return std::next(m_Steps.rbegin(), size() - m_LandIndex); }

        /**
        * @return Reverse iterator to the end of the air steps within the vector.
        */
        [[nodiscard]] auto airStepsREnd() { return m_Steps.rend(); }

        /**
        * @return Const iterator to the beginning of the ground steps within the vector.
        */
        [[nodiscard]] auto groundStepsBegin() const { return std::next(m_Steps.begin(), m_LandIndex + 1); }

        /**
        * @return Const iterator to the end of the ground steps within the vector.
        */
        [[nodiscard]] auto groundStepsEnd() const { return m_Steps.end(); }

        /**
        * @return Iterator to the beginning of the ground steps within the vector.
        */
        [[nodiscard]] auto groundStepsBegin() { return std::next(m_Steps.begin(), m_LandIndex + 1); }

        /**
        * @return Iterator to the end of the ground steps within the vector.
        */
        [[nodiscard]] auto groundStepsEnd() { return m_Steps.end(); }

        /**
       * @return Reverse iterator to the start of the ground steps within the vector.
       */
        [[nodiscard]] auto groundStepsRBegin() const { return m_Steps.rbegin(); }

        /**
        * @return Reverse iterator to the end of the air steps within the vector.
        */
        [[nodiscard]] auto groundStepsREnd() const { return std::next(m_Steps.rbegin(), size() - m_LandIndex); }

        /**
        * @return Reverse iterator to the start of the air steps within the vector.
        */
        [[nodiscard]] auto groundStepsRBegin() { return m_Steps.rbegin(); }

        /**
        * @return Reverse iterator to the end of the air steps within the vector.
        */
        [[nodiscard]] auto groundStepsREnd() { return std::next(m_Steps.rbegin(), size() - m_LandIndex); }

        /**
        * @brief Add a DescendDecelerate step at the start. Aerodynamic coefficients, start altitude AFE and start calibrated airspeed are taken from the next step, the descent angle defaults to -3.
        */
        void addDescendDecelerate() noexcept;

        /**
        * @brief Add a DescendIdle step at the start. Start altitude AFE and start calibrated airspeed are taken from the next step, the descent angle defaults to -3.
        */
        void addDescendIdle() noexcept;

        /**
        * @brief Add a Level step at the start. Aerodynamic coefficients are taken from the next step, the ground distance defaults to 100 meters.
        */
        void addLevel() noexcept;

        /**
        * @brief Add a LevelDecelerate step at the start. Aerodynamic coefficients and start calibrated airspeed are taken from the next step, the ground distance defaults to 100 meters.
        */
        void addLevelDecelerate() noexcept;

        /**
        * @brief Add a LevelIdle step at the start. Start altitude AFE and start calibrated airspeed are taken from the next step. Ground distance defaults to 100 meters.
        */
        void addLevelIdle() noexcept;

        /**
        * @brief Add a GroundDecelerate step at the end. Defaults to 100 meters ground distance, 0 end calibrated airspeed and 40% thrust percentage.
        */
        void addGroundDecelerate() noexcept;

        /**
        * @brief Add a DescendDecelerate step at the start.
        *
        * ASSERT that parent Doc29Performance contains AerodynamicCoefficientsName.
        * ASSERT DescentAngle in [-inf, 0[.
        * ASSERT StartCalibratedAirspeed in [0, inf].
        */
        void addDescendDecelerate(const std::string& AerodynamicCoefficientsName, double StartAltitudeAfe, double DescentAngle, double StartCalibratedAirspeed) noexcept;

        /**
        * @brief Add a DescendIdle step at the start.
        *
        * ASSERT DescentAngle in [-inf, 0[.
        * ASSERT StartCalibratedAirspeed in [0, inf].
        */
        void addDescendIdle(double StartAltitudeAfe, double DescentAngle, double StartCalibratedAirspeed) noexcept;

        /**
        * @brief Add a Level step at the start.
        *
        * ASSERT that parent Doc29Performance contains AerodynamicCoefficientsName.
        * ASSERT GroundDistance in ]0, inf].
        */
        void addLevel(const std::string& AerodynamicCoefficientsName, double GroundDistance) noexcept;

        /**
        * @brief Add a LevelDecelerate step at the start.
        *
        * ASSERT that parent Doc29Performance contains AerodynamicCoefficientsName.
        * ASSERT GroundDistance in ]0, inf].
        * ASSERT StartCalibratedAirspeed in [0.0, inf].
        */
        void addLevelDecelerate(const std::string& AerodynamicCoefficientsName, double GroundDistance, double StartCalibratedAirspeed) noexcept;

        /**
        * @brief Add a LevelIdle step at the start.
        *
        * ASSERT GroundDistance in ]0, inf].
        * ASSERT StartCalibratedAirspeed in [0.0, inf].
        */
        void addLevelIdle(double GroundDistance, double StartCalibratedAirspeed) noexcept;

        /**
        * @brief Set the DescendLand step parameters.
        *
        * ASSERT that parent Doc29Performance contains AerodynamicCoefficientsName and it is of type Land.
        * ASSERT DescentAngle in [-inf, 0[.
        * ASSERT TouchdownRoll in ]0, inf].
        */
        void setDescendLandParameters(const std::string& AerodynamicCoefficientsName, double DescentAngle, double ThresholdCrossingAltitudeAfe, double TouchdownRoll) noexcept;

        /**
        * @brief Set subset of the DescendLand parameters.
        *
        * Calls setDescendLandParameters(const std::string&, double, double, double). The missing parameters are not changed.
        */
        void setDescendLandParameters(const std::string& AerodynamicCoefficientsName, double TouchdownRoll) noexcept;

        /**
        * @brief Set subset of the DescendLand parameters.
        *
        * Calls setDescendLandParameters(const std::string&, double, double, double). The missing parameters are not changed.
        */
        void setDescendLandParameters(double DescentAngle, double ThresholdCrossingAltitudeAfe) noexcept;

        /**
        * @brief Add a GroundDecelerate step at the end.
        *
        * ASSERT GroundDistance in [0.0, inf].
        * ASSERT StartCalibratedAirspeed in [0.0, inf].
        * ASSERT ThrustPercentage in [0.0, 1.0].
        */
        void addGroundDecelerate(double GroundDistance, double StartCalibratedAirspeed, double ThrustPercentage) noexcept;

        /**
        * @brief Delete step at Index.
        *
        * ASSERT Index < size() AND Index != DescendLand index.
        */
        void deleteStep(std::size_t Index) noexcept;

        /**
        * @brief Delete the last step between the ArrivalStart and the DescendLand step. If no air steps exists, does nothing.
        */
        void deleteAirStep() noexcept;

        /**
        * @brief Delete the last GroundDecelerate step. If no GroundDecelerate steps exists, does nothing.
        */
        void deleteGroundDecelerate() noexcept;

        /**
        * @brief Delete all steps between the ArrivalStart and the DescendLand step. If no air steps exists, does nothing.
        */
        void clearAirSteps() noexcept;

        /**
        * @brief Delete all GroundDecelerate steps. If no GroundDecelerate steps exists, does nothing.
        */
        void clearGroundSteps() noexcept;

        /**
        * @brief Calls clearAirSteps() and clearGroundSteps().
        */
        void clear() noexcept;

        /**
        * @brief Throwing version of addDescendDecelerate(const std::string&, double, double, double).
        *
        * Throws if parent Doc29Performance does not contain AerodynamicCoefficientsName.
        * Throws if StartAltitudeAfe is NaN.
        * Throws if DescentAngle not in [-inf, 0[.
        * Throws if StartCalibratedAirspeed not in [0, inf].
        */
        void addDescendDecelerateE(const std::string& AerodynamicCoefficientsName, double StartAltitudeAfe, double DescentAngle, double StartCalibratedAirspeed);

        /**
        * @brief Throwing version of addDescendIdle(double, double, double).
        *
        * Throws if StartAltitudeAfe is NaN.
        * Throws if DescentAngle not in [-inf, 0[.
        * Throws if StartCalibratedAirspeed not in [0, inf].
        */
        void addDescendIdleE(double StartAltitudeAfe, double DescentAngle, double StartCalibratedAirspeed);

        /**
        * @brief Throwing version of addLevel(const std::string&, double).
        *
        * Throws if parent Doc29Performance does not contain AerodynamicCoefficientsName.
        * Throws if GroundDistance not in ]0, inf].
        */
        void addLevelE(const std::string& AerodynamicCoefficientsName, double GroundDistance);

        /**
        * @brief Throwing version of addLevelDecelerate(const std::string&, double, double).
        *
        * Throws if parent Doc29Performance does not contain AerodynamicCoefficientsName.
        * Throws if GroundDistance not in ]0, inf].
        * Throws if StartCalibratedAirspeed not in [0, inf].
        */
        void addLevelDecelerateE(const std::string& AerodynamicCoefficientsName, double GroundDistance, double StartCalibratedAirspeed);

        /**
        * @brief Throwing version of addLevelIdle(double, double, double).
        *
        * Throws if GroundDistance not in ]0, inf].
        * Throws if StartCalibratedAirspeed not in [0, inf].
        */
        void addLevelIdleE(double GroundDistance, double StartCalibratedAirspeed);

        /**
        * @brief Throwing version of setDescendLandParameters(const std::string&, double, double, double).
        *
        * Throws if parent Doc29Performance does not contain AerodynamicCoefficientsName or type is not Land.
        * Throws if DescentAngle not in [-inf, 0[.
        * Throws if ThresholdCrossingAltitudeAfe is NaN.
        * Throws if TouchdownRoll not in ]0, inf].
        */
        void setDescendLandParametersE(const std::string& AerodynamicCoefficientsName, double DescentAngle, double ThresholdCrossingAltitudeAfe, double TouchdownRoll);

        /**
        * @brief Throwing version of setDescendLandParameters(const std::string&, double).
        *
        * Calls setDescendLandParametersE(const std::string&, double, double, double). The missing parameters are not changed.
        */
        void setDescendLandParametersE(const std::string& AerodynamicCoefficientsName, double TouchdownRoll);

        /**
        * @brief Throwing version of setDescendLandParameters(double, double).
        *
        * Calls setDescendLandParametersE(const std::string&, double, double, double). The missing parameters are not changed.
        */
        void setDescendLandParametersE(double DescentAngle, double ThresholdCrossingAltitudeAfe);

        /**
        * @brief Throwing version of addGroundDecelerate(double, double, double).
        *
        * Throws if GroundDistance not in [0, inf].
        * Throws if StartCalibratedAirspeed not in [0, inf].
        * Throws if ThrustPercentage not in [0, 1].
        */
        void addGroundDecelerateE(double GroundDistance, double StartCalibratedAirspeed, double ThrustPercentage);

        /**
        * @return True if any air step is of type DescendIdle or LevelIdle
        */
        [[nodiscard]] bool usesIdleThrust() const { return m_UseIdleThrustCount; } // std::size_t to bool

        /**
        * @return True if no steps exist between the ArrivalStart and the DescendLand step.
        */
        [[nodiscard]] bool airStepsEmpty() const { return m_LandIndex == 0; }

        /**
        * @return True if no steps exist after the DescendLand step.
        */
        [[nodiscard]] bool groundStepsEmpty() const { return m_LandIndex == size() - 1; }

        /**
        * @return True if only the DescendLand steps exist.
        */
        [[nodiscard]] bool empty() const { return size() == 1; }

        /**
        * @return The number of steps including the ArrivalStart and the DescendLand step.
        */
        [[nodiscard]] std::size_t size() const { return m_Steps.size(); }

        /**
        * @return The index at which the DescendLand step is.
        */
        [[nodiscard]] std::size_t landIndex() const { return m_LandIndex; }

        /**
        * @return The threshold crossing altitude set in the DescendLand step.
        */
        [[nodiscard]] double thresholdCrossingAltitudeAfe() const { return std::get<DescendLand>(m_Steps.at(m_LandIndex)).ThresholdCrossingAltitudeAfe; }

        // Visitor pattern
        void accept(Doc29ProfileVisitor& Vis) override;
        void accept(Doc29ProfileVisitor& Vis) const override;
        void accept(Doc29ProfileArrivalVisitor& Vis) override;
        void accept(Doc29ProfileArrivalVisitor& Vis) const override;

    private:
        std::vector<Step> m_Steps;

        std::size_t m_UseIdleThrustCount = 0;
        std::size_t m_LandIndex = 0; // On creation only landing step exists, landing is at index 0
    private:
        void addDescendDecelerateImpl(const Doc29AerodynamicCoefficients* Coefficients, double StartAltitudeAfe, double DescentAngle, double StartCalibratedAirspeed);
        void addDescendIdleImpl(double StartAltitudeAfe, double DescentAngle, double StartCalibratedAirspeed);
        void addLevelImpl(const Doc29AerodynamicCoefficients* Coefficients, double GroundDistance);
        void addLevelDecelerateImpl(const Doc29AerodynamicCoefficients* Coefficients, double GroundDistance, double StartCalibratedAirspeed);
        void addLevelIdleImpl(double GroundDistance, double StartCalibratedAirspeed);
        void addGroundDecelerateImpl(double GroundDistance, double StartCalibratedAirspeed, double ThrustPercentage);
        void unblockCoefficients(const Step& ArrivalStep);

        // Aircraft State
        struct State {
            const Doc29AerodynamicCoefficients* Doc29AerodynamicCoefficients = nullptr;
            double AltitudeAfe = Constants::NaN;
            double CalibratedAirspeed = Constants::NaN;

            [[nodiscard]] bool valid() const { return Doc29AerodynamicCoefficients != nullptr && !std::isnan(AltitudeAfe) && !std::isnan(CalibratedAirspeed); }
        };

        [[nodiscard]] State nextState(std::size_t Index) const;

        struct VisitorState {
            explicit VisitorState(State& StateIn) : St(StateIn) {}
            void operator()(const DescendDecelerate& DescendDecelerateStep) const;
            void operator()(const DescendIdle& DescendIdleStep) const;
            void operator()(const Level& LevelStep) const;
            void operator()(const LevelDecelerate& LevelDecelerateStep) const;
            void operator()(const LevelIdle& LevelIdleStep) const;
            void operator()(const DescendLand& DescendLandStep) const;
            void operator()(const GroundDecelerate& GroundDecelerateStep) const;
            State& St;
        };
    };

    /**
    * @brief A departure profile defined by a sequence of procedural steps.
    */
    class Doc29ProfileDepartureProcedural : public Doc29ProfileDeparture {
    public:
        /**
        * @brief Every Doc29ProfileDepartureProcedural starts with a Takeoff step, which is the only Takeoff step in the profile. The initial calibrated airspeed can be given.
        */
        struct Takeoff {
            const Doc29AerodynamicCoefficients* Doc29AerodynamicCoefficients = nullptr;
            double InitialCalibratedAirspeed = Constants::NaN;
        };

        /**
        * @brief Climb at constant calibrated airspeed to EndAltitudeAfe.
        */
        struct Climb {
            const Doc29AerodynamicCoefficients* Doc29AerodynamicCoefficients = nullptr;
            double EndAltitudeAfe = Constants::NaN;
        };

        /**
        * @brief Climb to EndCalibratedAirspeed with a given climb rate.
        */
        struct ClimbAccelerate {
            const Doc29AerodynamicCoefficients* Doc29AerodynamicCoefficients = nullptr;
            double EndCalibratedAirspeed = Constants::NaN;
            double ClimbParameter = Constants::NaN; // Climb Rate
        };

        /**
        * @brief Climb to EndAltitudeAfe and EndCalibratedAirspeed with an acceleration percentage.
        */
        struct ClimbAcceleratePercentage {
            const Doc29AerodynamicCoefficients* Doc29AerodynamicCoefficients = nullptr;
            double EndCalibratedAirspeed = Constants::NaN;
            double ClimbParameter = Constants::NaN; // Acceleration Percentage
        };

        typedef std::variant<Takeoff, Climb, ClimbAccelerate, ClimbAcceleratePercentage> Step;

        enum class StepType {
            Takeoff = 0, Climb, ClimbAccelerate, ClimbAcceleratePercentage,
        };

        static constexpr EnumStrings<StepType> StepTypes{ "Takeoff", "Climb", "Climb Accelerate", "Climb Accelerate Percentage" };

        struct VisitorStepTypeString {
            std::string operator()(const Takeoff&) const { return StepTypes.toString(StepType::Takeoff); }
            std::string operator()(const Climb&) const { return StepTypes.toString(StepType::Climb); }
            std::string operator()(const ClimbAccelerate&) const { return StepTypes.toString(StepType::ClimbAccelerate); }
            std::string operator()(const ClimbAcceleratePercentage&) const { return StepTypes.toString(StepType::ClimbAcceleratePercentage); }
        };

        /**
        * @brief Construct a profile with a Takeoff step.
        * @param Doc29PerformanceIn The parent Doc29Performance.
        * @param NameIn The profile name.
        *
        * ASSERT that Doc29PerformanceIn has Doc29AerodynamicCoefficients of type Takeoff.
        */
        Doc29ProfileDepartureProcedural(Doc29Performance& Doc29PerformanceIn, const std::string& NameIn);
        virtual ~Doc29ProfileDepartureProcedural() override;

        /**
        * @return The Doc29Profile::Type.
        */
        [[nodiscard]] Type type() const override { return Type::Procedural; }

        /**
        * @return The index at which the thrust cutback occurs.
        */
        [[nodiscard]] std::size_t thrustCutback() const { return m_ThrustCutbackIndex; }

        /**
        * @return The vector containing all the steps.
        */
        [[nodiscard]] const auto& steps() const { return m_Steps; }

        /**
        * @return Const iterator to the beginning of the vector containing all the steps.
        */
        [[nodiscard]] auto begin() const { return m_Steps.begin(); }

        /**
        * @return Const iterator to the end of the vector containing all the steps.
        */
        [[nodiscard]] auto end() const { return m_Steps.end(); }

        /**
        * @return Iterator to the beginning of the vector containing all the steps.
        */
        auto begin() { return m_Steps.begin(); }

        /**
        * @return Iterator to the end of the vector containing all the steps.
        */
        auto end() { return m_Steps.end(); }

        /**
        * @brief Set the Index at which thrust cutback occurs.
        *
        * ASSERT Index < size().
        */
        void setThrustCutback(std::size_t Index) noexcept;

        /**
        * @brief Add a Climb step to the end. Aerodynamic coefficients and end altitude AFE are taken from the last step.
        */
        void addClimb() noexcept;

        /**
        * @brief Add a ClimbAccelerate step to the end. Aerodynamic coefficients, end altitude AFE and end calibrated airspeed are taken from the last step. Climb rate defaults to 500 ft/min.
        */
        void addClimbAccelerate() noexcept;

        /**
        * @brief Add a ClimbAcceleratePercentage step to the end. Aerodynamic coefficients, end altitude AFE and end calibrated airspeed are taken from the last step. Acceleration percentage defaults to 60%.
        */
        void addClimbAcceleratePercentage() noexcept;

        /**
        * @brief Set the Takeoff step parameters.
        *
        * ASSERT that parent Doc29Performance contains AerodynamicCoefficientsName and type is Takeoff.
        * ASSERT InitialCalibratedAirspeed in [0.0, inf].
        */
        void setTakeoffParameters(const std::string& AerodynamicCoefficientsName, double InitialCalibratedAirspeed) noexcept;

        /**
        * @brief Add a Climb step to the end.
        *
        * ASSERT that parent Doc29Performance contains AerodynamicCoefficientsName.
        * ASSERT EndAltitudeAfe is not NaN.
        */
        void addClimb(const std::string& AerodynamicCoefficientsName, double EndAltitudeAfe) noexcept;

        /**
        * @brief Add a ClimbAccelerate step to the end.
        *
        * ASSERT that parent Doc29Performance contains AerodynamicCoefficientsName.
        * ASSERT EndCalibratedAirspeed in [0, inf].
        * ASSERT ClimbRate in [0, inf].
        */
        void addClimbAccelerate(const std::string& AerodynamicCoefficientsName, double EndCalibratedAirspeed, double ClimbRate) noexcept;

        /**
        * @brief Add a ClimbAcceleratePercentage step to the end.
        *
        * ASSERT that parent Doc29Performance contains AerodynamicCoefficientsName.
        * ASSERT EndCalibratedAirspeed in [0, inf].
        * ASSERT AccelerationPercentage in ]0, 1].
        */
        void addClimbAcceleratePercentage(const std::string& AerodynamicCoefficientsName, double EndCalibratedAirspeed, double AccelerationPercentage) noexcept;

        /**
        * @brief Delete step at Index.
        *
        * ASSERT Index != 0 AND Index < size().
        */
        void deleteStep(std::size_t Index) noexcept;

        /**
        * @brief Delete the last step. If profile is empty does nothing.
        */
        void deleteStep() noexcept;

        /**
        * @brief Delete all steps. If profile is empty does nothing.
        */
        void clear() noexcept;

        /**
        * @brief Throwing version of setTakeoffParameters(const std::string&, double).
        *
        * Throws if parent Doc29Performance does not contain AerodynamicCoefficientsName or type is not Takeoff.
        * Throws if InitialCalibratedAirspeed not in [0, inf].
        */
        void setTakeoffParametersE(const std::string& AerodynamicCoefficientsName, double InitialCalibratedAirspeed);

        /**
        * @brief Throwing version of addClimb(const std::string&, double).
        *
        * Throws if parent Doc29Performance does not contain AerodynamicCoefficientsName.
        * Throws if EndAltitudeAfe is NaN.
        */
        void addClimbE(const std::string& AerodynamicCoefficientsName, double EndAltitudeAfe);

        /**
        * @brief Throwing version of addClimbAccelerate(const std::string&, double, double).
        *
        * Throws if parent Doc29Performance does not contain AerodynamicCoefficientsName.
        * Throws if EndCalibratedAirspeed not in [0, inf].
        * Throws if ClimbRate not in [0, inf].
        */
        void addClimbAccelerateE(const std::string& AerodynamicCoefficientsName, double EndCalibratedAirspeed, double ClimbRate);

        /**
        * @brief Throwing version of addClimbAcceleratePercentage(const std::string&, double, double).
        *
        * Throws if parent Doc29Performance does not contain AerodynamicCoefficientsName.
        * Throws if EndCalibratedAirspeed not in [0, inf].
        * Throws if AccelerationPercentage not in ]0, 1].
        */
        void addClimbAcceleratePercentageE(const std::string& AerodynamicCoefficientsName, double EndCalibratedAirspeed, double AccelerationPercentage);

        /**
        * @return True if only the Takeoff step exists.
        */
        [[nodiscard]] bool empty() const { return size() == 1; } // Takeoff always in the container

        /**
        * @return The number of steps including the Takeoff step.
        */
        [[nodiscard]] std::size_t size() const { return m_Steps.size(); }

        // Visitor pattern
        void accept(Doc29ProfileVisitor& Vis) override;
        void accept(Doc29ProfileVisitor& Vis) const override;
        void accept(Doc29ProfileDepartureVisitor& Vis) override;
        void accept(Doc29ProfileDepartureVisitor& Vis) const override;

    private:
        // Steps container
        std::vector<Step> m_Steps;
        std::size_t m_ThrustCutbackIndex = 0; // Note: 0 means no cutback
    private:
        void unblockCoefficients(const Step& DepartureStep);

        // Aircraft State
        struct State {
            const Doc29AerodynamicCoefficients* Doc29AerodynamicCoefficients = nullptr;
            double AltitudeAfe = Constants::NaN;
            double CalibratedAirspeed = Constants::NaN;

            [[nodiscard]] bool valid() const { return Doc29AerodynamicCoefficients != nullptr && !std::isnan(AltitudeAfe) && !std::isnan(CalibratedAirspeed); }
        };

        [[nodiscard]] State previousState(std::size_t Index) const;

        struct VisitorState {
            explicit VisitorState(State& StateIn) : St(StateIn) {}
            void operator()(const Takeoff& TakeoffStep) const;
            void operator()(const Climb& ClimbStep) const;
            void operator()(const ClimbAccelerate& ClimbAccelerateStep) const;
            void operator()(const ClimbAcceleratePercentage& ClimbAcceleratePercentageStep) const;
            State& St;
        };
    };

    struct Doc29ProfileVisitor {
        virtual void visitDoc29ProfileArrivalPoints(Doc29ProfileArrivalPoints& Profile) {}
        virtual void visitDoc29ProfileArrivalProcedural(Doc29ProfileArrivalProcedural& Profile) {}
        virtual void visitDoc29ProfileArrivalPoints(const Doc29ProfileArrivalPoints& Profile) {}
        virtual void visitDoc29ProfileArrivalProcedural(const Doc29ProfileArrivalProcedural& Profile) {}
        virtual void visitDoc29ProfileDeparturePoints(Doc29ProfileDeparturePoints& Profile) {}
        virtual void visitDoc29ProfileDepartureProcedural(Doc29ProfileDepartureProcedural& Profile) {}
        virtual void visitDoc29ProfileDeparturePoints(const Doc29ProfileDeparturePoints& Profile) {}
        virtual void visitDoc29ProfileDepartureProcedural(const Doc29ProfileDepartureProcedural& Profile) {}
        virtual ~Doc29ProfileVisitor() = default;
    };

    struct Doc29ProfileArrivalVisitor {
        virtual void visitDoc29ProfileArrivalPoints(Doc29ProfileArrivalPoints& Profile) {}
        virtual void visitDoc29ProfileArrivalProcedural(Doc29ProfileArrivalProcedural& Profile) {}
        virtual void visitDoc29ProfileArrivalPoints(const Doc29ProfileArrivalPoints& Profile) {}
        virtual void visitDoc29ProfileArrivalProcedural(const Doc29ProfileArrivalProcedural& Profile) {}
        virtual ~Doc29ProfileArrivalVisitor() = default;
    };

    struct Doc29ProfileDepartureVisitor {
        virtual void visitDoc29ProfileDeparturePoints(Doc29ProfileDeparturePoints& Profile) {}
        virtual void visitDoc29ProfileDepartureProcedural(Doc29ProfileDepartureProcedural& Profile) {}
        virtual void visitDoc29ProfileDeparturePoints(const Doc29ProfileDeparturePoints& Profile) {}
        virtual void visitDoc29ProfileDepartureProcedural(const Doc29ProfileDepartureProcedural& Profile) {}
        virtual ~Doc29ProfileDepartureVisitor() = default;
    };
}
