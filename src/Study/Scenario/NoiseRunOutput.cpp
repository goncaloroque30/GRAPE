// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "NoiseRunOutput.h"

#include "Schema/Schema.h"
#include "Scenario.h"

namespace GRAPE {
    NoiseRunOutput::NoiseRunOutput(const NoiseRun& NsRun, const Database& Db) : m_NoiseRun(NsRun), m_Db(Db) {}

    const NoiseRun& NoiseRunOutput::parentNoiseRun() const {
        return m_NoiseRun;
    }

    const PerformanceRun& NoiseRunOutput::parentPerformanceRun() const {
        return parentNoiseRun().parentPerformanceRun();
    }

    const Scenario& NoiseRunOutput::parentScenario() const {
        return parentPerformanceRun().parentScenario();
    }

    NoiseSingleEventOutput NoiseRunOutput::singleEventOutput(const Operation& Op) const {
        return load(Op);
    }

    NoiseSingleEventOutput NoiseRunOutput::singleEventOutput(const OperationArrival& Op) const {
        GRAPE_ASSERT(parentPerformanceRun().output().containsArrival(Op));
        return load(Op);
    }

    NoiseSingleEventOutput NoiseRunOutput::singleEventOutput(const OperationDeparture& Op) const {
        GRAPE_ASSERT(parentPerformanceRun().output().containsDeparture(Op));
        return load(Op);
    }

    const NoiseCumulativeOutput& NoiseRunOutput::cumulativeOutput(const NoiseCumulativeMetric& Metric) const {
        return m_CumulativeOutputs.at(&Metric);
    }

    void NoiseRunOutput::setReceptorOutput(ReceptorOutput&& ReceptOutput) {
        std::scoped_lock lck(m_DbMutex);
        m_ReceptorOutput = std::move(ReceptOutput);
        saveReceptorOutput();
    }

    void NoiseRunOutput::addSingleEvent(const Operation& Op, const NoiseSingleEventOutput& NsOut) const {
        GRAPE_ASSERT(NsOut.size() == m_ReceptorOutput.size());
        std::scoped_lock lck(m_DbMutex);
        saveSingleEvent(Op, NsOut);
    }

    void NoiseRunOutput::startCumulative() {
        std::scoped_lock lck(m_CumOutMutex);
        for (const auto& metric : parentNoiseRun().CumulativeMetrics | std::views::values)
        {
            auto [nsCumOut, added] = m_CumulativeOutputs.add(&metric, m_ReceptorOutput.size(), metric.numberAboveThresholds().size());
            GRAPE_ASSERT(added);
        }
    }

    void NoiseRunOutput::accumulate(const Operation& Op, const NoiseSingleEventOutput& NsOut) {
        std::scoped_lock lck(m_CumOutMutex);
        for (const auto& metric : parentNoiseRun().CumulativeMetrics | std::views::values)
        {
            if (parentNoiseRun().skipOperation(Op))
                continue;

            m_CumulativeOutputs.at(&metric).accumulateSingleEventOutput(NsOut, Op.Count, metric.weight(Op.timeOfDay()), metric.Threshold, metric.numberAboveThresholds());
        }
    }

    void NoiseRunOutput::finishCumulative() {
        {
            std::scoped_lock lck(m_CumOutMutex);
            for (const auto& metric : parentNoiseRun().CumulativeMetrics | std::views::values)
                m_CumulativeOutputs.at(&metric).finishAccumulation(metric.AveragingTimeConstant);
        }
        {
            std::scoped_lock lck(m_DbMutex);
            saveCumulative();
        }
    }

    void NoiseRunOutput::clear() {
        std::scoped_lock lck(m_DbMutex);

        if (empty())
            return;

        m_ReceptorOutput = ReceptorOutput();
        m_CumulativeOutputs.clear();

        m_Db.beginTransaction();
        m_Db.deleteD(Schema::noise_run_output_single_event, { 0, 1, 2 }, std::make_tuple(m_NoiseRun.parentScenario().Name, m_NoiseRun.parentPerformanceRun().Name, m_NoiseRun.Name));
        m_Db.deleteD(Schema::noise_run_output_cumulative, { 0, 1, 2 }, std::make_tuple(m_NoiseRun.parentScenario().Name, m_NoiseRun.parentPerformanceRun().Name, m_NoiseRun.Name));
        m_Db.deleteD(Schema::noise_run_output_cumulative_number_above, { 0, 1, 2 }, std::make_tuple(m_NoiseRun.parentScenario().Name, m_NoiseRun.parentPerformanceRun().Name, m_NoiseRun.Name));
        m_Db.deleteD(Schema::noise_run_output_receptors, { 0, 1, 2 }, std::make_tuple(m_NoiseRun.parentScenario().Name, m_NoiseRun.parentPerformanceRun().Name, m_NoiseRun.Name));
        m_Db.commitTransaction();
    }

