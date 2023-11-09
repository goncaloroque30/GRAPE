// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "AnpImport.h"

#include "Application.h"
#include "Aircraft/Aircraft.h"
#include "Aircraft/Doc29/Doc29Aircraft.h"
#include "Aircraft/Doc29/Doc29Noise.h"

#include "Csv.h"

namespace GRAPE::IO {
    AnpImport::AnpImport(const std::string& Folder, bool StopOnError) : m_FolderPath(Folder), m_StopOnError(StopOnError) {
        if (!is_directory(m_FolderPath))
        {
            Log::io()->error("Importing Doc29 tables. '{}' is not a directory", m_FolderPath.string());
            return;
        }

        try { parseFolder(); }
        catch (const std::exception& err)
        {
            Log::io()->error("Importing Doc29 tables from {}. {}", m_FolderPath.string(), err.what());
            return;
        }

        const auto& db = Application::study().db();

        db.beginTransaction();
        bool success = loadAircrafts();
        db.commitTransaction();
        if (m_StopOnError && !success)
            return;

        db.beginTransaction();
        success = loadSpectralClasses();
        db.commitTransaction();
        if (m_StopOnError && !success)
            return;

        db.beginTransaction();
        success = loadEngineCoefficientsJet();
        db.commitTransaction();
        if (m_StopOnError && !success)
            return;

        db.beginTransaction();
        success = loadEngineCoefficientsTurboprop();
        db.commitTransaction();
        if (m_StopOnError && !success)
            return;

        db.beginTransaction();
        success = loadAerodynamicCoefficients();
        db.commitTransaction();
        if (m_StopOnError && !success)
            return;

        db.beginTransaction();
        success = loadArrivalProceduralSteps();
        db.commitTransaction();
        if (m_StopOnError && !success)
            return;

        db.beginTransaction();
        success = loadDepartureProceduralSteps();
        db.commitTransaction();
        if (m_StopOnError && !success)
            return;

        db.beginTransaction();
        success = loadFixedPointProfiles();
        db.commitTransaction();
        if (m_StopOnError && !success)
            return;

        db.beginTransaction();
        success = loadNpdData();
        db.commitTransaction();
        if (m_StopOnError && !success)
            return;
    }

    void AnpImport::parseFolder() {
        for (const auto& [file, traits] : s_Files)
        {
            const auto& [fileName, columnCount] = traits;

            bool found = false;
            for (const auto& dirEntry : std::filesystem::directory_iterator(m_FolderPath))
            {
                std::string lowerFileName = dirEntry.path().stem().string();
                std::ranges::transform(lowerFileName, lowerFileName.begin(), [](const auto c) { return std::tolower(c); });
                if (lowerFileName.find(fileName) != std::string::npos)
                {
                    try
                    {
                        Csv csv;
                        csv.setImport(dirEntry.path().string(), columnCount);
                    }
                    catch (const GrapeException& err)
                    {
                        throw GrapeException(std::format("Importing {} from '{}'. {}.", fileName, dirEntry.path().string(), err.what()));
                    }

                    m_Files.try_emplace(file, dirEntry.path());
                    found = true;
                    break;
                }
            }
            if (!found)
                throw GrapeException(std::format("The {} file couldn't be found.", fileName));
        }
    }

    std::string AnpImport::getFilePathString(File Doc29File) const {
        GRAPE_ASSERT(m_Files.contains(Doc29File));

        return m_Files.at(Doc29File).string();
    }

    std::size_t AnpImport::getColumnCount(File Doc29File) {
        return s_Files.at(Doc29File).ColumnCount;
    }

