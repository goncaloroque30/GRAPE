// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "Doc29NoiseManager.h"

#include "Schema/Schema.h"

namespace GRAPE {
    Doc29NoiseManager::Doc29NoiseManager(const Database& Db, Constraints& Blocks) : Manager(Db, Blocks) {}

    std::pair<Doc29Noise&, bool> Doc29NoiseManager::addNoise(const std::string& Name) {
        const std::string newName = Name.empty() ? uniqueKeyGenerator(c_Doc29Noises, "New Doc29 Noise") : Name;

        auto [doc29Ns, added] = c_Doc29Noises.add(newName, newName);

        if (added)
        {
            m_Db.insert(Schema::doc29_noise, {}, std::make_tuple(doc29Ns.Name, Doc29Noise::LateralDirectivities.toString(doc29Ns.LateralDir), Doc29Noise::SORCorrections.toString(doc29Ns.SOR)));
            m_Db.insert(Schema::doc29_noise_spectrum, { 0, 1 }, std::make_tuple(doc29Ns.Name, OperationTypes.toString(OperationType::Arrival)));
            m_Db.insert(Schema::doc29_noise_spectrum, { 0, 1 }, std::make_tuple(doc29Ns.Name, OperationTypes.toString(OperationType::Departure)));
        }
        else { Log::dataLogic()->error("Adding Doc29 noise '{}'. Doc29 noise already exists in this study.", Name); }

        return { doc29Ns, added };
    }

    Doc29Noise& Doc29NoiseManager::addNoiseE(const std::string& Name) {
        if (Name.empty())
            throw GrapeException("Empty name not allowed.");

        auto [doc29Ns, added] = c_Doc29Noises.add(Name, Name);

        if (added)
        {
            m_Db.insert(Schema::doc29_noise, {}, std::make_tuple(doc29Ns.Name, Doc29Noise::LateralDirectivities.toString(doc29Ns.LateralDir), Doc29Noise::SORCorrections.toString(doc29Ns.SOR)));
            m_Db.insert(Schema::doc29_noise_spectrum, { 0, 1 }, std::make_tuple(doc29Ns.Name, OperationTypes.toString(OperationType::Arrival)));
            m_Db.insert(Schema::doc29_noise_spectrum, { 0, 1 }, std::make_tuple(doc29Ns.Name, OperationTypes.toString(OperationType::Departure)));
        }
        else
        {
            throw GrapeException(std::format("Doc29 Noise '{}' already exists in this study.", Name));
        }

        return doc29Ns;
    }

    void Doc29NoiseManager::eraseNoises() {
        c_Doc29Noises.eraseIf([&](const auto& Node) {
            const auto& [name, ns] = Node;
            if (m_Blocks.notRemovable(ns))
            {
                Log::dataLogic()->error("Removing Doc29 Noise '{}'. There are {} aircrafts which use this Doc29 Noise.", name, m_Blocks.blocking(ns).size());
                return false;
            }

            m_Db.deleteD(Schema::doc29_noise, { 0 }, std::make_tuple(name));
            return true;
            });
    }

    void Doc29NoiseManager::eraseNoise(const Doc29Noise& Doc29Ns) {
        if (m_Blocks.notRemovable(Doc29Ns))
        {
            Log::dataLogic()->error("Removing Doc29 Noise '{}'. There are {} aircrafts which use this Doc29 Noise.", Doc29Ns.Name, m_Blocks.blocking(Doc29Ns).size());
            return;
        }

        m_Db.deleteD(Schema::doc29_noise, { 0 }, std::make_tuple(Doc29Ns.Name));

        c_Doc29Noises.erase(Doc29Ns.Name);
    }

    bool Doc29NoiseManager::updateKeyNoise(Doc29Noise& Doc29Ns, const std::string Id) {
        if (Doc29Ns.Name.empty())
        {
            Log::dataLogic()->error("Updating Doc29 Noise '{}'. Empty name not allowed.", Id);
            Doc29Ns.Name = Id;
            return false;
        }

        const bool updated = c_Doc29Noises.update(Id, Doc29Ns.Name);

        if (updated) { m_Db.update(Schema::doc29_noise, { 0 }, std::make_tuple(Doc29Ns.Name), { 0 }, std::make_tuple(Id)); }
        else
        {
            Log::dataLogic()->error("Updating Doc29 Noise '{}'. Doc29 Noise new name '{}' already exists in this study.", Id, Doc29Ns.Name);
            Doc29Ns.Name = Id;
        }

        return updated;
    }

