// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

namespace GRAPE {
    struct Receptor {
        Receptor(std::string_view NameIn, double LongitudeIn, double LatitudeIn, double AltitudeMslIn) : Name(NameIn), Longitude(LongitudeIn), Latitude(LatitudeIn), Elevation(AltitudeMslIn) {}
        std::string Name;
        double Longitude, Latitude, Elevation;
    };

    enum class NoiseSingleMetric {
        Lamax = 0,
        Sel,
    };
    constexpr EnumStrings<NoiseSingleMetric> NoiseSingleMetrics{ "LAMAX", "SEL" };

    constexpr std::size_t OneThirdOctaveBandsSize = 24;
    typedef std::array<double, OneThirdOctaveBandsSize> OneThirdOctaveArray;

    inline const OneThirdOctaveArray OneThirdOctaveCenterFrequencies{
            std::pow(10.0, 17.0 / 10.0),
            std::pow(10.0, 18.0 / 10.0),
            std::pow(10.0, 19.0 / 10.0),
            std::pow(10.0, 20.0 / 10.0),
            std::pow(10.0, 21.0 / 10.0),
            std::pow(10.0, 22.0 / 10.0),
            std::pow(10.0, 23.0 / 10.0),
            std::pow(10.0, 24.0 / 10.0),
            std::pow(10.0, 25.0 / 10.0),
            std::pow(10.0, 26.0 / 10.0),
            std::pow(10.0, 27.0 / 10.0),
            std::pow(10.0, 28.0 / 10.0),
            std::pow(10.0, 29.0 / 10.0),
            std::pow(10.0, 30.0 / 10.0),
            std::pow(10.0, 31.0 / 10.0),
            std::pow(10.0, 32.0 / 10.0),
            std::pow(10.0, 33.0 / 10.0),
            std::pow(10.0, 34.0 / 10.0),
            std::pow(10.0, 35.0 / 10.0),
            std::pow(10.0, 36.0 / 10.0),
            std::pow(10.0, 37.0 / 10.0),
            std::pow(10.0, 38.0 / 10.0),
            std::pow(10.0, 39.0 / 10.0),
            std::pow(10.0, 40.0 / 10.0),
    };

    constexpr OneThirdOctaveArray OneThirdOctaveAWeight{
            -30.2,
            -26.2,
            -22.5,
            -19.1,
            -16.1,
            -13.4,
            -10.9,
            -8.6,
            -6.6,
            -4.8,
            -3.2,
            -1.9,
            -0.8,
            +0.0,
            +0.6,
            +1.0,
            +1.2,
            +1.3,
            +1.2,
            +1.0,
            +0.5,
            -0.1,
            -1.1,
            -2.5,
    };
}
