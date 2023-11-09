// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "ScenariosManager.h"

#include "Jobs/JobManager.h"
#include "Noise/ReceptorSets.h"
#include "OperationsManager.h"
#include "Schema/Schema.h"

namespace GRAPE {
    namespace {
        auto allValues(const NoiseRun& NsRun) {
            const auto& spec = NsRun.NsRunSpec;
            return std::make_tuple(NsRun.parentScenario().Name, NsRun.parentPerformanceRun().Name, NsRun.Name, NoiseModelTypes.toString(spec.NoiseMdl), AtmosphericAbsorption::Types.toString(spec.AtmAbsorptionType), ReceptorSet::Types.toString(spec.ReceptSet->type()), static_cast<int>(NsRun.NsRunSpec.SaveSingleMetrics));
        }

        auto allValues(const NoiseCumulativeMetric& NsCumMetric) { return std::make_tuple(NsCumMetric.parentScenario().Name, NsCumMetric.parentPerformanceRun().Name, NsCumMetric.parentNoiseRun().Name, NsCumMetric.Name, NsCumMetric.Threshold, NsCumMetric.AveragingTimeConstant, timeToUtcString(NsCumMetric.StartTimePoint), timeToUtcString(NsCumMetric.EndTimePoint)); }

        struct PerformanceRunUpdater : CoordinateSystemVisitor {
            PerformanceRunUpdater(const Database& Db, const PerformanceRun& PerfRun);
            void visitLocalCartesian(LocalCartesian& Cs) override;
            void visitGeodesic(Geodesic& Cs) override;

        private:
            const Database& m_Db;
            const PerformanceRun& m_PerfRun;
            Statement m_Stmt;
        };

        struct ReceptorSetUpdater : ReceptorSetVisitor {
            ReceptorSetUpdater(const Database& Db, const NoiseRun& NsRunIn) : m_Db(Db), m_NoiseRun(NsRunIn) { std::cref(*m_NoiseRun.NsRunSpec.ReceptSet).get().accept(*this); }
            void visitGrid(const ReceptorGrid& ReceptSet) override;
            void visitPoints(const ReceptorPoints& ReceptSet) override;

        private:
            const Database& m_Db;
            const NoiseRun& m_NoiseRun;
        };
    }

    ScenariosManager::ScenariosManager(const Database& Db, Constraints& Blocks, OperationsManager& Ops, JobManager& Jobs) : Manager(Db, Blocks), m_Operations(Ops), m_Jobs(Jobs) {}

    std::pair<Scenario&, bool> ScenariosManager::addScenario(const std::string& Name) {
        const std::string newName = Name.empty() ? uniqueKeyGenerator(m_Scenarios, "New Scenario") : Name;

        auto ret = m_Scenarios.add(newName, newName);
        auto& [scen, added] = ret;

        if (added)
            m_Db.insert(Schema::scenarios, {}, std::make_tuple(scen.Name));
        else
            Log::dataLogic()->error("Adding scenario '{}'. Scenario already exists in this study.", newName);

        return ret;
    }

    std::pair<PerformanceRun&, bool> ScenariosManager::addPerformanceRun(Scenario& Scen, const std::string& Name) const {
        const std::string newName = Name.empty() ? uniqueKeyGenerator(Scen.PerformanceRuns, "New Performance Run") : Name;

        auto ret = Scen.PerformanceRuns.add(newName, Scen, newName);
        auto& [perfRun, added] = ret;

        if (added)
        {
            m_Db.insert(Schema::performance_run, { 0, 1 }, std::make_tuple(perfRun.parentScenario().Name, perfRun.Name));
            perfRun.createJob(m_Db, m_Operations);
        }
        else { Log::dataLogic()->error("Adding performance run '{}'. Performance run already exists in scenario '{}'.", newName, Scen.Name); }

        return ret;
    }

    std::pair<NoiseRun&, bool> ScenariosManager::addNoiseRun(PerformanceRun& PerfRun, const std::string& Name) const {
        const std::string newName = Name.empty() ? uniqueKeyGenerator(PerfRun.NoiseRuns, "New Noise Run") : Name;

        auto ret = PerfRun.NoiseRuns.add(newName, PerfRun, newName);
        auto& [nsRun, added] = ret;

        if (added)
        {
            m_Db.insert(Schema::noise_run, { 0, 1, 2 }, std::make_tuple(nsRun.parentScenario().Name, nsRun.parentPerformanceRun().Name, nsRun.Name));
            ReceptorSetUpdater up(m_Db, nsRun);
            nsRun.createJob(m_Db, m_Blocks);
        }
        else { Log::dataLogic()->error("Adding noise run '{}'. Noise run already exists in performance run '{}' of scenario '{}'.", newName, PerfRun.Name, PerfRun.parentScenario().Name); }

        return ret;
    }

    std::pair<NoiseCumulativeMetric&, bool> ScenariosManager::addNoiseCumulativeMetric(NoiseRun& NsRun, const std::string& Name) const {
        const std::string newName = Name.empty() ? uniqueKeyGenerator(NsRun.CumulativeMetrics, "New Cumulative Metric") : Name;

        auto ret = NsRun.CumulativeMetrics.add(newName, NsRun, newName);
        auto& [cumMetric, added] = ret;

        if (added)
        {
            m_Db.insert(Schema::noise_run_cumulative_metrics, {}, allValues(cumMetric));
            update(cumMetric);
        }
        else { Log::dataLogic()->error("Adding noise run cumulative metric '{}'. Cumulative metric already exists in noise run '{}' of performance run '{}' of scenario '{}'.", newName, NsRun.Name, NsRun.parentPerformanceRun().Name, NsRun.parentScenario().Name); }

        return ret;
    }

    std::pair<EmissionsRun&, bool> ScenariosManager::addEmissionsRun(PerformanceRun& PerfRun, const std::string& Name) const {
        const std::string newName = Name.empty() ? uniqueKeyGenerator(PerfRun.EmissionsRuns, "New Emissions Run") : Name;

        auto ret = PerfRun.EmissionsRuns.add(newName, PerfRun, newName);
        auto& [emiRun, added] = ret;

        if (added)
        {
            m_Db.insert(Schema::emissions_run, { 0, 1, 2 }, std::make_tuple(emiRun.parentScenario().Name, emiRun.parentPerformanceRun().Name, emiRun.Name));
            emiRun.createJob(m_Db, m_Blocks);
        }
        else { Log::dataLogic()->error("Adding emissions run '{}'. Emissions run already exists in performance run '{}' of scenario '{}'.", newName, PerfRun.Name, PerfRun.parentScenario().Name); }

        return ret;
    }

    Scenario& ScenariosManager::addScenarioE(const std::string& Name) {
        if (Name.empty())
            throw GrapeException("Empty scenario name not allowed.");

        auto [scen, added] = m_Scenarios.add(Name, Name);

        if (added)
            m_Db.insert(Schema::scenarios, { 0 }, std::make_tuple(scen.Name));
        else
            throw GrapeException(std::format("Scenario '{}' already exists in this study.", Name));

        return scen;
    }

    PerformanceRun& ScenariosManager::addPerformanceRunE(Scenario& Scen, const std::string& Name) const {
        if (Name.empty())
            throw GrapeException("Empty performance run name not allowed.");

        auto [perfRun, added] = Scen.PerformanceRuns.add(Name, Scen, Name);

        if (added)
        {
            m_Db.insert(Schema::performance_run, { 0, 1 }, std::make_tuple(perfRun.parentScenario().Name, perfRun.Name));
            perfRun.createJob(m_Db, m_Operations);
        }
        else { throw GrapeException(std::format("Performance run '{}' already exists in scenario '{}'.", perfRun.Name, Scen.Name)); }

        return perfRun;
    }

    NoiseRun& ScenariosManager::addNoiseRunE(PerformanceRun& PerfRun, const std::string& Name) const {
        if (Name.empty())
            throw GrapeException("Empty noise run name not allowed");

        auto [nsRun, added] = PerfRun.NoiseRuns.add(Name, PerfRun, Name);

        if (added)
        {
            m_Db.insert(Schema::noise_run, { 0, 1, 2 }, std::make_tuple(nsRun.parentScenario().Name, nsRun.parentPerformanceRun().Name, nsRun.Name));
            ReceptorSetUpdater up(m_Db, nsRun);
            nsRun.createJob(m_Db, m_Blocks);
        }
        else { throw GrapeException(std::format("Noise run '{}' already exists in performance run '{}' of scenario '{}'.", nsRun.Name, PerfRun.Name, PerfRun.parentScenario().Name)); }

        return nsRun;
    }

