// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

namespace GRAPE {
    namespace Constants {
        constexpr double Re = 6356766.0; ///< Earth Radius
        constexpr double RAir = 287.05287; ///< Specific gas constant for dry air
        constexpr double g0 = 9.80665; ///< Gravity acceleration at mean sea level
        constexpr double t0 = 288.15; ///< Temperature in ISA atmosphere at mean sea level
        constexpr double p0 = 101325; ///< Pressure in ISA atmosphere at mean sea level
        constexpr double d0 = p0 / (RAir * t0); ///< Density in ISA atmosphere at mean sea level
        constexpr double tG = -0.0065; ///< Temperature gradient below the tropopause
        constexpr double GeoAltIsaTrop = 11000.0; ///< Geopotential altitude in ISA atmosphere of the tropopause

        constexpr double tTropopause = t0 + GeoAltIsaTrop * tG; ///< Standard temperature at Tropopause
    }

    /**
    * @brief Geopotential altitude is a mathematical construct used in the standard atmosphere.
    * Changes in gravity acceleration with latitude are not considered.
    * @return Geopotential altitude.
    */
    constexpr double geopotentialAltitude(double GeometricAltitude) {
        return Constants::Re * GeometricAltitude / (Constants::Re + GeometricAltitude);
    }

    /**
    * @brief Geopotential altitude is a mathematical construct used in the standard atmosphere.
    * Changes in gravity acceleration with latitude are not considered.
    * @return Geometric altitude.
    */
    constexpr double geometricAltitude(double GeopotentialAltitude) {
        return Constants::Re * GeopotentialAltitude / (Constants::Re - GeopotentialAltitude);
    }

    /**
     * @brief Calculates the temperature at a given geopotential altitude for defined temperature and pressure deltas.
     * @return Temperature.
     */
    constexpr double temperature(double GeopotentialAltitude, double TemperatureDelta) {
        return GeopotentialAltitude <= Constants::GeoAltIsaTrop ? Constants::t0 + TemperatureDelta + Constants::tG * GeopotentialAltitude : Constants::tTropopause + TemperatureDelta;
    }

    /**
     * @brief Calculates the pressure at a given geopotential altitude for defined temperature and pressure deltas.
     * @return Pressure.
     */
    constexpr double pressure(double GeopotentialAltitude, double TemperatureDelta, double PressureDelta) {
        return GeopotentialAltitude <= Constants::GeoAltIsaTrop ? (Constants::p0 + PressureDelta) * std::pow(1.0 + Constants::tG * GeopotentialAltitude / (Constants::t0 + TemperatureDelta), -Constants::g0 / (Constants::tG * Constants::RAir)) : pressure(Constants::GeoAltIsaTrop, TemperatureDelta, PressureDelta) * std::exp(-Constants::g0 * (GeopotentialAltitude - Constants::GeoAltIsaTrop) / ((Constants::tTropopause + TemperatureDelta) * Constants::RAir));
    }

    /**
     * @brief Calculates the density at a given geopotential Altitude for defined temperature and pressure deltas.
     * @return Density.
     */
    constexpr double density(double GeopotentialAltitude, double TemperatureDelta, double PressureDelta) {
        return pressure(GeopotentialAltitude, TemperatureDelta, PressureDelta) / (Constants::RAir * temperature(GeopotentialAltitude, TemperatureDelta));
    }

    /**
    * @brief Calculates the temperature delta at sea level based on the conditions observed at an altitude.
    * @return Temperature delta.
    */
    constexpr double temperatureDelta(double GeopotentialAltitude, double Temperature) {
        return Temperature - temperature(GeopotentialAltitude, 0.0);
    }

