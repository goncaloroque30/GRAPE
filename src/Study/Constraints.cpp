// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "Constraints.h"

#include "Aircraft/Aircraft.h"

namespace GRAPE {
    void Constraints::aircraftBlockDoc29Acft(const Aircraft& Acft) {
        if (Acft.Doc29Acft)
            m_NrDoc29Aircraft.block(*Acft.Doc29Acft, Acft);
    }

    void Constraints::aircraftUnblockDoc29Acft(const Aircraft& Acft) {
        if (Acft.Doc29Acft)
            m_NrDoc29Aircraft.unblock(*Acft.Doc29Acft, Acft);
    }

    void Constraints::aircraftBlockDoc29Noise(const Aircraft& Acft) {
        if (Acft.Doc29Ns)
            m_NrDoc29Noise.block(*Acft.Doc29Ns, Acft);
    }

    void Constraints::aircraftUnblockDoc29Noise(const Aircraft& Acft) {
        if (Acft.Doc29Ns)
            m_NrDoc29Noise.unblock(*Acft.Doc29Ns, Acft);
    }

    void Constraints::aircraftBlockSfi(const Aircraft& Acft) {
        if (Acft.SFIFuel)
            m_NrSFI.block(*Acft.SFIFuel, Acft);
    }

    void Constraints::aircraftUnblockSfi(const Aircraft& Acft) {
        if (Acft.SFIFuel)
            m_NrSFI.unblock(*Acft.SFIFuel, Acft);
    }

    void Constraints::aircraftBlockLTOEngine(const Aircraft& Acft) {
        if (Acft.LTOEng)
            m_NrLTOEngine.block(*Acft.LTOEng, Acft);
    }

    void Constraints::aircraftUnblockLTOEngine(const Aircraft& Acft) {
        if (Acft.LTOEng)
            m_NrLTOEngine.unblock(*Acft.LTOEng, Acft);
    }

    void Constraints::aircraftBlock(const Aircraft& Acft) {
        aircraftBlockDoc29Acft(Acft);
        aircraftBlockDoc29Noise(Acft);
        aircraftBlockSfi(Acft);
        aircraftBlockLTOEngine(Acft);
    }

    void Constraints::aircraftUnblock(const Aircraft& Acft) {
        aircraftUnblockDoc29Acft(Acft);
        aircraftUnblockDoc29Noise(Acft);
        aircraftUnblockSfi(Acft);
        aircraftUnblockLTOEngine(Acft);
    }

    void Constraints::operationBlockAircraft(Operation& Op) { m_NrAircrafts.block(Op.aircraft(), Op); }

    void Constraints::operationUnblockAircraft(Operation& Op) { m_NrAircrafts.unblock(Op.aircraft(), Op); }

    void Constraints::operationBlockRoute(const Flight& Op) {
        if (!Op.hasRoute())
            return;

        m_NrAirports.block(Op.route().parentAirport(), Op);
        m_NrRunways.block(Op.route().parentRunway(), Op);
        m_NrRoutes.block(Op.route(), Op);
    }

    void Constraints::operationUnblockRoute(const Flight& Op) {
        if (!Op.hasRoute())
            return;

        m_NrAirports.unblock(Op.route().parentAirport(), Op);
        m_NrRunways.unblock(Op.route().parentRunway(), Op);
        m_NrRoutes.unblock(Op.route(), Op);
    }

    void Constraints::operationBlockDoc29Profile(const Flight& Op) {
        if (Op.hasDoc29Profile())
            m_NrDoc29Profiles.block(*Op.doc29Profile(), Op);
    }

    void Constraints::operationUnblockDoc29Profile(const Flight& Op) {
        if (Op.hasDoc29Profile())
            m_NrDoc29Profiles.unblock(*Op.doc29Profile(), Op);
    }

    void Constraints::operationBlock(Flight& Op) {
        operationBlockAircraft(Op);
        operationBlockRoute(Op);
        operationBlockDoc29Profile(Op);
    }

    void Constraints::operationUnblock(Flight& Op) {
        operationUnblockAircraft(Op);
        operationUnblockRoute(Op);
        operationUnblockDoc29Profile(Op);
    }

    void Constraints::operationBlock(Track4d& Op) { operationBlockAircraft(Op); }

    void Constraints::operationUnblock(Track4d& Op) { operationUnblockAircraft(Op); }