    void Doc29NoiseManager::updateNoise(const Doc29Noise& Doc29Ns) const {
        m_Db.update(Schema::doc29_noise, std::make_tuple(Doc29Ns.Name, Doc29Noise::LateralDirectivities.toString(Doc29Ns.LateralDir), Doc29Noise::SORCorrections.toString(Doc29Ns.SOR)), { 0 }, std::make_tuple(Doc29Ns.Name));

        {
            Statement stmt(m_Db, Schema::doc29_noise_spectrum.queryUpdate({}, { 0, 1 }));
            stmt.bindValues(Doc29Ns.Name, OperationTypes.toString(OperationType::Arrival));
            for (std::size_t i = 0; i < Doc29Ns.ArrivalSpectrum.size(); ++i)
                stmt.bind(static_cast<int>(i) + 2, Doc29Ns.ArrivalSpectrum(i));
            stmt.bind(static_cast<int>(Schema::doc29_noise_spectrum.size()), Doc29Ns.Name);
            stmt.bind(static_cast<int>(Schema::doc29_noise_spectrum.size()) + 1, OperationTypes.toString(OperationType::Arrival));
            stmt.step();
        }

        {
            Statement stmt(m_Db, Schema::doc29_noise_spectrum.queryUpdate({}, { 0, 1 }));
            stmt.bindValues(Doc29Ns.Name, OperationTypes.toString(OperationType::Departure));
            for (std::size_t i = 0; i < Doc29Ns.DepartureSpectrum.size(); ++i)
                stmt.bind(static_cast<int>(i) + 2, Doc29Ns.DepartureSpectrum(i));
            stmt.bind(static_cast<int>(Schema::doc29_noise_spectrum.size()), Doc29Ns.Name);
            stmt.bind(static_cast<int>(Schema::doc29_noise_spectrum.size()) + 1, OperationTypes.toString(OperationType::Departure));
            stmt.step();
        }
    }

    void Doc29NoiseManager::updateMetric(const Doc29Noise& Doc29Ns, OperationType OpType, NoiseSingleMetric NsMetric) const {
        switch (OpType)
        {
        case OperationType::Arrival:
            {
                switch (NsMetric)
                {
                case NoiseSingleMetric::Lamax: updateNpdData(Doc29Ns, OpType, NsMetric, Doc29Ns.ArrivalLamax);
                    break;
                case NoiseSingleMetric::Sel: updateNpdData(Doc29Ns, OpType, NsMetric, Doc29Ns.ArrivalSel);
                    break;
                default: GRAPE_ASSERT(false);
                    break;
                }
                break;
            }
        case OperationType::Departure:
            {
                switch (NsMetric)
                {
                case NoiseSingleMetric::Lamax: updateNpdData(Doc29Ns, OpType, NsMetric, Doc29Ns.DepartureLamax);
                    break;
                case NoiseSingleMetric::Sel: updateNpdData(Doc29Ns, OpType, NsMetric, Doc29Ns.DepartureSel);
                    break;
                default: GRAPE_ASSERT(false);
                    break;
                }
                break;
            }
        default: GRAPE_ASSERT(false);
            break;
        }
    }

    void Doc29NoiseManager::updateNpdData(const Doc29Noise& Doc29Ns, OperationType OpType, NoiseSingleMetric NsMetric, const NpdData& Npd) const {
        // Erase
        m_Db.deleteD(Schema::doc29_noise_npd_data, { 0, 1, 2 }, std::make_tuple(Doc29Ns.Name, OperationTypes.toString(OpType), NoiseSingleMetrics.toString(NsMetric)));

        // Insert
        Statement stmt(m_Db, Schema::doc29_noise_npd_data.queryInsert());
        stmt.bindValues(Doc29Ns.Name, OperationTypes.toString(OpType), NoiseSingleMetrics.toString(NsMetric));
        for (const auto& [power, noiseLevelsArray] : Npd)
        {
            stmt.bind(3, power);
            for (std::size_t i = 0; i < noiseLevelsArray.size(); ++i)
                stmt.bind(static_cast<int>(i) + 4, noiseLevelsArray.at(i));
            stmt.step();
            stmt.reset();
        }
    }