    TEST_CASE("Atmosphere Functions: Standard State") {
        constexpr double tD = 0.0;
        constexpr double tP = 0.0;

        //Temperature
        CHECK_EQ(temperature(1000.0, tD), doctest::Approx(281.65).epsilon(Constants::PrecisionTest));
        CHECK_EQ(temperature(2000.0, tD), doctest::Approx(275.15).epsilon(Constants::PrecisionTest));
        CHECK_EQ(temperature(4000.0, tD), doctest::Approx(262.15).epsilon(Constants::PrecisionTest));
        CHECK_EQ(temperature(6000.0, tD), doctest::Approx(249.15).epsilon(Constants::PrecisionTest));
        CHECK_EQ(temperature(8000.0, tD), doctest::Approx(236.15).epsilon(Constants::PrecisionTest));
        CHECK_EQ(temperature(10000.0, tD), doctest::Approx(223.15).epsilon(Constants::PrecisionTest));
        CHECK_EQ(temperature(14000.0, tD), doctest::Approx(216.65).epsilon(Constants::PrecisionTest));
        CHECK_EQ(temperature(15000.0, tD), doctest::Approx(216.65).epsilon(Constants::PrecisionTest));
        CHECK_EQ(temperature(16000.0, tD), doctest::Approx(216.65).epsilon(Constants::PrecisionTest));

        // Pressure
        CHECK_EQ(pressure(1000.0, tD, tP), doctest::Approx(89874.571552).epsilon(Constants::PrecisionTest));
        CHECK_EQ(pressure(2000.0, tD, tP), doctest::Approx(79495.217389).epsilon(Constants::PrecisionTest));
        CHECK_EQ(pressure(4000.0, tD, tP), doctest::Approx(61640.238287).epsilon(Constants::PrecisionTest));
        CHECK_EQ(pressure(6000.0, tD, tP), doctest::Approx(47181.031080).epsilon(Constants::PrecisionTest));
        CHECK_EQ(pressure(8000.0, tD, tP), doctest::Approx(35599.815049).epsilon(Constants::PrecisionTest));
        CHECK_EQ(pressure(10000.0, tD, tP), doctest::Approx(26436.271053).epsilon(Constants::PrecisionTest));
        CHECK_EQ(pressure(11000.0, tD, tP), doctest::Approx(22632.067277).epsilon(Constants::PrecisionTest));
        CHECK_EQ(pressure(14000.0, tD, tP), doctest::Approx(14101.802314).epsilon(Constants::PrecisionTest));
        CHECK_EQ(pressure(15000.0, tD, tP), doctest::Approx(12044.573360).epsilon(Constants::PrecisionTest));
        CHECK_EQ(pressure(16000.0, tD, tP), doctest::Approx(10287.461433).epsilon(Constants::PrecisionTest));

        // Density
        CHECK_EQ(density(1000.0, tD, tP), doctest::Approx(1.111642).epsilon(Constants::PrecisionTest));
        CHECK_EQ(density(2000.0, tD, tP), doctest::Approx(1.006490).epsilon(Constants::PrecisionTest));
        CHECK_EQ(density(4000.0, tD, tP), doctest::Approx(0.819129).epsilon(Constants::PrecisionTest));
        CHECK_EQ(density(6000.0, tD, tP), doctest::Approx(0.659697).epsilon(Constants::PrecisionTest));
        CHECK_EQ(density(8000.0, tD, tP), doctest::Approx(0.525168).epsilon(Constants::PrecisionTest));
        CHECK_EQ(density(10000.0, tD, tP), doctest::Approx(0.412707).epsilon(Constants::PrecisionTest));
        CHECK_EQ(density(14000.0, tD, tP), doctest::Approx(0.226754).epsilon(Constants::PrecisionTest));
        CHECK_EQ(density(15000.0, tD, tP), doctest::Approx(0.193674).epsilon(Constants::PrecisionTest));
        CHECK_EQ(density(16000.0, tD, tP), doctest::Approx(0.165420).epsilon(Constants::PrecisionTest));
    }

