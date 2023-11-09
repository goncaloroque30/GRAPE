// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "Atmosphere.h"

#include "AtmosphericConstants.h"

namespace GRAPE {
    Atmosphere::Atmosphere() : m_TemperatureDelta(0.0), m_PressureDelta(0.0), m_RelativeHumidity(0.7), m_WindSpeed(0.0), m_WindDirection(0.0) {}

    Atmosphere::Atmosphere(double TemperatureDelta, double PressureDelta) noexcept : Atmosphere() {
        setDeltas(TemperatureDelta, PressureDelta);
    }

    double Atmosphere::seaLevelTemperature() const noexcept {
        return Constants::t0 + m_TemperatureDelta;
    }

    double Atmosphere::seaLevelPressure() const noexcept {
        return Constants::p0 + m_PressureDelta;
    }

    double Atmosphere::temperatureDelta() const noexcept {
        return m_TemperatureDelta;
    }

    double Atmosphere::pressureDelta() const noexcept {
        return m_PressureDelta;
    }

    double Atmosphere::headwind(double Heading) const noexcept {
        if (std::isnan(m_WindDirection))
            return m_WindSpeed;

        return m_WindSpeed * std::cos(toRadians(m_WindDirection - Heading));
    }

    double Atmosphere::crosswind(double Heading) const noexcept {
        if (std::isnan(m_WindDirection))
            return 0.0;

        return m_WindSpeed * std::sin(toRadians(m_WindDirection - Heading));
    }

    void Atmosphere::setStandard() noexcept {
        m_TemperatureDelta = 0.0;
        m_PressureDelta = 0.0;
    }

    void Atmosphere::setDeltas(double TemperatureDelta, double PressureDelta) noexcept {
        GRAPE_ASSERT(TemperatureDelta > -100.0 && TemperatureDelta < 100.0 && PressureDelta > -15000.0 && PressureDelta < 15000.0);
        m_TemperatureDelta = TemperatureDelta;
        m_PressureDelta = PressureDelta;
    }

    void Atmosphere::setTemperatureDelta(double TemperatureDelta) noexcept {
        GRAPE_ASSERT(TemperatureDelta > -100.0 && TemperatureDelta < 100.0);
        m_TemperatureDelta = TemperatureDelta;
    }

    void Atmosphere::setPressureDelta(double PressureDelta) noexcept {
        GRAPE_ASSERT(PressureDelta > -15000.0 && PressureDelta < 15000.0);
        m_PressureDelta = PressureDelta;
    }

    void Atmosphere::setWindSpeed(double WindSpeed) noexcept {
        GRAPE_ASSERT(!std::isnan(WindSpeed));
        m_WindSpeed = WindSpeed;
    }

    void Atmosphere::setWindDirection(double WindDirection) noexcept {
        GRAPE_ASSERT(WindDirection >= 0.0 && WindDirection <= 360.0);

        m_WindDirection = WindDirection;
    }

    void Atmosphere::setConstantHeadwind(double Headwind) noexcept {
        GRAPE_ASSERT(!std::isnan(Headwind));

        m_WindDirection = Constants::NaN;
        m_WindSpeed = Headwind;
    }

    void Atmosphere::setRelativeHumidity(double RelativeHumidity) noexcept {
        GRAPE_ASSERT(RelativeHumidity >= 0.0 & RelativeHumidity <= 1.0);
        m_RelativeHumidity = RelativeHumidity;
    }

    void Atmosphere::setDeltasE(double TemperatureDelta, double PressureDelta) {
        if (!(TemperatureDelta > -100.0 && TemperatureDelta < 100.0))
            throw GrapeException("Temperature delta must be between -100 and +100 K.");

        if (!(PressureDelta > -15000.0 && PressureDelta < 15000.0))
            throw GrapeException("Pressure delta must be between -15000 and 15000 Pa.");

        setDeltas(TemperatureDelta, PressureDelta);
    }

    void Atmosphere::setTemperatureDeltaE(double TemperatureDelta) {
        if (!(TemperatureDelta > -100.0 && TemperatureDelta < 100.0))
            throw GrapeException("Temperature delta must be between -100 and +100 K.");

        setTemperatureDelta(TemperatureDelta);
    }