    bool AnpImport::loadAircrafts() {
        auto& study = Application::study();

        const auto filePath = getFilePathString(File::Aircraft);
        Csv csv;

        try { csv.setImport(filePath, getColumnCount(File::Aircraft)); }
        catch (const std::exception& err)
        {
            Log::io()->error("Importing ANP aircraft from '{}'. {}", filePath, err.what());
            return false;
        }

        for (std::size_t row = 0; row < csv.rowCount(); row++)
        {
            Aircraft* newAcft = nullptr;
            const Doc29Aircraft* newDoc29Acft = nullptr;
            const Doc29Noise* newDoc29Ns = nullptr;
            try
            {
                // Doc29 Aircraft
                auto doc29AcftName = csv.getCell<std::string>(row, 0);
                if (doc29AcftName.empty())
                    throw GrapeException("Empty Doc29 Aircraft name.");
                auto& doc29Acft = study.Doc29Aircrafts.addPerformanceE(doc29AcftName);
                newDoc29Acft = &doc29Acft;

                const auto typeStr = csv.getCell<std::string>(row, 2);
                if (typeStr == "Jet")
                    doc29Acft.setThrustType(Doc29Thrust::Type::Rating);
                else if (typeStr == "Turboprop")
                    doc29Acft.setThrustType(Doc29Thrust::Type::RatingPropeller);
                else if (typeStr == "Piston")
                    m_PistonAircraft.emplace_back(doc29AcftName);

                try { doc29Acft.setMaximumSeaLevelStaticThrust(fromPoundsOfForce(csv.getCell<double>(row, 9))); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid maximum sea level static thrust."); }

                study.Doc29Aircrafts.updatePerformance(doc29Acft);

                // Aircraft
                if (s_ImportFleet)
                {
                    if (study.Aircrafts().contains(doc29Acft.Name))
                    {
                        Log::io()->warn("Aircraft '{0}' already exists in this study. ANP data for '{0}' will overwrite it.", doc29Acft.Name);
                        newAcft = &study.Aircrafts(doc29Acft.Name);
                    }
                    else
                    {
                        newAcft = &study.Aircrafts.addAircraftE(doc29Acft.Name);
                    }
                    auto& acft = *newAcft;

                    study.Aircrafts.setDoc29Performance(acft, newDoc29Acft);

                    // Engine Count
                    try { acft.setEngineCountE(csv.getCell<int>(row, 3)); }
                    catch (const GrapeException&) { throw; }
                    catch (...) { throw std::invalid_argument("Invalid engine count."); }
                }

                // Doc29 Noise

                // Npd ID
                auto noiseId = csv.getCell<std::string>(row, 11);

                // Power Parameter
                const auto powerParamStr = csv.getCell<std::string>(row, 12);
                auto powerParam = PowerParameter::Pounds;

                if (powerParamStr == "Pounds" || powerParamStr == "CNT (lb)")
                    powerParam = PowerParameter::Pounds;
                else if (powerParamStr == "Percent" || powerParamStr == "CNT (% of Max Static Thrust)")
                    powerParam = PowerParameter::Percentage;
                else
                    throw GrapeException(std::format("Power parameter '{}' not supported.", powerParamStr));

                if (powerParam == PowerParameter::Percentage) // Percentage will be converted to thrust and multiple created
                {
                    const std::string noiseIdGrape = std::string(noiseId).append(" ").append(doc29AcftName);
                    m_PercentagePowerParameters[noiseId].emplace_back(noiseIdGrape, doc29Acft.MaximumSeaLevelStaticThrust);
                    noiseId = noiseIdGrape;
                }

                if (study.Doc29Noises().contains(noiseId))
                {
                    if (s_ImportFleet)
                    {
                        study.Aircrafts.setDoc29Noise(*newAcft, &study.Doc29Noises(noiseId));
                        study.Aircrafts.update(*newAcft);
                    }
                    continue;
                }


                auto& doc29Ns = study.Doc29Noises.addNoiseE(noiseId);
                newDoc29Ns = &doc29Ns;

                if (s_ImportFleet)
                {
                    study.Aircrafts.setDoc29Noise(*newAcft, newDoc29Ns);
                    study.Aircrafts.update(*newAcft);
                }

                // Arrival Spectral Class
                const auto arrivalSpectralClass = csv.getCell<std::string>(row, 13);
                m_ArrivalSpectralClasses[arrivalSpectralClass].emplace_back(&doc29Ns);

                // Departure Spectral Class
                const auto departureSpectralClass = csv.getCell<std::string>(row, 14);
                m_DepartureSpectralClasses[departureSpectralClass].emplace_back(&doc29Ns);

                // Lateral directivity
                const auto lateralDirectivityStr = csv.getCell<std::string>(row, 15);
                if (lateralDirectivityStr == "Wing")
                    doc29Ns.LateralDir = Doc29Noise::LateralDirectivity::Wing;
                else if (lateralDirectivityStr == "Fuselage")
                    doc29Ns.LateralDir = Doc29Noise::LateralDirectivity::Fuselage;
                else if (lateralDirectivityStr == "Prop")
                    doc29Ns.LateralDir = Doc29Noise::LateralDirectivity::Propeller;
                else
                    throw GrapeException(std::format("Invalid lateral directivity identifier '{}'.", lateralDirectivityStr));

                // Start of Roll Correction (Depends on aircraft type)
                if (typeStr == "Jet")
                    doc29Ns.SOR = Doc29Noise::SORCorrection::Jet;
                else if (typeStr == "Turboprop")
                    doc29Ns.SOR = Doc29Noise::SORCorrection::Turboprop;
                else
                    doc29Ns.SOR = Doc29Noise::SORCorrection::None;

                study.Doc29Noises.updateNoise(doc29Ns);
            }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing ANP aircraft at row {}. {}", row + 2, err.what());
                if (newAcft)
                    study.Aircrafts.erase(*newAcft);
                if (newDoc29Acft)
                    study.Doc29Aircrafts.erasePerformance(*newDoc29Acft);
                if (newDoc29Ns)
                    study.Doc29Noises.eraseNoise(*newDoc29Ns);

                if (m_StopOnError)
                    return false;
            }
        }
        return true;
    }

    bool AnpImport::loadSpectralClasses() {
        const auto& study = Application::study();

        const auto filePath = getFilePathString(File::SpectralClasses);
        Csv csv;

        try { csv.setImport(filePath, getColumnCount(File::SpectralClasses)); }
        catch (const std::exception& err)
        {
            Log::io()->error("Importing spectral classes from '{}'. {}", filePath, err.what());
            return false;
        }

        for (std::size_t row = 0; row < csv.rowCount(); row++)
        {
            try
            {
                // Name
                const auto name = csv.getCell<std::string>(row, 0);

                std::array<double, OneThirdOctaveBandsSize> spectrum{};
                for (std::size_t i = 0; i != OneThirdOctaveBandsSize; ++i)
                {
                    try { spectrum.at(i) = csv.getCell<double>(row, i + 3); }
                    catch (...) { throw GrapeException(std::format("Invalid spectrum value in column {}.", i + 3)); }
                }

                // Op Type
                const auto opTypeStr = csv.getCell<std::string>(row, 1);
                auto opType = OperationType::Arrival;
                if (opTypeStr == "A" || opTypeStr == "Approach")
                {
                    if (!m_ArrivalSpectralClasses.contains(name))
                        continue; // Silently ignore unused spectral classes
                    opType = OperationType::Arrival;
                }
                else if (opTypeStr == "D" || opTypeStr == "Departure")
                {
                    if (!m_DepartureSpectralClasses.contains(name))
                        continue; // Silently ignore unused spectral classes
                    opType = OperationType::Departure;
                }
                else { throw GrapeException(std::format("Invalid operation type '{}'", opTypeStr)); }

                switch (opType)
                {
                case OperationType::Arrival:
                    {
                        for (const auto doc29Ns : m_ArrivalSpectralClasses.at(name))
                        {
                            for (std::size_t i = 0; i != OneThirdOctaveBandsSize; ++i)
                                doc29Ns->ArrivalSpectrum.setValue(i, spectrum.at(i));
                            study.Doc29Noises.updateNoise(*doc29Ns);
                        }
                        m_ArrivalSpectralClasses.erase(name);
                        break;
                    }
                case OperationType::Departure:
                    {
                        for (const auto doc29Ns : m_DepartureSpectralClasses.at(name))
                        {
                            for (std::size_t i = 0; i != OneThirdOctaveBandsSize; ++i)
                                doc29Ns->DepartureSpectrum.setValue(i, spectrum.at(i));
                            study.Doc29Noises.updateNoise(*doc29Ns);
                        }
                        m_DepartureSpectralClasses.erase(name);
                        break;
                    }
                default: GRAPE_ASSERT(false);
                }
            }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing spectral class at row {}. {}", row + 2, err.what());
                if (m_StopOnError)
                    return false;
            }
        }

        if (!m_ArrivalSpectralClasses.empty())
        {
            for (const auto& [spectralClass, doc29NoisesVec] : m_ArrivalSpectralClasses)
                Log::io()->error("There were {} aircraft with arrival spectral class '{}'. This spectral class couldn't be imported or was not found.", doc29NoisesVec.size(), spectralClass);
            if (m_StopOnError)
                return false;
        }

