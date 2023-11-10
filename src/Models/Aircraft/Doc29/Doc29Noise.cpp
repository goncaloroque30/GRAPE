// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "Doc29Noise.h"

namespace GRAPE {
    Doc29Spectrum::Doc29Spectrum(const Doc29Noise& Doc29Ns) : m_Doc29Noise(Doc29Ns) {}

    const Doc29Noise& Doc29Spectrum::parentDoc29Noise() const noexcept { return m_Doc29Noise; }

    void Doc29Spectrum::setValue(std::size_t Index, double Value) {
        GRAPE_ASSERT(Index < size());

        if (!(Value >= 0.0))
            throw GrapeException(std::format("Invalid spectrum level for {:.0f} Hz.", OneThirdOctaveCenterFrequencies.at(Index)));

        m_OctaveNoiseLevels.at(Index) = Value;
    }

    bool NpdData::addThrust(double Thrust) noexcept {
        // Empty
        if (m_NpdData.empty())
        {
            auto [it, added] = m_NpdData.try_emplace(Thrust, PowerNoiseLevelsArray{});
            return added;
        }

        // Higher Thrust value exists
        if (auto it = m_NpdData.upper_bound(Thrust); it != m_NpdData.end())
        {
            const auto& [thr, levels] = *it;
            auto [newIt, added] = m_NpdData.emplace(Thrust, levels);
            return added;
        }

        // Add at the end
        const auto& [thr, levels] = *m_NpdData.rbegin();
        auto [newIt, added] = m_NpdData.emplace(Thrust, levels);
        return added;
    }

    bool NpdData::addThrust(double Thrust, const PowerNoiseLevelsArray& ThrustNoiseLevels) noexcept {
        auto [newIt, added] = m_NpdData.emplace(Thrust, ThrustNoiseLevels);
        return added;
    }

    bool NpdData::updateThrust(double Current, double New) noexcept {
        GRAPE_ASSERT(m_NpdData.contains(Current));

        if (m_NpdData.contains(New))
            return false;

        auto node = m_NpdData.extract(Current);
        node.key() = New;
        m_NpdData.insert(std::move(node));
        return true;
    }

    void NpdData::clear() noexcept {
        m_NpdData.clear();
        m_InterpolationMatrix.clear();
    }

    void NpdData::addThrustE(double Thrust, const PowerNoiseLevelsArray& ThrustNoiseLevels) {
        if (!(Thrust > 0.0))
            throw GrapeException("Thrust must be higher than 0 N.");

        for (const auto& level : ThrustNoiseLevels)
        {
            if (!(level >= 0.0))
                throw GrapeException("Noise level must be higher or equal to 0 dB.");
        }

        auto [newIt, added] = m_NpdData.emplace(Thrust, ThrustNoiseLevels);

        if (!added)
            throw GrapeException(std::format("Noise levels at thrust {:.0f} N already exist.", Thrust));
    }

    double NpdData::interpolate(double Thrust, double Distance) const {
        // Get thrust and noise levels that bound Thrust
        // If thrust is not between min and max npd levels the first two or the last two are selected
        auto itThrust1 = m_NpdData.lower_bound(Thrust);
        if (itThrust1 == m_NpdData.end()) // Thrust is higher than maximum NPD thrust
            itThrust1 = std::prev(itThrust1, 2); // Npd MUST have at least 2 entries
        else if (itThrust1 != m_NpdData.begin())
            itThrust1 = std::prev(itThrust1, 1);
        auto itThrust2 = std::next(itThrust1, 1);
        const std::size_t indexThrust = std::distance(m_NpdData.begin(), itThrust1);
        auto& [Thrust1, NoiseLevels1] = *itThrust1;
        auto& [Thrust2, NoiseLevels2] = *itThrust2;

        auto itDist1 = std::ranges::lower_bound(NpdStandardDistances, Distance);

        std::size_t indexBase;
        std::size_t indexDist;

        // Distance is higher than max npd distance, extrapolate outwards
        if (itDist1 == NpdStandardDistances.end()) // 
        {
            itDist1 = std::prev(itDist1, 2);
            indexDist = std::distance(NpdStandardDistances.begin(), itDist1);
            indexBase = indexDist;

        }
        // Distance is between min and max npd distances, interpolate
        else if (itDist1 != NpdStandardDistances.begin())
        {
            itDist1 = std::prev(itDist1, 1);
            indexDist = std::distance(NpdStandardDistances.begin(), itDist1);
            indexBase = indexDist;
        }
        // Distance is lower than min npd distance, extrapolate inwards
        else
        {
            Distance = std::max(Distance, 30.0); // Distance lower than min npd distance -> apply minimum of 30.0 meters
            indexDist = 0;
            indexBase = 1;

        }

        const double iFactor = std::log10(Distance) - std::log10(NpdStandardDistances.at(indexBase));
        const double noiseLvlThrust1Dist = NoiseLevels1.at(indexBase) + iFactor * m_InterpolationMatrix.at(indexThrust).at(indexDist);
        const double noiseLvlThrust2Dist = NoiseLevels2.at(indexBase) + iFactor * m_InterpolationMatrix.at(indexThrust + 1).at(indexDist);

        return noiseLvlThrust1Dist + (Thrust - Thrust1) * (noiseLvlThrust2Dist - noiseLvlThrust1Dist) / (Thrust2 - Thrust1);
    }

    void NpdData::applyDelta(const PowerNoiseLevelsArray& Deltas) {
        for (auto& powerNoiseValues : m_NpdData | std::views::values)
        {
            for (std::size_t i = 0; i < NpdStandardDistancesSize; ++i)
                powerNoiseValues.at(i) += Deltas.at(i);
        }
        updateInterpolationMatrix();
    }

    void NpdData::updateInterpolationMatrix() {
        m_InterpolationMatrix.clear();

        for (const auto& noiseLevels : m_NpdData | std::views::values)
        {
            std::array<double, NpdStandardDistancesSize - 1> interpValues{};
            for (auto itDist = NpdStandardDistances.begin(); itDist != std::prev(NpdStandardDistances.end(), 1); ++itDist)
            {
                const std::size_t indexDist = std::distance(NpdStandardDistances.begin(), itDist);
                interpValues.at(indexDist) = (noiseLevels.at(indexDist + 1) - noiseLevels.at(indexDist))
                    / (std::log10(NpdStandardDistances.at(indexDist + 1)) - std::log10(NpdStandardDistances.at(indexDist)));
            }
            m_InterpolationMatrix.emplace_back(interpValues);
        }
    }

    Doc29Noise::Doc29Noise(std::string_view NameIn) : Name(NameIn), ArrivalSpectrum(*this), DepartureSpectrum(*this) {}

    bool Doc29Noise::valid() const {
        return validArrival() && validDeparture();
    }

    bool Doc29Noise::validArrival() const {
        return ArrivalLamax.size() > 1 && ArrivalSel.size() > 1;
    }

    bool Doc29Noise::validDeparture() const {
        return DepartureLamax.size() > 1 && DepartureSel.size() > 1;
    }
}