    void Atmosphere::setPressureDeltaE(double PressureDelta) {
        if (!(PressureDelta > -15000.0 && PressureDelta < 15000.0))
            throw GrapeException("Pressure delta must be between -15000 and 15000 Pa.");
        setPressureDelta(PressureDelta);
    }

    void Atmosphere::setWindDirectionE(double WindDirection) {
        if (!(WindDirection >= 0.0 && WindDirection <= 360.0))
            throw GrapeException("Wind direction must be between 0 and 360.");
        setWindDirection(WindDirection);
    }

    void Atmosphere::setRelativeHumidityE(double RelativeHumidity) {
        if (!(RelativeHumidity >= 0.0 && RelativeHumidity <= 1.0))
            throw GrapeException("Relative humidity must be between 0 and 100%.");

        setRelativeHumidity(RelativeHumidity);
    }

    bool Atmosphere::isNonStandard() const noexcept {
        return m_TemperatureDelta != 0.0 || m_PressureDelta != 0.0;
    }

    bool Atmosphere::isHeadwind() const noexcept {
        return std::isnan(m_WindDirection);
    }

    double Atmosphere::temperature(double GeometricAltitude) const {
        return GRAPE::temperature(geopotentialAltitude(GeometricAltitude), m_TemperatureDelta);
    }

    //Two barometric formulas depending on temperature gradient being 0 or not
    double Atmosphere::pressure(double GeometricAltitude) const {
        return GRAPE::pressure(geopotentialAltitude(GeometricAltitude), m_TemperatureDelta, m_PressureDelta);
    }

    double Atmosphere::density(double GeometricAltitude) const {
        return pressure(GeometricAltitude) / (Constants::RAir * temperature(GeometricAltitude));
    }

    // Ratios
    double Atmosphere::temperatureRatio(double GeometricAltitude) const {
        return GRAPE::temperature(geopotentialAltitude(GeometricAltitude), m_TemperatureDelta) / Constants::t0;
    }

    double Atmosphere::pressureRatio(double GeometricAltitude) const {
        return GRAPE::pressure(geopotentialAltitude(GeometricAltitude), m_TemperatureDelta, m_PressureDelta) / Constants::p0;
    }

    double Atmosphere::densityRatio(double GeometricAltitude) const {
        return pressureRatio(GeometricAltitude) / temperatureRatio(GeometricAltitude);
    }