    NoiseCumulativeMetric& ScenariosManager::addNoiseCumulativeMetricE(NoiseRun& NsRun, const std::string& Name) const {
        if (Name.empty())
            throw GrapeException("Empty noise cumulative metric name not allowed");

        auto [cumMetric, added] = NsRun.CumulativeMetrics.add(Name, NsRun, Name);

        if (added)
        {
            m_Db.insert(Schema::noise_run_cumulative_metrics, {}, allValues(cumMetric));
            update(cumMetric);
        }
        else { throw GrapeException(std::format("Cumulative metric '{}' already exists in noise run '{}' of performance run '{}' of scenario '{}'.", Name, NsRun.Name, NsRun.parentPerformanceRun().Name, NsRun.parentScenario().Name)); }

        return cumMetric;
    }

    EmissionsRun& ScenariosManager::addEmissionsRunE(PerformanceRun& PerfRun, const std::string& Name) const {
        if (Name.empty())
            throw GrapeException("Empty emissions run name not allowed");

        auto [emiRun, added] = PerfRun.EmissionsRuns.add(Name, PerfRun, Name);

        if (added)
        {
            m_Db.insert(Schema::emissions_run, { 0, 1, 2 }, std::make_tuple(emiRun.parentScenario().Name, emiRun.parentPerformanceRun().Name, emiRun.Name));
            emiRun.createJob(m_Db, m_Blocks);
        }
        else { throw GrapeException(std::format("Emissions run '{}' already exists in performance run '{}' of scenario '{}'.", emiRun.Name, PerfRun.Name, PerfRun.parentScenario().Name)); }

        return emiRun;
    }

    void ScenariosManager::eraseScenarios() {
        m_Scenarios.eraseIf([&](auto& Node) {
            auto& [scenName, scen] = Node;

            for (const auto& perfRun : scen.PerformanceRuns | std::views::values)
            {
                for (const auto& nsRun : perfRun.NoiseRuns | std::views::values)
                    m_Jobs.resetJob(nsRun.job());
                for (const auto& emiRun : perfRun.EmissionsRuns | std::views::values)
                    m_Jobs.resetJob(emiRun.job());
                m_Jobs.resetJob(perfRun.job());
            }

            m_Blocks.scenarioUnblock(scen);
            m_Db.deleteD(Schema::scenarios, { 0 }, std::make_tuple(scen.Name));

            return true;
            });
    }

    void ScenariosManager::eraseOutputs() {
        for (const auto& scen : m_Scenarios | std::views::values)
        {
            for (const auto& perfRun : scen.PerformanceRuns | std::views::values)
            {
                for (const auto& nsRun : perfRun.NoiseRuns | std::views::values)
                    m_Jobs.resetJob(nsRun.job());
                for (const auto& emiRun : perfRun.EmissionsRuns | std::views::values)
                    m_Jobs.resetJob(emiRun.job());
                m_Jobs.resetJob(perfRun.job());
            }
        }

        m_Db.deleteD(Schema::performance_run_output);
        m_Db.deleteD(Schema::noise_run_output_receptors);
        m_Db.deleteD(Schema::emissions_run_output);
    }

    void ScenariosManager::erase(const Scenario& Scen) {
        for (const auto& perfRun : Scen.PerformanceRuns | std::views::values)
        {
            for (const auto& nsRun : perfRun.NoiseRuns | std::views::values)
                m_Jobs.resetJob(nsRun.job());
            for (const auto& emiRun : perfRun.EmissionsRuns | std::views::values)
                m_Jobs.resetJob(emiRun.job());
            m_Jobs.resetJob(perfRun.job());
        }

        m_Blocks.scenarioUnblock(Scen);
        m_Db.deleteD(Schema::scenarios, { 0 }, std::make_tuple(Scen.Name));

        m_Scenarios.erase(Scen.Name);
    }

    void ScenariosManager::erase(const PerformanceRun& PerfRun) const {
        for (const auto& nsRun : PerfRun.NoiseRuns | std::views::values)
            m_Jobs.resetJob(nsRun.job());
        for (const auto& emiRun : PerfRun.EmissionsRuns | std::views::values)
            m_Jobs.resetJob(emiRun.job());
        m_Jobs.resetJob(PerfRun.job());

        m_Db.deleteD(Schema::performance_run, { 0, 1 }, std::make_tuple(PerfRun.parentScenario().Name, PerfRun.Name));
        PerfRun.parentScenario().PerformanceRuns.erase(PerfRun.Name);
    }

    void ScenariosManager::erase(const NoiseRun& NsRun) const {
        m_Jobs.resetJob(NsRun.job());
        m_Db.deleteD(Schema::noise_run, { 0, 1, 2 }, std::make_tuple(NsRun.parentScenario().Name, NsRun.parentPerformanceRun().Name, NsRun.Name));

        NsRun.parentPerformanceRun().NoiseRuns.erase(NsRun.Name);
    }

    void ScenariosManager::erase(const NoiseCumulativeMetric& NsCumMetric) const {
        if (!NsCumMetric.parentNoiseRun().job()->ready())
        {
            Log::dataLogic()->error("Removing noise run cumulative metric '{}'. Noise run '{}' of performance run '{}' of scenario '{}' has started.", NsCumMetric.Name, NsCumMetric.parentNoiseRun().Name, NsCumMetric.parentPerformanceRun().Name, NsCumMetric.parentScenario().Name);
            return;
        }

        m_Db.deleteD(Schema::noise_run_cumulative_metrics, { 0, 1, 2, 3 }, std::make_tuple(NsCumMetric.parentScenario().Name, NsCumMetric.parentPerformanceRun().Name, NsCumMetric.parentNoiseRun().Name, NsCumMetric.Name));

        NsCumMetric.parentNoiseRun().CumulativeMetrics.erase(NsCumMetric.Name);
    }

    void ScenariosManager::eraseNoiseCumulativeMetrics(NoiseRun& NsRun) const {
        NsRun.CumulativeMetrics.eraseIf([&](auto& Node) {
            auto& [cumMetricId, cumMetric] = Node;

            if (!cumMetric.parentNoiseRun().job()->ready())
            {
                Log::dataLogic()->error("Removing noise run cumulative metric '{}'. Noise run '{}' of performance run '{}' of scenario '{}' has started.", cumMetric.Name, cumMetric.parentNoiseRun().Name, cumMetric.parentPerformanceRun().Name, cumMetric.parentScenario().Name);
                return false;
            }

            m_Db.deleteD(Schema::noise_run_cumulative_metrics, { 0, 1, 2, 3 }, std::make_tuple(cumMetric.parentScenario().Name, cumMetric.parentPerformanceRun().Name, cumMetric.parentNoiseRun().Name, cumMetric.Name));

            return true;
            });
    }

    void ScenariosManager::erase(const EmissionsRun& EmiRun) const {
        m_Jobs.resetJob(EmiRun.job());
        m_Db.deleteD(Schema::emissions_run, { 0, 1, 2 }, std::make_tuple(EmiRun.parentScenario().Name, EmiRun.parentPerformanceRun().Name, EmiRun.Name));

        EmiRun.parentPerformanceRun().EmissionsRuns.erase(EmiRun.Name);
    }

    bool ScenariosManager::updateKey(Scenario& Scen, const std::string Id) {
        if (Scen.Name.empty())
        {
            Log::dataLogic()->error("Updating scenario '{}'. Empty name not allowed.", Id);
            Scen.Name = Id;
            return false;
        }

        const bool updated = m_Scenarios.update(Id, Scen.Name);

        if (updated) { m_Db.update(Schema::scenarios, { 0 }, std::make_tuple(Scen.Name), { 0 }, std::make_tuple(Id)); }
        else
        {
            Log::dataLogic()->error("Updating scenario '{}'. Scenario new name '{}' already exists in this study.", Id, Scen.Name);
            Scen.Name = Id;
        }

        return updated;
    }

    bool ScenariosManager::updateKey(PerformanceRun& PerfRun, const std::string Id) const {
        if (PerfRun.Name.empty())
        {
            Log::dataLogic()->error("Updating performance run '{}'. Empty name not allowed.", Id);
            PerfRun.Name = Id;
            return false;
        }

        const bool updated = PerfRun.parentScenario().PerformanceRuns.update(Id, PerfRun.Name);

        if (updated) { m_Db.update(Schema::performance_run, { 1 }, std::make_tuple(PerfRun.Name), { 0, 1 }, std::make_tuple(PerfRun.parentScenario().Name, Id)); }
        else
        {
            Log::dataLogic()->error("Updating scenario run '{}'. Performance run new name '{}' already exists in scenario '{}'.", Id, PerfRun.Name, PerfRun.parentScenario().Name);
            PerfRun.Name = Id;
        }

        return updated;
    }