    void Doc29NoiseManager::loadFromFile() {
        Statement stmtNs(m_Db, Schema::doc29_noise.querySelect());
        stmtNs.step();
        while (stmtNs.hasRow())
        {
            // Doc29 Noise
            const std::string doc29NsName = stmtNs.getColumn(0);
            auto [doc29Ns, added] = c_Doc29Noises.add(doc29NsName, doc29NsName);
            GRAPE_ASSERT(added);

            doc29Ns.LateralDir = Doc29Noise::LateralDirectivities.fromString(stmtNs.getColumn(1));
            doc29Ns.SOR = Doc29Noise::SORCorrections.fromString(stmtNs.getColumn(2));

            // Doc29 Spectrum
            Statement stmtSpectrum(m_Db, Schema::doc29_noise_spectrum.querySelect({}, { 0, 1 }));
            stmtSpectrum.bindValues(doc29NsName, OperationTypes.toString(OperationType::Arrival));
            stmtSpectrum.step();
            if (stmtSpectrum.hasRow())
            {
                for (std::size_t i = 0; i < OneThirdOctaveBandsSize; ++i)
                    doc29Ns.ArrivalSpectrum(i) = stmtSpectrum.getColumn(static_cast<int>(i + 2));
            }
            else
            {
                Statement stmt(m_Db, Schema::doc29_noise_spectrum.queryInsert({ 0, 1 }));
                stmt.bindValues(doc29Ns.Name, OperationTypes.toString(OperationType::Arrival));
                stmt.step();
            }

            stmtSpectrum.reset();
            stmtSpectrum.bind(1, OperationTypes.toString(OperationType::Departure));
            stmtSpectrum.step();
            if (stmtSpectrum.hasRow())
            {
                for (std::size_t i = 0; i < OneThirdOctaveBandsSize; ++i)
                    doc29Ns.DepartureSpectrum(i) = stmtSpectrum.getColumn(static_cast<int>(i + 2));
            }
            else
            {
                Statement stmt(m_Db, Schema::doc29_noise_spectrum.queryInsert({ 0, 1 }));
                stmt.bindValues(doc29Ns.Name, OperationTypes.toString(OperationType::Departure));
                stmt.step();
            }

            // Doc29 Noise Metrics
            auto npdLoader = [&](NpdData& NpdData, const OperationType OpType, const NoiseSingleMetric NsMetric) {
                Statement stmtNpd(m_Db, Schema::doc29_noise_npd_data.querySelect({ 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 }, { 0, 1, 2 }));
                stmtNpd.bindValues(doc29NsName, OperationTypes.toString(OpType), NoiseSingleMetrics.toString(NsMetric));
                stmtNpd.step();
                while (stmtNpd.hasRow())
                {
                    const auto npdThrust = stmtNpd.getColumn(0);
                    NpdData::PowerNoiseLevelsArray nsLevels{};
                    for (std::size_t i = 0; i < nsLevels.size(); ++i)
                        nsLevels.at(i) = stmtNpd.getColumn(static_cast<int>(i + 1));
                    NpdData.addThrust(npdThrust, nsLevels);
                    stmtNpd.step();
                }
                };
            npdLoader(doc29Ns.ArrivalLamax, OperationType::Arrival, NoiseSingleMetric::Lamax);
            npdLoader(doc29Ns.ArrivalSel, OperationType::Arrival, NoiseSingleMetric::Sel);
            npdLoader(doc29Ns.DepartureLamax, OperationType::Departure, NoiseSingleMetric::Lamax);
            npdLoader(doc29Ns.DepartureSel, OperationType::Departure, NoiseSingleMetric::Sel);

            stmtNs.step();
        }
    }
}
