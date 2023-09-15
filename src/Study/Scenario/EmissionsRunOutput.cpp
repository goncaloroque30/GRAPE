// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "EmissionsRunOutput.h"

#include "Schema/Schema.h"
#include "Scenario.h"

namespace GRAPE {
    EmissionsRunOutput::EmissionsRunOutput(const EmissionsRun& EmissionsRn, const Database& Db) : m_EmissionsRun(EmissionsRn), m_Db(Db) {}

    const EmissionsRun& EmissionsRunOutput::parentFuelEmissionsRun() const {
        return m_EmissionsRun;
    }

    const PerformanceRun& EmissionsRunOutput::parentPerformanceRun() const {
        return parentFuelEmissionsRun().parentPerformanceRun();
    }

    const Scenario& EmissionsRunOutput::parentScenario() const {
        return parentPerformanceRun().parentScenario();
    }

    const EmissionsOperationOutput& EmissionsRunOutput::operationOutput(const Operation& Op) const {
        GRAPE_ASSERT(m_OperationOutputs.contains(&Op));
        return m_OperationOutputs.at(&Op);
    }

    const EmissionsOperationOutput EmissionsRunOutput::operationOutputWithSegments(const Operation& Op) const {
        GRAPE_ASSERT(m_OperationOutputs.contains(&Op));
        return loadSegments(Op);
    }

    void EmissionsRunOutput::createOutput() const {
        std::scoped_lock lck(m_Mutex);

        m_Db.insert(Schema::emissions_run_output, {}, std::make_tuple(
            parentScenario().Name,
            parentPerformanceRun().Name,
            parentFuelEmissionsRun().Name,
            0.0, // fuel
            0.0, // hc
            0.0, // co
            0.0) // nox
        );
    }

    void EmissionsRunOutput::addOperationOutput(const Operation& Op, const EmissionsOperationOutput& EmissionsOpOut, bool SaveSegments) {
        std::scoped_lock lck(m_Mutex);

        // Operation in DB
        saveOperation(Op, EmissionsOpOut);

        // Segments in DB
        if (SaveSegments)
            saveSegments(Op, EmissionsOpOut);

        // Operation in Study
        auto [fuelEmiOpOut, added] = m_OperationOutputs.add(&Op, EmissionsOpOut);
        GRAPE_ASSERT(added);
        fuelEmiOpOut.clearSegmentOutput(true);

        // Totals in Study
        m_TotalFuel += EmissionsOpOut.totalFuel();
        m_TotalEmissions += EmissionsOpOut.totalEmissions();

        // Totals in DB
        m_Db.update(Schema::emissions_run_output, { 3, 4, 5, 6 }, std::make_tuple(m_TotalFuel, m_TotalEmissions.HC, m_TotalEmissions.CO, m_TotalEmissions.NOx), { 0, 1, 2 }, std::make_tuple(parentScenario().Name, parentPerformanceRun().Name, parentFuelEmissionsRun().Name));
    }

    void EmissionsRunOutput::clear() {
        std::scoped_lock lck(m_Mutex);
        m_TotalFuel = 0.0;
        m_TotalEmissions = EmissionValues();
        m_OperationOutputs.clear();
        m_Db.beginTransaction();
        m_Db.deleteD(Schema::emissions_run_output, { 0, 1, 2 }, std::make_tuple(parentScenario().Name, parentPerformanceRun().Name, parentFuelEmissionsRun().Name));
        m_Db.commitTransaction();
    }

    void EmissionsRunOutput::saveOperation(const Operation& Op, const EmissionsOperationOutput& EmissionsOpOut) const {
        m_Db.beginTransaction();
        m_Db.insert(Schema::emissions_run_output_operations, {}, std::make_tuple(
            parentScenario().Name,
            parentPerformanceRun().Name,
            parentFuelEmissionsRun().Name,
            Op.Name,
            OperationTypes.toString(Op.operationType()),
            Operation::Types.toString(Op.type()),
            EmissionsOpOut.totalFuel(), // fuel
            EmissionsOpOut.totalEmissions().HC, // hc
            EmissionsOpOut.totalEmissions().CO, // co
            EmissionsOpOut.totalEmissions().NOx) // nox
        );
        m_Db.commitTransaction();
    }

    void EmissionsRunOutput::saveSegments(const Operation& Op, const EmissionsOperationOutput& EmissionsOpOut) const {
        m_Db.beginTransaction();
        for (const auto& segOut : EmissionsOpOut.segmentOutput())
        {
            m_Db.insert(Schema::emissions_run_output_segments, {}, std::make_tuple(
                parentScenario().Name,
                parentPerformanceRun().Name,
                parentFuelEmissionsRun().Name,
                Op.Name,
                OperationTypes.toString(Op.operationType()),
                Operation::Types.toString(Op.type()),
                static_cast<int>(segOut.Index),
                segOut.Fuel,
                segOut.Emissions.HC,
                segOut.Emissions.CO,
                segOut.Emissions.NOx)
            );
        }
        m_Db.commitTransaction();
    }

    const EmissionsOperationOutput EmissionsRunOutput::loadSegments(const Operation& Op) const {
        EmissionsOperationOutput emiOpOut;

        m_Db.beginTransaction();

        Statement stmt(m_Db, Schema::emissions_run_output_segments.querySelect({ 6, 7, 8, 9, 10 }, { 0, 1, 2, 3, 4, 5 }));
        stmt.bindValues(m_EmissionsRun.parentScenario().Name, m_EmissionsRun.parentPerformanceRun().Name, m_EmissionsRun.Name, Op.Name, OperationTypes.toString(Op.operationType()), Operation::Types.toString(Op.type()));
        stmt.step();
        while (stmt.hasRow())
        {
            EmissionsSegmentOutput segOut;
            segOut.Index = static_cast<std::size_t>(stmt.getColumn(0).getInt());
            segOut.Fuel = stmt.getColumn(1);
            segOut.Emissions = EmissionValues(stmt.getColumn(2), stmt.getColumn(3), stmt.getColumn(4));
            emiOpOut.addSegmentOutput(segOut);
            stmt.step();
        }
        m_Db.commitTransaction();

        return emiOpOut;
    }
}