    void Constraints::scenarioBlockOperation(const Scenario& Scen, const Operation& Op) { m_NrOperations.block(Op, Scen); }

    void Constraints::scenarioUnblockOperation(const Scenario& Scen, const Operation& Op) { m_NrOperations.unblock(Op, Scen); }

    void Constraints::scenarioBlockFlights(const Scenario& Scen) {
        for (auto op : Scen.FlightArrivals)
            scenarioBlockOperation(Scen, op);

        for (auto op : Scen.FlightDepartures)
            scenarioBlockOperation(Scen, op);
    }

    void Constraints::scenarioUnblockFlights(const Scenario& Scen) {
        for (auto op : Scen.FlightArrivals)
            scenarioUnblockOperation(Scen, op);

        for (auto op : Scen.FlightDepartures)
            scenarioUnblockOperation(Scen, op);
    }

    void Constraints::scenarioBlockTracks4d(const Scenario& Scen) {
        for (auto op : Scen.Track4dArrivals)
            scenarioBlockOperation(Scen, op);

        for (auto op : Scen.Track4dDepartures)
            scenarioBlockOperation(Scen, op);
    }

    void Constraints::scenarioUnblockTracks4d(const Scenario& Scen) {
        for (auto op : Scen.Track4dArrivals)
            scenarioUnblockOperation(Scen, op);

        for (auto op : Scen.Track4dDepartures)
            scenarioUnblockOperation(Scen, op);
    }

    void Constraints::scenarioBlockArrivals(const Scenario& Scen) {
        for (auto op : Scen.FlightArrivals)
            scenarioBlockOperation(Scen, op);

        for (auto op : Scen.Track4dArrivals)
            scenarioBlockOperation(Scen, op);
    }

    void Constraints::scenarioUnblockArrivals(const Scenario& Scen) {
        for (auto op : Scen.FlightArrivals)
            scenarioUnblockOperation(Scen, op);

        for (auto op : Scen.Track4dArrivals)
            scenarioUnblockOperation(Scen, op);
    }

    void Constraints::scenarioBlockDepartures(const Scenario& Scen) {
        for (auto op : Scen.FlightDepartures)
            scenarioBlockOperation(Scen, op);

        for (auto op : Scen.Track4dDepartures)
            scenarioBlockOperation(Scen, op);
    }

    void Constraints::scenarioUnblockDepartures(const Scenario& Scen) {
        for (auto op : Scen.FlightDepartures)
            scenarioUnblockOperation(Scen, op);

        for (auto op : Scen.Track4dDepartures)
            scenarioUnblockOperation(Scen, op);
    }

    void Constraints::scenarioBlock(const Scenario& Scen) {
        scenarioBlockFlights(Scen);
        scenarioBlockTracks4d(Scen);
    }

    void Constraints::scenarioUnblock(const Scenario& Scen) {
        scenarioUnblockFlights(Scen);
        scenarioUnblockTracks4d(Scen);
    }

    void Constraints::performanceRunBlock(const PerformanceRun& PerfRun) {
        const auto& scen = PerfRun.parentScenario();

        m_NeScenarios.block(scen, PerfRun);

        for (const auto& flight : scen.FlightArrivals)
            performanceRunBlock(flight, PerfRun);

        for (const auto& flight : scen.FlightDepartures)
            performanceRunBlock(flight, PerfRun);

        for (const auto& track4d : scen.Track4dArrivals)
            performanceRunBlock(track4d, PerfRun);

        for (const auto& track4d : scen.Track4dDepartures)
            performanceRunBlock(track4d, PerfRun);
    }

    void Constraints::performanceRunUnblock(const PerformanceRun& PerfRun) {
        const auto& scen = PerfRun.parentScenario();

        m_NeScenarios.unblock(scen, PerfRun);

        for (const auto& flight : scen.FlightArrivals)
            performanceRunUnblock(flight, PerfRun);

        for (const auto& flight : scen.FlightDepartures)
            performanceRunUnblock(flight, PerfRun);

        for (const auto& track4d : scen.Track4dArrivals)
            performanceRunUnblock(track4d, PerfRun);

        for (const auto& track4d : scen.Track4dDepartures)
            performanceRunUnblock(track4d, PerfRun);
    }