    NoiseSingleEventOutput NoiseRunOutput::load(const Operation& Op) const {
        NoiseSingleEventOutput out(m_ReceptorOutput.size());

        m_Db.beginTransaction();

        Statement stmt(m_Db, Schema::noise_run_output_single_event.querySelect({ 7, 8 }, { 0, 1, 2, 4, 5, 6 }, { 3 }));
        stmt.bindValues(m_NoiseRun.parentScenario().Name, m_NoiseRun.parentPerformanceRun().Name, m_NoiseRun.Name, Op.Name, OperationTypes.toString(Op.operationType()), Operation::Types.toString(Op.type()));
        stmt.step();
        while (stmt.hasRow())
        {
            const double lamax = stmt.getColumn(0);
            const double sel = stmt.getColumn(1);
            out.addValues(lamax, sel);
            stmt.step();
        }
        m_Db.commitTransaction();

        return out;
    }

    void NoiseRunOutput::saveReceptorOutput() const {
        m_Db.beginTransaction();
        for (const auto& recept : m_ReceptorOutput)
        {
            m_Db.insert(Schema::noise_run_output_receptors, {}, std::make_tuple(
                m_NoiseRun.parentScenario().Name,
                m_NoiseRun.parentPerformanceRun().Name,
                m_NoiseRun.Name,
                recept.Name,
                recept.Longitude,
                recept.Latitude,
                recept.Elevation
            ));
        }
        m_Db.commitTransaction();
    }

    void NoiseRunOutput::saveSingleEvent(const Operation& Op, const NoiseSingleEventOutput& NsOutput) const {
        m_Db.beginTransaction();
        for (std::size_t i = 0; i < NsOutput.size(); ++i)
        {
            auto& [lamax, sel] = NsOutput.values(i);
            m_Db.insert(Schema::noise_run_output_single_event, {}, std::make_tuple(
                m_NoiseRun.parentScenario().Name,
                m_NoiseRun.parentPerformanceRun().Name,
                m_NoiseRun.Name,
                m_ReceptorOutput(i).Name,
                Op.Name,
                OperationTypes.toString(Op.operationType()),
                Operation::Types.toString(Op.type()),
                lamax,
                sel)
            );
        }
        m_Db.commitTransaction();
    }

    void NoiseRunOutput::saveCumulative() const {
        m_Db.beginTransaction();
        for (const auto& [cumMetric, cumOutput] : m_CumulativeOutputs)
        {
            {
                // Cumulative Output
                for (std::size_t i = 0; i < m_ReceptorOutput.size(); ++i)
                {
                    m_Db.insert(Schema::noise_run_output_cumulative, {}, std::make_tuple(
                        m_NoiseRun.parentScenario().Name,
                        m_NoiseRun.parentPerformanceRun().Name,
                        m_NoiseRun.Name,
                        cumMetric->Name,
                        m_ReceptorOutput.receptor(i).Name,
                        cumOutput.Count.at(i),
                        cumOutput.CountWeighted.at(i),
                        cumOutput.MaximumAbsolute.at(i),
                        cumOutput.MaximumAverage.at(i),
                        cumOutput.Exposure.at(i))
                    );
                }
            }
            {
                // Cumulative Output Number above Thresholds
                for (std::size_t i = 0; i < cumMetric->numberAboveThresholds().size(); ++i)
                {
                    const auto& outNat = cumOutput.NumberAboveThresholds.at(i);
                    for (std::size_t j = 0; j < m_ReceptorOutput.size(); ++j)
                    {
                        m_Db.insert(Schema::noise_run_output_cumulative_number_above, {}, std::make_tuple(
                            m_NoiseRun.parentScenario().Name,
                            m_NoiseRun.parentPerformanceRun().Name,
                            m_NoiseRun.Name,
                            cumMetric->Name,
                            cumMetric->numberAboveThresholds().at(i),
                            m_ReceptorOutput.receptor(j).Name,
                            outNat.at(j)
                        ));
                    }
                }
            }
        }
        m_Db.commitTransaction();
    }
}