    bool ScenariosManager::updateKey(NoiseRun& NsRun, const std::string Id) const {
        if (NsRun.Name.empty())
        {
            Log::dataLogic()->error("Updating noise run '{}'. Empty name not allowed.", Id);
            NsRun.Name = Id;
            return false;
        }

        const bool updated = NsRun.parentPerformanceRun().NoiseRuns.update(Id, NsRun.Name);

        if (updated) { m_Db.update(Schema::noise_run, { 2 }, std::make_tuple(NsRun.Name), { 0, 1, 2 }, std::make_tuple(NsRun.parentScenario().Name, NsRun.parentPerformanceRun().Name, Id)); }
        else
        {
            Log::dataLogic()->error("Updating noise run '{}'. Noise run new name '{}' already exists in performance run '{}' of scenario '{}'.", Id, NsRun.Name, NsRun.parentPerformanceRun().Name, NsRun.parentScenario().Name);
            NsRun.Name = Id;
        }

        return updated;
    }

    bool ScenariosManager::updateKey(NoiseCumulativeMetric& NsCumMetric, std::string Id) const {
        if (NsCumMetric.Name.empty())
        {
            Log::dataLogic()->error("Updating noise run cumulative metric '{}'. Empty name not allowed.", Id);
            NsCumMetric.Name = Id;
            return false;
        }

        const bool updated = NsCumMetric.parentNoiseRun().CumulativeMetrics.update(Id, NsCumMetric.Name);

        if (updated) { m_Db.update(Schema::noise_run_cumulative_metrics, { 3 }, std::make_tuple(NsCumMetric.Name), { 0, 1, 2, 3 }, std::make_tuple(NsCumMetric.parentScenario().Name, NsCumMetric.parentPerformanceRun().Name, NsCumMetric.parentNoiseRun().Name, Id)); }
        else
        {
            Log::dataLogic()->error("Updating noise run cumulative metric '{}'. Noise run cumulative metric new name '{}' already exists in noise run '{}' of performance run '{}' of scenario '{}'.", Id, NsCumMetric.Name, NsCumMetric.parentNoiseRun().Name, NsCumMetric.parentPerformanceRun().Name, NsCumMetric.parentScenario().Name);
            NsCumMetric.Name = Id;
        }

        return updated;
    }

    bool ScenariosManager::updateKey(EmissionsRun& EmiRun, std::string Id) const {
        if (EmiRun.Name.empty())
        {
            Log::dataLogic()->error("Updating emissions run '{}'. Empty name not allowed.", Id);
            EmiRun.Name = Id;
            return false;
        }

        const bool updated = EmiRun.parentPerformanceRun().EmissionsRuns.update(Id, EmiRun.Name);

        if (updated) { m_Db.update(Schema::emissions_run, { 2 }, std::make_tuple(EmiRun.Name), { 0, 1, 2 }, std::make_tuple(EmiRun.parentScenario().Name, EmiRun.parentPerformanceRun().Name, Id)); }
        else
        {
            Log::dataLogic()->error("Updating emissions run '{}'. Emissions run new name '{}' already exists in performance run '{}' of scenario '{}'.", Id, EmiRun.Name, EmiRun.parentPerformanceRun().Name, EmiRun.parentScenario().Name);
            EmiRun.Name = Id;
        }

        return updated;
    }

    void ScenariosManager::update(const PerformanceRun& PerfRun) const { PerformanceRunUpdater(m_Db, PerfRun); }

    void ScenariosManager::update(const NoiseRun& NsRun) const {
        m_Db.update(Schema::noise_run, allValues(NsRun), { 0, 1, 2 }, std::make_tuple(NsRun.parentScenario().Name, NsRun.parentPerformanceRun().Name, NsRun.Name));
        ReceptorSetUpdater up(m_Db, NsRun);
    }

    void ScenariosManager::update(const NoiseCumulativeMetric& NsCumMetric) const {
        // Reinsert
        m_Db.deleteD(Schema::noise_run_cumulative_metrics, { 0, 1, 2, 3 }, std::make_tuple(NsCumMetric.parentScenario().Name, NsCumMetric.parentPerformanceRun().Name, NsCumMetric.parentNoiseRun().Name, NsCumMetric.Name));

        m_Db.insert(Schema::noise_run_cumulative_metrics, {}, allValues(NsCumMetric));

        for (const auto& [time, weight] : NsCumMetric.weights())
            m_Db.insert(Schema::noise_run_cumulative_metrics_weights, {}, std::make_tuple(NsCumMetric.parentScenario().Name, NsCumMetric.parentPerformanceRun().Name, NsCumMetric.parentNoiseRun().Name, NsCumMetric.Name, durationToString(time), weight));

        for (const auto threshold : NsCumMetric.numberAboveThresholds())
            m_Db.insert(Schema::noise_run_cumulative_metrics_number_above_thresholds, {}, std::make_tuple(NsCumMetric.parentScenario().Name, NsCumMetric.parentPerformanceRun().Name, NsCumMetric.parentNoiseRun().Name, NsCumMetric.Name, threshold));
    }

    void ScenariosManager::update(const EmissionsRun& EmiRun) const {
        const auto& spec = EmiRun.EmissionsRunSpec;

        std::size_t col = 0;
        Statement stmt(m_Db, Schema::emissions_run.queryUpdate({}, { 0, 1, 2 }));
        stmt.bind(col++, EmiRun.parentScenario().Name);
        stmt.bind(col++, EmiRun.parentPerformanceRun().Name);
        stmt.bind(col++, EmiRun.Name);
        stmt.bind(col++, static_cast<int>(spec.CalculateGasEmissions));
        stmt.bind(col++, static_cast<int>(spec.CalculateParticleEmissions));
        stmt.bind(col++, EmissionsModelTypes.toString(spec.EmissionsMdl));
        stmt.bind(col++, static_cast<int>(spec.BFFM2Model));
        stmt.bind(col++, EmissionsParticleSmokeNumberModelTypes.toString(spec.ParticleSmokeNumberModel));
        stmt.bind(col++, spec.LTOCycle.at(0));
        stmt.bind(col++, spec.LTOCycle.at(1));
        stmt.bind(col++, spec.LTOCycle.at(2));
        stmt.bind(col++, spec.LTOCycle.at(3));
        stmt.bind(col++, spec.ParticleEffectiveDensity);
        stmt.bind(col++, spec.ParticleGeometricStandardDeviation);
        stmt.bind(col++, spec.ParticleGeometricMeanDiameter.at(0));
        stmt.bind(col++, spec.ParticleGeometricMeanDiameter.at(1));
        stmt.bind(col++, spec.ParticleGeometricMeanDiameter.at(2));
        stmt.bind(col++, spec.ParticleGeometricMeanDiameter.at(3));
        std::isinf(spec.FilterMinimumAltitude) ? stmt.bind(col++, std::monostate()) : stmt.bind(col++, spec.FilterMinimumAltitude);
        std::isinf(spec.FilterMaximumAltitude) ? stmt.bind(col++, std::monostate()) : stmt.bind(col++, spec.FilterMaximumAltitude);
        std::isinf(spec.FilterMinimumCumulativeGroundDistance) ? stmt.bind(col++, std::monostate()) : stmt.bind(col++, spec.FilterMinimumCumulativeGroundDistance);
        std::isinf(spec.FilterMaximumCumulativeGroundDistance) ? stmt.bind(col++, std::monostate()) : stmt.bind(col++, spec.FilterMaximumCumulativeGroundDistance);
        stmt.bind(col++, static_cast<int>(spec.SaveSegmentResults));

        stmt.bind(col++, EmiRun.parentScenario().Name);
        stmt.bind(col++, EmiRun.parentPerformanceRun().Name);
        stmt.bind(col++, EmiRun.Name);

        stmt.step();
    }

    bool ScenariosManager::addFlightArrival(Scenario& Scen, const FlightArrival& Op) const {
        if (Scen.contains(Op))
            return false;

        m_Blocks.scenarioBlockOperation(Scen, Op);
        m_Db.insert(Schema::scenarios_flights, {}, std::make_tuple(Scen.Name, Op.Name, OperationTypes.toString(Op.operationType())));

        Scen.FlightArrivals.emplace_back(Op);
        return true;
    }