    TEST_CASE("Atmosphere Class") {
        Atmosphere atm;

        SUBCASE("Standard State") {
            //Temperature
            CHECK_EQ(atm.temperature(geometricAltitude(1000.0)), doctest::Approx(281.65).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperature(geometricAltitude(2000.0)), doctest::Approx(275.15).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperature(geometricAltitude(4000.0)), doctest::Approx(262.15).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperature(geometricAltitude(6000.0)), doctest::Approx(249.15).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperature(geometricAltitude(8000.0)), doctest::Approx(236.15).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperature(geometricAltitude(10000.0)), doctest::Approx(223.15).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperature(geometricAltitude(14000.0)), doctest::Approx(216.65).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperature(geometricAltitude(15000.0)), doctest::Approx(216.65).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperature(geometricAltitude(16000.0)), doctest::Approx(216.65).epsilon(Constants::PrecisionTest));

            //Pressure
            CHECK_EQ(atm.pressure(geometricAltitude(1000.0)), doctest::Approx(89874.571552).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressure(geometricAltitude(2000.0)), doctest::Approx(79495.217389).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressure(geometricAltitude(4000.0)), doctest::Approx(61640.238287).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressure(geometricAltitude(6000.0)), doctest::Approx(47181.031080).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressure(geometricAltitude(8000.0)), doctest::Approx(35599.815049).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressure(geometricAltitude(10000.0)), doctest::Approx(26436.271053).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressure(geometricAltitude(14000.0)), doctest::Approx(14101.802314).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressure(geometricAltitude(15000.0)), doctest::Approx(12044.573360).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressure(geometricAltitude(16000.0)), doctest::Approx(10287.461433).epsilon(Constants::PrecisionTest));

            //Density
            CHECK_EQ(atm.density(geometricAltitude(1000.0)), doctest::Approx(1.111642).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.density(geometricAltitude(2000.0)), doctest::Approx(1.006490).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.density(geometricAltitude(4000.0)), doctest::Approx(0.819129).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.density(geometricAltitude(6000.0)), doctest::Approx(0.659697).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.density(geometricAltitude(8000.0)), doctest::Approx(0.525168).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.density(geometricAltitude(10000.0)), doctest::Approx(0.412707).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.density(geometricAltitude(14000.0)), doctest::Approx(0.226754).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.density(geometricAltitude(15000.0)), doctest::Approx(0.193674).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.density(geometricAltitude(16000.0)), doctest::Approx(0.165420).epsilon(Constants::PrecisionTest));

            //Ratios
            CHECK_EQ(atm.temperatureRatio(geometricAltitude(0.0)), doctest::Approx(1.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperatureRatio(geometricAltitude(2000.0)), doctest::Approx(0.954885).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperatureRatio(geometricAltitude(4000.0)), doctest::Approx(0.909769).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperatureRatio(geometricAltitude(6000.0)), doctest::Approx(0.864654).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperatureRatio(geometricAltitude(8000.0)), doctest::Approx(0.819538).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperatureRatio(geometricAltitude(10000.0)), doctest::Approx(0.774423).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperatureRatio(geometricAltitude(14000.0)), doctest::Approx(0.751865).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperatureRatio(geometricAltitude(15000.0)), doctest::Approx(0.751865).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperatureRatio(geometricAltitude(16000.0)), doctest::Approx(0.751865).epsilon(Constants::PrecisionTest));

            CHECK_EQ(atm.pressureRatio(geometricAltitude(0.0)), doctest::Approx(1.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressureRatio(geometricAltitude(2000.0)), doctest::Approx(0.784557).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressureRatio(geometricAltitude(4000.0)), doctest::Approx(0.608342).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressureRatio(geometricAltitude(6000.0)), doctest::Approx(0.465640).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressureRatio(geometricAltitude(8000.0)), doctest::Approx(0.351343).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressureRatio(geometricAltitude(10000.0)), doctest::Approx(0.260905).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressureRatio(geometricAltitude(14000.0)), doctest::Approx(0.139174).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressureRatio(geometricAltitude(15000.0)), doctest::Approx(0.118870).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressureRatio(geometricAltitude(16000.0)), doctest::Approx(0.101529).epsilon(Constants::PrecisionTest));

            CHECK_EQ(atm.densityRatio(geometricAltitude(0.0)), doctest::Approx(1.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.densityRatio(geometricAltitude(2000.0)), doctest::Approx(0.821625).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.densityRatio(geometricAltitude(4000.0)), doctest::Approx(0.668677).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.densityRatio(geometricAltitude(6000.0)), doctest::Approx(0.538528).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.densityRatio(geometricAltitude(8000.0)), doctest::Approx(0.428708).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.densityRatio(geometricAltitude(10000.0)), doctest::Approx(0.336903).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.densityRatio(geometricAltitude(14000.0)), doctest::Approx(0.185105).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.densityRatio(geometricAltitude(15000.0)), doctest::Approx(0.158101).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.densityRatio(geometricAltitude(16000.0)), doctest::Approx(0.135036).epsilon(Constants::PrecisionTest));
        }

        SUBCASE("Temperature Delta = 10 K | Pressure Delta = 10 hPa") {
            atm.setDeltas(10.0, 1000.0);

            CHECK_EQ(atm.temperature(geometricAltitude(0.0)), doctest::Approx(298.15).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperature(geometricAltitude(2000.0)), doctest::Approx(285.15).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperature(geometricAltitude(4000.0)), doctest::Approx(272.15).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperature(geometricAltitude(6000.0)), doctest::Approx(259.15).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperature(geometricAltitude(8000.0)), doctest::Approx(246.15).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperature(geometricAltitude(10000.0)), doctest::Approx(233.15).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperature(geometricAltitude(14000.0)), doctest::Approx(226.65).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperature(geometricAltitude(15000.0)), doctest::Approx(226.65).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperature(geometricAltitude(16000.0)), doctest::Approx(226.65).epsilon(Constants::PrecisionTest));

            CHECK_EQ(atm.pressure(geometricAltitude(0.0)), doctest::Approx(102325.0000).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressure(geometricAltitude(2000.0)), doctest::Approx(80950.654313).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressure(geometricAltitude(4000.0)), doctest::Approx(63344.624081).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressure(geometricAltitude(6000.0)), doctest::Approx(48976.175274).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressure(geometricAltitude(8000.0)), doctest::Approx(37368.764218).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressure(geometricAltitude(10000.0)), doctest::Approx(28096.807433).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressure(geometricAltitude(14000.0)), doctest::Approx(15407.466268).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressure(geometricAltitude(15000.0)), doctest::Approx(13251.636423).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressure(geometricAltitude(16000.0)), doctest::Approx(11397.472736).epsilon(Constants::PrecisionTest));

            CHECK_EQ(atm.density(geometricAltitude(0.0)), doctest::Approx(1.195598).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.density(geometricAltitude(2000.0)), doctest::Approx(0.988975).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.density(geometricAltitude(4000.0)), doctest::Approx(0.810848).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.density(geometricAltitude(6000.0)), doctest::Approx(0.658373).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.density(geometricAltitude(8000.0)), doctest::Approx(0.528868).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.density(geometricAltitude(10000.0)), doctest::Approx(0.419817).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.density(geometricAltitude(14000.0)), doctest::Approx(0.236817).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.density(geometricAltitude(15000.0)), doctest::Approx(0.203682).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.density(geometricAltitude(16000.0)), doctest::Approx(0.175182).epsilon(Constants::PrecisionTest));
        }

        SUBCASE("Temperature Delta = -1 K | Pressure Delta = 1 hPa") {
            atm.setDeltas(-1.0, 100.0);

            CHECK_EQ(atm.temperature(geometricAltitude(0.0)), doctest::Approx(287.15).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperature(geometricAltitude(2000.0)), doctest::Approx(274.15).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperature(geometricAltitude(4000.0)), doctest::Approx(261.15).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperature(geometricAltitude(6000.0)), doctest::Approx(248.15).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperature(geometricAltitude(8000.0)), doctest::Approx(235.15).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperature(geometricAltitude(10000.0)), doctest::Approx(222.15).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperature(geometricAltitude(14000.0)), doctest::Approx(215.65).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperature(geometricAltitude(15000.0)), doctest::Approx(215.65).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperature(geometricAltitude(16000.0)), doctest::Approx(215.65).epsilon(Constants::PrecisionTest));

            CHECK_EQ(atm.pressure(geometricAltitude(0.0)), doctest::Approx(101425.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressure(geometricAltitude(2000.0)), doctest::Approx(79504.867219).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressure(geometricAltitude(4000.0)), doctest::Approx(61589.121268).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressure(geometricAltitude(6000.0)), doctest::Approx(47092.411215).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressure(geometricAltitude(8000.0)), doctest::Approx(35491.529387).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressure(geometricAltitude(10000.0)), doctest::Approx(26321.552226).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressure(geometricAltitude(14000.0)), doctest::Approx(13999.892746).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressure(geometricAltitude(15000.0)), doctest::Approx(11948.788828).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressure(geometricAltitude(16000.0)), doctest::Approx(10198.189162).epsilon(Constants::PrecisionTest));

            CHECK_EQ(atm.density(geometricAltitude(0.0)), doctest::Approx(1.230479).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.density(geometricAltitude(2000.0)), doctest::Approx(1.010284).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.density(geometricAltitude(4000.0)), doctest::Approx(0.821584).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.density(geometricAltitude(6000.0)), doctest::Approx(0.661112).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.density(geometricAltitude(8000.0)), doctest::Approx(0.525797).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.density(geometricAltitude(10000.0)), doctest::Approx(0.412765).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.density(geometricAltitude(14000.0)), doctest::Approx(0.226159).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.density(geometricAltitude(15000.0)), doctest::Approx(0.193025).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.density(geometricAltitude(16000.0)), doctest::Approx(0.164745).epsilon(Constants::PrecisionTest));

            /* TODO: Update atmosphere and atmosphere tests
            CHECK_EQ(atm.temperatureRatio(geometricAltitude(0.0)), doctest::Approx(1.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperatureRatio(geometricAltitude(2000.0)), doctest::Approx(0.954727).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperatureRatio(geometricAltitude(4000.0)), doctest::Approx(0.909455).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperatureRatio(geometricAltitude(6000.0)), doctest::Approx(0.864182).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperatureRatio(geometricAltitude(8000.0)), doctest::Approx(0.818910).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperatureRatio(geometricAltitude(10000.0)), doctest::Approx(0.773637).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperatureRatio(geometricAltitude(14000.0)), doctest::Approx(0.751001).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperatureRatio(geometricAltitude(15000.0)), doctest::Approx(0.751001).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.temperatureRatio(geometricAltitude(16000.0)), doctest::Approx(0.751001).epsilon(Constants::PrecisionTest));

            CHECK_EQ(atm.pressureRatio(geometricAltitude(0.0)), doctest::Approx(1.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressureRatio(geometricAltitude(2000.0)), doctest::Approx(0.783878).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressureRatio(geometricAltitude(4000.0)), doctest::Approx(0.607238).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressureRatio(geometricAltitude(6000.0)), doctest::Approx(0.464308).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressureRatio(geometricAltitude(8000.0)), doctest::Approx(0.349929).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressureRatio(geometricAltitude(10000.0)), doctest::Approx(0.259517).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressureRatio(geometricAltitude(14000.0)), doctest::Approx(0.138032).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressureRatio(geometricAltitude(15000.0)), doctest::Approx(0.117809).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.pressureRatio(geometricAltitude(16000.0)), doctest::Approx(0.100549).epsilon(Constants::PrecisionTest));

            CHECK_EQ(atm.densityRatio(geometricAltitude(0.0)), doctest::Approx(1.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.densityRatio(geometricAltitude(2000.0)), doctest::Approx(0.821049).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.densityRatio(geometricAltitude(4000.0)), doctest::Approx(0.667694).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.densityRatio(geometricAltitude(6000.0)), doctest::Approx(0.537280).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.densityRatio(geometricAltitude(8000.0)), doctest::Approx(0.427310).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.densityRatio(geometricAltitude(10000.0)), doctest::Approx(0.335451).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.densityRatio(geometricAltitude(14000.0)), doctest::Approx(0.183797).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.densityRatio(geometricAltitude(15000.0)), doctest::Approx(0.156869).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.densityRatio(geometricAltitude(16000.0)), doctest::Approx(0.133887).epsilon(Constants::PrecisionTest));
            */
        }

        SUBCASE("Relative Humidity") {
            atm.setRelativeHumidity(0.8);
            CHECK_EQ(atm.relativeHumidity(), doctest::Approx(0.8).epsilon(Constants::PrecisionTest));
        }

        SUBCASE("Wind") {
            atm.setWindDirection(360.0);
            atm.setWindSpeed(10.0);
            CHECK_EQ(atm.headwind(0.0), doctest::Approx(10.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.headwind(180), doctest::Approx(-10.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.crosswind(0.0), doctest::Approx(0.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.crosswind(180), doctest::Approx(0.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.headwind(90.0), doctest::Approx(0.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.headwind(270.0), doctest::Approx(0.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.crosswind(90.0), doctest::Approx(-10.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(atm.crosswind(270.0), doctest::Approx(10.0).epsilon(Constants::PrecisionTest));
        }

        SUBCASE("State Changes") {
            CHECK_FALSE_MESSAGE(atm.isNonStandard(), "Object should be in standard state!");

            atm.setPressureDelta(100.0);
            atm.setTemperatureDelta(15.0);
            CHECK_MESSAGE(atm.isNonStandard(), "Object should be in non standard state!");

            // resetting Atmosphere to ISA layers
            atm.setStandard();
            CHECK_FALSE_MESSAGE(atm.isNonStandard(), "Object should be in standard state!");
        }
    }
}
