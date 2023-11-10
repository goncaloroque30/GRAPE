// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "PerformanceRunOutput.h"

#include "Schema/Schema.h"
#include "Scenario.h"

namespace GRAPE {
    PerformanceRunOutput::PerformanceRunOutput(const PerformanceRun& PerfRun, const Database& Db) : m_PerfRun(PerfRun), m_Db(Db) {}

    void PerformanceRunOutput::clear() {
        std::scoped_lock lck(m_Mutex);
        if (empty())
            return;
        m_ArrivalOutputs.clear();
        m_ArrivalOutputs.shrink_to_fit();
        m_DepartureOutputs.clear();
        m_DepartureOutputs.shrink_to_fit();
        m_Db.beginTransaction();
        m_Db.deleteD(Schema::performance_run_output, { 0, 1 }, std::make_tuple(m_PerfRun.parentScenario().Name, m_PerfRun.Name));
        m_Db.commitTransaction();
    }

    bool PerformanceRunOutput::containsArrival(const OperationArrival& Op) const {
        return std::ranges::find_if(m_ArrivalOutputs, [&](const OperationArrival& ArrOp) { return &ArrOp == &Op; }) != m_ArrivalOutputs.end();
    }

    bool PerformanceRunOutput::containsDeparture(const OperationDeparture& Op) const {
        return std::ranges::find_if(m_DepartureOutputs, [&](const OperationDeparture& DepOp) { return &DepOp == &Op; }) != m_DepartureOutputs.end();
    }

    PerformanceOutput PerformanceRunOutput::output(const Operation& Op) const {
        std::scoped_lock lck(m_Mutex);
        return load(Op);
    }

    PerformanceOutput PerformanceRunOutput::arrivalOutput(const OperationArrival& Op) const {
        std::scoped_lock lck(m_Mutex);
        GRAPE_ASSERT(containsArrival(Op));
        return load(Op);
    }

    PerformanceOutput PerformanceRunOutput::departureOutput(const OperationDeparture& Op) const {
        std::scoped_lock lck(m_Mutex);
        GRAPE_ASSERT(containsDeparture(Op));
        return load(Op);
    }

    void PerformanceRunOutput::addArrivalOutput(const OperationArrival& Op, const PerformanceOutput& PerfOut) {
        std::scoped_lock lck(m_Mutex);
        m_ArrivalOutputs.emplace_back(Op);
        save(Op, PerfOut);
    }

    void PerformanceRunOutput::addDepartureOutput(const OperationDeparture& Op, const PerformanceOutput& PerfOut) {
        std::scoped_lock lck(m_Mutex);
        m_DepartureOutputs.emplace_back(Op);
        save(Op, PerfOut);
    }

    PerformanceOutput PerformanceRunOutput::load(const Operation& Op) const {
        PerformanceOutput perfOutput;
        m_Db.beginTransaction();

        Statement stmt(m_Db, Schema::performance_run_output_points.querySelect({ 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17 }, { 0, 1, 2, 3, 4 }, { 5 }));
        stmt.bindValues(m_PerfRun.parentScenario().Name, m_PerfRun.Name, Op.Name, OperationTypes.toString(Op.operationType()), Operation::Types.toString(Op.type()));
        stmt.step();
        while (stmt.hasRow())
        {
            std::size_t col = 0;

            const PerformanceOutput::PointOrigin origin = PerformanceOutput::Origins.fromString(stmt.getColumn(col++));
            TimePoint time = now();

            const auto timeOpt = utcStringToTime(stmt.getColumn(col++).getString());
            if (timeOpt)
                time = timeOpt.value();
            GRAPE_ASSERT(timeOpt);

            const FlightPhase flPhase = FlightPhases.fromString(stmt.getColumn(col++));
            const double cumGroundDist = stmt.getColumn(col++);
            const double lon = stmt.getColumn(col++);
            const double lat = stmt.getColumn(col++);
            const double altMsl = stmt.getColumn(col++);
            const double trueAirspeed = stmt.getColumn(col++);
            const double groundSpeed = stmt.getColumn(col++);
            const double corrNetThrustPerEng = stmt.getColumn(col++);
            const double bankAngle = stmt.getColumn(col++);
            const double fuelFlowPerEng = stmt.getColumn(col++);
            perfOutput.addPoint(origin, time, flPhase, cumGroundDist, lon, lat, altMsl, trueAirspeed, groundSpeed, corrNetThrustPerEng, bankAngle, fuelFlowPerEng);
            stmt.step();
        }
        m_Db.commitTransaction();
        return perfOutput;
    }

    void PerformanceRunOutput::save(const Operation& Op, const PerformanceOutput& PerfOutput) const {
        m_Db.beginTransaction();

        m_Db.insert(Schema::performance_run_output, {}, std::make_tuple(
            m_PerfRun.parentScenario().Name,
            m_PerfRun.Name,
            Op.Name,
            OperationTypes.toString(Op.operationType()),
            Operation::Types.toString(Op.type())
        ));

        Statement stmt(m_Db, Schema::performance_run_output_points.queryInsert());
        stmt.bindValues(m_PerfRun.parentScenario().Name, m_PerfRun.Name, Op.Name, OperationTypes.toString(Op.operationType()), Operation::Types.toString(Op.type()));

        for (auto it = PerfOutput.begin(); it != PerfOutput.end(); ++it)
        {
            auto& [cumGroundDist, pt] = *it;
            m_Db.insert(Schema::performance_run_output_points, {}, std::make_tuple(
                m_PerfRun.parentScenario().Name,
                m_PerfRun.Name,
                Op.Name,
                OperationTypes.toString(Op.operationType()),
                Operation::Types.toString(Op.type()),
                static_cast<int>(std::distance(PerfOutput.begin(), it)) + 1,
                PerformanceOutput::Origins.toString(pt.PtOrigin),
                timeToUtcString(pt.Time),
                FlightPhases.toString(pt.FlPhase),
                cumGroundDist,
                pt.Longitude,
                pt.Latitude,
                pt.AltitudeMsl,
                pt.TrueAirspeed,
                pt.Groundspeed,
                pt.CorrNetThrustPerEng,
                pt.BankAngle,
                pt.FuelFlowPerEng
            ));
        }

        m_Db.commitTransaction();
    }
}