    void Constraints::noiseRunBlock(const NoiseRun& NsRun) {
        const auto& scen = NsRun.parentScenario();

        for (const auto& op : scen.FlightArrivals)
            noiseRunBlock(op, NsRun);

        for (const auto& op : scen.FlightDepartures)
            noiseRunBlock(op, NsRun);

        for (const auto& op : scen.Track4dArrivals)
            noiseRunBlock(op, NsRun);

        for (const auto& op : scen.Track4dDepartures)
            noiseRunBlock(op, NsRun);
    }

    void Constraints::noiseRunUnblock(const NoiseRun& NsRun) {
        const auto& scen = NsRun.parentScenario();

        for (const auto& op : scen.FlightArrivals)
            noiseRunUnblock(op, NsRun);

        for (const auto& op : scen.FlightDepartures)
            noiseRunUnblock(op, NsRun);

        for (const auto& op : scen.Track4dArrivals)
            noiseRunUnblock(op, NsRun);

        for (const auto& op : scen.Track4dDepartures)
            noiseRunUnblock(op, NsRun);
    }

    void Constraints::performanceRunBlock(const Flight& Op, const PerformanceRun& PerfRun) {
        if (Op.hasRoute())
        {
            const auto& opRte = Op.route();
            m_NeAirports.block(opRte.parentAirport(), PerfRun);
            m_NeRunways.block(opRte.parentRunway(), PerfRun);
            m_NeRoutes.block(opRte, PerfRun);
        }

        m_NeAircrafts.block(Op.aircraft(), PerfRun);
        if (Op.aircraft().Doc29Acft)
            m_NeDoc29Aircrafts.block(*Op.aircraft().Doc29Acft, PerfRun);
        if (Op.aircraft().SFIFuel)
            m_NeSFI.block(*Op.aircraft().SFIFuel, PerfRun);
        if (Op.aircraft().LTOEng)
            m_NeLTOEngine.block(*Op.aircraft().LTOEng, PerfRun);
        m_NeOperations.block(Op, PerfRun);
    }

    void Constraints::performanceRunBlock(const Track4d& Op, const PerformanceRun& PerfRun) {
        m_NeAircrafts.block(Op.aircraft(), PerfRun);
        if (Op.aircraft().SFIFuel)
            m_NeSFI.block(*Op.aircraft().SFIFuel, PerfRun);
        if (Op.aircraft().LTOEng)
            m_NeLTOEngine.block(*Op.aircraft().LTOEng, PerfRun);
        m_NeOperations.block(Op, PerfRun);
    }

    void Constraints::performanceRunUnblock(const Flight& Op, const PerformanceRun& PerfRun) {
        if (Op.hasRoute())
        {
            const auto& opRte = Op.route();
            m_NeAirports.unblock(opRte.parentAirport(), PerfRun);
            m_NeRunways.unblock(opRte.parentRunway(), PerfRun);
            m_NeRoutes.unblock(opRte, PerfRun);
        }

        m_NeAircrafts.unblock(Op.aircraft(), PerfRun);
        if (Op.aircraft().Doc29Acft)
            m_NeDoc29Aircrafts.unblock(*Op.aircraft().Doc29Acft, PerfRun);
        if (Op.aircraft().SFIFuel)
            m_NeSFI.unblock(*Op.aircraft().SFIFuel, PerfRun);
        if (Op.aircraft().LTOEng)
            m_NeLTOEngine.unblock(*Op.aircraft().LTOEng, PerfRun);
        m_NeOperations.unblock(Op, PerfRun);
    }

    void Constraints::performanceRunUnblock(const Track4d& Op, const PerformanceRun& PerfRun) {
        m_NeAircrafts.unblock(Op.aircraft(), PerfRun);
        if (Op.aircraft().SFIFuel)
            m_NeSFI.unblock(*Op.aircraft().SFIFuel, PerfRun);
        if (Op.aircraft().LTOEng)
            m_NeLTOEngine.unblock(*Op.aircraft().LTOEng, PerfRun);
        m_NeOperations.unblock(Op, PerfRun);
    }

    void Constraints::noiseRunBlock(const Operation& Op, const NoiseRun& NsRun) {
        if (Op.aircraft().Doc29Ns)
            m_NeDoc29Noises.block(*Op.aircraft().Doc29Ns, NsRun);
    }

    void Constraints::noiseRunUnblock(const Operation& Op, const NoiseRun& NsRun) {
        if (Op.aircraft().Doc29Ns)
            m_NeDoc29Noises.unblock(*Op.aircraft().Doc29Ns, NsRun);
    }
}