        if (!m_DepartureSpectralClasses.empty())
        {
            for (const auto& [spectralClass, doc29NoisesVec] : m_DepartureSpectralClasses)
                Log::io()->error("There were {} aircraft with departure spectral class '{}'. This spectral class couldn't be imported or was not found.", doc29NoisesVec.size(), spectralClass);
            if (m_StopOnError)
                return false;
        }

        return true;
    }

    bool AnpImport::loadEngineCoefficientsJet() const {
        auto& study = Application::study();

        const auto filePath = getFilePathString(File::JetEngineCoefficients);
        Csv csv;

        try { csv.setImport(filePath, getColumnCount(File::JetEngineCoefficients)); }
        catch (const std::exception& err)
        {
            Log::io()->error("Importing jet engine coefficients from '{}'. {}", filePath, err.what());
            return false;
        }

        for (std::size_t row = 0; row < csv.rowCount(); ++row)
        {
            try
            {
                // Aircraft Name
                auto doc29AcftName = csv.getCell<std::string>(row, 0);
                if (!study.Doc29Aircrafts().contains(doc29AcftName))
                    throw GrapeException(std::format("Doc29 Aircraft '{}' not found in the study.", doc29AcftName));
                auto& doc29Acft = study.Doc29Aircrafts(doc29AcftName);

                // Check that thrust type is set to rating
                if (doc29Acft.thrust().type() != Doc29Thrust::Type::Rating)
                {
                    Log::io()->warn("Jet engine coefficients found for Doc29 Aircraft '{}'. Thrust type will be set to rating.", doc29AcftName);
                    doc29Acft.setThrustType(Doc29Thrust::Type::Rating);
                }

                // Thrust rating
                auto thrustRatingStr = csv.getCell<std::string>(row, 1);
                auto thrustRating = Doc29Thrust::Rating::MaximumTakeoff;
                if (thrustRatingStr == "MaxTakeoff")
                    thrustRating = Doc29Thrust::Rating::MaximumTakeoff;
                else if (thrustRatingStr == "MaxClimb")
                    thrustRating = Doc29Thrust::Rating::MaximumClimb;
                else if (thrustRatingStr == "IdleApproach")
                    thrustRating = Doc29Thrust::Rating::Idle;
                else if (thrustRatingStr == "MaxTkoffHiTemp")
                    thrustRating = Doc29Thrust::Rating::MaximumTakeoffHighTemperature;
                else if (thrustRatingStr == "MaxClimbHiTemp")
                    thrustRating = Doc29Thrust::Rating::MaximumClimbHighTemperature;
                else if (thrustRatingStr == "IdleApproachHiTemp")
                    thrustRating = Doc29Thrust::Rating::IdleHighTemperature;
                else if (thrustRatingStr == "General")
                    continue; // Silently skip General thrust coefficients
                else
                    throw GrapeException(std::format("Thrust rating '{}' not supported.", thrustRatingStr));

                // Coefficients
                Doc29ThrustRating::Coefficients coeffs;

                // E
                if (!csv.getCell<std::string>(row, 2).empty())
                {
                    try { coeffs.E = fromPoundsOfForce(csv.getCell<double>(row, 2)); }
                    catch (...) { throw std::invalid_argument("Invalid E coefficient."); }
                }

                // F
                if (!csv.getCell<std::string>(row, 3).empty())
                {
                    try { coeffs.F = fromPoundsOfForcePerKnot(csv.getCell<double>(row, 3)); }
                    catch (...) { throw std::invalid_argument("Invalid F coefficient."); }
                }

                // Ga
                if (!csv.getCell<std::string>(row, 4).empty())
                {
                    try { coeffs.Ga = fromPoundsOfForcePerFoot(csv.getCell<double>(row, 4)); }
                    catch (...) { throw std::invalid_argument("Invalid Ga coefficient."); }
                }

                // Gb
                if (!csv.getCell<std::string>(row, 5).empty())
                {
                    try { coeffs.Gb = fromPoundsOfForcePerFoot2(csv.getCell<double>(row, 5)); }
                    catch (...) { throw std::invalid_argument("Invalid Gb coefficient."); }
                }

                // H
                if (!csv.getCell<std::string>(row, 6).empty())
                {
                    try { coeffs.H = fromPoundsOfForcePerCelsius(csv.getCell<double>(row, 6)); }
                    catch (...) { throw std::invalid_argument("Invalid H coefficient."); }
                }

                auto& doc29Thrust = static_cast<Doc29ThrustRating&>(doc29Acft.thrust());
                auto [newCoeffs, added] = doc29Thrust.Coeffs.add(thrustRating, coeffs);
                if (!added)
                    throw GrapeException(std::format("The thrust rating {} ({}) has already been added.", thrustRatingStr, Doc29Thrust::Ratings.toString(thrustRating)));

                study.Doc29Aircrafts.updateThrust(doc29Acft);
            }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing jet engine coefficients at row {}. {}", row + 2, err.what());
                if (m_StopOnError)
                    return false;
            }
        }
        return true;
    }

    bool AnpImport::loadEngineCoefficientsTurboprop() const {
        auto& study = Application::study();

        const auto filePath = getFilePathString(File::PropellerEngineCoefficients);
        Csv csv;

        try { csv.setImport(filePath, getColumnCount(File::PropellerEngineCoefficients)); }
        catch (const std::exception& err)
        {
            Log::io()->error("Importing propeller engine coefficients from '{}'. {}", filePath, err.what());
            return false;
        }

        for (std::size_t row = 0; row < csv.rowCount(); ++row)
        {
            try
            {
                auto doc29AcftName = csv.getCell<std::string>(row, 0);
                if (!study.Doc29Aircrafts().contains(doc29AcftName))
                    throw GrapeException(std::format("Doc29 Aircraft '{}' not found in the study.", doc29AcftName));
                auto& doc29Acft = study.Doc29Aircrafts(doc29AcftName);

                if (doc29Acft.thrust().type() != Doc29Thrust::Type::RatingPropeller)
                {
                    Log::io()->warn("Propeller engine coefficients found for Doc29 Aircraft '{}'. Thrust type will be set to propeller.", doc29AcftName);
                    doc29Acft.setThrustType(Doc29Thrust::Type::RatingPropeller);
                }

                // Thrust rating
                auto thrustRatingStr = csv.getCell<std::string>(row, 1);
                auto thrustRating = Doc29Thrust::Rating::MaximumTakeoff;
                if (thrustRatingStr == "MaxTakeoff")
                    thrustRating = Doc29Thrust::Rating::MaximumTakeoff;
                else if (thrustRatingStr == "MaxClimb")
                    thrustRating = Doc29Thrust::Rating::MaximumClimb;
                else if (thrustRatingStr == "General")
                    continue; // Silently skip General thrust coefficients
                else
                    throw GrapeException(std::format("Thrust rating propeller '{}' not supported.", thrustRatingStr));

                // Coefficients
                Doc29ThrustRatingPropeller::Coefficients coeffs;

                // Propeller Efficiency
                if (!csv.getCell<std::string>(row, 2).empty())
                {
                    try { coeffs.setEfficiency(csv.getCell<double>(row, 2)); }
                    catch (const GrapeException&) { throw; }
                    catch (...) { throw std::invalid_argument("Invalid propeller efficiency."); }
                }

                // Propeller Power
                if (!csv.getCell<std::string>(row, 3).empty())
                {
                    try { coeffs.setPower(fromHorsePower(csv.getCell<double>(row, 3))); }
                    catch (const GrapeException&) { throw; }
                    catch (...) { throw std::invalid_argument("Invalid propeller power."); }
                }

                auto& doc29ThrustPropeller = static_cast<Doc29ThrustRatingPropeller&>(doc29Acft.thrust());
                auto [newCoeffs, added] = doc29ThrustPropeller.addCoefficients(thrustRating, coeffs);
                if (!added)
                    throw GrapeException(std::format("The thrust rating {} ({}) has already been added.", thrustRatingStr, Doc29Thrust::Ratings.toString(thrustRating)));

                study.Doc29Aircrafts.updateThrust(doc29Acft);
            }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing propeller engine coefficients at row {}. {}", row + 2, err.what());
                if (m_StopOnError)
                    return false;
            }
        }
        return true;
    }

    bool AnpImport::loadAerodynamicCoefficients() const {
        auto& study = Application::study();

        const auto filePath = getFilePathString(File::AerodynamicCoefficients);
        Csv csv;

        try { csv.setImport(filePath, getColumnCount(File::AerodynamicCoefficients)); }
        catch (const std::exception& err)
        {
            Log::io()->error("Importing aerodynamic coefficients from '{}'. {}", filePath, err.what());
            return false;
        }

        for (std::size_t row = 0; row < csv.rowCount(); ++row)
        {
            try
            {
                // Aircraft
                const auto doc29AcftName = csv.getCell<std::string>(row, 0);
                if (!study.Doc29Aircrafts().contains(doc29AcftName))
                    throw GrapeException(std::format("Doc29 Aircraft '{}' not found in the study.", doc29AcftName));
                auto& doc29Acft = study.Doc29Aircrafts(doc29AcftName);

                // Operation Type
                const auto opTypeStr = csv.getCell<std::string>(row, 1);
                if (opTypeStr != "A" && opTypeStr != "D")
                    throw GrapeException(std::format("Operation type '{}' is not supported.", opTypeStr));

                // Name
                const auto aeroCoeffName = csv.getCell<std::string>(row, 2);
                if (aeroCoeffName.empty())
                    throw GrapeException("Flap ID can't be empty.");

                const std::string insertCoeffName = std::string(aeroCoeffName).append(" ").append(opTypeStr);

                auto [coeff, added] = doc29Acft.AerodynamicCoefficients.add(insertCoeffName, insertCoeffName);
                if (!added)
                    throw GrapeException(std::format("Duplicate flap ID '{}' for operation type {}.", aeroCoeffName, opTypeStr));

                if (!csv.getCell<std::string>(row, 6).empty())
                {
                    try { coeff.setRCoeffE(csv.getCell<double>(row, 6)); }
                    catch (const GrapeException&) { throw; }
                    catch (...) { throw std::invalid_argument("Invalid R coefficient."); }
                }
                else
                {
                    throw GrapeException("R coefficient can't be empty.");
                }

                // Not empty D
                if (!csv.getCell<std::string>(row, 5).empty())
                {
                    try { coeff.setDCoeffE(fromKnotsPerPoundOfForceSqrt(csv.getCell<double>(row, 5))); }
                    catch (const GrapeException&) { throw; }
                    catch (...) { throw std::invalid_argument("Invalid D coefficient."); }
                    coeff.CoefficientType = Doc29AerodynamicCoefficients::Type::Land;
                }
                // Not empty B and C
                else if (!csv.getCell<std::string>(row, 3).empty() && !csv.getCell<std::string>(row, 4).empty())
                {
                    try { coeff.setBCoeffE(fromFeetPerPoundOfForce(csv.getCell<double>(row, 3))); }
                    catch (const GrapeException&) { throw; }
                    catch (...) { throw std::invalid_argument("Invalid B coefficient."); }

                    try { coeff.setCCoeffE(fromKnotsPerPoundOfForceSqrt(csv.getCell<double>(row, 4))); }
                    catch (const GrapeException&) { throw; }
                    catch (...) { throw std::invalid_argument("Invalid C coefficient."); }

                    coeff.CoefficientType = Doc29AerodynamicCoefficients::Type::Takeoff;
                }

                study.Doc29Aircrafts.updateAerodynamicCoefficients(doc29Acft);
            }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing aerodynamic coefficients at row {}. {}", row + 2, err.what());
                if (m_StopOnError)
                    return false;
            }
        }
        return true;
    }

    bool AnpImport::loadFixedPointProfiles() const {
        auto& study = Application::study();

        const auto filePath = getFilePathString(File::DefaultFixedPointProfiles);
        Csv csv;

        try { csv.setImport(filePath, getColumnCount(File::DefaultFixedPointProfiles)); }
        catch (const std::exception& err)
        {
            Log::io()->error("Importing default fixed point profiles from '{}'. {}", filePath, err.what());
            return false;
        }

        for (std::size_t row = 0; row < csv.rowCount(); ++row)
        {
            try
            {
                // Aircraft
                auto doc29AcftName = csv.getCell<std::string>(row, 0);
                if (!study.Doc29Aircrafts().contains(doc29AcftName))
                    throw GrapeException(std::format("Doc29 Aircraft '{}' not found in the study.", doc29AcftName));
                auto& doc29Acft = study.Doc29Aircrafts(doc29AcftName);

                if (std::ranges::find(m_PistonAircraft, doc29AcftName) != m_PistonAircraft.end())
                    throw GrapeException(std::format("ANP aircraft '{}' is of piston type. Fixed point profiles not suported", doc29AcftName));
                // Operation Type
                auto opTypeStr = csv.getCell<std::string>(row, 1);
                auto opType = OperationType::Arrival;
                if (opTypeStr == "A")
                    opType = OperationType::Arrival;
                else if (opTypeStr == "D")
                    opType = OperationType::Departure;
                else
                    throw GrapeException(std::format("Operation type '{}' is not supported.", opTypeStr));

                const auto profileId = csv.getCell<std::string>(row, 2);
                auto stageLength = csv.getCell<std::string>(row, 3);
                auto pointNumber = csv.getCell<std::string>(row, 4);
                std::string profileName = profileId;
                profileName.append(" ").append(stageLength);

                // Cumulative Ground Distance
                double cumGroundDist = Constants::NaN;
                try { cumGroundDist = fromFeet(csv.getCell<double>(row, 5)); }
                catch (...) { throw GrapeException("Invalid cumulative ground distance."); }

                // Altitude AFE
                double altitudeAfe = Constants::NaN;
                try { altitudeAfe = fromFeet(csv.getCell<double>(row, 6)); }
                catch (...) { throw GrapeException("Invalid altitude AFE."); }

                // True Airspeed
                double tas = Constants::NaN;
                try { tas = fromKnots(csv.getCell<double>(row, 7)); }
                catch (...) { throw GrapeException("Invalid cumulative ground distance."); }

                // Thrust
                double thrust = Constants::NaN;
                try { thrust = fromPoundsOfForce(csv.getCell<double>(row, 8)); }
                catch (...) { throw GrapeException("Invalid cumulative ground distance."); }

                switch (opType)
                {
                case OperationType::Arrival:
                    {
                        Doc29ProfileArrivalPoints* doc29ProfPts = nullptr;
                        if (doc29Acft.ArrivalProfiles.contains(profileName))
                        {
                            auto& doc29Prof = doc29Acft.ArrivalProfiles(profileName);
                            if (doc29Prof->type() != Doc29Profile::Type::Points)
                                throw GrapeException("Arrival profile with the same name but of different type already exists in the study.");

                            doc29ProfPts = static_cast<Doc29ProfileArrivalPoints*>(doc29Prof.get());
                        }
                        else
                        {
                            auto& doc29Prof = study.Doc29Aircrafts.addProfileArrivalE(doc29Acft, Doc29Profile::Type::Points, profileName);
                            doc29ProfPts = static_cast<Doc29ProfileArrivalPoints*>(&doc29Prof);
                        }

                        // Convert from 0 Cumulative Ground Distance at touchdown to 0 cumulative ground distance at the threshold
                        cumGroundDist += fromFeet(50.0) / std::tan(toRadians(3.0)); // Assumes 3 degree descent angle

                        doc29ProfPts->addPointE(cumGroundDist, altitudeAfe, tas, thrust);
                        study.Doc29Aircrafts.updateProfile(*doc29ProfPts);
                        break;
                    }
                case OperationType::Departure:
                    {
                        Doc29ProfileDeparturePoints* doc29ProfPts = nullptr;
                        if (doc29Acft.DepartureProfiles.contains(profileName))
                        {
                            auto& doc29Prof = doc29Acft.DepartureProfiles(profileName);
                            if (doc29Prof->type() != Doc29Profile::Type::Points)
                                throw GrapeException("Departure profile with the same name but of different type already exists in the study.");

                            doc29ProfPts = static_cast<Doc29ProfileDeparturePoints*>(doc29Prof.get());
                        }
                        else
                        {
                            auto& doc29Prof = study.Doc29Aircrafts.addProfileDepartureE(doc29Acft, Doc29Profile::Type::Points, profileName);
                            doc29ProfPts = static_cast<Doc29ProfileDeparturePoints*>(&doc29Prof);
                        }

                        doc29ProfPts->addPointE(cumGroundDist, altitudeAfe, tas, thrust);
                        study.Doc29Aircrafts.updateProfile(*doc29ProfPts);
                        break;
                    }
                default: GRAPE_ASSERT(false);
                }
            }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing default fixed point profiles at row {}. {}", row + 2, err.what());
                if (m_StopOnError)
                    return false;
            }
        }
        return true;
    }

    bool AnpImport::loadArrivalProceduralSteps() const {
        auto& study = Application::study();

        const auto filePath = getFilePathString(File::DefaultApproachProceduralSteps);
        Csv csv;

        try { csv.setImport(filePath, getColumnCount(File::DefaultApproachProceduralSteps)); }
        catch (const std::exception& err)
        {
            Log::io()->error("Importing default approach procedural steps from '{}'. {}", filePath, err.what());
            return false;
        }

        for (std::size_t row = 0; row < csv.rowCount(); ++row)
        {
            try
            {
                // Aircraft
                auto doc29AcftName = csv.getCell<std::string>(row, 0);
                if (!study.Doc29Aircrafts().contains(doc29AcftName))
                    throw GrapeException(std::format("Doc29 Aircraft '{}' not found in the study.", doc29AcftName));
                auto& doc29Acft = study.Doc29Aircrafts(doc29AcftName);

                // Profile
                auto profileId = csv.getCell<std::string>(row, 1);

                // Step Number
                auto stepNumber = csv.getCell<std::string>(row, 2);

                // Step Type
                const auto stepTypeStr = csv.getCell<std::string>(row, 3);
                auto stepType = Doc29ProfileArrivalProcedural::StepType::DescendDecelerate;
                if (stepTypeStr == "Descend")
                    stepType = Doc29ProfileArrivalProcedural::StepType::DescendDecelerate;
                else if (stepTypeStr == "Descend-Decel")
                    stepType = Doc29ProfileArrivalProcedural::StepType::DescendDecelerate;
                else if (stepTypeStr == "Descend-Idle")
                    stepType = Doc29ProfileArrivalProcedural::StepType::DescendIdle;
                else if (stepTypeStr == "Level")
                    stepType = Doc29ProfileArrivalProcedural::StepType::Level;
                else if (stepTypeStr == "Level-Decel")
                    stepType = Doc29ProfileArrivalProcedural::StepType::LevelDecelerate;
                else if (stepTypeStr == "Level-Idle")
                    stepType = Doc29ProfileArrivalProcedural::StepType::LevelIdle;
                else if (stepTypeStr == "Land")
                    stepType = Doc29ProfileArrivalProcedural::StepType::DescendLand;
                else if (stepTypeStr == "Decelerate")
                    stepType = Doc29ProfileArrivalProcedural::StepType::GroundDecelerate;
                else
                    throw GrapeException(std::format("Step type '{}' not supported.", stepTypeStr));

                // Flap ID
                const auto flapId = csv.getCell<std::string>(row, 4);
                const std::string aeroCoeffs = std::string(flapId).append(" A");

                // Start Altitude
                double startAltitude = Constants::NaN;
                if (!csv.getCell<std::string>(row, 5).empty())
                {
                    try { startAltitude = fromFeet(csv.getCell<double>(row, 5)); }
                    catch (...) { throw GrapeException("Invalid start point altitude."); }
                }

                // Start CAS
                double startCas = Constants::NaN;
                if (!csv.getCell<std::string>(row, 6).empty())
                {
                    try { startCas = fromKnots(csv.getCell<double>(row, 6)); }
                    catch (...) { throw GrapeException("Invalid start calibrated airspeed."); }
                }

                // Descent Angle
                double descentAngle = Constants::NaN;
                if (!csv.getCell<std::string>(row, 7).empty())
                {
                    try { descentAngle = -1.0 * csv.getCell<double>(row, 7); }
                    catch (...) { throw GrapeException("Invalid descent angle."); }
                }

                // Touchdown Roll
                double touchdownRoll = Constants::NaN;
                if (!csv.getCell<std::string>(row, 8).empty())
                {
                    try { touchdownRoll = fromFeet(csv.getCell<double>(row, 8)); }
                    catch (...) { throw GrapeException("Invalid touchdown roll."); }
                }

                // Ground Distance
                double groundDistance = Constants::NaN;
                if (!csv.getCell<std::string>(row, 9).empty())
                {
                    try { groundDistance = fromFeet(csv.getCell<double>(row, 9)); }
                    catch (...) { throw GrapeException("Invalid ground distance."); }
                }

                // Create or get profile
                Doc29ProfileArrivalProcedural* doc29ProfProc = nullptr;
                if (doc29Acft.ArrivalProfiles.contains(profileId))
                {
                    auto& doc29Prof = doc29Acft.ArrivalProfiles(profileId);
                    if (doc29Prof->type() != Doc29Profile::Type::Procedural)
                        throw GrapeException("Arrival profile with the same name but of different type already exists in the study.");

                    doc29ProfProc = static_cast<Doc29ProfileArrivalProcedural*>(doc29Prof.get());
                }
                else
                {
                    // New profile
                    auto& doc29Prof = study.Doc29Aircrafts.addProfileArrivalE(doc29Acft, Doc29Profile::Type::Procedural, profileId);
                    doc29ProfProc = static_cast<Doc29ProfileArrivalProcedural*>(&doc29Prof);
                }

                // Detect if Descend step is actually intended for land
                bool descendAsLand = stepType == Doc29ProfileArrivalProcedural::StepType::DescendDecelerate && startAltitude <= s_MaxThresholdCrossingAltitude;

                switch (stepType)
                {
                case Doc29ProfileArrivalProcedural::StepType::DescendDecelerate:
                    {
                        if (flapId.empty())
                            throw GrapeException(std::format("Flap ID required.", stepTypeStr));

                        if (std::isnan(startAltitude))
                            throw GrapeException("Invalid start altitude.");

                        if (std::isnan(descentAngle))
                            throw GrapeException("Invalid descent angle.");

                        if (std::isnan(startCas))
                            throw GrapeException("Invalid start calibrated airspeed.");

                        if (descendAsLand)
                            doc29ProfProc->setDescendLandParametersE(descentAngle, startAltitude);
                        else
                            doc29ProfProc->addDescendDecelerateE(aeroCoeffs, startAltitude, descentAngle, startCas);
                        break;
                    }
                case Doc29ProfileArrivalProcedural::StepType::DescendIdle:
                    {
                        if (std::isnan(startAltitude))
                            throw GrapeException("Invalid start altitude.");

                        if (std::isnan(descentAngle))
                            throw GrapeException("Invalid descent angle.");

                        if (std::isnan(startCas))
                            throw GrapeException("Invalid start calibrated airspeed.");

                        doc29ProfProc->addDescendIdleE(startAltitude, descentAngle, startCas);
                        break;
                    }
                case Doc29ProfileArrivalProcedural::StepType::Level:
                    {
                        if (flapId.empty())
                            throw GrapeException(std::format("Flap ID required.", stepTypeStr));

                        if (std::isnan(groundDistance))
                            throw GrapeException("Invalid ground distance.");

                        doc29ProfProc->addLevelE(aeroCoeffs, groundDistance);
                        break;
                    }
                case Doc29ProfileArrivalProcedural::StepType::LevelDecelerate:
                    {
                        if (flapId.empty())
                            throw GrapeException(std::format("Flap ID required.", stepTypeStr));

                        if (std::isnan(groundDistance))
                            throw GrapeException("Invalid ground distance.");

                        if (std::isnan(startCas))
                            throw GrapeException("Invalid start calibrated airspeed.");

                        doc29ProfProc->addLevelDecelerateE(aeroCoeffs, groundDistance, startCas);
                        break;
                    }
                case Doc29ProfileArrivalProcedural::StepType::LevelIdle:
                    {
                        if (std::isnan(groundDistance))
                            throw GrapeException("Invalid ground distance.");

                        if (std::isnan(startCas))
                            throw GrapeException("Invalid start calibrated airspeed.");

                        doc29ProfProc->addLevelIdleE(groundDistance, startCas);
                        break;
                    }
                case Doc29ProfileArrivalProcedural::StepType::DescendLand:
                    {
                        if (flapId.empty())
                            throw GrapeException(std::format("Flap ID required.", stepTypeStr));

                        if (std::isnan(touchdownRoll))
                            throw GrapeException("Invalid touchdown roll.");

                        doc29ProfProc->setDescendLandParametersE(aeroCoeffs, touchdownRoll);
                        break;
                    }
                case Doc29ProfileArrivalProcedural::StepType::GroundDecelerate:
                    {
                        double thrustPercentage = Constants::NaN;
                        try { thrustPercentage = csv.getCell<double>(row, 10) / 100.0; }
                        catch (...) { throw GrapeException("Invalid start thrust."); }

                        if (std::isnan(groundDistance))
                            throw GrapeException("Invalid ground distance.");

                        if (std::isnan(startCas))
                            throw GrapeException("Invalid start calibrated airspeed.");

                        doc29ProfProc->addGroundDecelerateE(groundDistance, startCas, thrustPercentage);
                        break;
                    }
                default: GRAPE_ASSERT(false);
                }
                study.Doc29Aircrafts.updateProfile(*doc29ProfProc);
            }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing default approach procedural steps at row {}. {}", row + 2, err.what());
                if (m_StopOnError)
                    return false;
            }
        }
        return true;
    }

    bool AnpImport::loadDepartureProceduralSteps() const {
        auto& study = Application::study();

        const auto filePath = getFilePathString(File::DefaultDepartureProceduralSteps);
        Csv csv;

        try { csv.setImport(filePath, getColumnCount(File::DefaultDepartureProceduralSteps)); }
        catch (const std::exception& err)
        {
            Log::io()->error("Importing default departure procedural steps from '{}'. {}", filePath, err.what());
            return false;
        }

        for (std::size_t row = 0; row < csv.rowCount(); ++row)
        {
            try
            {
                // Doc29 Performance
                auto doc29AcftName = csv.getCell<std::string>(row, 0);
                if (!study.Doc29Aircrafts().contains(doc29AcftName))
                    throw GrapeException(std::format("Doc29 Aircraft '{}' not found in the study.", doc29AcftName));
                auto& doc29Acft = study.Doc29Aircrafts(doc29AcftName);

                // Profile
                const auto profileId = csv.getCell<std::string>(row, 1);

                // Stage Length
                auto stageLength = csv.getCell<std::string>(row, 2);

                // Step Number
                auto stepNumber = csv.getCell<std::string>(row, 3);

                // Add stage length to profile name
                std::string profileName = profileId;
                profileName.append(" ").append(stageLength);

                Doc29ProfileDepartureProcedural* doc29ProfProc = nullptr;
                if (doc29Acft.DepartureProfiles.contains(profileName))
                {
                    auto& doc29Prof = doc29Acft.DepartureProfiles(profileName);
                    if (doc29Prof->type() != Doc29Profile::Type::Procedural)
                        throw GrapeException(std::format("Departure profile with the same name but of different type already exists for Doc29 Aircraft '{}'.", doc29Acft.Name));

                    doc29ProfProc = static_cast<Doc29ProfileDepartureProcedural*>(doc29Prof.get());
                }
                else
                {
                    auto& doc29Prof = study.Doc29Aircrafts.addProfileDepartureE(doc29Acft, Doc29Profile::Type::Procedural, profileName);
                    doc29ProfProc = static_cast<Doc29ProfileDepartureProcedural*>(&doc29Prof);
                }

                // Step Type
                const auto stepTypeStr = csv.getCell<std::string>(row, 4);
                auto stepType = Doc29ProfileDepartureProcedural::StepType::Takeoff;
                if (stepTypeStr == "Takeoff")
                    stepType = Doc29ProfileDepartureProcedural::StepType::Takeoff;
                else if (stepTypeStr == "Climb")
                    stepType = Doc29ProfileDepartureProcedural::StepType::Climb;
                else if (stepTypeStr == "Accelerate")
                    stepType = Doc29ProfileDepartureProcedural::StepType::ClimbAccelerate;
                else
                    throw GrapeException(std::format("Step type '{}' not supported.", stepTypeStr));

                // Thrust Rating
                auto thrustRatingStr = csv.getCell<std::string>(row, 5);
                auto thrustRating = Doc29Thrust::Rating::MaximumTakeoff;
                if (thrustRatingStr == "MaxTakeoff")
                    thrustRating = Doc29Thrust::Rating::MaximumTakeoff;
                else if (thrustRatingStr == "MaxClimb")
                    thrustRating = Doc29Thrust::Rating::MaximumClimb;
                else
                    throw GrapeException(std::format("Thrust rating '{}' not supported for procedural profiles.", thrustRatingStr));

                // Aerodynamic Coefficients
                const auto flapId = csv.getCell<std::string>(row, 6);
                if (flapId.empty())
                    throw GrapeException("Flap ID can't be empty.");
                std::string aeroCoeffs = std::string(flapId).append(" D");

                // End Altitude
                double endAltitude = Constants::NaN;
                if (!csv.getCell<std::string>(row, 7).empty())
                {
                    try { endAltitude = fromFeet(csv.getCell<double>(row, 7)); }
                    catch (...) { throw GrapeException("Invalid end point altitude."); }
                }

                // Rate of Climb
                double rateOfClimb = Constants::NaN;
                if (!csv.getCell<std::string>(row, 8).empty())
                {
                    try { rateOfClimb = fromFeetPerMinute(csv.getCell<double>(row, 8)); }
                    catch (...) { throw std::invalid_argument("Invalid rate of climb."); }
                }

                // End calibrated airspeed
                double endCas = Constants::NaN;
                if (!csv.getCell<std::string>(row, 9).empty())
                {
                    try { endCas = fromKnots(csv.getCell<double>(row, 9)); }
                    catch (...) { throw GrapeException("Invalid end point calibrated airspeed."); }
                }

                // Acceleration percentage
                double accelerationPercentage = Constants::NaN;
                if (!csv.getCell<std::string>(row, 10).empty())
                {
                    try { accelerationPercentage = csv.getCell<double>(row, 10) / 100.0; }
                    catch (...) {} // Acceleration Percentage may not exist
                }

                // Change step type if acceleration percentage is given
                if (stepType == Doc29ProfileDepartureProcedural::StepType::ClimbAccelerate && !std::isnan(accelerationPercentage))
                    stepType = Doc29ProfileDepartureProcedural::StepType::ClimbAcceleratePercentage;

                switch (stepType)
                {
                case Doc29ProfileDepartureProcedural::StepType::Takeoff:
                    {
                        doc29ProfProc->setTakeoffParametersE(aeroCoeffs, 0.0);
                        break;
                    }
                case Doc29ProfileDepartureProcedural::StepType::Climb:
                    {
                        doc29ProfProc->addClimbE(aeroCoeffs, endAltitude);

                        if (thrustRating == Doc29Thrust::Rating::MaximumClimb && doc29ProfProc->thrustCutback() == 0)
                            doc29ProfProc->setThrustCutback(doc29ProfProc->size() - 1);
                        break;
                    }
                case Doc29ProfileDepartureProcedural::StepType::ClimbAccelerate:
                    {
                        doc29ProfProc->addClimbAccelerateE(aeroCoeffs, endCas, rateOfClimb);
                        if (thrustRating == Doc29Thrust::Rating::MaximumClimb && doc29ProfProc->thrustCutback() == 0)
                            doc29ProfProc->setThrustCutback(doc29ProfProc->size() - 1);
                        break;
                    }
                case Doc29ProfileDepartureProcedural::StepType::ClimbAcceleratePercentage:
                    {
                        doc29ProfProc->addClimbAcceleratePercentageE(aeroCoeffs, endCas, accelerationPercentage);
                        if (thrustRating == Doc29Thrust::Rating::MaximumClimb && doc29ProfProc->thrustCutback() == 0)
                            doc29ProfProc->setThrustCutback(doc29ProfProc->size() - 1);
                        break;
                    }
                default: GRAPE_ASSERT(false);
                }

                study.Doc29Aircrafts.updateProfile(*doc29ProfProc);
            }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing default departure procedural steps at row {}. {}", row + 2, err.what());
                if (m_StopOnError)
                    return false;
            }
        }
        return true;
    }

    namespace {
        void addNpdData(Doc29Noise& Doc29Ns, OperationType OpType, NoiseSingleMetric NoiseMetric, double Thrust, const NpdData::PowerNoiseLevelsArray& NoiseLevels);
    }

    bool AnpImport::loadNpdData() const {
        auto& study = Application::study();

        const auto filePath = getFilePathString(File::NpdData);
        Csv csv;

        try { csv.setImport(filePath, getColumnCount(File::NpdData)); }
        catch (const std::exception& err)
        {
            Log::io()->error("Importing NPD data from '{}'. {}", filePath, err.what());
            return false;
        }

        for (std::size_t row = 0; row < csv.rowCount(); ++row)
        {
            try
            {
                // NPD ID
                auto npdId = csv.getCell<std::string>(row, 0);
                if (!m_PercentagePowerParameters.contains(npdId) && !study.Doc29Noises().contains(npdId))
                    throw GrapeException(std::format("No aircraft with NPD ID '{}' were imported.", npdId));

                // Noise Metric
                const auto noiseMetricStr = csv.getCell<std::string>(row, 1);
                auto noiseMetric = NoiseSingleMetric::Lamax;
                if (noiseMetricStr == "LAmax")
                    noiseMetric = NoiseSingleMetric::Lamax;
                else if (noiseMetricStr == "SEL")
                    noiseMetric = NoiseSingleMetric::Sel;
                else if (noiseMetricStr == "EPNL" || noiseMetricStr == "PNLTM")
                    continue; // Silently skip EPNL and PNLTM data
                else
                    throw GrapeException(std::format("Noise metric '{}' is not supported.", noiseMetricStr));

                // Operation Type
                const auto opTypeStr = csv.getCell<std::string>(row, 2);
                auto opType = OperationType::Arrival;
                if (opTypeStr == "A")
                    opType = OperationType::Arrival;
                else if (opTypeStr == "D")
                    opType = OperationType::Departure;
                else
                    throw GrapeException(std::format("Operation type '{}' is not supported.", opTypeStr));

                double powerParam = Constants::NaN;
                try { powerParam = csv.getCell<double>(row, 3); }
                catch (...) { throw GrapeException("Invalid power parameter."); }

                NpdData::PowerNoiseLevelsArray noiseLevels;
                for (std::size_t i = 0; i < noiseLevels.size(); ++i)
                {
                    try { noiseLevels.at(i) = csv.getCell<double>(row, i + 4); }
                    catch (...) { throw GrapeException(std::format("Invalid noise level at column '{}'", i + 4 + 1)); }
                }

                if (m_PercentagePowerParameters.contains(npdId))
                {
                    for (const auto& nsPowerPercentageParams : m_PercentagePowerParameters.at(npdId))
                    {
                        const double thrust = powerParam / 100.0 * nsPowerPercentageParams.MaximumStaticThrust;
                        auto& doc29Ns = study.Doc29Noises(nsPowerPercentageParams.Name);
                        addNpdData(doc29Ns, opType, noiseMetric, thrust, noiseLevels);
                    }
                }
                else
                {
                    auto& doc29Ns = study.Doc29Noises(npdId);
                    const double thrust = fromPoundsOfForce(powerParam);
                    addNpdData(doc29Ns, opType, noiseMetric, thrust, noiseLevels);
                }
            }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing NPD data at row {}. {}", row + 2, err.what());
                if (m_StopOnError)
                    return false;
            }
        }
        return true;
    }

    namespace {
        void addNpdData(Doc29Noise& Doc29Ns, OperationType OpType, NoiseSingleMetric NoiseMetric, double Thrust, const NpdData::PowerNoiseLevelsArray& NoiseLevels) {
            const auto& study = Application::study();

            switch (OpType)
            {
            case OperationType::Arrival:
                {
                    switch (NoiseMetric)
                    {
                    case NoiseSingleMetric::Lamax:
                        {
                            Doc29Ns.ArrivalLamax.addThrustE(Thrust, NoiseLevels);
                            study.Doc29Noises.updateMetric(Doc29Ns, OperationType::Arrival, NoiseSingleMetric::Lamax);
                            break;
                        }
                    case NoiseSingleMetric::Sel:
                        {
                            Doc29Ns.ArrivalSel.addThrustE(Thrust, NoiseLevels);
                            study.Doc29Noises.updateMetric(Doc29Ns, OperationType::Arrival, NoiseSingleMetric::Sel);
                            break;
                        }
                    default: GRAPE_ASSERT(false);
                    }
                    break;
                }
            case OperationType::Departure:
                {
                    switch (NoiseMetric)
                    {
                    case NoiseSingleMetric::Lamax:
                        {
                            Doc29Ns.DepartureLamax.addThrustE(Thrust, NoiseLevels);
                            study.Doc29Noises.updateMetric(Doc29Ns, OperationType::Departure, NoiseSingleMetric::Lamax);
                            break;
                        }
                    case NoiseSingleMetric::Sel:
                        {
                            Doc29Ns.DepartureSel.addThrustE(Thrust, NoiseLevels);
                            study.Doc29Noises.updateMetric(Doc29Ns, OperationType::Departure, NoiseSingleMetric::Sel);
                            break;
                        }
                    default: GRAPE_ASSERT(false);
                    }
                    break;
                }
            default: GRAPE_ASSERT(false);
            }
        }
    }
}