    bool ScenariosManager::addFlightDeparture(Scenario& Scen, const FlightDeparture& Op) const {
        if (Scen.contains(Op))
            return false;

        m_Blocks.scenarioBlockOperation(Scen, Op);
        m_Db.insert(Schema::scenarios_flights, {}, std::make_tuple(Scen.Name, Op.Name, OperationTypes.toString(Op.operationType())));

        Scen.FlightDepartures.emplace_back(Op);
        return true;
    }

    bool ScenariosManager::addTrack4dArrival(Scenario& Scen, const Track4dArrival& Op) const {
        if (Scen.contains(Op))
            return false;

        m_Blocks.scenarioBlockOperation(Scen, Op);
        m_Db.insert(Schema::scenarios_tracks_4d, {}, std::make_tuple(Scen.Name, Op.Name, OperationTypes.toString(Op.operationType())));

        Scen.Track4dArrivals.emplace_back(Op);
        return true;
    }

    bool ScenariosManager::addTrack4dDeparture(Scenario& Scen, const Track4dDeparture& Op) const {
        if (Scen.contains(Op))
            return false;

        m_Blocks.scenarioBlockOperation(Scen, Op);
        m_Db.insert(Schema::scenarios_tracks_4d, {}, std::make_tuple(Scen.Name, Op.Name, OperationTypes.toString(Op.operationType())));

        Scen.Track4dDepartures.emplace_back(Op);
        return true;
    }

    void ScenariosManager::addFlightArrivalE(Scenario& Scen, const std::string& OpName) const {
        if (!m_Operations.flightArrivals().contains(OpName))
            throw GrapeException(std::format("Arrival flight '{}' does not exist in this study.", OpName));
        const FlightArrival& op = m_Operations.flightArrivals()(OpName);

        if (Scen.contains(op))
            throw GrapeException(std::format("Arrival flight '{}' is already in scenario '{}'.", OpName, Scen.Name));

        m_Blocks.scenarioBlockOperation(Scen, op);
        m_Db.insert(Schema::scenarios_flights, {}, std::make_tuple(Scen.Name, op.Name, OperationTypes.toString(op.operationType())));

        Scen.FlightArrivals.emplace_back(op);
    }

    void ScenariosManager::addFlightDepartureE(Scenario& Scen, const std::string& OpName) const {
        if (!m_Operations.flightDepartures().contains(OpName))
            throw GrapeException(std::format("Departure flight '{}' does not exist in this study.", OpName));
        const FlightDeparture& op = m_Operations.flightDepartures()(OpName);

        if (Scen.contains(op))
            throw GrapeException(std::format("Departure flight '{}' is already in scenario '{}'.", OpName, Scen.Name));

        m_Blocks.scenarioBlockOperation(Scen, op);
        m_Db.insert(Schema::scenarios_flights, {}, std::make_tuple(Scen.Name, op.Name, OperationTypes.toString(op.operationType())));

        Scen.FlightDepartures.emplace_back(op);
    }

    void ScenariosManager::addTrack4dArrivalE(Scenario& Scen, const std::string& OpName) const {
        if (!m_Operations.track4dArrivals().contains(OpName))
            throw GrapeException(std::format("Arrival track 4D '{}' does not exist in this study.", OpName));
        const Track4dArrival& op = m_Operations.track4dArrivals()(OpName);

        if (Scen.contains(op))
            throw GrapeException(std::format("Arrival track 4D '{}' is already in scenario '{}'.", OpName, Scen.Name));

        m_Blocks.scenarioBlockOperation(Scen, op);
        m_Db.insert(Schema::scenarios_tracks_4d, {}, std::make_tuple(Scen.Name, op.Name, OperationTypes.toString(op.operationType())));

        Scen.Track4dArrivals.emplace_back(op);
    }

    void ScenariosManager::addTrack4dDepartureE(Scenario& Scen, const std::string& OpName) const {
        if (!m_Operations.track4dDepartures().contains(OpName))
            throw GrapeException(std::format("Departure track 4D '{}' does not exist in this study.", OpName));
        const Track4dDeparture& op = m_Operations.track4dDepartures()(OpName);

        if (Scen.contains(op))
            throw GrapeException(std::format("Departure track 4D '{}' is already in scenario '{}'.", OpName, Scen.Name));

        m_Blocks.scenarioBlockOperation(Scen, op);
        m_Db.insert(Schema::scenarios_tracks_4d, {}, std::make_tuple(Scen.Name, op.Name, OperationTypes.toString(op.operationType())));

        Scen.Track4dDepartures.emplace_back(op);
    }

    bool ScenariosManager::eraseFlightArrival(Scenario& Scen, const FlightArrival& Op) const {
        if (!Scen.contains(Op))
            return false;

        m_Blocks.scenarioUnblockOperation(Scen, Op);
        m_Db.deleteD(Schema::scenarios_flights, { 0, 1, 2 }, std::make_tuple(Scen.Name, Op.Name, OperationTypes.toString(Op.operationType())));

        Scen.FlightArrivals.erase(std::ranges::find_if(Scen.FlightArrivals, [&](const std::reference_wrapper<const FlightArrival> CompOp) { return &Op == &CompOp.get(); }));
        return true;
    }

    bool ScenariosManager::eraseFlightDeparture(Scenario& Scen, const FlightDeparture& Op) const {
        if (!Scen.contains(Op))
            return false;

        m_Blocks.scenarioUnblockOperation(Scen, Op);
        m_Db.deleteD(Schema::scenarios_flights, { 0, 1, 2 }, std::make_tuple(Scen.Name, Op.Name, OperationTypes.toString(Op.operationType())));

        Scen.FlightDepartures.erase(std::ranges::find_if(Scen.FlightDepartures, [&](const std::reference_wrapper<const FlightDeparture> CompOp) { return &Op == &CompOp.get(); }));
        return true;
    }

    bool ScenariosManager::eraseTrack4dArrival(Scenario& Scen, const Track4dArrival& Op) const {
        if (!Scen.contains(Op))
            return false;

        m_Blocks.scenarioUnblockOperation(Scen, Op);
        m_Db.deleteD(Schema::scenarios_tracks_4d, { 0, 1, 2 }, std::make_tuple(Scen.Name, Op.Name, OperationTypes.toString(Op.operationType())));

        Scen.Track4dArrivals.erase(std::ranges::find_if(Scen.Track4dArrivals, [&](const std::reference_wrapper<const Track4dArrival> CompOp) { return &Op == &CompOp.get(); }));
        return true;
    }

    bool ScenariosManager::eraseTrack4dDeparture(Scenario& Scen, const Track4dDeparture& Op) const {
        if (!Scen.contains(Op))
            return false;

        m_Blocks.scenarioUnblockOperation(Scen, Op);
        m_Db.deleteD(Schema::scenarios_tracks_4d, { 0, 1, 2 }, std::make_tuple(Scen.Name, Op.Name, OperationTypes.toString(Op.operationType())));

        Scen.Track4dDepartures.erase(std::ranges::find_if(Scen.Track4dDepartures, [&](const std::reference_wrapper<const Track4dDeparture> CompOp) { return &Op == &CompOp.get(); }));
        return true;
    }

    void ScenariosManager::eraseFlights(Scenario& Scen) const {
        m_Blocks.scenarioUnblockFlights(Scen);
        m_Db.deleteD(Schema::scenarios_flights, { 0 }, std::make_tuple(Scen.Name));

        Scen.FlightArrivals.clear();
        Scen.FlightDepartures.clear();
    }

    void ScenariosManager::eraseTracks4d(Scenario& Scen) const {
        m_Blocks.scenarioUnblockTracks4d(Scen);
        m_Db.deleteD(Schema::scenarios_tracks_4d, { 0 }, std::make_tuple(Scen.Name));

        Scen.Track4dArrivals.clear();
        Scen.Track4dDepartures.clear();
    }

