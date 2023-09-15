// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "AtmosphericAbsorption.h"

#include "Base/Conversions.h"

namespace GRAPE {
    AtmosphericAbsorption::AtmosphericAbsorption(double Temperature, double Pressure, double RelativeHumidity) {
        applySaeArp5534(Temperature, Pressure, RelativeHumidity);
    }

    AtmosphericAbsorption::AtmosphericAbsorption(double Temperature, double RelativeHumidity) {
        applySaeArp866(Temperature, RelativeHumidity);
    }

    void AtmosphericAbsorption::applySaeArp866(double Temperature, double RelativeHumidity) {
        GRAPE_ASSERT(false, "SAE ARP 866 not implemented!");
    }

    void AtmosphericAbsorption::applySaeArp5534(double Temperature, double Pressure, double RelativeHumidity) {
        GRAPE_ASSERT(false, "SAE ARP 5534 not implemented!");
    }

    /**
    TEST_CASE("Atmospheric Absorption") {
        AtmosphericAbsorption saeArp866(fromCelsius(10.0), 0.8);
        AtmosphericAbsorption saeArp5534(fromCelsius(10.0), 101325.0, 0.8);

        SUBCASE("SAE ARP 5534") {
            CHECK_EQ(saeArp5534(0), doctest::Approx(0.007 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp5534(1), doctest::Approx(0.011 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp5534(2), doctest::Approx(0.017 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp5534(3), doctest::Approx(0.026 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp5534(4), doctest::Approx(0.039 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp5534(5), doctest::Approx(0.056 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp5534(6), doctest::Approx(0.078 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp5534(7), doctest::Approx(0.104 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp5534(8), doctest::Approx(0.134 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp5534(9), doctest::Approx(0.166 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp5534(10), doctest::Approx(0.201 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp5534(11), doctest::Approx(0.241 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp5534(12), doctest::Approx(0.292 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp5534(13), doctest::Approx(0.364 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp5534(14), doctest::Approx(0.471 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp5534(15), doctest::Approx(0.636 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp5534(16), doctest::Approx(0.893 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp5534(17), doctest::Approx(1.297 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp5534(18), doctest::Approx(1.931 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp5534(19), doctest::Approx(2.922 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp5534(20), doctest::Approx(4.461 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp5534(21), doctest::Approx(6.826 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp5534(22), doctest::Approx(10.398 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp5534(23), doctest::Approx(15.661 / 100.0).epsilon(0.01));
        }

        SUBCASE("SAE ARP 866A") {
            CHECK_EQ(saeArp866(0), doctest::Approx(0.021 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp866(1), doctest::Approx(0.027 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp866(2), doctest::Approx(0.034 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp866(3), doctest::Approx(0.043 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp866(4), doctest::Approx(0.053 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp866(5), doctest::Approx(0.068 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp866(6), doctest::Approx(0.086 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp866(7), doctest::Approx(0.107 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp866(8), doctest::Approx(0.135 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp866(9), doctest::Approx(0.172 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp866(10), doctest::Approx(0.216 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp866(11), doctest::Approx(0.273 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp866(12), doctest::Approx(0.349 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp866(13), doctest::Approx(0.439 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp866(14), doctest::Approx(0.552 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp866(15), doctest::Approx(0.738 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp866(16), doctest::Approx(0.985 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp866(17), doctest::Approx(1.322 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp866(18), doctest::Approx(1.853 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp866(19), doctest::Approx(2.682 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp866(20), doctest::Approx(3.216 / 100.0).epsilon(0.01));
            CHECK_EQ(saeArp866(21), doctest::Approx(4.580 / 100.0).epsilon(0.01));
            //CHECK_EQ(saeArp866(22), doctest::Approx(6.722/100.0).epsilon(0.01));
            //CHECK_EQ(saeArp866(23), doctest::Approx(9.774/100.0).epsilon(0.01));
            // Reference values used are taken from ECAC CEAC Doc 29 4th Edition. Commented test cases may fail due to the process of quadratic interpolation that is not described as such in the reference.
        }

        SUBCASE("SAE ARP 866") {
            SUBCASE("T = 15, H = 0.7") {
                AtmosphericAbsorption saeArp866(fromCelsius(15.0), 0.7);
                CHECK_EQ(saeArp866(0), doctest::Approx(0 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(1), doctest::Approx(0 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(2), doctest::Approx(0 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(3), doctest::Approx(0 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(4), doctest::Approx(0.1 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(5), doctest::Approx(0.1 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(6), doctest::Approx(0.1 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(7), doctest::Approx(0.1 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(8), doctest::Approx(0.1 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(9), doctest::Approx(0.2 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(10), doctest::Approx(0.2 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(11), doctest::Approx(0.3 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(12), doctest::Approx(0.4 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(13), doctest::Approx(0.5 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(14), doctest::Approx(0.6 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(15), doctest::Approx(0.8 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(16), doctest::Approx(1.0 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(17), doctest::Approx(1.3 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(18), doctest::Approx(1.8 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(19), doctest::Approx(2.5 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(20), doctest::Approx(3.0 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(21), doctest::Approx(4.2 / 100.0).epsilon(0.01));
                //CHECK_EQ(saeArp866(22), doctest::Approx(6.1/100.0).epsilon(0.01));
                //CHECK_EQ(saeArp866(23), doctest::Approx(9.0/100.0).epsilon(0.01));
                // Reference values used are taken from SAE ARP 866. Commented test cases may fail due to the process of quadratic interpolation that is not described as such in the reference.
            }

            SUBCASE("T = 25, H = 0.7") {
                AtmosphericAbsorption saeArp866(fromCelsius(25.0), 0.7);
                CHECK_EQ(saeArp866(0), doctest::Approx(0 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(1), doctest::Approx(0 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(2), doctest::Approx(0 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(3), doctest::Approx(0.1 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(4), doctest::Approx(0.1 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(5), doctest::Approx(0.1 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(6), doctest::Approx(0.1 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(7), doctest::Approx(0.1 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(8), doctest::Approx(0.2 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(9), doctest::Approx(0.2 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(10), doctest::Approx(0.3 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(11), doctest::Approx(0.4 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(12), doctest::Approx(0.5 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(13), doctest::Approx(0.6 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(14), doctest::Approx(0.7 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(15), doctest::Approx(0.9 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(16), doctest::Approx(1.2 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(17), doctest::Approx(1.5 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(18), doctest::Approx(1.9 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(19), doctest::Approx(2.5 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(20), doctest::Approx(2.9 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(21), doctest::Approx(3.6 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(22), doctest::Approx(4.9 / 100.0).epsilon(0.01));
                CHECK_EQ(saeArp866(23), doctest::Approx(6.8 / 100.0).epsilon(0.01));
            }
        }
    }
    */
}
