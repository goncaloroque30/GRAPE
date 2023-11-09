// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Airport/Airport.h"
#include "Aircraft/Doc29/Doc29Aircraft.h"
#include "Aircraft/Doc29/Doc29Noise.h"
#include "Aircraft/FuelEmissions/SFI.h"
#include "Aircraft/FuelEmissions/LTO.h"
#include "Operation/Operation.h"
#include "Scenario/Scenario.h"

namespace GRAPE {
    class Constraints {
    public:
        // Not removable because Aircraft has pointer to
        [[nodiscard]] bool notRemovable(const Doc29Aircraft& Doc29Acft) const { return m_NrDoc29Aircraft.contains(Doc29Acft); }
        [[nodiscard]] bool notRemovable(const Doc29Noise& Doc29Ns) const { return m_NrDoc29Noise.contains(Doc29Ns); }
        [[nodiscard]] bool notRemovable(const SFI& Sfi) const { return m_NrSFI.contains(Sfi); }
        [[nodiscard]] bool notRemovable(const LTOEngine& LtoEng) const { return m_NrLTOEngine.contains(LtoEng); }

        [[nodiscard]] auto& blocking(const Doc29Aircraft& Doc29Acft) const { return m_NrDoc29Aircraft.blocking(Doc29Acft); }
        [[nodiscard]] auto& blocking(const Doc29Noise& Doc29Ns) const { return m_NrDoc29Noise.blocking(Doc29Ns); }
        [[nodiscard]] auto& blocking(const SFI& Sfi) const { return m_NrSFI.blocking(Sfi); }
        [[nodiscard]] auto& blocking(const LTOEngine& LtoEng) const { return m_NrLTOEngine.blocking(LtoEng); }

        // Not removable because Flight has pointer to
        [[nodiscard]] bool notRemovable(const Airport& Apt) const { return m_NrAirports.contains(Apt); }
        [[nodiscard]] bool notRemovable(const Runway& Rwy) const { return m_NrRunways.contains(Rwy); }
        [[nodiscard]] bool notRemovable(const Route& Rte) const { return m_NrRoutes.contains(Rte); }
        [[nodiscard]] bool notRemovable(const Aircraft& Acft) const { return m_NrAircrafts.contains(Acft); }
        [[nodiscard]] bool notRemovable(const Doc29Profile& Doc29Prof) const { return m_NrDoc29Profiles.contains(Doc29Prof); }

        [[nodiscard]] auto& blocking(const Airport& Apt) const { return m_NrAirports.blocking(Apt); }
        [[nodiscard]] auto& blocking(const Runway& Rwy) const { return m_NrRunways.blocking(Rwy); }
        [[nodiscard]] auto& blocking(const Route& Rte) const { return m_NrRoutes.blocking(Rte); }
        [[nodiscard]] auto& blocking(const Aircraft& Acft) const { return m_NrAircrafts.blocking(Acft); }
        [[nodiscard]] auto& blocking(const Doc29Profile& Doc29Prof) const { return m_NrDoc29Profiles.blocking(Doc29Prof); }

        // Not removable because scenario has pointer to
        [[nodiscard]] bool notRemovable(const Operation& Op) const { return m_NrOperations.contains(Op); }

        [[nodiscard]] auto& blocking(const Operation& Op) const { return m_NrOperations.blocking(Op); }

        // Not editable because performance run blocks
        [[nodiscard]] bool notEditable(const Airport& Apt) const { return m_NeAirports.contains(Apt); }
        [[nodiscard]] bool notEditable(const Runway& Rwy) const { return m_NeRunways.contains(Rwy); }
        [[nodiscard]] bool notEditable(const Route& Rte) const { return m_NeRoutes.contains(Rte); }
        [[nodiscard]] bool notEditable(const Doc29Aircraft& Doc29Acft) const { return m_NeDoc29Aircrafts.contains(Doc29Acft); }
        [[nodiscard]] bool notEditable(const SFI& Sfi) const { return m_NeSFI.contains(Sfi); }
        [[nodiscard]] bool notEditable(const LTOEngine& LtoEng) const { return m_NeLTOEngine.contains(LtoEng); }
        [[nodiscard]] bool notEditable(const Aircraft& Acft) const { return m_NeAircrafts.contains(Acft); }
        [[nodiscard]] bool notEditable(const Operation& Op) const { return m_NeOperations.contains(Op); }
        [[nodiscard]] bool notEditable(const Scenario& Scen) const { return m_NeScenarios.contains(Scen); }

        // Not editable because noise run blocks
        [[nodiscard]] bool notEditable(const Doc29Noise& Doc29Ns) const { return m_NeDoc29Noises.contains(Doc29Ns); }