    PerformanceRunUpdater::PerformanceRunUpdater(const Database& Db, const PerformanceRun& PerfRun) : m_Db(Db), m_PerfRun(PerfRun), m_Stmt(Db, Schema::performance_run.queryUpdate({}, { 0, 1 })) {
        const auto& spec = PerfRun.PerfRunSpec;
        m_Stmt.bind(0, PerfRun.parentScenario().Name);
        m_Stmt.bind(1, PerfRun.Name);
        m_Stmt.bind(2, CoordinateSystem::Types.toString(spec.CoordSys->type()));
        m_PerfRun.PerfRunSpec.CoordSys->accept(*this);
        std::isinf(spec.FilterMinimumAltitude) ? m_Stmt.bind(5, std::monostate()) : m_Stmt.bind(5, spec.FilterMinimumAltitude);
        std::isinf(spec.FilterMaximumAltitude) ? m_Stmt.bind(6, std::monostate()) : m_Stmt.bind(6, spec.FilterMaximumAltitude);
        std::isinf(spec.FilterMinimumCumulativeGroundDistance) ? m_Stmt.bind(7, std::monostate()) : m_Stmt.bind(7, spec.FilterMinimumCumulativeGroundDistance);
        std::isinf(spec.FilterMaximumCumulativeGroundDistance) ? m_Stmt.bind(8, std::monostate()) : m_Stmt.bind(8, spec.FilterMaximumCumulativeGroundDistance);
        m_Stmt.bind(9, spec.FilterGroundDistanceThreshold);
        m_Stmt.bind(10, spec.SpeedDeltaSegmentationThreshold);
        m_Stmt.bind(11, PerformanceModelTypes.toString(spec.FlightsPerformanceMdl));
        m_Stmt.bind(12, spec.FlightsDoc29Segmentation);
        m_Stmt.bind(13, static_cast<int>(spec.Tracks4dCalculatePerformance));
        m_Stmt.bind(14, spec.Tracks4dMinimumPoints);
        m_Stmt.bind(15, static_cast<int>(spec.Tracks4dRecalculateCumulativeGroundDistance));
        m_Stmt.bind(16, static_cast<int>(spec.Tracks4dRecalculateGroundspeed));
        m_Stmt.bind(17, static_cast<int>(spec.Tracks4dRecalculateFuelFlow));
        m_Stmt.bind(18, FuelFlowModelTypes.toString(spec.FuelFlowMdl));
        m_Stmt.bind(19, static_cast<int>(spec.FuelFlowLTOAltitudeCorrection));

        m_Stmt.bind(20, PerfRun.parentScenario().Name);
        m_Stmt.bind(21, PerfRun.Name);

        m_Stmt.step();

        m_Db.deleteD(Schema::performance_run_atmospheres, { 0, 1 }, std::make_tuple(m_PerfRun.parentScenario().Name, m_PerfRun.Name));
        for (const auto& [time, atm] : m_PerfRun.PerfRunSpec.Atmospheres)
            m_Db.insert(Schema::performance_run_atmospheres, {}, std::make_tuple(
                m_PerfRun.parentScenario().Name,
                m_PerfRun.Name,
                timeToUtcString(time),
                atm.temperatureDelta(),
                atm.pressureDelta(),
                atm.windSpeed(),
                atm.windDirection(),
                atm.relativeHumidity()
            ));
    }

    void PerformanceRunUpdater::visitLocalCartesian(LocalCartesian& Cs) {
        auto [lon, lat] = Cs.origin();
        m_Stmt.bind(3, lon);
        m_Stmt.bind(4, lat);
    };

    void PerformanceRunUpdater::visitGeodesic(Geodesic& Cs) {
        m_Stmt.bind(3, std::monostate());
        m_Stmt.bind(4, std::monostate());
    };

    void ReceptorSetUpdater::visitGrid(const ReceptorGrid& ReceptSet) {
        m_Db.deleteD(Schema::noise_run_receptor_grid, { 0, 1, 2 }, std::make_tuple(m_NoiseRun.parentScenario().Name, m_NoiseRun.parentPerformanceRun().Name, m_NoiseRun.Name));
        m_Db.deleteD(Schema::noise_run_receptor_points, { 0, 1, 2 }, std::make_tuple(m_NoiseRun.parentScenario().Name, m_NoiseRun.parentPerformanceRun().Name, m_NoiseRun.Name));

        m_Db.insert(Schema::noise_run_receptor_grid, {}, std::make_tuple(m_NoiseRun.parentScenario().Name, m_NoiseRun.parentPerformanceRun().Name, m_NoiseRun.Name, ReceptorGrid::Locations.toString(ReceptSet.RefLocation), ReceptSet.RefLongitude, ReceptSet.RefLatitude, ReceptSet.RefAltitudeMsl, ReceptSet.HorizontalSpacing, ReceptSet.VerticalSpacing, static_cast<int>(ReceptSet.HorizontalCount), static_cast<int>(ReceptSet.VerticalCount), ReceptSet.GridRotation));
    }

    void ReceptorSetUpdater::visitPoints(const ReceptorPoints& ReceptSet) {
        m_Db.deleteD(Schema::noise_run_receptor_grid, { 0, 1, 2 }, std::make_tuple(m_NoiseRun.parentScenario().Name, m_NoiseRun.parentPerformanceRun().Name, m_NoiseRun.Name));
        m_Db.deleteD(Schema::noise_run_receptor_points, { 0, 1, 2 }, std::make_tuple(m_NoiseRun.parentScenario().Name, m_NoiseRun.parentPerformanceRun().Name, m_NoiseRun.Name));

        for (const auto& [Name, Rec] : ReceptSet)
            m_Db.insert(Schema::noise_run_receptor_points, {}, std::make_tuple(m_NoiseRun.parentScenario().Name, m_NoiseRun.parentPerformanceRun().Name, m_NoiseRun.Name, Name, Rec.Longitude, Rec.Latitude, Rec.Elevation));
    }

