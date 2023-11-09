// Copyright (C) 2023 Goncalo Soares Roque 

#pragma once

#include "Base/Conversions.h"
#include <filesystem>

namespace GRAPE {
    class Aircraft;
    class Doc29Noise;

    namespace IO {
        class AnpImport {
        public:
            explicit AnpImport(const std::string& Folder, bool StopOnError = false);

            inline static bool s_ImportFleet = false;
            inline static double s_MaxThresholdCrossingAltitude = fromFeet(50.0);
        private:
            std::filesystem::path m_FolderPath;

            enum class File {
                Aircraft = 0,
                JetEngineCoefficients,
                PropellerEngineCoefficients,
                AerodynamicCoefficients,
                DefaultApproachProceduralSteps,
                DefaultDepartureProceduralSteps,
                DefaultFixedPointProfiles,
                SpectralClasses,
                NpdData,
            };

            std::unordered_map<File, std::filesystem::path> m_Files; // Stores the actual file paths found
        private:
            bool m_StopOnError;

            struct FileTraits {
                const char* Name = nullptr;
                std::size_t ColumnCount = 0;
            };

            inline const static std::unordered_map<File, FileTraits> s_Files{
                { File::Aircraft, { "aircraft", 16 } },
                { File::JetEngineCoefficients, { "jet_engine_coefficients", 11 } },
                { File::PropellerEngineCoefficients, { "propeller_engine_coefficients", 4 } },
                { File::AerodynamicCoefficients, { "aerodynamic_coefficients", 7 } },
                { File::DefaultApproachProceduralSteps, { "default_approach_procedural_steps", 11 } },
                { File::DefaultDepartureProceduralSteps, { "default_departure_procedural_steps", 11 } },
                { File::DefaultFixedPointProfiles, { "default_fixed_point_profiles", 9 } },
                { File::SpectralClasses, { "spectral_classes", 27 } },
                { File::NpdData, { "npd_data", 14 } },
            };


            std::vector<std::string> m_PistonAircraft;
            enum class PowerParameter {
                Pounds = 0, Percentage,
            };

            std::unordered_map<std::string, std::vector<Doc29Noise*>> m_ArrivalSpectralClasses;
            std::unordered_map<std::string, std::vector<Doc29Noise*>> m_DepartureSpectralClasses;

            struct NoisePowerPercentageParams {
                NoisePowerPercentageParams(std::string_view NameIn, double MaximumStaticThrustIn) : Name(NameIn), MaximumStaticThrust(MaximumStaticThrustIn) {}
                std::string Name;
                double MaximumStaticThrust = Constants::NaN;
            };
            std::unordered_map<std::string, std::vector<NoisePowerPercentageParams>> m_PercentagePowerParameters;

        private:
            void parseFolder();
            [[nodiscard]] std::string getFilePathString(File Doc29File) const;
            [[nodiscard]] static std::size_t getColumnCount(File Doc29File);

            [[nodiscard]] bool loadAircrafts();
            [[nodiscard]] bool loadSpectralClasses();
            [[nodiscard]] bool loadEngineCoefficientsJet() const;
            [[nodiscard]] bool loadEngineCoefficientsTurboprop() const;
            [[nodiscard]] bool loadAerodynamicCoefficients() const;
            [[nodiscard]] bool loadFixedPointProfiles() const;
            [[nodiscard]] bool loadArrivalProceduralSteps() const;
            [[nodiscard]] bool loadDepartureProceduralSteps() const;
            [[nodiscard]] bool loadNpdData() const;
        };
    }
}
