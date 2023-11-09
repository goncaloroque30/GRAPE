// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Doc29Noise.h"

#include "Noise/Noise.h"
#include "Performance/PerformanceOutput.h"

namespace GRAPE {
    class Atmosphere;
    class CoordinateSystem;

    class Doc29NoiseGenerator {
    public:
        Doc29NoiseGenerator(const NpdData& Sel, const NpdData& Lamax, const Doc29Spectrum& Spectrum, const Doc29Noise::LateralDirectivity& LateralDir);

        inline static double s_MaximumDistance = Constants::Inf;

        /**
        * @brief Change the NPD maps to the atmospheric absorption defined by AtmAbsorption.
        */
        void applyAtmosphericAbsorption(const AtmosphericAbsorption& AtmAbsorption);

        const NpdData::PowerNoiseLevelsArray& deltas() const { return m_Deltas; }
    protected:
        NpdData m_Sel;
        NpdData m_Lamax;
        Doc29Spectrum m_Spectrum;
        Doc29Noise::LateralDirectivity m_LateralDir;
        NpdData::PowerNoiseLevelsArray m_Deltas{};
    private:
        typedef std::array<OneThirdOctaveArray, NpdStandardDistancesSize> SpectrumArray;

        void calculateAtmosphericAbsorptionDeltas(const AtmosphericAbsorption& AtmAbsorption);
        void resetAtmosphericAbsorption();
    };

    class Doc29NoiseGeneratorArrival : public Doc29NoiseGenerator {
    public:
        Doc29NoiseGeneratorArrival(const Doc29Noise& Doc29Ns);

        std::pair<double, double> calculateArrivalNoise(double Length, double Angle, double Delta, const PerformanceOutput::Point& P1, const PerformanceOutput::Point& P2, const Receptor& Recept, const CoordinateSystem& Cs, const Atmosphere& Atm) const;
    };

    class Doc29NoiseGeneratorDeparture : public Doc29NoiseGenerator {
    public:
        Doc29NoiseGeneratorDeparture(const Doc29Noise& Doc29Ns);

        std::pair<double, double> calculateDepartureNoise(double Length, double Angle, double Delta, const PerformanceOutput::Point& P1, const PerformanceOutput::Point& P2, const Receptor& Recept, const CoordinateSystem& Cs, const Atmosphere& Atm) const;
    private:
        Doc29Noise::SORCorrection m_SOR;
    };
}