    // Tests for user-defined, non-standard atmosphere
    TEST_CASE("Non Standard Atmosphere") {
        SUBCASE("Temperature Delta = 10 K | Pressure Delta = 10 hPa") {
            constexpr double tD = 10.0;
            constexpr double tP = 1000.0;

            CHECK_EQ(temperature(0.0, tD), doctest::Approx(298.15).epsilon(Constants::PrecisionTest));
            CHECK_EQ(temperature(2000.0, tD), doctest::Approx(285.15).epsilon(Constants::PrecisionTest));
            CHECK_EQ(temperature(4000.0, tD), doctest::Approx(272.15).epsilon(Constants::PrecisionTest));
            CHECK_EQ(temperature(6000.0, tD), doctest::Approx(259.15).epsilon(Constants::PrecisionTest));
            CHECK_EQ(temperature(8000.0, tD), doctest::Approx(246.15).epsilon(Constants::PrecisionTest));
            CHECK_EQ(temperature(10000.0, tD), doctest::Approx(233.15).epsilon(Constants::PrecisionTest));
            CHECK_EQ(temperature(14000.0, tD), doctest::Approx(226.65).epsilon(Constants::PrecisionTest));
            CHECK_EQ(temperature(15000.0, tD), doctest::Approx(226.65).epsilon(Constants::PrecisionTest));
            CHECK_EQ(temperature(16000.0, tD), doctest::Approx(226.65).epsilon(Constants::PrecisionTest));

            CHECK_EQ(pressure(0.0, tD, tP), doctest::Approx(102325.0000).epsilon(Constants::PrecisionTest));
            CHECK_EQ(pressure(2000.0, tD, tP), doctest::Approx(80950.654313).epsilon(Constants::PrecisionTest));
            CHECK_EQ(pressure(4000.0, tD, tP), doctest::Approx(63344.624081).epsilon(Constants::PrecisionTest));
            CHECK_EQ(pressure(6000.0, tD, tP), doctest::Approx(48976.175274).epsilon(Constants::PrecisionTest));
            CHECK_EQ(pressure(8000.0, tD, tP), doctest::Approx(37368.764218).epsilon(Constants::PrecisionTest));
            CHECK_EQ(pressure(10000.0, tD, tP), doctest::Approx(28096.807433).epsilon(Constants::PrecisionTest));
            CHECK_EQ(pressure(14000.0, tD, tP), doctest::Approx(15407.466268).epsilon(Constants::PrecisionTest));
            CHECK_EQ(pressure(15000.0, tD, tP), doctest::Approx(13251.636423).epsilon(Constants::PrecisionTest));
            CHECK_EQ(pressure(16000.0, tD, tP), doctest::Approx(11397.472736).epsilon(Constants::PrecisionTest));

            CHECK_EQ(density(0.0, tD, tP), doctest::Approx(1.195598).epsilon(Constants::PrecisionTest));
            CHECK_EQ(density(2000.0, tD, tP), doctest::Approx(0.988975).epsilon(Constants::PrecisionTest));
            CHECK_EQ(density(4000.0, tD, tP), doctest::Approx(0.810848).epsilon(Constants::PrecisionTest));
            CHECK_EQ(density(6000.0, tD, tP), doctest::Approx(0.658373).epsilon(Constants::PrecisionTest));
            CHECK_EQ(density(8000.0, tD, tP), doctest::Approx(0.528868).epsilon(Constants::PrecisionTest));
            CHECK_EQ(density(10000.0, tD, tP), doctest::Approx(0.419817).epsilon(Constants::PrecisionTest));
            CHECK_EQ(density(14000.0, tD, tP), doctest::Approx(0.236817).epsilon(Constants::PrecisionTest));
            CHECK_EQ(density(15000.0, tD, tP), doctest::Approx(0.203682).epsilon(Constants::PrecisionTest));
            CHECK_EQ(density(16000.0, tD, tP), doctest::Approx(0.175182).epsilon(Constants::PrecisionTest));
        }

        SUBCASE("Temperature Delta = 15 K | Pressure Delta = 15 hPa") {
            constexpr double tD = 15.0;
            constexpr double tP = 1500.0;

            CHECK_EQ(temperature(0.0, tD), doctest::Approx(303.15).epsilon(Constants::PrecisionTest));
            CHECK_EQ(temperature(2000.0, tD), doctest::Approx(290.15).epsilon(Constants::PrecisionTest));
            CHECK_EQ(temperature(4000.0, tD), doctest::Approx(277.15).epsilon(Constants::PrecisionTest));
            CHECK_EQ(temperature(6000.0, tD), doctest::Approx(264.15).epsilon(Constants::PrecisionTest));
            CHECK_EQ(temperature(8000.0, tD), doctest::Approx(251.15).epsilon(Constants::PrecisionTest));
            CHECK_EQ(temperature(10000.0, tD), doctest::Approx(238.15).epsilon(Constants::PrecisionTest));
            CHECK_EQ(temperature(14000.0, tD), doctest::Approx(231.65).epsilon(Constants::PrecisionTest));
            CHECK_EQ(temperature(15000.0, tD), doctest::Approx(231.65).epsilon(Constants::PrecisionTest));
            CHECK_EQ(temperature(16000.0, tD), doctest::Approx(231.65).epsilon(Constants::PrecisionTest));

            CHECK_EQ(pressure(0.0, tD, tP), doctest::Approx(102825.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(pressure(2000.0, tD, tP), doctest::Approx(81668.214014).epsilon(Constants::PrecisionTest));
            CHECK_EQ(pressure(4000.0, tD, tP), doctest::Approx(64183.090016).epsilon(Constants::PrecisionTest));
            CHECK_EQ(pressure(6000.0, tD, tP), doctest::Approx(49860.948679).epsilon(Constants::PrecisionTest));
            CHECK_EQ(pressure(8000.0, tD, tP), doctest::Approx(38244.161736).epsilon(Constants::PrecisionTest));
            CHECK_EQ(pressure(10000.0, tD, tP), doctest::Approx(28923.163488).epsilon(Constants::PrecisionTest));
            CHECK_EQ(pressure(14000.0, tD, tP), doctest::Approx(16066.927815).epsilon(Constants::PrecisionTest));
            CHECK_EQ(pressure(15000.0, tD, tP), doctest::Approx(13863.857098).epsilon(Constants::PrecisionTest));
            CHECK_EQ(pressure(16000.0, tD, tP), doctest::Approx(11962.867815).epsilon(Constants::PrecisionTest));

            CHECK_EQ(density(0.0, tD, tP), doctest::Approx(1.181624).epsilon(Constants::PrecisionTest));
            CHECK_EQ(density(2000.0, tD, tP), doctest::Approx(0.980547).epsilon(Constants::PrecisionTest));
            CHECK_EQ(density(4000.0, tD, tP), doctest::Approx(0.806759).epsilon(Constants::PrecisionTest));
            CHECK_EQ(density(6000.0, tD, tP), doctest::Approx(0.657579).epsilon(Constants::PrecisionTest));
            CHECK_EQ(density(8000.0, tD, tP), doctest::Approx(0.530481).epsilon(Constants::PrecisionTest));
            CHECK_EQ(density(10000.0, tD, tP), doctest::Approx(0.423091).epsilon(Constants::PrecisionTest));
            CHECK_EQ(density(14000.0, tD, tP), doctest::Approx(0.241623).epsilon(Constants::PrecisionTest));
            CHECK_EQ(density(15000.0, tD, tP), doctest::Approx(0.208492).epsilon(Constants::PrecisionTest));
            CHECK_EQ(density(16000.0, tD, tP), doctest::Approx(0.179904).epsilon(Constants::PrecisionTest));
        }

        SUBCASE("Temperature Delta = -1 K | Pressure Delta = 1 hPa") {
            constexpr double tD = -1;
            constexpr double tP = 100.0;

            CHECK_EQ(temperature(0.0, tD), doctest::Approx(287.15).epsilon(Constants::PrecisionTest));
            CHECK_EQ(temperature(2000.0, tD), doctest::Approx(274.15).epsilon(Constants::PrecisionTest));
            CHECK_EQ(temperature(4000.0, tD), doctest::Approx(261.15).epsilon(Constants::PrecisionTest));
            CHECK_EQ(temperature(6000.0, tD), doctest::Approx(248.15).epsilon(Constants::PrecisionTest));
            CHECK_EQ(temperature(8000.0, tD), doctest::Approx(235.15).epsilon(Constants::PrecisionTest));
            CHECK_EQ(temperature(10000.0, tD), doctest::Approx(222.15).epsilon(Constants::PrecisionTest));
            CHECK_EQ(temperature(14000.0, tD), doctest::Approx(215.65).epsilon(Constants::PrecisionTest));
            CHECK_EQ(temperature(15000.0, tD), doctest::Approx(215.65).epsilon(Constants::PrecisionTest));
            CHECK_EQ(temperature(16000.0, tD), doctest::Approx(215.65).epsilon(Constants::PrecisionTest));

            CHECK_EQ(pressure(0.0, tD, tP), doctest::Approx(101425.0).epsilon(Constants::PrecisionTest));
            CHECK_EQ(pressure(2000.0, tD, tP), doctest::Approx(79504.867219).epsilon(Constants::PrecisionTest));
            CHECK_EQ(pressure(4000.0, tD, tP), doctest::Approx(61589.121268).epsilon(Constants::PrecisionTest));
            CHECK_EQ(pressure(6000.0, tD, tP), doctest::Approx(47092.411215).epsilon(Constants::PrecisionTest));
            CHECK_EQ(pressure(8000.0, tD, tP), doctest::Approx(35491.529387).epsilon(Constants::PrecisionTest));
            CHECK_EQ(pressure(10000.0, tD, tP), doctest::Approx(26321.552226).epsilon(Constants::PrecisionTest));
            CHECK_EQ(pressure(14000.0, tD, tP), doctest::Approx(13999.892746).epsilon(Constants::PrecisionTest));
            CHECK_EQ(pressure(15000.0, tD, tP), doctest::Approx(11948.788828).epsilon(Constants::PrecisionTest));
            CHECK_EQ(pressure(16000.0, tD, tP), doctest::Approx(10198.189162).epsilon(Constants::PrecisionTest));

            CHECK_EQ(pressure(19000.0, tD, tP), doctest::Approx(6340.457493).epsilon(Constants::PrecisionTest));

            CHECK_EQ(density(0.0, tD, tP), doctest::Approx(1.230479).epsilon(Constants::PrecisionTest));
            CHECK_EQ(density(2000.0, tD, tP), doctest::Approx(1.010284).epsilon(Constants::PrecisionTest));
            CHECK_EQ(density(4000.0, tD, tP), doctest::Approx(0.821584).epsilon(Constants::PrecisionTest));
            CHECK_EQ(density(6000.0, tD, tP), doctest::Approx(0.661112).epsilon(Constants::PrecisionTest));
            CHECK_EQ(density(8000.0, tD, tP), doctest::Approx(0.525797).epsilon(Constants::PrecisionTest));
            CHECK_EQ(density(10000.0, tD, tP), doctest::Approx(0.412765).epsilon(Constants::PrecisionTest));
            CHECK_EQ(density(14000.0, tD, tP), doctest::Approx(0.226159).epsilon(Constants::PrecisionTest));
            CHECK_EQ(density(15000.0, tD, tP), doctest::Approx(0.193025).epsilon(Constants::PrecisionTest));
            CHECK_EQ(density(16000.0, tD, tP), doctest::Approx(0.164745).epsilon(Constants::PrecisionTest));
        }
    }
}