        // Removes
        void aircraftBlockDoc29Acft(const Aircraft& Acft);
        void aircraftUnblockDoc29Acft(const Aircraft& Acft);
        void aircraftBlockDoc29Noise(const Aircraft& Acft);
        void aircraftUnblockDoc29Noise(const Aircraft& Acft);
        void aircraftBlockSfi(const Aircraft& Acft);
        void aircraftUnblockSfi(const Aircraft& Acft);
        void aircraftBlockLTOEngine(const Aircraft& Acft);
        void aircraftUnblockLTOEngine(const Aircraft& Acft);
        void aircraftBlock(const Aircraft& Acft);
        void aircraftUnblock(const Aircraft& Acft);

        void operationBlockAircraft(Operation& Op);
        void operationUnblockAircraft(Operation& Op);
        void operationBlockRoute(const Flight& Op);
        void operationUnblockRoute(const Flight& Op);
        void operationBlockDoc29Profile(const Flight& Op);
        void operationUnblockDoc29Profile(const Flight& Op);
        void operationBlock(Flight& Op);
        void operationUnblock(Flight& Op);
        void operationBlock(Track4d& Op);
        void operationUnblock(Track4d& Op);

        void scenarioBlockOperation(const Scenario& Scen, const Operation& Op);
        void scenarioUnblockOperation(const Scenario& Scen, const Operation& Op);
        void scenarioBlockFlights(const Scenario& Scen);
        void scenarioUnblockFlights(const Scenario& Scen);
        void scenarioBlockTracks4d(const Scenario& Scen);
        void scenarioUnblockTracks4d(const Scenario& Scen);
        void scenarioBlockArrivals(const Scenario& Scen);
        void scenarioUnblockArrivals(const Scenario& Scen);
        void scenarioBlockDepartures(const Scenario& Scen);
        void scenarioUnblockDepartures(const Scenario& Scen);
        void scenarioBlock(const Scenario& Scen);
        void scenarioUnblock(const Scenario& Scen);

        // Edits
        void performanceRunBlock(const PerformanceRun& PerfRun);
        void performanceRunUnblock(const PerformanceRun& PerfRun);

        void noiseRunBlock(const NoiseRun& NsRun);
        void noiseRunUnblock(const NoiseRun& NsRun);

    private:
        // Not Removable
        BlockMap<const Doc29Aircraft*, const Aircraft*> m_NrDoc29Aircraft;
        BlockMap<const Doc29Noise*, const Aircraft*> m_NrDoc29Noise;
        BlockMap<const SFI*, const Aircraft*> m_NrSFI;
        BlockMap<const LTOEngine*, const Aircraft*> m_NrLTOEngine;

        BlockMap<const Airport*, const Flight*> m_NrAirports;
        BlockMap<const Runway*, const Flight*> m_NrRunways;
        BlockMap<const Route*, const Flight*> m_NrRoutes;
        BlockMap<const Aircraft*, Operation*> m_NrAircrafts; // Editing Aircraft can change blocked operation
        BlockMap<const Doc29Profile*, const Flight*> m_NrDoc29Profiles;

        BlockMap<const Operation*, const Scenario*> m_NrOperations;

        // Not Editable
        BlockMap<const Airport*, const PerformanceRun*> m_NeAirports;
        BlockMap<const Runway*, const PerformanceRun*> m_NeRunways;
        BlockMap<const Route*, const PerformanceRun*> m_NeRoutes;
        BlockMap<const SFI*, const PerformanceRun*> m_NeSFI;
        BlockMap<const LTOEngine*, const PerformanceRun*> m_NeLTOEngine;
        BlockMap<const Doc29Aircraft*, const PerformanceRun*> m_NeDoc29Aircrafts;
        BlockMap<const Aircraft*, const PerformanceRun*> m_NeAircrafts;
        BlockMap<const Operation*, const PerformanceRun*> m_NeOperations;
        BlockMap<const Scenario*, const PerformanceRun*> m_NeScenarios;

        BlockMap<const Doc29Noise*, const NoiseRun*> m_NeDoc29Noises;

    private:
        void performanceRunBlock(const Flight& Op, const PerformanceRun& PerfRun);
        void performanceRunUnblock(const Flight& Op, const PerformanceRun& PerfRun);
        void performanceRunBlock(const Track4d& Op, const PerformanceRun& PerfRun);
        void performanceRunUnblock(const Track4d& Op, const PerformanceRun& PerfRun);

        void noiseRunBlock(const Operation& Op, const NoiseRun& NsRun);
        void noiseRunUnblock(const Operation& Op, const NoiseRun& NsRun);
    };
}
