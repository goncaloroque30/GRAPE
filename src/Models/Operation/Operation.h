// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Aircraft/Aircraft.h"
#include "Base/BaseModels.h"

namespace GRAPE {
    struct OperationVisitor;
    /**
    * @file Operation.h
    *
    * Operations in GRAPE have the following class structure
    *
    *         Operation
    *        /         \
    *		/           \
    * OperationType  OperationOp  <- virtual inheritance
    *       \           /
    *		 \         /
    * 	    OperationOpType
    */

    /**
    * @brief Base class for all operations. The possible types are stored in the #Type enum.
    */
    class Operation {
    public:
        enum class Type {
            Flight = 0,
            Track4d
        };
        static constexpr EnumStrings<Type> Types{ "Flight", "Track 4D" };

        /**
        * @brief An operation must have a name and an associated Aircraft.
        */
        Operation(std::string_view NameIn, const Aircraft& AircraftIn, const TimePoint& TimeIn = std::chrono::round<Duration>(std::chrono::tai_clock::now()), double CountIn = 1.0) : Name(NameIn), Time(TimeIn), Count(CountIn), m_Aircraft(AircraftIn) {}
        Operation(const Operation&) = delete;
        Operation(Operation&&) = delete;
        Operation& operator=(const Operation&) = delete;
        Operation& operator=(Operation&&) = delete;
        virtual ~Operation() = default;

        std::string Name;

        TimePoint Time{ std::chrono::round<Duration>(std::chrono::tai_clock::now()) };

        /**
        * @brief Convenience throwing set method for #Time.
        * @param UtcTimeStr A string representing a UTC time in the format "yyyy-mm-dd HH:MM:SS".
        *
        * Throws if failed to convert the UtcTimeStr to a valid UTC time.
        */
        void setTime(const std::string& UtcTimeStr);

        /**
        * @return The number of seconds between 00:00:00 and the second of the day at which the Operation occurs.
        */
        [[nodiscard]] Duration timeOfDay() const;

        double Count;

        /**
        * @brief Throwing set method for #Count.
        *
        * Throws if Count not in [0, inf].
        */
        void setCount(double CountIn);

        /**
        * @return The aircraft associated with this Operation.
        */
        [[nodiscard]] const Aircraft& aircraft() const;

        /**
        * @brief Change the associated Aircraft
        */
        void setAircraft(const Aircraft& Acft) noexcept;

        /**
        * @return The OperationType.
        */
        [[nodiscard]] virtual OperationType operationType() const = 0;

        /**
        * @return The Type of the operation.
        */
        [[nodiscard]] virtual Type type() const = 0;

        /**
        * @return The FlightPhases supported by the OperationType of this Operation.
        */
        [[nodiscard]] virtual std::vector<FlightPhase> phases() const = 0;

        // Visitor Pattern
        virtual void accept(OperationVisitor&) = 0;
        virtual void accept(OperationVisitor&) const = 0;
    private:
        std::reference_wrapper<const Aircraft> m_Aircraft;
    };

    /**
    * @brief OperationOp defines the OperationType of an Operation.
    */
    template<OperationType OpType>
    struct OperationOp : virtual Operation {
        OperationOp(std::string_view, const Aircraft&) {}
        virtual ~OperationOp() override = default;

        /**
        * @return The OperationType of this Operation.
        */
        [[nodiscard]] OperationType operationType() const override { return OpType; }

        /**
        * @return The FlightPhases supported by the OperationType of this Operation.
        */
        [[nodiscard]] std::vector<FlightPhase> phases() const override;
    };
    typedef OperationOp<OperationType::Arrival> OperationArrival;
    typedef OperationOp<OperationType::Departure> OperationDeparture;

    /**
    * @return The FlightPhase types supported by an OperationArrival.
    */
    template<>
    inline std::vector<FlightPhase> OperationOp<OperationType::Arrival>::phases() const {
        return { FlightPhase::Approach, FlightPhase::LandingRoll };
    }

    /**
    * @return The FlightPhase types supported by an OperationDeparture.
    */
    template<>
    inline std::vector<FlightPhase> OperationOp<OperationType::Departure>::phases() const {
        return { FlightPhase::TakeoffRoll, FlightPhase::InitialClimb, FlightPhase::Climb };
    }
}
