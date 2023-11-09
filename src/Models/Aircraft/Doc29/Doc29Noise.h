// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Base/Conversions.h"
#include "Noise/AtmosphericAbsorption.h"
#include "Noise/Noise.h"

namespace GRAPE {
    class Doc29Noise;

    constexpr std::size_t NpdStandardDistancesSize = 10;

    constexpr std::array<double, NpdStandardDistancesSize> NpdStandardDistances = {
        fromFeet(200.0),   // 1
        fromFeet(400.0),   // 2
        fromFeet(630.0),   // 3
        fromFeet(1000.0),  // 4
        fromFeet(2000.0),  // 5
        fromFeet(4000.0),  // 6
        fromFeet(6300.0),  // 7
        fromFeet(10000.0), // 8
        fromFeet(16000.0), // 9
        fromFeet(25000.0), // 10
    };

    constexpr std::array<double, OneThirdOctaveBandsSize> NpdStandardAverageAttenuationRates{ 0.00033, 0.00033, 0.00033, 0.00066, 0.00066, 0.00098, 0.00131, 0.00131, 0.00197, 0.00230, 0.00295, 0.00361, 0.00459, 0.00590, 0.00754, 0.00983, 0.01311, 0.01705, 0.02295, 0.03115, 0.03607, 0.05246, 0.07213, 0.09836, };

    /**
    * @brief A Doc29Spectrum is owned by a Doc29Noise. It keeps one spectrum, meaning the unweighted sound at the one third octave bands for a reference distance of 1000 m normalized to the SAE-AIR-1845 atmosphere.
    */
    class Doc29Spectrum {
    public:
        explicit Doc29Spectrum(const Doc29Noise& Doc29Ns);

        /**
        * @return The parent Doc29Noise.
        */
        [[nodiscard]] const Doc29Noise& parentDoc29Noise() const noexcept;

        /**
        * @return The sound levels for this spectrum.
        */
        [[nodiscard]] const OneThirdOctaveArray& noiseLevels() const noexcept { return m_OctaveNoiseLevels; }

        /**
        * @return The sound level at Index.
        *
        * ASSERT Index < size().
        */
        [[nodiscard]] auto operator()(std::size_t Index) const noexcept { GRAPE_ASSERT(Index < size()); return m_OctaveNoiseLevels.at(Index); }

        /**
        * @return Const iterator to the beginning of the spectrum array.
        */
        [[nodiscard]] auto begin() const noexcept { return m_OctaveNoiseLevels.begin(); }

        /**
        * @return Const iterator to the end of the spectrum array.
        */
        [[nodiscard]] auto end() const noexcept { return m_OctaveNoiseLevels.end(); }

        /**
        * @return The size of the spectrum array.
        */
        [[nodiscard]] std::size_t size() const { return m_OctaveNoiseLevels.size(); }

        /**
        * @return The sound level at Index.
        *
        * ASSERT Index < size().
        */
        [[nodiscard]] auto& operator()(std::size_t Index) noexcept { GRAPE_ASSERT(Index < size()); return m_OctaveNoiseLevels.at(Index); }

        /**
        * @return Iterator to the beginning of the spectrum array.
        */
        [[nodiscard]] auto begin() noexcept { return m_OctaveNoiseLevels.begin(); }

        /**
        * @return Iterator to the end of the spectrum array.
        */
        [[nodiscard]] auto end() noexcept { return m_OctaveNoiseLevels.end(); }

        /**
        * @brief Throwing version to set Value at Index.
        *
        * ASSERT Index < size().
        * Throws if Value not in [0, inf].
        */
        void setValue(std::size_t Index, double Value);
    private:
        std::reference_wrapper<const Doc29Noise> m_Doc29Noise;
        OneThirdOctaveArray m_OctaveNoiseLevels{};
    };

    /**
    * @brief A NpdData is owned by a Doc29Noise. It maps thrust levels to noise levels at the 10 standard NPD distances.
    */
    class NpdData {
    public:
        typedef std::array<double, NpdStandardDistancesSize> PowerNoiseLevelsArray;

        NpdData() = default;

        /**
        * @return Const iterator to the beginning of the NPD map.
        */
        [[nodiscard]] auto begin() const { return m_NpdData.begin(); }

