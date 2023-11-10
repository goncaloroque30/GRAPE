// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "Track4d.h"

#include "Operations.h"

namespace GRAPE {
    void Track4d::Point::setTime(const std::string& UtcTimeStr) {
        const auto timeOpt = utcStringToTime(UtcTimeStr);
        if (timeOpt)
            Time = timeOpt.value();
        else
            throw GrapeException(std::format("Invalid track 4D point time '{}'.", UtcTimeStr));
    }

    void Track4d::Point::setLongitude(double LongitudeIn) {
        if (!(LongitudeIn >= -180.0 && LongitudeIn <= 180.0))
            throw GrapeException("Longitude must be between -180.0 and 180.0.");

        Longitude = LongitudeIn;
    }

    void Track4d::Point::setLatitude(double LatitudeIn) {
        if (!(LatitudeIn >= -90.0 && LatitudeIn <= 90.0))
            throw GrapeException("Latitude must be between -90.0 and 90.0.");

        Latitude = LatitudeIn;
    }


    void Track4d::Point::setTrueAirspeed(double TrueAirspeedIn) {
        if (!(TrueAirspeedIn >= 0.0))
            throw GrapeException("True airspeed must be at least 0.0.");

        TrueAirspeed = TrueAirspeedIn;
    }

    void Track4d::Point::setGroundspeed(double GroundspeedIn) {
        if (!(GroundspeedIn >= 0.0))
            throw GrapeException("Groundspeed must be at least 0.0.");

        Groundspeed = GroundspeedIn;
    }

    void Track4d::Point::setBankAngle(double BankAngleIn) {
        if (!(BankAngleIn >= -90.0 && BankAngleIn <= 90.0))
            throw GrapeException("Bank angle must be between -90.0 and 90.0.");

        BankAngle = BankAngleIn;
    }

    void Track4d::Point::setFuelFlowPerEng(double FuelFlowPerEngIn) {
        if (!(FuelFlowPerEngIn >= 0.0))
            throw GrapeException("Fuel flow per engine must be at least 0.");

        FuelFlowPerEng = FuelFlowPerEngIn;
    }

    void Track4d::addPoint(TimePoint Time, FlightPhase FlPhase, double CumulativeGroundDistance, double Longitude, double Latitude, double AltitudeMsl, double TrueAirspeed, double Groundspeed, double CorrNetThrustPerEng, double BankAngle, double FuelFlowPerEng) noexcept {
        GRAPE_ASSERT(Longitude >= -180.0 && Longitude <= 180.0);
        GRAPE_ASSERT(Latitude >= -90.0 && Latitude <= 90.0);
        GRAPE_ASSERT(TrueAirspeed >= 0.0);
        GRAPE_ASSERT(Groundspeed >= 0.0);
        GRAPE_ASSERT(BankAngle >= -90.0 && BankAngle <= 90.0);
        GRAPE_ASSERT(FuelFlowPerEng >= 0.0);

        m_Points.emplace_back(Time, FlPhase, CumulativeGroundDistance, Longitude, Latitude, AltitudeMsl, TrueAirspeed, Groundspeed, CorrNetThrustPerEng, BankAngle, FuelFlowPerEng);
    }

    void Track4d::addPoint(const Point& Pt) noexcept {
        m_Points.emplace_back(Pt);
    }

    void Track4d::insertPoint(std::size_t Index) noexcept {
        GRAPE_ASSERT(Index <= size());

        if (empty()) // Index = 0
            addPoint();

        m_Points.insert(begin() + Index, m_Points.at(Index));
    }

    void Track4d::deletePoint(std::size_t Index) noexcept {
        GRAPE_ASSERT(Index < size());
        m_Points.erase(begin() + Index);
    }

    void Track4d::deletePoint() noexcept {
        GRAPE_ASSERT(!empty());
        m_Points.pop_back();
    }

    void Track4d::clear(bool Shrink) noexcept {
        m_Points.clear();
        if (Shrink)
            m_Points.shrink_to_fit();
    }

    template<>
    void Track4dOp<OperationType::Arrival>::addPoint() noexcept {
        if (empty())
        {
            Point p;
            p.FlPhase = FlightPhase::Approach;
            m_Points.push_back(p);
        }
        else
        {
            m_Points.push_back(m_Points.back());
        }
    }

    template<>
    void Track4dOp<OperationType::Departure>::addPoint() noexcept {
        if (empty())
        {
            Point p;
            p.FlPhase = FlightPhase::TakeoffRoll;
            m_Points.push_back(p);
        }
        else
        {
            m_Points.push_back(m_Points.back());
        }
    }

    void Track4dOp<OperationType::Arrival>::accept(OperationVisitor& Vis) {
        Vis.visitTrack4dArrival(*this);
    }

    void Track4dOp<OperationType::Arrival>::accept(OperationVisitor& Vis) const {
        Vis.visitTrack4dArrival(*this);
    }

    void Track4dOp<OperationType::Departure>::accept(OperationVisitor& Vis) {
        Vis.visitTrack4dDeparture(*this);
    }

    void Track4dOp<OperationType::Departure>::accept(OperationVisitor& Vis) const {
        Vis.visitTrack4dDeparture(*this);
    }

    template struct Track4dOp<OperationType::Arrival>;
    template struct Track4dOp<OperationType::Departure>;
}