    void ScenariosManager::loadFromFile() {
        // Scenarios
        Statement stmtScen(m_Db, Schema::scenarios.querySelect());
        stmtScen.step();
        while (stmtScen.hasRow())
        {
            const std::string scenName = stmtScen.getColumn(0);
            auto [scen, scenAdded] = m_Scenarios.add(scenName, scenName);
            GRAPE_ASSERT(scenAdded);

            // Scenario Flights
            Statement stmtFlights(m_Db, Schema::scenarios_flights.querySelect({ 1, 2 }, { 0 }));
            stmtFlights.bind(0, scenName);
            stmtFlights.step();
            while (stmtFlights.hasRow())
            {
                const std::string opName = stmtFlights.getColumn(0);
                switch (OperationTypes.fromString(stmtFlights.getColumn(1))) // Operation Type
                {
                case OperationType::Arrival:
                    {
                        auto& arrOp = m_Operations.flightArrivals()(opName);
                        scen.FlightArrivals.emplace_back(arrOp);
                        break;
                    }
                case OperationType::Departure:
                    {
                        auto& depOp = m_Operations.flightDepartures()(opName);
                        scen.FlightDepartures.emplace_back(depOp);
                        break;
                    }
                default: GRAPE_ASSERT(false);
                    break;
                }
                stmtFlights.step();
            }
            m_Blocks.scenarioBlock(scen);

            // Scenario Tracks 4D
            Statement stmtTracks4d(m_Db, Schema::scenarios_tracks_4d.querySelect({ 1, 2 }, { 0 }));
            stmtTracks4d.bindValues(scenName);
            stmtTracks4d.step();
            while (stmtTracks4d.hasRow())
            {
                const std::string opName = stmtTracks4d.getColumn(0);
                switch (OperationTypes.fromString(stmtTracks4d.getColumn(1))) // Operation Type
                {
                case OperationType::Arrival:
                    {
                        auto& arrOp = m_Operations.track4dArrivals()(opName);
                        scen.Track4dArrivals.emplace_back(arrOp);
                        break;
                    }
                case OperationType::Departure:
                    {
                        auto& depOp = m_Operations.track4dDepartures()(opName);
                        scen.Track4dDepartures.emplace_back(depOp);
                        break;
                    }
                default: GRAPE_ASSERT(false);
                    break;
                }
                stmtTracks4d.step();
            }

            // Performance Runs
            Statement stmtPerfRuns(m_Db, Schema::performance_run.querySelect({}, { 0 }));
            stmtPerfRuns.bind(0, scenName);
            stmtPerfRuns.step();
            while (stmtPerfRuns.hasRow())
            {
                const std::string perfRunName = stmtPerfRuns.getColumn(1);
                auto [perfRun, perfRunAdded] = scen.PerformanceRuns.add(perfRunName, scen, perfRunName);
                GRAPE_ASSERT(perfRunAdded);

                switch (CoordinateSystem::Types.fromString(stmtPerfRuns.getColumn(2))) // Coordinate System Type
                {
                case CoordinateSystem::Type::LocalCartesian:
                    {
                        double lon0 = stmtPerfRuns.getColumn(3);
                        double lat0 = stmtPerfRuns.getColumn(4);
                        perfRun.PerfRunSpec.CoordSys = std::make_unique<LocalCartesian>(lon0, lat0);
                        break;
                    }
                case CoordinateSystem::Type::Geodesic: perfRun.PerfRunSpec.CoordSys = std::make_unique<Geodesic>();
                    break;
                default: GRAPE_ASSERT(false);
                    break;
                }

                if (!stmtPerfRuns.isColumnNull(5))
                    perfRun.PerfRunSpec.FilterMinimumAltitude = stmtPerfRuns.getColumn(5);
                if (!stmtPerfRuns.isColumnNull(6))
                    perfRun.PerfRunSpec.FilterMaximumAltitude = stmtPerfRuns.getColumn(6);
                if (!stmtPerfRuns.isColumnNull(7))
                    perfRun.PerfRunSpec.FilterMinimumCumulativeGroundDistance = stmtPerfRuns.getColumn(7);
                if (!stmtPerfRuns.isColumnNull(8))
                    perfRun.PerfRunSpec.FilterMaximumCumulativeGroundDistance = stmtPerfRuns.getColumn(8);
                if (!stmtPerfRuns.isColumnNull(9))
                    perfRun.PerfRunSpec.FilterGroundDistanceThreshold = stmtPerfRuns.getColumn(9);

                if (!stmtPerfRuns.isColumnNull(10))
                    perfRun.PerfRunSpec.SpeedDeltaSegmentationThreshold = stmtPerfRuns.getColumn(10);

                perfRun.PerfRunSpec.FlightsPerformanceMdl = PerformanceModelTypes.fromString(stmtPerfRuns.getColumn(11));
                if (!stmtPerfRuns.isColumnNull(12))
                    perfRun.PerfRunSpec.FlightsDoc29Segmentation = static_cast<bool>(stmtPerfRuns.getColumn(12).getInt());

                if (!stmtPerfRuns.isColumnNull(13))
                    perfRun.PerfRunSpec.Tracks4dCalculatePerformance = static_cast<bool>(stmtPerfRuns.getColumn(13).getInt());
                if (!stmtPerfRuns.isColumnNull(14))
                    perfRun.PerfRunSpec.Tracks4dMinimumPoints = stmtPerfRuns.getColumn(14);
                if (!stmtPerfRuns.isColumnNull(15))
                    perfRun.PerfRunSpec.Tracks4dRecalculateCumulativeGroundDistance = static_cast<bool>(stmtPerfRuns.getColumn(15).getInt());
                if (!stmtPerfRuns.isColumnNull(16))
                    perfRun.PerfRunSpec.Tracks4dRecalculateGroundspeed = static_cast<bool>(stmtPerfRuns.getColumn(16).getInt());
                if (!stmtPerfRuns.isColumnNull(17))
                    perfRun.PerfRunSpec.Tracks4dRecalculateFuelFlow = static_cast<bool>(stmtPerfRuns.getColumn(17).getInt());

                perfRun.PerfRunSpec.FuelFlowMdl = FuelFlowModelTypes.fromString(stmtPerfRuns.getColumn(18));
                if (!stmtPerfRuns.isColumnNull(19))
                    perfRun.PerfRunSpec.FuelFlowLTOAltitudeCorrection = static_cast<bool>(stmtPerfRuns.getColumn(19).getInt());

                // Atmospheres
                Statement stmtAtms(m_Db, Schema::performance_run_atmospheres.querySelect({}, { 0, 1 }));
                stmtAtms.bindValues(scenName, perfRunName);
                stmtAtms.step();
                while (stmtAtms.hasRow())
                {
                    const std::string timeStr = stmtAtms.getColumn(2);
                    const auto timeOpt = utcStringToTime(timeStr);
                    if (!timeOpt)
                    {
                        Log::database()->warn("Loading atmospheres of performance run '{}' of scenario run '{}'. Invalid time '{}'.", perfRunName, scenName, timeStr);
                        stmtAtms.step();
                        continue;
                    }
                    const auto time = timeOpt.value();

                    Atmosphere atm;
                    atm.setTemperatureDelta(stmtAtms.getColumn(3));
                    atm.setPressureDelta(stmtAtms.getColumn(4));

                    const double windSpeed = stmtAtms.getColumn(5);
                    if (stmtAtms.isColumnNull(6))
                    {
                        atm.setConstantHeadwind(windSpeed);
                    }
                    else
                    {
                        atm.setWindSpeed(windSpeed);
                        atm.setWindDirection(stmtAtms.getColumn(6));
                    }
                    atm.setRelativeHumidity(stmtAtms.getColumn(7));
                    perfRun.PerfRunSpec.Atmospheres.addAtmosphere(time, atm);

                    stmtAtms.step();
                }

                // Job
                perfRun.createJob(m_Db, m_Operations);

                // Performance Outputs
                Statement stmtPerfOut(m_Db, Schema::performance_run_output.querySelect({ 2, 3, 4 }, { 0, 1 }));
                stmtPerfOut.bindValues(scenName, perfRunName);
                stmtPerfOut.step();

                if (stmtPerfOut.hasRow())
                {
                    perfRun.job()->queue();
                    perfRun.job()->setFinished();
                }

                bool perfRunReset = false;
                while (stmtPerfOut.hasRow())
                {
                    const std::string opId = stmtPerfOut.getColumn(0);
                    OperationType op = OperationTypes.fromString(stmtPerfOut.getColumn(1));
                    Operation::Type opType = Operation::Types.fromString(stmtPerfOut.getColumn(2));

                    switch (op)
                    {
                    case OperationType::Arrival:
                        {
                            switch (opType)
                            {
                            case Operation::Type::Flight:
                                {
                                    if (!m_Operations.flightArrivals().contains(opId))
                                    {
                                        Log::database()->error("Loading performance output for arrival flight '{}' of performance run '{}' of scenario '{}'. The arrival flight is not part of this scenario. Performance run should be reset.", opId, perfRunName, scenName);
                                        perfRunReset = true;
                                        break;
                                    }
                                    perfRun.output().m_ArrivalOutputs.emplace_back(m_Operations.flightArrivals()(opId));
                                    break;
                                }
                            case Operation::Type::Track4d:
                                {
                                    if (!m_Operations.track4dArrivals().contains(opId))
                                    {
                                        Log::database()->error("Loading performance output for arrival track 4D '{}' of performance run '{}' of scenario '{}'. The arrival track 4D is not part of this scenario. Performance run should be reset.", opId, perfRunName, scenName);
                                        perfRunReset = true;
                                        break;
                                    }
                                    perfRun.output().m_ArrivalOutputs.emplace_back(m_Operations.track4dArrivals()(opId));
                                    break;
                                }
                            default: GRAPE_ASSERT(false);
                                break;
                            }
                            break;
                        }
                    case OperationType::Departure:
                        {
                            switch (opType)
                            {
                            case Operation::Type::Flight:
                                {
                                    if (!m_Operations.flightDepartures().contains(opId))
                                    {
                                        Log::database()->error("Loading performance output for departure flight '{}' of performance run '{}' of scenario '{}'. The departure flight is not part of this scenario. Performance run should be reset.", opId, perfRunName, scenName);
                                        perfRunReset = true;
                                        break;
                                    }
                                    perfRun.output().m_DepartureOutputs.emplace_back(m_Operations.flightDepartures()(opId));
                                    break;
                                }
                            case Operation::Type::Track4d:
                                {
                                    if (!m_Operations.track4dDepartures().contains(opId))
                                    {
                                        Log::database()->error("Loading performance output for departure track 4D '{}' of performance run '{}' of scenario '{}'. The departure track 4D is not part of this scenario. Performance run should be reset.", opId, perfRunName, scenName);
                                        perfRunReset = true;
                                        break;
                                    }
                                    perfRun.output().m_DepartureOutputs.emplace_back(m_Operations.track4dDepartures()(opId));
                                    break;
                                }
                            default: GRAPE_ASSERT(false);
                                break;
                            }
                            break;
                        }
                    default: GRAPE_ASSERT(false);
                    }
                    stmtPerfOut.step();
                }

                // Noise Runs
                Statement stmtNsRuns(m_Db, Schema::noise_run.querySelect({ 2, 3, 4, 5, 6 }, { 0, 1 }));
                stmtNsRuns.bindValues(scenName, perfRunName);
                stmtNsRuns.step();
                while (stmtNsRuns.hasRow())
                {
                    const std::string nsRunName = stmtNsRuns.getColumn(0);
                    auto [nsRun, nsRunAdded] = perfRun.NoiseRuns.add(nsRunName, perfRun, nsRunName);
                    GRAPE_ASSERT(nsRunAdded);

                    nsRun.NsRunSpec.NoiseMdl = NoiseModelTypes.fromString(stmtNsRuns.getColumn(1));
                    nsRun.NsRunSpec.AtmAbsorptionType = AtmosphericAbsorption::Types.fromString(stmtNsRuns.getColumn(2));

                    switch (ReceptorSet::Types.fromString(stmtNsRuns.getColumn(3))) // Receptor Set Type
                    {
                    case ReceptorSet::Type::Grid:
                        {
                            Statement stmtNsRunReceptGrid(m_Db, Schema::noise_run_receptor_grid.querySelect({ 3, 4, 5, 6, 7, 8, 9, 10, 11 }, { 0, 1, 2 }));
                            stmtNsRunReceptGrid.bindValues(scenName, perfRunName, nsRunName);
                            stmtNsRunReceptGrid.step();
                            if (stmtNsRunReceptGrid.hasRow())
                            {
                                ReceptorGrid grd;
                                grd.RefLocation = ReceptorGrid::Locations.fromString(stmtNsRunReceptGrid.getColumn(0));
                                grd.RefLongitude = stmtNsRunReceptGrid.getColumn(1);
                                grd.RefLatitude = stmtNsRunReceptGrid.getColumn(2);
                                grd.RefAltitudeMsl = stmtNsRunReceptGrid.getColumn(3);
                                grd.HorizontalSpacing = stmtNsRunReceptGrid.getColumn(4);
                                grd.VerticalSpacing = stmtNsRunReceptGrid.getColumn(5);
                                grd.HorizontalCount = static_cast<std::size_t>(stmtNsRunReceptGrid.getColumn(6).getInt());
                                grd.VerticalCount = static_cast<std::size_t>(stmtNsRunReceptGrid.getColumn(7).getInt());
                                grd.GridRotation = stmtNsRunReceptGrid.getColumn(8);
                                nsRun.NsRunSpec.ReceptSet = std::make_unique<ReceptorGrid>(grd);
                            }
                            else { nsRun.NsRunSpec.ReceptSet = std::make_unique<ReceptorGrid>(); }
                            break;
                        }
                    case ReceptorSet::Type::Points:
                        {
                            ReceptorPoints pts;
                            Statement stmtNsRunReceptPts(m_Db, Schema::noise_run_receptor_points.querySelect({ 3, 4, 5, 6 }, { 0, 1, 2 }));
                            stmtNsRunReceptPts.bindValues(scenName, perfRunName, nsRunName);
                            stmtNsRunReceptPts.step();
                            while (stmtNsRunReceptPts.hasRow())
                            {
                                const std::string name = stmtNsRunReceptPts.getColumn(0);
                                double lon = stmtNsRunReceptPts.getColumn(1);
                                double lat = stmtNsRunReceptPts.getColumn(2);
                                double altMsl = stmtNsRunReceptPts.getColumn(3);
                                pts.addPoint(name, lon, lat, altMsl);

                                stmtNsRunReceptPts.step();
                            }
                            nsRun.NsRunSpec.ReceptSet = std::make_unique<ReceptorPoints>(std::move(pts));
                            break;
                        }
                    default: GRAPE_ASSERT(false);
                        break;
                    }

                    nsRun.NsRunSpec.SaveSingleMetrics = static_cast<bool>(stmtNsRuns.getColumn(4).getInt());

                    // Job
                    nsRun.createJob(m_Db, m_Blocks);

                    // Receptor Output
                    Statement stmtReceptOut(m_Db, Schema::noise_run_output_receptors.querySelect({ 3, 4, 5, 6 }, { 0, 1, 2 }));
                    stmtReceptOut.bindValues(scenName, perfRunName, nsRunName);
                    stmtReceptOut.step();

                    bool nsRunHasOutput = false;
                    if (!perfRunReset && stmtReceptOut.hasRow()) // If performance run reset, nsRunHasOutput will be always false
                    {
                        nsRunHasOutput = true;
                        nsRun.job()->queue();
                        nsRun.job()->setFinished();

                        auto& receptOutput = nsRun.output().m_ReceptorOutput;
                        while (stmtReceptOut.hasRow())
                        {
                            const std::string id = stmtReceptOut.getColumn(0);
                            double lon = stmtReceptOut.getColumn(1);
                            double lat = stmtReceptOut.getColumn(2);
                            double elevation = stmtReceptOut.getColumn(3);
                            receptOutput.addReceptor(id, lon, lat, elevation);

                            stmtReceptOut.step();
                        }
                    }

                    // Cumulative Metrics
                    Statement stmtCumMetric(m_Db, Schema::noise_run_cumulative_metrics.querySelect({ 3, 4, 5, 6, 7 }, { 0, 1, 2 }));
                    stmtCumMetric.bindValues(scenName, perfRunName, nsRunName);
                    stmtCumMetric.step();
                    while (stmtCumMetric.hasRow())
                    {
                        const std::string cumMetricName = stmtCumMetric.getColumn(0);
                        auto [cumMetric, addedMetric] = nsRun.CumulativeMetrics.add(cumMetricName, nsRun, cumMetricName);
                        GRAPE_ASSERT(addedMetric);
                        cumMetric.Threshold = stmtCumMetric.getColumn(1);
                        cumMetric.AveragingTimeConstant = stmtCumMetric.getColumn(2);
                        const auto startTimePointOpt = utcStringToTime(stmtCumMetric.getColumn(3));
                        if (startTimePointOpt)
                            cumMetric.StartTimePoint = startTimePointOpt.value();
                        else
                            Log::database()->warn("Loading cumulative metric '{}' of noise run '{}' of performance run '{}' of scenario run '{}'. Invalid start time.", cumMetricName, nsRunName, perfRunName, scenName);
                        const auto endTimePointOpt = utcStringToTime(stmtCumMetric.getColumn(4));
                        if (endTimePointOpt)
                            cumMetric.EndTimePoint = endTimePointOpt.value();
                        else
                            Log::database()->warn("Loading cumulative metric '{}' of noise run '{}' of performance run '{}' of scenario run '{}'. Invalid end time.", cumMetricName, nsRunName, perfRunName, scenName);

                        // Cumulative Metrics Weights
                        Statement stmtCumMetricWeights(m_Db, Schema::noise_run_cumulative_metrics_weights.querySelect({ 4, 5 }, { 0, 1, 2, 3 }));
                        stmtCumMetricWeights.bindValues(scenName, perfRunName, nsRunName, cumMetricName);
                        stmtCumMetricWeights.step();
                        while (stmtCumMetricWeights.hasRow())
                        {
                            const std::string timeStr = stmtCumMetricWeights.getColumn(0);
                            const auto timeOpt = stringToDuration(timeStr);
                            if (timeOpt)
                            {
                                const auto time = timeOpt.value();
                                double weight = stmtCumMetricWeights.getColumn(1);
                                if (time == Duration(0))
                                    cumMetric.setBaseWeight(weight);
                                else
                                    cumMetric.addWeight(time, weight);
                            }
                            else
                            {
                                Log::database()->error("Loading weights for cumulative metric '{}' of noise run '{}' of performance run '{}' of scenario run '{}'. Invalid time of day '{}'.", cumMetricName, nsRunName, perfRunName, scenName, timeStr);
                            }
                            stmtCumMetricWeights.step();
                        }

                        // Cumulative Metrics Output
                        if (nsRunHasOutput)
                        {
                            auto [cumOut, addedMetricOutput] = nsRun.output().m_CumulativeOutputs.add(&cumMetric, nsRun.output().receptors().size(), cumMetric.numberAboveThresholds().size());
                            GRAPE_ASSERT(addedMetricOutput);

                            Statement stmtCumMetricOut(m_Db, Schema::noise_run_output_cumulative.querySelect({ 4, 5, 6, 7, 8, 9 }, { 0, 1, 2, 3 }));
                            stmtCumMetricOut.bindValues(scenName, perfRunName, nsRunName, cumMetricName);
                            stmtCumMetricOut.step();
                            std::size_t i = 0;
                            while (stmtCumMetricOut.hasRow())
                            {
                                // Receptor ID at pos 0
                                cumOut.Count.at(i) = stmtCumMetricOut.getColumn(1);
                                cumOut.CountWeighted.at(i) = stmtCumMetricOut.getColumn(2);
                                cumOut.MaximumAbsolute.at(i) = stmtCumMetricOut.getColumn(3);
                                cumOut.MaximumAverage.at(i) = stmtCumMetricOut.getColumn(4);
                                cumOut.Exposure.at(i) = stmtCumMetricOut.getColumn(5);

                                ++i;
                                stmtCumMetricOut.step();
                            }
                        }

                        // Cumulative Metrics Number Above Thresholds
                        Statement stmtCumMetricNat(m_Db, Schema::noise_run_cumulative_metrics_number_above_thresholds.querySelect({ 4 }, { 0, 1, 2, 3 }, { 4 }));
                        stmtCumMetricNat.bindValues(scenName, perfRunName, nsRunName, cumMetricName);
                        stmtCumMetricNat.step();
                        while (stmtCumMetricNat.hasRow())
                        {
                            const double threshold = stmtCumMetricNat.getColumn(0);
                            cumMetric.addNumberAboveThreshold(threshold);

                            // Cumulative Metrics Number Above Output
                            if (nsRunHasOutput)
                            {
                                auto& cumOut = nsRun.output().m_CumulativeOutputs.at(&cumMetric);
                                auto& cumOutNat = cumOut.NumberAboveThresholds.emplace_back();
                                cumOutNat.reserve(nsRun.output().receptors().size());

                                Statement stmtCumMetricNatOut(m_Db, Schema::noise_run_output_cumulative_number_above.querySelect({ 5, 6 }, { 0, 1, 2, 3, 4 }));
                                stmtCumMetricNatOut.bindValues(scenName, perfRunName, nsRunName, cumMetricName, threshold);
                                stmtCumMetricNatOut.step();
                                while (stmtCumMetricNatOut.hasRow())
                                {
                                    // Receptor ID at pos 0
                                    cumOutNat.emplace_back(stmtCumMetricNatOut.getColumn(1));

                                    stmtCumMetricNatOut.step();
                                }
                            }
                            stmtCumMetricNat.step();
                        }
                        stmtCumMetric.step();
                    }
                    stmtNsRuns.step();
                }

                // Emissions Runs
                Statement stmtEmiRuns(m_Db, Schema::emissions_run.querySelect({}, { 0, 1 }));
                stmtEmiRuns.bindValues(scenName, perfRunName);
                stmtEmiRuns.step();
                while (stmtEmiRuns.hasRow())
                {
                    std::size_t col = 2;
                    const std::string emiRunName = stmtEmiRuns.getColumn(col++);
                    auto [emiRun, emiRunAdded] = perfRun.EmissionsRuns.add(emiRunName, perfRun, emiRunName);
                    GRAPE_ASSERT(emiRunAdded);

                    emiRun.EmissionsRunSpec.CalculateGasEmissions = static_cast<bool>(stmtEmiRuns.getColumn(col++).getInt());
                    emiRun.EmissionsRunSpec.CalculateParticleEmissions = static_cast<bool>(stmtEmiRuns.getColumn(col++).getInt());
                    emiRun.EmissionsRunSpec.EmissionsMdl = EmissionsModelTypes.fromString(stmtEmiRuns.getColumn(col++));
                    emiRun.EmissionsRunSpec.BFFM2Model = static_cast<bool>(stmtEmiRuns.getColumn(col++).getInt());
                    emiRun.EmissionsRunSpec.ParticleSmokeNumberModel = EmissionsParticleSmokeNumberModelTypes.fromString(stmtEmiRuns.getColumn(col++));
                    emiRun.EmissionsRunSpec.LTOCycle.at(0) = stmtEmiRuns.getColumn(col++);
                    emiRun.EmissionsRunSpec.LTOCycle.at(1) = stmtEmiRuns.getColumn(col++);
                    emiRun.EmissionsRunSpec.LTOCycle.at(2) = stmtEmiRuns.getColumn(col++);
                    emiRun.EmissionsRunSpec.LTOCycle.at(3) = stmtEmiRuns.getColumn(col++);
                    emiRun.EmissionsRunSpec.ParticleEffectiveDensity = stmtEmiRuns.getColumn(col++);
                    emiRun.EmissionsRunSpec.ParticleGeometricStandardDeviation = stmtEmiRuns.getColumn(col++);
                    emiRun.EmissionsRunSpec.ParticleGeometricMeanDiameter.at(0) = stmtEmiRuns.getColumn(col++);
                    emiRun.EmissionsRunSpec.ParticleGeometricMeanDiameter.at(1) = stmtEmiRuns.getColumn(col++);
                    emiRun.EmissionsRunSpec.ParticleGeometricMeanDiameter.at(2) = stmtEmiRuns.getColumn(col++);
                    emiRun.EmissionsRunSpec.ParticleGeometricMeanDiameter.at(3) = stmtEmiRuns.getColumn(col++);
                    if (!stmtEmiRuns.isColumnNull(col++))
                        emiRun.EmissionsRunSpec.FilterMinimumAltitude = stmtEmiRuns.getColumn(col - 1);
                    if (!stmtEmiRuns.isColumnNull(col++))
                        emiRun.EmissionsRunSpec.FilterMaximumAltitude = stmtEmiRuns.getColumn(col - 1);
                    if (!stmtEmiRuns.isColumnNull(col++))
                        emiRun.EmissionsRunSpec.FilterMinimumCumulativeGroundDistance = stmtEmiRuns.getColumn(col - 1);
                    if (!stmtEmiRuns.isColumnNull(col++))
                        emiRun.EmissionsRunSpec.FilterMaximumCumulativeGroundDistance = stmtEmiRuns.getColumn(col - 1);
                    emiRun.EmissionsRunSpec.SaveSegmentResults = static_cast<bool>(stmtEmiRuns.getColumn(col++).getInt());

                    // Job
                    emiRun.createJob(m_Db, m_Blocks);

                    // Outputs
                    Statement stmtEmiOut(m_Db, Schema::emissions_run_output.querySelect({}, { 0, 1, 2 }));
                    stmtEmiOut.bindValues(scenName, perfRunName, emiRunName);
                    stmtEmiOut.step();
                    if (!perfRunReset && stmtEmiOut.hasRow())
                    {
                        emiRun.job()->queue();
                        emiRun.job()->setFinished();

                        emiRun.output().m_TotalFuel = stmtEmiOut.getColumn(3);
                        emiRun.output().m_TotalEmissions.HC = stmtEmiOut.getColumn(4);
                        emiRun.output().m_TotalEmissions.CO = stmtEmiOut.getColumn(5);
                        emiRun.output().m_TotalEmissions.NOx = stmtEmiOut.getColumn(6);
                        emiRun.output().m_TotalEmissions.nvPM = stmtEmiOut.getColumn(7);
                        emiRun.output().m_TotalEmissions.nvPMNumber = stmtEmiOut.getColumn(8);

                        // Operation Outputs
                        Statement stmtEmiOpOut(m_Db, Schema::emissions_run_output_operations.querySelect({}, { 0, 1, 2 }));
                        stmtEmiOpOut.bindValues(scenName, perfRunName, emiRunName);
                        stmtEmiOpOut.step();
                        while (stmtEmiOpOut.hasRow())
                        {
                            const std::string opId = stmtEmiOpOut.getColumn(3);
                            OperationType op = OperationTypes.fromString(stmtEmiOpOut.getColumn(4));
                            Operation::Type opType = Operation::Types.fromString(stmtEmiOpOut.getColumn(5));

                            const double fuel = stmtEmiOpOut.getColumn(6);
                            const double hc = stmtEmiOpOut.getColumn(7);
                            const double co = stmtEmiOpOut.getColumn(8);
                            const double nox = stmtEmiOpOut.getColumn(9);
                            const double nvpm = stmtEmiOpOut.getColumn(10);
                            const double nvpmNumber = stmtEmiOpOut.getColumn(11);

                            EmissionsOperationOutput opOut;
                            opOut.setTotals(fuel, EmissionValues(hc, co, nox, nvpm, nvpmNumber));

                            const Operation* studyOp = nullptr;
                            switch (op)
                            {
                            case OperationType::Arrival:
                                {
                                    switch (opType)
                                    {
                                    case Operation::Type::Flight: studyOp = &m_Operations.flightArrivals().at(opId);
                                        break;
                                    case Operation::Type::Track4d: studyOp = &m_Operations.track4dArrivals().at(opId);
                                        break;
                                    default: GRAPE_ASSERT(false);
                                    }
                                    break;
                                }
                            case OperationType::Departure:
                                {
                                    switch (opType)
                                    {
                                    case Operation::Type::Flight: studyOp = &m_Operations.flightDepartures().at(opId);
                                        break;
                                    case Operation::Type::Track4d: studyOp = &m_Operations.track4dDepartures().at(opId);
                                        break;
                                    default: GRAPE_ASSERT(false);
                                    }
                                    break;
                                }
                            default: GRAPE_ASSERT(false);
                            }
                            emiRun.output().m_OperationOutputs.add(studyOp, opOut);

                            stmtEmiOpOut.step();
                        }
                    }
                    stmtEmiRuns.step();
                }
                stmtPerfRuns.step();
            }
            stmtScen.step();
        }
    }
}