        /**
        * @return Const iterator to the end of the NPD map.
        */
        [[nodiscard]] auto end() const { return m_NpdData.end(); }

        /**
        * @return Iterator to the beginning of the NPD map.
        */
        [[nodiscard]] auto begin() { return m_NpdData.begin(); }

        /**
        * @return Iterator to the end of the NPD map.
        */
        [[nodiscard]] auto end() { return m_NpdData.end(); }

        /**
        * @brief Add Thrust and noise values to the map.
        * @return True if Thrust was added, false if it already existed.
        */
        bool addThrust(double Thrust) noexcept;

        /**
        * @brief Add Thrust and noise values to the map.
        * @return True if Thrust was added, false if it already existed.
        */
        bool addThrust(double Thrust, const PowerNoiseLevelsArray& ThrustNoiseLevels) noexcept;

        /**
        * @brief Update the thrust value Current to New.
        * @return True if thrust was updated, false if New already existed.
        */
        bool updateThrust(double Current, double New) noexcept;

        /**
        * @brief Delete the whole NPD map.
        */
        void clear() noexcept;

        /**
        * @brief Throwing version of addThrust(double, const PowerNoiseLevelsArray&).
        *
        * Throws if Thrust not in ]0.0, inf].
        * Throws if any noise level not int [0.0, inf].
        * Throws if Thrust already existed in the NPD map.
        */
        void addThrustE(double Thrust, const PowerNoiseLevelsArray& ThrustNoiseLevels);

        /**
        * @return The size of the NPD map, meaning the number of thrust values for which noise levels are defined.
        */
        [[nodiscard]] std::size_t size() const { return m_NpdData.size(); }

        /**
        * @return True if the NPD map is empty.
        */
        [[nodiscard]] bool empty() const { return m_NpdData.empty(); }

        /**
        * @return True if the Thrust is in the NPD map.
        */
        [[nodiscard]] bool contains(double Thrust) const { return m_NpdData.contains(Thrust); }

        /**
        * @brief Change the stored noise values by the deltas specified.
        */
        void applyDelta(const PowerNoiseLevelsArray& Deltas);

        /**
        * @return Interpolates the NPD map to get a noise value at Thrust and Distance.
        */
        [[nodiscard]] double interpolate(double Thrust, double Distance) const;
    private:
        // Data
        std::map<double, PowerNoiseLevelsArray> m_NpdData;
        std::vector<std::array<double, NpdStandardDistancesSize - 1>> m_InterpolationMatrix;

    private:
        void updateInterpolationMatrix();
    };

    /**
    * @brief A Doc29Noise contains the data needed to calculate noise at a given receptor with the Doc29 noise model.
    */
    class Doc29Noise {
    public:
        enum class LateralDirectivity {
            Wing = 0, Fuselage, Propeller,
        };

        static constexpr EnumStrings<LateralDirectivity> LateralDirectivities{ "Wing", "Fuselage", "Propeller" };

        enum class SORCorrection {
            None = 0, Jet, Turboprop,
        };

        static constexpr EnumStrings<SORCorrection> SORCorrections{ "None", "Jet", "Turboprop" };

        explicit Doc29Noise(std::string_view NameIn);
        Doc29Noise(const Doc29Noise&) = delete;
        Doc29Noise(Doc29Noise&&) = default;
        Doc29Noise& operator=(const Doc29Noise&) = delete;
        Doc29Noise& operator=(Doc29Noise&&) = default;
        ~Doc29Noise() = default;

        std::string Name;
        LateralDirectivity LateralDir = LateralDirectivity::Wing;
        SORCorrection SOR = SORCorrection::Jet;

        Doc29Spectrum ArrivalSpectrum;
        Doc29Spectrum DepartureSpectrum;

        NpdData ArrivalLamax;
        NpdData ArrivalSel;
        NpdData DepartureLamax;
        NpdData DepartureSel;

        /**
        * @return True if all NPD maps have at least two entries.
        */
        [[nodiscard]] bool valid() const;

        /**
        * @return True if all NPD maps for arrivals have at least two entries.
        */
        [[nodiscard]] bool validArrival() const;

        /**
        * @return True if all NPD maps for departure have at least two entries.
        */
        [[nodiscard]] bool validDeparture() const;
    };
}
