// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "CsvImport.h"

#include "Application.h"
#include "Csv.h"

namespace GRAPE::IO::CSV {
    namespace {
        struct CsvImport {
            CsvImport(const std::string& CsvPath, std::string_view Description, std::size_t ColumnCount);
            ~CsvImport();

            [[nodiscard]] bool valid() const { return m_Valid; }
            Csv CsvImp;
            std::size_t ErrorCount = 0;
        private:
            std::string m_Path;
            std::string m_Description;
            bool m_Valid = false;
        };
    }

    CsvImport::CsvImport(const std::string& CsvPath, std::string_view Description, std::size_t ColumnCount) : m_Path(CsvPath), m_Description(Description) {
        Application::study().db().beginTransaction();

        try { CsvImp.setImport(CsvPath, ColumnCount); }
        catch (const std::exception& err)
        {
            Log::io()->error("Importing {} from '{}'. {}", m_Description, m_Path, err.what());
            return;
        }

        m_Valid = true;
    }

    CsvImport::~CsvImport() {
        if (!ErrorCount)
            Log::io()->info("Successfully imported all {} from '{}'.", m_Description, m_Path);
        else
            Log::io()->warn("Importing {} from '{}'. {} errors occurred (see logs below).", m_Description, m_Path, ErrorCount);

        Application::study().db().commitTransaction();
    }

    void importDoc29Performance(const std::string& CsvPath) {
        auto& study = Application::study();
        const auto& set = Application::settings();

        CsvImport csvImp(CsvPath, "Doc29 Aircraft", 4);
        if (!csvImp.valid())
            return;
        auto& csv = csvImp.CsvImp;
        const auto columnNames = csv.columnNames();

        for (std::size_t row = 0; row < csv.rowCount(); row++)
        {
            Doc29Aircraft* newDoc29Acft = nullptr;
            try
            {
                auto& doc29Acft = study.Doc29Aircrafts.addPerformanceE(csv.getCell<std::string>(row, 0));
                newDoc29Acft = &doc29Acft;

                try { doc29Acft.setMaximumSeaLevelStaticThrust(set.ThrustUnits.toSi(csv.getCell<double>(row, 1), columnNames.at(1))); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid maximum sea level static thrust."); }

                const auto doc29ThrStr = csv.getCell<std::string>(row, 2);
                if (doc29ThrStr.empty())
                    throw GrapeException("Empty thrust type not allowed.");
                if (!Doc29Thrust::Types.contains(doc29ThrStr))
                    throw GrapeException(std::format("Invalid thrust type '{}'.", doc29ThrStr));
                doc29Acft.setThrustType(Doc29Thrust::Types.fromString(doc29ThrStr));

                try { doc29Acft.setEngineBreakpointTemperature(set.TemperatureUnits.toSi(csv.getCell<double>(row, 3), columnNames.at(3))); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid engine breakpoint temperature."); }
            }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing Doc29 Aircraft at row {}. {}", row + 2, err.what());
                if (newDoc29Acft)
                    study.Doc29Aircrafts.erasePerformance(*newDoc29Acft);
                ++csvImp.ErrorCount;
            }
        }
    }

    void importDoc29PerformanceAerodynamicCoefficients(const std::string& CsvPath) {
        auto& study = Application::study();
        const auto& set = Application::settings();

        CsvImport csvImp(CsvPath, "Doc29 aerodynamic coefficients", 7);
        if (!csvImp.valid())
            return;
        auto& csv = csvImp.CsvImp;
        const auto columnNames = csv.columnNames();

        for (std::size_t row = 0; row < csv.rowCount(); row++)
        {
            Doc29Aircraft* updateDoc29Acft = nullptr;
            try
            {
                const auto doc29AcftName = csv.getCell<std::string>(row, 0);
                if (doc29AcftName.empty())
                    throw GrapeException("Empty Doc29 Aircraft name not allowed.");
                if (!study.Doc29Aircrafts().contains(doc29AcftName))
                    throw GrapeException(std::format("Doc29 Aircraft '{}' does not exist in this study.", doc29AcftName));
                auto& doc29Acft = study.Doc29Aircrafts()(doc29AcftName);

                const auto aeroCoeffName = csv.getCell<std::string>(row, 1);
                if (aeroCoeffName.empty())
                    throw GrapeException("Empty aerodynamic coefficients name not allowed.");
                if (doc29Acft.AerodynamicCoefficients.contains(aeroCoeffName))
                    throw GrapeException(std::format("Aerodynamic coefficients '{}' already exist in Doc Performance '{}'.", aeroCoeffName, doc29AcftName));

                const auto aeroCoeffTypeStr = csv.getCell<std::string>(row, 2);
                if (!Doc29AerodynamicCoefficients::Types.contains(aeroCoeffTypeStr))
                    throw GrapeException(std::format("Invalid aerodynamic coefficient type '{}'.", aeroCoeffTypeStr));

                auto [aeroCoeffs, added] = doc29Acft.AerodynamicCoefficients.add(aeroCoeffName, aeroCoeffName);
                GRAPE_ASSERT(added);
                updateDoc29Acft = &doc29Acft;
                aeroCoeffs.CoefficientType = Doc29AerodynamicCoefficients::Types.fromString(aeroCoeffTypeStr);

                try { aeroCoeffs.setRCoeffE(csv.getCell<double>(row, 3)); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid R coefficient."); }

                if (aeroCoeffs.CoefficientType == Doc29AerodynamicCoefficients::Type::Takeoff)
                {
                    try { aeroCoeffs.setBCoeffE(set.Doc29AeroBUnits.toSi(csv.getCell<double>(row, 4), columnNames.at(4))); }
                    catch (const GrapeException&) { throw; }
                    catch (...) { throw std::invalid_argument("Invalid B coefficient."); }

                    try { aeroCoeffs.setCCoeffE(set.Doc29AeroCDUnits.toSi(csv.getCell<double>(row, 5), columnNames.at(5))); }
                    catch (const GrapeException&) { throw; }
                    catch (...) { throw std::invalid_argument("Invalid C coefficient."); }
                }

                if (aeroCoeffs.CoefficientType == Doc29AerodynamicCoefficients::Type::Land)
                {
                    try { aeroCoeffs.setDCoeffE(set.Doc29AeroCDUnits.toSi(csv.getCell<double>(row, 6), columnNames.at(6))); }
                    catch (const GrapeException&) { throw; }
                    catch (...) { throw std::invalid_argument("Invalid D coefficient."); }
                }

                study.Doc29Aircrafts.updateAerodynamicCoefficients(doc29Acft);
            }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing Doc29 aerodynamic coefficients at row {}. {}", row + 2, err.what());
                if (updateDoc29Acft)
                    study.Doc29Aircrafts.updateAerodynamicCoefficients(*updateDoc29Acft);
                ++csvImp.ErrorCount;
            }
        }
    }

    namespace {
        struct ThrustRatingInserter : Doc29ThrustVisitor {
            ThrustRatingInserter(Doc29Thrust& Doc29Thr, Doc29Thrust::Rating Rating, double E, double F, double Ga, double Gb, double H) : m_Rating(Rating), m_E(E), m_F(F), m_Ga(Ga), m_Gb(Gb), m_H(H) { Doc29Thr.accept(*this); }
            void visitDoc29ThrustRating(Doc29ThrustRating& Doc29Thr) override { Doc29Thr.Coeffs.add(m_Rating, m_E, m_F, m_Ga, m_Gb, m_H); }
        private:
            Doc29Thrust::Rating m_Rating;
            double m_E;
            double m_F;
            double m_Ga;
            double m_Gb;
            double m_H;
        };
    }
    void importDoc29PerformanceThrustRatings(const std::string& CsvPath) {
        auto& study = Application::study();
        const auto& set = Application::settings();

        CsvImport csvImp(CsvPath, "Doc29 thrust ratings", 7);
        if (!csvImp.valid())
            return;
        auto& csv = csvImp.CsvImp;
        const auto columnNames = csv.columnNames();

        for (std::size_t row = 0; row < csv.rowCount(); row++)
        {
            Doc29Aircraft* updatedDoc29Acft = nullptr;
            try
            {
                const auto doc29AcftName = csv.getCell<std::string>(row, 0);
                if (doc29AcftName.empty())
                    throw GrapeException("Empty Doc29 Aircraft name not allowed.");
                if (!study.Doc29Aircrafts().contains(doc29AcftName))
                    throw GrapeException(std::format("Doc29 Aircraft '{}' does not exist in this study.", doc29AcftName));
                auto& doc29Acft = study.Doc29Aircrafts()(doc29AcftName);
                updatedDoc29Acft = &doc29Acft;

                if (doc29Acft.thrust().type() != Doc29Thrust::Type::Rating)
                {
                    Log::io()->warn("Importing thrust rating coefficients for Doc29 Aircraft '{}' with thrust type '{}'. Thrust type will be changed to thrust rating.", doc29AcftName, Doc29Thrust::Types.toString(doc29Acft.thrust().type()));
                    doc29Acft.setThrustType(Doc29Thrust::Type::Rating);
                }

                const auto thrustRatingStr = csv.getCell<std::string>(row, 1);
                if (!Doc29Thrust::Ratings.contains(thrustRatingStr))
                    throw GrapeException(std::format("Invalid thrust rating '{}'.", thrustRatingStr));
                const auto thrustRating = Doc29Thrust::Ratings.fromString(thrustRatingStr);
                if (doc29Acft.thrust().isRatingSet(thrustRating))
                    throw GrapeException(std::format("Thrust rating {} already exists in Doc29 Aircraft '{}'.", thrustRatingStr, doc29AcftName));

                double e = Constants::NaN;
                double f = Constants::NaN;
                double ga = Constants::NaN;
                double gb = Constants::NaN;
                double h = Constants::NaN;

                try { e = set.ThrustUnits.toSi(csv.getCell<double>(row, 2), columnNames.at(2)); }
                catch (...) { throw std::invalid_argument("Invalid E coefficient."); }

                try { f = set.Doc29ThrustFUnits.toSi(csv.getCell<double>(row, 3), columnNames.at(3)); }
                catch (...) { throw std::invalid_argument("Invalid F coefficient."); }

                try { ga = set.Doc29ThrustGaUnits.toSi(csv.getCell<double>(row, 4), columnNames.at(4)); }
                catch (...) { throw std::invalid_argument("Invalid Ga coefficient."); }

                try { gb = set.Doc29ThrustGbUnits.toSi(csv.getCell<double>(row, 5), columnNames.at(5)); }
                catch (...) { throw std::invalid_argument("Invalid Gb coefficient."); }

                try { h = set.Doc29ThrustHUnits.toSi(csv.getCell<double>(row, 6), columnNames.at(6)); }
                catch (...) { throw std::invalid_argument("Invalid H coefficient."); }

                ThrustRatingInserter(doc29Acft.thrust(), thrustRating, e, f, ga, gb, h);
                study.Doc29Aircrafts.updateThrust(doc29Acft);
            }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing Doc29 thrust ratings at row {}. {}", row + 2, err.what());
                if (updatedDoc29Acft)
                    study.Doc29Aircrafts.updateThrust(*updatedDoc29Acft);
                ++csvImp.ErrorCount;
            }
        }
    }

    namespace {
        struct ThrustRatingPropellerInserter : Doc29ThrustVisitor {
            ThrustRatingPropellerInserter(Doc29Thrust& Doc29Thr, Doc29Thrust::Rating Rating, double E, double Pp) : m_Rating(Rating), m_E(E), m_Pp(Pp) { Doc29Thr.accept(*this); }
            void visitDoc29ThrustPropeller(Doc29ThrustRatingPropeller& Doc29Thr) override { Doc29Thr.addCoefficients(m_Rating, m_E, m_Pp); }
        private:
            Doc29Thrust::Rating m_Rating;
            double m_E;
            double m_Pp;
        };
    }
    void importDoc29PerformanceThrustRatingsPropeller(const std::string& CsvPath) {
        auto& study = Application::study();
        const auto& set = Application::settings();

        CsvImport csvImp(CsvPath, "Doc29 thrust ratings propeller", 4);
        if (!csvImp.valid())
            return;
        auto& csv = csvImp.CsvImp;

        for (std::size_t row = 0; row < csv.rowCount(); row++)
        {
            Doc29Aircraft* updatedDoc29Acft = nullptr;
            try
            {
                const auto doc29AcftName = csv.getCell<std::string>(row, 0);
                if (doc29AcftName.empty())
                    throw GrapeException("Empty Doc29 Aircraft name not allowed.");
                if (!study.Doc29Aircrafts().contains(doc29AcftName))
                    throw GrapeException(std::format("Doc29 Aircraft '{}' does not exist in this study.", doc29AcftName));
                auto& doc29Acft = study.Doc29Aircrafts()(doc29AcftName);
                updatedDoc29Acft = &doc29Acft;

                if (doc29Acft.thrust().type() != Doc29Thrust::Type::RatingPropeller)
                {
                    Log::io()->warn("Importing thrust rating propeller coefficients for Doc29 Aircraft '{}' with thrust type '{}'. Thrust type will be changed to thrust rating propeller.");
                    doc29Acft.setThrustType(Doc29Thrust::Type::RatingPropeller);
                }

                const auto thrustRatingStr = csv.getCell<std::string>(row, 1);
                if (!Doc29Thrust::Ratings.contains(thrustRatingStr))
                    throw GrapeException(std::format("Invalid thrust rating '{}'.", thrustRatingStr));
                const auto thrustRating = Doc29Thrust::Ratings.fromString(thrustRatingStr);
                if (doc29Acft.thrust().isRatingSet(thrustRating))
                    throw GrapeException(std::format("Thrust rating {} already exists in Doc29 Aircraft '{}'.", thrustRatingStr, doc29AcftName));

                double eff = Constants::NaN;
                double pp = Constants::NaN;

                try { eff = csv.getCell<double>(row, 2); }
                catch (...) { throw std::invalid_argument("Invalid propeller efficiency."); }

                try { pp = set.PowerUnits.toSi(csv.getCell<double>(row, 3), csv.columnName(3)); }
                catch (...) { throw std::invalid_argument("Invalid propeller power."); }

                ThrustRatingPropellerInserter(doc29Acft.thrust(), thrustRating, eff, pp);
                study.Doc29Aircrafts.updateThrust(doc29Acft);
            }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing Doc29 thrust ratings propeller at row {}. {}", row + 2, err.what());
                if (updatedDoc29Acft)
                    study.Doc29Aircrafts.updateThrust(*updatedDoc29Acft);
                ++csvImp.ErrorCount;
            }
        }
    }

    namespace {
        class PointProfileInserter : Doc29ProfileVisitor {
        public:
            PointProfileInserter(Doc29Profile& Doc29Prof, double CumDist, double Alt, double Tas, double Thrust) : m_CumDist(CumDist), m_Alt(Alt), m_Tas(Tas), m_Thrust(Thrust) { Doc29Prof.accept(*this); }

            void visitDoc29ProfileArrivalPoints(Doc29ProfileArrivalPoints& Profile) override { Profile.addPointE(m_CumDist, m_Alt, m_Tas, m_Thrust); }
            void visitDoc29ProfileDeparturePoints(Doc29ProfileDeparturePoints& Profile) override { Profile.addPointE(m_CumDist, m_Alt, m_Tas, m_Thrust); }
        private:
            double m_CumDist;
            double m_Alt;
            double m_Tas;
            double m_Thrust;
        };
    }
    void importDoc29PerformanceProfilesPoints(const std::string& CsvPath) {
        auto& study = Application::study();
        const auto& set = Application::settings();

        CsvImport csvImp(CsvPath, "Doc29 point profiles", 7);
        if (!csvImp.valid())
            return;
        auto& csv = csvImp.CsvImp;
        const auto columnNames = csv.columnNames();

        std::vector<Doc29ProfileArrival*> addedArrivalProfiles;
        std::vector<Doc29ProfileDeparture*> addedDepartureProfiles;

        for (std::size_t row = 0; row < csv.rowCount(); row++)
        {
            Doc29Profile* updateProf = nullptr;
            try
            {
                const auto doc29AcftName = csv.getCell<std::string>(row, 0);
                if (doc29AcftName.empty())
                    throw GrapeException("Empty Doc29 Aircraft name not allowed.");
                if (!study.Doc29Aircrafts().contains(doc29AcftName))
                    throw GrapeException(std::format("Doc29 Aircraft '{}' does not exist in this study.", doc29AcftName));
                auto& doc29Acft = study.Doc29Aircrafts()(doc29AcftName);

                const auto opTypeStr = csv.getCell<std::string>(row, 1);
                if (!OperationTypes.contains(opTypeStr))
                    throw GrapeException(std::format("Invalid operation type '{}'.", opTypeStr));
                const OperationType opType = OperationTypes.fromString(opTypeStr);

                const auto profName = csv.getCell<std::string>(row, 2);

                double cumDist = Constants::NaN;
                double alt = Constants::NaN;
                double tas = Constants::NaN;
                double thrust = Constants::NaN;

                try { cumDist = set.DistanceUnits.toSi(csv.getCell<double>(row, 3), columnNames.at(3)); }
                catch (...) { throw std::invalid_argument("Invalid cumulative ground distance."); }

                try { alt = set.AltitudeUnits.toSi(csv.getCell<double>(row, 4), columnNames.at(4)); }
                catch (...) { throw std::invalid_argument("Invalid altitude ATE."); }

                try { tas = set.SpeedUnits.toSi(csv.getCell<double>(row, 5), columnNames.at(5)); }
                catch (...) { throw std::invalid_argument("Invalid true airspeed."); }

                try { thrust = set.ThrustUnits.toSi(csv.getCell<double>(row, 6), columnNames.at(6)); }
                catch (...) { throw std::invalid_argument("Invalid corrected net thrust per engine."); }

                switch (opType)
                {
                case OperationType::Arrival:
                    {
                        if (doc29Acft.ArrivalProfiles.contains(profName))
                        {
                            if (std::ranges::find(addedArrivalProfiles, doc29Acft.ArrivalProfiles(profName).get()) == addedArrivalProfiles.end())
                                throw GrapeException(std::format("Arrival profile '{}' already exists in Doc29 Aircraft '{}'.", profName, doc29AcftName));
                        }
                        else
                        {
                            auto& addedProfile = study.Doc29Aircrafts.addProfileArrivalE(doc29Acft, Doc29Profile::Type::Points, profName);
                            addedArrivalProfiles.emplace_back(&addedProfile);
                        }

                        updateProf = doc29Acft.ArrivalProfiles(profName).get();
                        auto& prof = *updateProf;
                        PointProfileInserter profIns(prof, cumDist, alt, tas, thrust);
                        study.Doc29Aircrafts.updateProfile(prof);
                        break;
                    }
                case OperationType::Departure:
                    {
                        if (doc29Acft.DepartureProfiles.contains(profName))
                        {
                            if (std::ranges::find(addedDepartureProfiles, doc29Acft.DepartureProfiles(profName).get()) == addedDepartureProfiles.end())
                                throw GrapeException(std::format("Departure profile '{}' already exists in Doc29 Aircraft '{}'.", profName, doc29AcftName));
                        }
                        else
                        {
                            auto& addedProfile = study.Doc29Aircrafts.addProfileDepartureE(doc29Acft, Doc29Profile::Type::Points, profName);
                            addedDepartureProfiles.emplace_back(&addedProfile);
                        }

                        updateProf = doc29Acft.DepartureProfiles(profName).get();
                        auto& prof = *updateProf;
                        PointProfileInserter profIns(prof, cumDist, alt, tas, thrust);
                        study.Doc29Aircrafts.updateProfile(prof);
                        break;
                    }
                default: GRAPE_ASSERT(false); break;
                }
            }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing Doc29 point profiles at row {}. {}", row + 2, err.what());
                if (updateProf)
                    study.Doc29Aircrafts.updateProfile(*updateProf);
                ++csvImp.ErrorCount;
            }
        }
    }

    namespace {
        struct ArrivalProceduralParams {
            Doc29ProfileArrivalProcedural::StepType StepType = Doc29ProfileArrivalProcedural::StepType::DescendDecelerate;
            std::string Coeffs;
            double StartAlt = Constants::NaN;
            double DescentAngle = Constants::NaN;
            double StartCas = Constants::NaN;
            double GroundDistance = Constants::NaN;
            double LandDescentAngle = Constants::NaN;
            double LandThresholdAlt = Constants::NaN;
            double LandTouchdownRoll = Constants::NaN;
            double GroundGroundDistance = Constants::NaN;
            double GroundStartCas = Constants::NaN;
            double GroundStartThrust = Constants::NaN;
        };

        class ArrivalProceduralInserter : Doc29ProfileArrivalVisitor {
        public:
            ArrivalProceduralInserter(Doc29ProfileArrival& Doc29Prof, const ArrivalProceduralParams& Params) : m_Params(Params) { Doc29Prof.accept(*this); }

            void visitDoc29ProfileArrivalProcedural(Doc29ProfileArrivalProcedural& Doc29Prof) override {
                switch (m_Params.StepType)
                {
                case Doc29ProfileArrivalProcedural::StepType::DescendDecelerate: Doc29Prof.addDescendDecelerateE(m_Params.Coeffs, m_Params.StartAlt, m_Params.DescentAngle, m_Params.StartCas); break;
                case Doc29ProfileArrivalProcedural::StepType::DescendIdle: Doc29Prof.addDescendIdleE(m_Params.StartAlt, m_Params.DescentAngle, m_Params.StartCas); break;
                case Doc29ProfileArrivalProcedural::StepType::Level: Doc29Prof.addLevelE(m_Params.Coeffs, m_Params.GroundDistance); break;
                case Doc29ProfileArrivalProcedural::StepType::LevelDecelerate: Doc29Prof.addLevelDecelerateE(m_Params.Coeffs, m_Params.GroundDistance, m_Params.StartCas); break;
                case Doc29ProfileArrivalProcedural::StepType::LevelIdle: Doc29Prof.addLevelIdleE(m_Params.GroundDistance, m_Params.StartCas); break;
                case Doc29ProfileArrivalProcedural::StepType::DescendLand: Doc29Prof.setDescendLandParametersE(m_Params.Coeffs, m_Params.LandDescentAngle, m_Params.LandThresholdAlt, m_Params.LandTouchdownRoll); break;
                case Doc29ProfileArrivalProcedural::StepType::GroundDecelerate: Doc29Prof.addGroundDecelerateE(m_Params.GroundGroundDistance, m_Params.GroundStartCas, m_Params.GroundStartThrust); break;
                default: GRAPE_ASSERT(false); break;
                }
            }
        private:
            ArrivalProceduralParams m_Params;
        };
    }
    void importDoc29PerformanceProfilesArrivalSteps(const std::string& CsvPath) {
        auto& study = Application::study();
        const auto& set = Application::settings();

        CsvImport csvImp(CsvPath, "Doc29 arrival procedural profiles", 14);
        if (!csvImp.valid())
            return;
        auto& csv = csvImp.CsvImp;
        const auto columnNames = csv.columnNames();

        std::vector<Doc29ProfileArrival*> addedProfiles;

        for (std::size_t row = 0; row < csv.rowCount(); row++)
        {
            Doc29ProfileArrival* updateProf = nullptr;
            try
            {
                const auto doc29AcftName = csv.getCell<std::string>(row, 0);
                if (doc29AcftName.empty())
                    throw GrapeException("Empty Doc29 Aircraft name not allowed.");
                if (!study.Doc29Aircrafts().contains(doc29AcftName))
                    throw GrapeException(std::format("Doc29 Aircraft '{}' does not exist in this study.", doc29AcftName));
                auto& doc29Acft = study.Doc29Aircrafts()(doc29AcftName);

                const auto profName = csv.getCell<std::string>(row, 1);

                ArrivalProceduralParams params;

                const auto stepTypeStr = csv.getCell<std::string>(row, 2);
                if (!Doc29ProfileArrivalProcedural::StepTypes.contains(stepTypeStr))
                    throw GrapeException(std::format("Invalid arrival step type '{}'.", stepTypeStr));
                params.StepType = Doc29ProfileArrivalProcedural::StepTypes.fromString(stepTypeStr);

                switch (params.StepType)
                {
                case Doc29ProfileArrivalProcedural::StepType::DescendDecelerate:
                    {
                        params.Coeffs = csv.getCell<std::string>(row, 3);
                        if (params.Coeffs.empty())
                            throw GrapeException(std::format("Aerodynamic coefficients name can't be empty for step type {}.", stepTypeStr));

                        try { params.StartAlt = set.AltitudeUnits.toSi(csv.getCell<double>(row, 4), columnNames.at(4)); }
                        catch (...) { throw std::invalid_argument(std::format("Invalid start altitude ATE for step type {}.", stepTypeStr)); }

                        try { params.DescentAngle = csv.getCell<double>(row, 5); }
                        catch (...) { throw std::invalid_argument(std::format("Invalid descent angle for step type {}.", stepTypeStr)); }

                        try { params.StartCas = set.SpeedUnits.toSi(csv.getCell<double>(row, 6), columnNames.at(6)); }
                        catch (...) { throw std::invalid_argument(std::format("Invalid start calibrated airspeed for step type {}.", stepTypeStr)); }

                        break;
                    }
                case Doc29ProfileArrivalProcedural::StepType::DescendIdle:
                    {
                        try { params.StartAlt = set.AltitudeUnits.toSi(csv.getCell<double>(row, 4), columnNames.at(4)); }
                        catch (...) { throw std::invalid_argument(std::format("Invalid start altitude ATE for step type {}.", stepTypeStr)); }

                        try { params.DescentAngle = csv.getCell<double>(row, 5); }
                        catch (...) { throw std::invalid_argument(std::format("Invalid descent angle for step type {}.", stepTypeStr)); }

                        try { params.StartCas = set.SpeedUnits.toSi(csv.getCell<double>(row, 6), columnNames.at(6)); }
                        catch (...) { throw std::invalid_argument(std::format("Invalid start calibrated airspeed for step type {}.", stepTypeStr)); }

                        break;
                    }
                case Doc29ProfileArrivalProcedural::StepType::Level:
                    {
                        params.Coeffs = csv.getCell<std::string>(row, 3);
                        if (params.Coeffs.empty())
                            throw GrapeException(std::format("Aerodynamic coefficients name can't be empty for step type {}.", stepTypeStr));

                        try { params.GroundDistance = set.DistanceUnits.toSi(csv.getCell<double>(row, 7), columnNames.at(7)); }
                        catch (...) { throw std::invalid_argument(std::format("Invalid ground distance for step type {}.", stepTypeStr)); }

                        break;
                    }
                case Doc29ProfileArrivalProcedural::StepType::LevelDecelerate:
                    {
                        params.Coeffs = csv.getCell<std::string>(row, 3);
                        if (params.Coeffs.empty())
                            throw GrapeException(std::format("Aerodynamic coefficients name can't be empty for step type {}.", stepTypeStr));

                        try { params.StartCas = set.SpeedUnits.toSi(csv.getCell<double>(row, 6), columnNames.at(6)); }
                        catch (...) { throw std::invalid_argument(std::format("Invalid start calibrated airspeed for step type {}.", stepTypeStr)); }

                        try { params.GroundDistance = set.DistanceUnits.toSi(csv.getCell<double>(row, 7), columnNames.at(7)); }
                        catch (...) { throw std::invalid_argument(std::format("Invalid ground distance for step type {}.", stepTypeStr)); }

                        break;
                    }
                case Doc29ProfileArrivalProcedural::StepType::LevelIdle:
                    {
                        try { params.StartCas = set.SpeedUnits.toSi(csv.getCell<double>(row, 6), columnNames.at(6)); }
                        catch (...) { throw std::invalid_argument(std::format("Invalid start calibrated airspeed for step type {}.", stepTypeStr)); }

                        try { params.GroundDistance = set.DistanceUnits.toSi(csv.getCell<double>(row, 7), columnNames.at(7)); }
                        catch (...) { throw std::invalid_argument(std::format("Invalid ground distance for step type {}.", stepTypeStr)); }

                        break;
                    }
                case Doc29ProfileArrivalProcedural::StepType::DescendLand:
                    {
                        params.Coeffs = csv.getCell<std::string>(row, 3);
                        if (params.Coeffs.empty())
                            throw GrapeException(std::format("Aerodynamic coefficients name can't be empty for step type {}.", stepTypeStr));

                        try { params.LandDescentAngle = csv.getCell<double>(row, 8); }
                        catch (...) { throw std::invalid_argument(std::format("Invalid descent angle for step type {}.", stepTypeStr)); }

                        try { params.LandThresholdAlt = set.AltitudeUnits.toSi(csv.getCell<double>(row, 9), columnNames.at(9)); }
                        catch (...) { throw std::invalid_argument(std::format("Invalid threshold crossing altitude ATE for step type {}.", stepTypeStr)); }

                        try { params.LandTouchdownRoll = set.DistanceUnits.toSi(csv.getCell<double>(row, 10), columnNames.at(10)); }
                        catch (...) { throw std::invalid_argument(std::format("Invalid touchdown roll for step type {}.", stepTypeStr)); }

                        break;
                    }
                case Doc29ProfileArrivalProcedural::StepType::GroundDecelerate:
                    {
                        try { params.GroundGroundDistance = set.DistanceUnits.toSi(csv.getCell<double>(row, 11), columnNames.at(11)); }
                        catch (...) { throw std::invalid_argument(std::format("Invalid ground distance for step type {}.", stepTypeStr)); }

                        try { params.GroundStartCas = set.SpeedUnits.toSi(csv.getCell<double>(row, 12), columnNames.at(12)); }
                        catch (...) { throw std::invalid_argument(std::format("Invalid start calibrated airspeed for step type {}.", stepTypeStr)); }

                        try { params.GroundStartThrust = csv.getCell<double>(row, 13); }
                        catch (...) { throw std::invalid_argument(std::format("Invalid ground thrust percentage for step type {}.", stepTypeStr)); }

                        break;
                    }
                default: GRAPE_ASSERT(false); break;
                }

                if (doc29Acft.ArrivalProfiles.contains(profName))
                {
                    if (std::ranges::find(addedProfiles, doc29Acft.ArrivalProfiles(profName).get()) == addedProfiles.end())
                        throw GrapeException(std::format("Arrival profile '{}' already exists in Doc29 Aircraft '{}'.", profName, doc29AcftName));
                }
                else
                {
                    auto& addedProfile = study.Doc29Aircrafts.addProfileArrivalE(doc29Acft, Doc29Profile::Type::Procedural, profName);
                    addedProfiles.emplace_back(&addedProfile);
                }

                updateProf = doc29Acft.ArrivalProfiles(profName).get();
                auto& prof = *updateProf;
                ArrivalProceduralInserter profIns(prof, params);
                study.Doc29Aircrafts.updateProfile(prof);
            }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing Doc29 arrival procedural profiles at row {}. {}", row + 2, err.what());
                if (updateProf)
                    study.Doc29Aircrafts.updateProfile(*updateProf);
                ++csvImp.ErrorCount;
            }
        }
    }

    namespace {
        struct DepartureProceduralParams {
            Doc29ProfileDepartureProcedural::StepType StepType = Doc29ProfileDepartureProcedural::StepType::Takeoff;
            bool ThrustCutback = false;
            std::string Coeffs;
            double EndAltitude = Constants::NaN;
            double EndCas = Constants::NaN;
            double ClimbRate = Constants::NaN;
            double ClimbPercentage = Constants::NaN;
            double TakeoffInitialCas = Constants::NaN;
        };

        class DepartureProceduralInserter : Doc29ProfileDepartureVisitor {
        public:
            DepartureProceduralInserter(Doc29ProfileDeparture& Doc29Prof, const DepartureProceduralParams& Params) : m_Params(Params) { Doc29Prof.accept(*this); }

            void visitDoc29ProfileDepartureProcedural(Doc29ProfileDepartureProcedural& Doc29Prof) override {
                switch (m_Params.StepType)
                {
                case Doc29ProfileDepartureProcedural::StepType::Takeoff: Doc29Prof.setTakeoffParametersE(m_Params.Coeffs, m_Params.TakeoffInitialCas); break;
                case Doc29ProfileDepartureProcedural::StepType::Climb: Doc29Prof.addClimbE(m_Params.Coeffs, m_Params.EndAltitude); break;
                case Doc29ProfileDepartureProcedural::StepType::ClimbAccelerate: Doc29Prof.addClimbAccelerateE(m_Params.Coeffs, m_Params.EndCas, m_Params.ClimbRate); break;
                case Doc29ProfileDepartureProcedural::StepType::ClimbAcceleratePercentage: Doc29Prof.addClimbAcceleratePercentageE(m_Params.Coeffs, m_Params.EndCas, m_Params.ClimbPercentage); break;
                default: GRAPE_ASSERT(false); break;
                }

                if (m_Params.ThrustCutback)
                    Doc29Prof.setThrustCutback(Doc29Prof.size() - 1);
            }
        private:
            DepartureProceduralParams m_Params;
        };
    }
    void importDoc29PerformanceProfilesDepartureSteps(const std::string& CsvPath) {
        auto& study = Application::study();
        const auto& set = Application::settings();

        CsvImport csvImp(CsvPath, "Doc29 departure procedural profiles", 10);
        if (!csvImp.valid())
            return;
        auto& csv = csvImp.CsvImp;
        const auto columnNames = csv.columnNames();

        std::vector<Doc29ProfileDeparture*> addedProfiles;

        for (std::size_t row = 0; row < csv.rowCount(); row++)
        {
            Doc29ProfileDeparture* updateProf = nullptr;
            try
            {
                const auto doc29AcftName = csv.getCell<std::string>(row, 0);
                if (doc29AcftName.empty())
                    throw GrapeException("Empty Doc29 Aircraft name not allowed.");
                if (!study.Doc29Aircrafts().contains(doc29AcftName))
                    throw GrapeException(std::format("Doc29 Aircraft '{}' does not exist in this study.", doc29AcftName));
                auto& doc29Acft = study.Doc29Aircrafts()(doc29AcftName);

                const auto profName = csv.getCell<std::string>(row, 1);

                DepartureProceduralParams params;

                const auto stepTypeStr = csv.getCell<std::string>(row, 2);
                if (!Doc29ProfileDepartureProcedural::StepTypes.contains(stepTypeStr))
                    throw GrapeException(std::format("Invalid departure step type '{}'.", stepTypeStr));
                params.StepType = Doc29ProfileDepartureProcedural::StepTypes.fromString(stepTypeStr);

                const auto thrustCutbackStr = csv.getCell<std::string>(row, 3);
                if (!thrustCutbackStr.empty())
                    params.ThrustCutback = true;

                params.Coeffs = csv.getCell<std::string>(row, 4);
                if (params.Coeffs.empty())
                    throw GrapeException(std::format("Aerodynamic coefficients name can't be empty for step type {}.", stepTypeStr));

                switch (params.StepType)
                {
                case Doc29ProfileDepartureProcedural::StepType::Takeoff:
                    {
                        try { params.TakeoffInitialCas = set.SpeedUnits.toSi(csv.getCell<double>(row, 9), columnNames.at(9)); }
                        catch (...) { throw GrapeException(std::format("Invalid initial calibrated airspeed for step type {}.", stepTypeStr)); }
                        break;
                    }
                case Doc29ProfileDepartureProcedural::StepType::Climb:
                    {
                        try { params.EndAltitude = set.AltitudeUnits.toSi(csv.getCell<double>(row, 5), columnNames.at(5)); }
                        catch (...) { throw std::invalid_argument(std::format("Invalid end altitude ATE for step type {}.", stepTypeStr)); }
                        break;
                    }
                case Doc29ProfileDepartureProcedural::StepType::ClimbAccelerate:
                    {
                        try { params.EndCas = set.SpeedUnits.toSi(csv.getCell<double>(row, 6), columnNames.at(6)); }
                        catch (...) { throw std::invalid_argument(std::format("Invalid end calibrated airspeed for step type {}.", stepTypeStr)); }

                        try { params.ClimbRate = set.VerticalSpeedUnits.toSi(csv.getCell<double>(row, 7), columnNames.at(7)); }
                        catch (...) { throw std::invalid_argument(std::format("Invalid climb rate for step type {}.", stepTypeStr)); }

                        break;
                    }
                case Doc29ProfileDepartureProcedural::StepType::ClimbAcceleratePercentage:
                    {
                        try { params.EndCas = set.SpeedUnits.toSi(csv.getCell<double>(row, 6), columnNames.at(6)); }
                        catch (...) { throw std::invalid_argument(std::format("Invalid end calibrated airspeed for step type {}.", stepTypeStr)); }

                        try { params.ClimbPercentage = csv.getCell<double>(row, 8); }
                        catch (...) { throw std::invalid_argument(std::format("Invalid acceleration percentage for step type {}.", stepTypeStr)); }

                        break;
                    }
                default: GRAPE_ASSERT(false); break;
                }

                if (doc29Acft.DepartureProfiles.contains(profName))
                {
                    if (std::ranges::find(addedProfiles, doc29Acft.DepartureProfiles(profName).get()) == addedProfiles.end())
                        throw GrapeException(std::format("Departure profile '{}' already exists in Doc29 Aircraft '{}'.", profName, doc29AcftName));
                }
                else
                {
                    auto& addedProfile = study.Doc29Aircrafts.addProfileDepartureE(doc29Acft, Doc29Profile::Type::Procedural, profName);
                    addedProfiles.emplace_back(&addedProfile);
                }

                updateProf = doc29Acft.DepartureProfiles(profName).get();
                auto& prof = *updateProf;
                DepartureProceduralInserter profIns(prof, params);
                study.Doc29Aircrafts.updateProfile(prof);
            }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing Doc29 departure procedural profiles at row {}. {}", row + 2, err.what());
                if (updateProf)
                    study.Doc29Aircrafts.updateProfile(*updateProf);
                ++csvImp.ErrorCount;
            }
        }
    }

    void importDoc29Noise(const std::string& CsvPath) {
        auto& study = Application::study();

        CsvImport csvImp(CsvPath, "Doc29 Noise entries", 3);
        if (!csvImp.valid())
            return;
        auto& csv = csvImp.CsvImp;
        const auto columnNames = csv.columnNames();

        for (std::size_t row = 0; row < csv.rowCount(); row++)
        {
            Doc29Noise* newDoc29Ns = nullptr;
            try
            {
                auto& doc29Ns = study.Doc29Noises.addNoiseE(csv.getCell<std::string>(row, 0));
                newDoc29Ns = &doc29Ns;

                const auto latDirStr = csv.getCell<std::string>(row, 1);
                if (!Doc29Noise::LateralDirectivities.contains(latDirStr))
                    throw GrapeException(std::format("Invalid lateral directivity type '{}'.", latDirStr));
                doc29Ns.LateralDir = Doc29Noise::LateralDirectivities.fromString(latDirStr);

                const auto sorStr = csv.getCell<std::string>(row, 2);
                if (!Doc29Noise::SORCorrections.contains(sorStr))
                    throw GrapeException(std::format("Invalid start-of-roll correction type '{}'.", sorStr));
                doc29Ns.SOR = Doc29Noise::SORCorrections.fromString(sorStr);

                study.Doc29Noises.updateNoise(doc29Ns);
            }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing Doc29 Noise at row {}. {}", row + 2, err.what());
                if (newDoc29Ns)
                    study.Doc29Noises.eraseNoise(*newDoc29Ns);
                csvImp.ErrorCount++;
            }
        }
    }

    void importDoc29NoiseNpd(const std::string& CsvPath) {
        auto& study = Application::study();
        const Settings& set = Application::settings();

        CsvImport csvImp(CsvPath, "Doc29 Noise NPD data", 14);
        if (!csvImp.valid())
            return;
        auto& csv = csvImp.CsvImp;
        const auto columnNames = csv.columnNames();

        for (std::size_t row = 0; row < csv.rowCount(); row++)
        {
            try
            {
                const auto doc29NsName = csv.getCell<std::string>(row, 0);
                if (doc29NsName.empty())
                    throw GrapeException("Empty Doc29 Noise name not allowed.");
                if (!study.Doc29Noises().contains(doc29NsName))
                    throw GrapeException(std::format("Doc29 Noise '{}' does not exist in this study.", doc29NsName));
                auto& doc29Ns = study.Doc29Noises()(doc29NsName);

                const auto opTypeStr = csv.getCell<std::string>(row, 1);
                if (!OperationTypes.contains(opTypeStr))
                    throw GrapeException(std::format("Invalid operation type '{}'.", opTypeStr));
                const OperationType opType = OperationTypes.fromString(opTypeStr);

                const auto nsMetricStr = csv.getCell<std::string>(row, 2);
                if (!NoiseSingleMetrics.contains(nsMetricStr))
                    throw GrapeException(std::format("Invalid noise metric '{}'.", nsMetricStr));
                const NoiseSingleMetric nsMetric = NoiseSingleMetrics.fromString(nsMetricStr);

                double thrust = Constants::NaN;
                NpdData::PowerNoiseLevelsArray nsLevels{};

                try { thrust = set.ThrustUnits.toSi(csv.getCell<double>(row, 3), csv.columnName(3)); }
                catch (...) { throw std::invalid_argument("Invalid thrust."); }

                for (std::size_t i = 0; i < NpdStandardDistancesSize; ++i)
                {
                    try { nsLevels.at(i) = csv.getCell<double>(row, 4 + i); }
                    catch (...) { throw std::invalid_argument(std::format("Invalid noise level at {:.0f} ft.", toFeet(NpdStandardDistances.at(i)))); }
                }

                switch (opType)
                {
                case OperationType::Arrival:
                    {
                        switch (nsMetric)
                        {
                        case NoiseSingleMetric::Lamax: doc29Ns.ArrivalLamax.addThrustE(thrust, nsLevels); break;
                        case NoiseSingleMetric::Sel: doc29Ns.ArrivalSel.addThrustE(thrust, nsLevels); break;
                        default: GRAPE_ASSERT(false); break;
                        }
                        break;
                    }
                case OperationType::Departure:
                    {
                        switch (nsMetric)
                        {
                        case NoiseSingleMetric::Lamax: doc29Ns.DepartureLamax.addThrustE(thrust, nsLevels); break;
                        case NoiseSingleMetric::Sel: doc29Ns.DepartureSel.addThrustE(thrust, nsLevels); break;
                        default: GRAPE_ASSERT(false); break;
                        }
                        break;
                    }
                default: GRAPE_ASSERT(false); break;
                }

                study.Doc29Noises.updateMetric(doc29Ns, opType, nsMetric);
            }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing Doc29 Noise NPD data at row {}. {}", row + 2, err.what());
                csvImp.ErrorCount++;
            }
        }
    }

    void importDoc29NoiseSpectrum(const std::string& CsvPath) {
        auto& study = Application::study();

        CsvImport csvImp(CsvPath, "Doc29 Noise spectrum", 26);
        if (!csvImp.valid())
            return;
        auto& csv = csvImp.CsvImp;
        const auto columnNames = csv.columnNames();

        for (std::size_t row = 0; row < csv.rowCount(); row++)
        {
            try
            {
                const auto doc29NsName = csv.getCell<std::string>(row, 0);
                if (doc29NsName.empty())
                    throw GrapeException("Empty Doc29 Noise name not allowed.");
                if (!study.Doc29Noises().contains(doc29NsName))
                    throw GrapeException(std::format("Doc29 Noise '{}' does not exist in this study.", doc29NsName));
                auto& doc29Ns = study.Doc29Noises()(doc29NsName);

                const auto opTypeStr = csv.getCell<std::string>(row, 1);
                if (!OperationTypes.contains(opTypeStr))
                    throw GrapeException(std::format("Invalid operation type '{}'.", opTypeStr));
                const OperationType opType = OperationTypes.fromString(opTypeStr);

                OneThirdOctaveArray spectrumArray{};
                for (std::size_t i = 0; i < OneThirdOctaveBandsSize; ++i)
                {
                    try { spectrumArray.at(i) = csv.getCell<double>(row, 2 + i); }
                    catch (...) { throw std::invalid_argument(std::format("Invalid noise level at {:.0f} Hz.", OneThirdOctaveCenterFrequencies.at(i))); }
                }

                switch (opType)
                {
                case OperationType::Arrival:
                    {
                        for (std::size_t i = 0; i < OneThirdOctaveBandsSize; ++i)
                            doc29Ns.ArrivalSpectrum.setValue(i, spectrumArray.at(i));
                        break;
                    }
                case OperationType::Departure:
                    {
                        for (std::size_t i = 0; i < OneThirdOctaveBandsSize; ++i)
                            doc29Ns.DepartureSpectrum.setValue(i, spectrumArray.at(i));
                        break;
                    }
                default: GRAPE_ASSERT(false); break;
                }

                study.Doc29Noises.updateNoise(doc29Ns);
            }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing Doc29 Noise spectrum at row {}. {}", row + 2, err.what());
                csvImp.ErrorCount++;
            }
        }
    }

    void importLTO(const std::string& CsvPath) {
        auto& study = Application::study();
        const auto& set = Application::settings();

        CsvImport csvImp(CsvPath, "LTO engines", 40);
        if (!csvImp.valid())
            return;
        auto& csv = csvImp.CsvImp;
        const auto columnNames = csv.columnNames();

        for (std::size_t row = 0; row < csv.rowCount(); row++)
        {
            LTOEngine* newLtoEngine = nullptr;
            try
            {
                auto& lto = study.LTOEngines.addLTOEngineE(csv.getCell<std::string>(row, 0));
                newLtoEngine = &lto;

                try { lto.setMaximumSeaLevelStaticThrust(set.ThrustUnits.toSi(csv.getCell<double>(row, 1), columnNames.at(1))); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid maximum sea level static thrust."); }

                try { lto.setFuelFlow(LTOPhase::Idle, set.FuelFlowUnits.toSi(csv.getCell<double>(row, 2), columnNames.at(2))); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid fuel flow for idle phase."); }

                try { lto.setFuelFlow(LTOPhase::Approach, set.FuelFlowUnits.toSi(csv.getCell<double>(row, 3), columnNames.at(3))); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid fuel flow for approach phase."); }

                try { lto.setFuelFlow(LTOPhase::ClimbOut, set.FuelFlowUnits.toSi(csv.getCell<double>(row, 4), columnNames.at(4))); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid fuel flow for climb out phase."); }

                try { lto.setFuelFlow(LTOPhase::Takeoff, set.FuelFlowUnits.toSi(csv.getCell<double>(row, 5), columnNames.at(5))); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid fuel flow for takeoff phase."); }

                const auto strCorrIdle = csv.getCell<std::string>(row, 6);
                if (!strCorrIdle.empty())
                {
                    try { lto.setFuelFlowCorrection(LTOPhase::Idle, csv.getCell<double>(row, 6)); }
                    catch (const GrapeException&) { throw; }
                    catch (...) { throw std::invalid_argument("Invalid fuel flow correction factor for idle phase."); }
                }

                const auto strCorrApproach = csv.getCell<std::string>(row, 7);
                if (!strCorrApproach.empty())
                {
                    try { lto.setFuelFlowCorrection(LTOPhase::Approach, csv.getCell<double>(row, 7)); }
                    catch (const GrapeException&) { throw; }
                    catch (...) { throw std::invalid_argument("Invalid fuel flow correction factor for approach phase."); }
                }

                const auto strCorrClimbOut = csv.getCell<std::string>(row, 8);
                if (!strCorrClimbOut.empty())
                {
                    try { lto.setFuelFlowCorrection(LTOPhase::ClimbOut, csv.getCell<double>(row, 8)); }
                    catch (const GrapeException&) { throw; }
                    catch (...) { throw std::invalid_argument("Invalid fuel flow correction factor for climb out phase."); }
                }

                const auto strCorrTakeoff = csv.getCell<std::string>(row, 9);
                if (!strCorrTakeoff.empty())
                {
                    try { lto.setFuelFlowCorrection(LTOPhase::Takeoff, csv.getCell<double>(row, 9)); }
                    catch (const GrapeException&) { throw; }
                    catch (...) { throw std::invalid_argument("Invalid fuel flow correction factor for takeoff phase."); }
                }

                try { lto.setEmissionIndexHC(LTOPhase::Idle, set.EmissionIndexUnits.toSi(csv.getCell<double>(row, 10), columnNames.at(10))); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid HC emission index for idle phase."); }

                try { lto.setEmissionIndexHC(LTOPhase::Approach, set.EmissionIndexUnits.toSi(csv.getCell<double>(row, 11), columnNames.at(11))); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid HC emission index for approach phase."); }

                try { lto.setEmissionIndexHC(LTOPhase::ClimbOut, set.EmissionIndexUnits.toSi(csv.getCell<double>(row, 12), columnNames.at(12))); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid HC emission index for climb out phase."); }

                try { lto.setEmissionIndexHC(LTOPhase::Takeoff, set.EmissionIndexUnits.toSi(csv.getCell<double>(row, 13), columnNames.at(13))); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid HC emission index for takeoff phase."); }

                try { lto.setEmissionIndexCO(LTOPhase::Idle, set.EmissionIndexUnits.toSi(csv.getCell<double>(row, 14), columnNames.at(14))); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid CO emission index for idle phase."); }

                try { lto.setEmissionIndexCO(LTOPhase::Approach, set.EmissionIndexUnits.toSi(csv.getCell<double>(row, 15), columnNames.at(15))); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid CO emission index for approach phase."); }

                try { lto.setEmissionIndexCO(LTOPhase::ClimbOut, set.EmissionIndexUnits.toSi(csv.getCell<double>(row, 16), columnNames.at(16))); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid CO emission index for climb out phase."); }

                try { lto.setEmissionIndexCO(LTOPhase::Takeoff, set.EmissionIndexUnits.toSi(csv.getCell<double>(row, 17), columnNames.at(17))); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid CO emission index for takeoff phase."); }

                try { lto.setEmissionIndexNOx(LTOPhase::Idle, set.EmissionIndexUnits.toSi(csv.getCell<double>(row, 18), columnNames.at(18))); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid NOx emission index for idle phase."); }

                try { lto.setEmissionIndexNOx(LTOPhase::Approach, set.EmissionIndexUnits.toSi(csv.getCell<double>(row, 19), columnNames.at(19))); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid NOx emission index for approach phase."); }

                try { lto.setEmissionIndexNOx(LTOPhase::ClimbOut, set.EmissionIndexUnits.toSi(csv.getCell<double>(row, 20), columnNames.at(20))); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid NOx emission index for climb out phase."); }

                try { lto.setEmissionIndexNOx(LTOPhase::Takeoff, set.EmissionIndexUnits.toSi(csv.getCell<double>(row, 21), columnNames.at(21))); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid NOx emission index for takeoff phase."); }

                try { lto.MixedNozzle = static_cast<bool>(csv.getCell<int>(row, 22)); }
                catch (...) { throw std::invalid_argument("Invalid mixed nozzle flag."); }

                try { lto.setBypassRatio(csv.getCell<double>(row, 23)); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid bypass ratio."); }

                {
                    const auto str = csv.getCell<std::string>(row, 24);
                    if (!str.empty())
                    {
                        try { lto.setAirFuelRatio(LTOPhase::Idle, csv.getCell<double>(row, 24)); }
                        catch (const GrapeException&) { throw; }
                        catch (...) { throw std::invalid_argument("Invalid air to fuel ratio for idle phase."); }
                    }
                }

                {
                    const auto str = csv.getCell<std::string>(row, 25);
                    if (!str.empty())
                    {
                        try { lto.setAirFuelRatio(LTOPhase::Approach, csv.getCell<double>(row, 25)); }
                        catch (const GrapeException&) { throw; }
                        catch (...) { throw std::invalid_argument("Invalid air to fuel ratio for approach phase."); }
                    }
                }

                {
                    const auto str = csv.getCell<std::string>(row, 26);
                    if (!str.empty())
                    {
                        try { lto.setAirFuelRatio(LTOPhase::ClimbOut, csv.getCell<double>(row, 26)); }
                        catch (const GrapeException&) { throw; }
                        catch (...) { throw std::invalid_argument("Invalid air to fuel ratio for climb out phase."); }
                    }
                }

                {
                    const auto str = csv.getCell<std::string>(row, 27);
                    if (!str.empty())
                    {
                        try { lto.setAirFuelRatio(LTOPhase::Takeoff, csv.getCell<double>(row, 27)); }
                        catch (const GrapeException&) { throw; }
                        catch (...) { throw std::invalid_argument("Invalid air to fuel ratio for takeoff phase."); }
                    }
                }

                {
                    const auto str = csv.getCell<std::string>(row, 28);
                    if (!str.empty())
                    {
                        try { lto.setSmokeNumber(LTOPhase::Idle, csv.getCell<double>(row, 28)); }
                        catch (const GrapeException&) { throw; }
                        catch (...) { throw std::invalid_argument("Invalid smoke number for idle phase."); }
                    }
                }
                {
                    const auto str = csv.getCell<std::string>(row, 29);
                    if (!str.empty())
                    {
                        try { lto.setSmokeNumber(LTOPhase::Approach, csv.getCell<double>(row, 29)); }
                        catch (const GrapeException&) { throw; }
                        catch (...) { throw std::invalid_argument("Invalid smoke number for approach phase."); }
                    }
                }
                {
                    const auto str = csv.getCell<std::string>(row, 30);
                    if (!str.empty())
                    {
                        try { lto.setSmokeNumber(LTOPhase::ClimbOut, csv.getCell<double>(row, 30)); }
                        catch (const GrapeException&) { throw; }
                        catch (...) { throw std::invalid_argument("Invalid smoke number for climb out phase."); }
                    }
                }
                {
                    const auto str = csv.getCell<std::string>(row, 31);
                    if (!str.empty())
                    {
                        try { lto.setSmokeNumber(LTOPhase::Takeoff, csv.getCell<double>(row, 31)); }
                        catch (const GrapeException&) { throw; }
                        catch (...) { throw std::invalid_argument("Invalid smoke number for takeoff phase."); }
                    }
                }

                {
                    const auto str = csv.getCell<std::string>(row, 32);
                    if (!str.empty())
                    {
                        try { lto.setEmissionIndexNVPM(LTOPhase::Idle, fromMilligramsPerKilogram(csv.getCell<double>(row, 32))); }
                        catch (const GrapeException&) { throw; }
                        catch (...) { throw std::invalid_argument("Invalid nvPm emission index for idle phase."); }
                    }
                }
                {
                    const auto str = csv.getCell<std::string>(row, 33);
                    if (!str.empty())
                    {
                        try { lto.setEmissionIndexNVPM(LTOPhase::Approach, fromMilligramsPerKilogram(csv.getCell<double>(row, 33))); }
                        catch (const GrapeException&) { throw; }
                        catch (...) { throw std::invalid_argument("Invalid nvPm emission index for approach phase."); }
                    }
                }
                {
                    const auto str = csv.getCell<std::string>(row, 34);
                    if (!str.empty())
                    {
                        try { lto.setEmissionIndexNVPM(LTOPhase::ClimbOut, fromMilligramsPerKilogram(csv.getCell<double>(row, 34))); }
                        catch (const GrapeException&) { throw; }
                        catch (...) { throw std::invalid_argument("Invalid nvPm emission index for climb out phase."); }
                    }
                }
                {
                    const auto str = csv.getCell<std::string>(row, 35);
                    if (!str.empty())
                    {
                        try { lto.setEmissionIndexNVPM(LTOPhase::Takeoff, fromMilligramsPerKilogram(csv.getCell<double>(row, 35))); }
                        catch (const GrapeException&) { throw; }
                        catch (...) { throw std::invalid_argument("Invalid nvPm emission index for takeoff phase."); }
                    }
                }

                {
                    const auto str = csv.getCell<std::string>(row, 36);
                    if (!str.empty())
                    {
                        try { lto.setEmissionIndexNVPMNumber(LTOPhase::Idle, csv.getCell<double>(row, 36)); }
                        catch (const GrapeException&) { throw; }
                        catch (...) { throw std::invalid_argument("Invalid nvPm number emission index for idle phase."); }
                    }
                }
                {
                    const auto str = csv.getCell<std::string>(row, 37);
                    if (!str.empty())
                    {
                        try { lto.setEmissionIndexNVPMNumber(LTOPhase::Approach, csv.getCell<double>(row, 37)); }
                        catch (const GrapeException&) { throw; }
                        catch (...) { throw std::invalid_argument("Invalid nvPm number emission index for approach phase."); }
                    }
                }
                {
                    const auto str = csv.getCell<std::string>(row, 38);
                    if (!str.empty())
                    {
                        try { lto.setEmissionIndexNVPMNumber(LTOPhase::ClimbOut, csv.getCell<double>(row, 38)); }
                        catch (const GrapeException&) { throw; }
                        catch (...) { throw std::invalid_argument("Invalid nvPm number emission index for climb out phase."); }
                    }
                }
                {
                    const auto str = csv.getCell<std::string>(row, 39);
                    if (!str.empty())
                    {
                        try { lto.setEmissionIndexNVPMNumber(LTOPhase::Takeoff, csv.getCell<double>(row, 39)); }
                        catch (const GrapeException&) { throw; }
                        catch (...) { throw std::invalid_argument("Invalid nvPm number emission index for takeoff phase."); }
                    }
                }
                study.LTOEngines.update(lto);
            }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing LTO engine at row {}. {}", row + 2, err.what());
                if (newLtoEngine)
                    study.LTOEngines.erase(*newLtoEngine);
                csvImp.ErrorCount++;
            }
        }
    }

    void importSFI(const std::string& CsvPath) {
        auto& study = Application::study();
        const auto& set = Application::settings();

        CsvImport csvImp(CsvPath, "SFI coefficients", 10);
        if (!csvImp.valid())
            return;
        auto& csv = csvImp.CsvImp;
        const auto columnNames = csv.columnNames();

        for (std::size_t row = 0; row < csv.rowCount(); row++)
        {
            SFI* newSfi = nullptr;
            try
            {
                auto& sfi = study.SFIs.addSFIE(csv.getCell<std::string>(row, 0));
                newSfi = &sfi;

                try { sfi.setMaximumSeaLevelStaticThrust(set.ThrustUnits.toSi(csv.getCell<double>(row, 1), columnNames.at(1))); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid maximum sea level static thrust."); }

                try { sfi.A = csv.getCell<double>(row, 2); }
                catch (...) { throw std::invalid_argument("Invalid A coefficient."); }

                try { sfi.B1 = csv.getCell<double>(row, 3); }
                catch (...) { throw std::invalid_argument("Invalid B1 coefficient."); }

                try { sfi.B2 = csv.getCell<double>(row, 4); }
                catch (...) { throw std::invalid_argument("Invalid B2 coefficient."); }

                try { sfi.B3 = csv.getCell<double>(row, 5); }
                catch (...) { throw std::invalid_argument("Invalid B3 coefficient."); }

                try { sfi.K1 = csv.getCell<double>(row, 6); }
                catch (...) { throw std::invalid_argument("Invalid K1 coefficient."); }

                try { sfi.K2 = csv.getCell<double>(row, 7); }
                catch (...) { throw std::invalid_argument("Invalid K2 coefficient."); }

                try { sfi.K3 = csv.getCell<double>(row, 8); }
                catch (...) { throw std::invalid_argument("Invalid K3 coefficient."); }

                try { sfi.K4 = csv.getCell<double>(row, 9); }
                catch (...) { throw std::invalid_argument("Invalid K4 coefficient."); }

                study.SFIs.update(sfi);
            }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing SFI coefficients at row {}. {}", row + 2, err.what());
                if (newSfi)
                    study.SFIs.erase(*newSfi);
                csvImp.ErrorCount++;
            }
        }
    }

    void importFleet(const std::string& CsvPath) {
        auto& study = Application::study();
        const Settings& set = Application::settings();

        CsvImport csvImp(CsvPath, "fleet", 8);
        if (!csvImp.valid())
            return;
        auto& csv = csvImp.CsvImp;
        const auto columnNames = csv.columnNames();

        for (std::size_t row = 0; row < csv.rowCount(); row++)
        {
            Aircraft* newAcft = nullptr;
            try
            {
                auto& acft = study.Aircrafts.addAircraftE(csv.getCell<std::string>(row, 0));
                newAcft = &acft;

                try { acft.setEngineCountE(csv.getCell<int>(row, 1)); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid number of engines."); }

                auto doc29AcftStr = csv.getCell<std::string>(row, 2);
                if (!doc29AcftStr.empty())
                {
                    if (study.Doc29Aircrafts().contains(doc29AcftStr))
                        study.Aircrafts.setDoc29Performance(acft, &study.Doc29Aircrafts(doc29AcftStr));
                    else
                        throw GrapeException(std::format("Doc29 Aircraft '{}' does not exist in this study.", doc29AcftStr));
                }

                auto sfiStr = csv.getCell<std::string>(row, 3);
                if (!sfiStr.empty())
                {
                    if (study.SFIs().contains(sfiStr))
                        acft.SFIFuel = &study.SFIs()(sfiStr);
                    else
                        throw GrapeException(std::format("SFI ID '{}' does not exist in this study.", sfiStr));
                }

                auto ltoStr = csv.getCell<std::string>(row, 4);
                if (!ltoStr.empty())
                {
                    if (study.LTOEngines().contains(ltoStr))
                        acft.LTOEng = &study.LTOEngines()(ltoStr);
                    else
                        throw GrapeException(std::format("LTO engine '{}' does not exist in this study.", ltoStr));
                }

                auto doc29NsStr = csv.getCell<std::string>(row, 5);
                if (!doc29NsStr.empty())
                {
                    if (study.Doc29Noises().contains(doc29NsStr))
                        study.Aircrafts.setDoc29Noise(acft, &study.Doc29Noises(doc29NsStr));
                    else
                        throw GrapeException(std::format("Doc29 noise ID '{}' does not exist in this study.", doc29NsStr));
                }

                try { acft.Doc29NoiseDeltaArrivals = csv.getCell<double>(row, 6); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid Doc29 noise delta for arrivals."); }

                try { acft.Doc29NoiseDeltaDepartures = csv.getCell<double>(row, 7); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid Doc29 noise delta for arrivals."); }

                study.Aircrafts.update(acft);
            }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing aircraft at row {}. {}", row + 2, err.what());
                if (newAcft)
                    study.Aircrafts.erase(*newAcft);
                csvImp.ErrorCount++;
            }
        }
    }

    void importAirports(const std::string& CsvPath) {
        auto& study = Application::study();
        const Settings& set = Application::settings();

        CsvImport csvImp(CsvPath, "airports", 6);
        if (!csvImp.valid())
            return;
        auto& csv = csvImp.CsvImp;
        const auto columnNames = csv.columnNames();

        for (std::size_t row = 0; row < csv.rowCount(); row++)
        {
            const Airport* newApt = nullptr;
            try
            {
                auto& apt = study.Airports.addAirportE(csv.getCell<std::string>(row, 0));
                newApt = &apt;

                try { apt.setLongitude(csv.getCell<double>(row, 1)); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid longitude."); }

                try { apt.setLatitude(csv.getCell<double>(row, 2)); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid latitude."); }

                try { apt.Elevation = set.AltitudeUnits.toSi(csv.getCell<double>(row, 3), columnNames.at(3)); }
                catch (...) { throw std::invalid_argument("Invalid elevation."); }

                const auto refTempStr = csv.getCell<std::string>(row, 4);
                if (!refTempStr.empty())
                {
                    try { apt.setReferenceTemperature(set.TemperatureUnits.toSi(csv.getCell<double>(row, 4), columnNames.at(4))); }
                    catch (const GrapeException&) { throw; }
                    catch (...) { throw std::invalid_argument("Invalid reference temperature."); }
                }

                const auto refPressStr = csv.getCell<std::string>(row, 5);
                if (!refPressStr.empty())
                {
                    try { apt.setReferenceSeaLevelPressure(set.PressureUnits.toSi(csv.getCell<double>(row, 5), columnNames.at(5))); }
                    catch (const GrapeException&) { throw; }
                    catch (...) { throw std::invalid_argument("Invalid reference pressure."); }
                }

                study.Airports.update(apt);
            }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing airport at row {}. {}", row + 2, err.what());
                if (newApt)
                    study.Airports.erase(*newApt);
                ++csvImp.ErrorCount;
            }
        }
    }

    void importRunways(const std::string& CsvPath) {
        auto& study = Application::study();
        const auto& set = Application::settings();

        CsvImport csvImp(CsvPath, "runways", 8);
        if (!csvImp.valid())
            return;
        auto& csv = csvImp.CsvImp;
        const auto columnNames = csv.columnNames();

        for (std::size_t row = 0; row < csv.rowCount(); row++)
        {
            const Runway* newRwy = nullptr;
            try
            {
                const auto aptName = csv.getCell<std::string>(row, 0);
                if (aptName.empty())
                    throw GrapeException("Empty airport name not allowed.");
                if (!study.Airports().contains(aptName))
                    throw GrapeException(std::format("Airport '{}' does not exist in this study.", aptName));
                auto& apt = study.Airports()(aptName);

                auto& rwy = study.Airports.addRunwayE(apt, csv.getCell<std::string>(row, 1));
                newRwy = &rwy;

                try { rwy.setLongitude(csv.getCell<double>(row, 2)); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid longitude."); }

                try { rwy.setLatitude(csv.getCell<double>(row, 3)); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid latitude."); }

                try { rwy.Elevation = set.AltitudeUnits.toSi(csv.getCell<double>(row, 4), columnNames.at(4)); }
                catch (...) { throw std::invalid_argument("Invalid elevation."); }

                try { rwy.setLength(set.DistanceUnits.toSi(csv.getCell<double>(row, 5), columnNames.at(5))); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid length."); }

                try { rwy.setHeading(csv.getCell<double>(row, 6)); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid heading."); }

                const auto strGradient = csv.getCell<std::string>(row, 7);
                if (!strGradient.empty())
                    try { rwy.setGradient(csv.getCell<double>(row, 7)); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid gradient."); }

                study.Airports.update(rwy);
            }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing runway at row {}. {}", row + 2, err.what());
                if (newRwy)
                    study.Airports.erase(*newRwy);
                csvImp.ErrorCount++;
            }
        }
    }

    namespace {
        struct RouteSimpleInserter : RouteTypeVisitor {
            RouteSimpleInserter(Route& Rte, double Longitude, double Latitude) : m_Longitude(Longitude), m_Latitude(Latitude) { Rte.accept(*this); }
            void visitSimple(RouteTypeSimple& Rte) override { Rte.addPointE(m_Longitude, m_Latitude); }

        private:
            double m_Longitude;
            double m_Latitude;
        };
    }

    void importRoutesSimple(const std::string& CsvPath) {
        auto& study = Application::study();

        CsvImport csvImp(CsvPath, "simple routes", 6);
        if (!csvImp.valid())
            return;
        auto& csv = csvImp.CsvImp;
        const auto columnNames = csv.columnNames();

        std::vector<const Route*> addedArrivalRoutes;
        std::vector<const Route*> addedDepartureRoutes;

        for (std::size_t row = 0; row < csv.rowCount(); row++)
        {
            Route* updateRte = nullptr;
            try
            {
                auto aptName = csv.getCell<std::string>(row, 0);
                if (aptName.empty())
                    throw GrapeException("Empty airport name not allowed.");
                if (!study.Airports().contains(aptName))
                    throw GrapeException(std::format("Airport '{}' does not exist in this study.", aptName));
                auto& apt = study.Airports()(aptName);

                auto rwyName = csv.getCell<std::string>(row, 1);
                if (rwyName.empty())
                    throw GrapeException("Empty runway name not allowed.");
                if (!apt.Runways.contains(rwyName))
                    throw GrapeException(std::format("Runway '{}' does not exist in airport '{}'.", rwyName, aptName));
                auto& rwy = apt.Runways(rwyName);

                auto opTypeStr = csv.getCell<std::string>(row, 2);
                if (!OperationTypes.contains(opTypeStr))
                    throw GrapeException(std::format("Invalid operation type '{}'.", opTypeStr));
                const OperationType opType = OperationTypes.fromString(opTypeStr);

                auto rteName = csv.getCell<std::string>(row, 3);
                if (rteName.empty())
                    throw GrapeException("Empty name not allowed.");

                double lon = Constants::NaN;
                double lat = Constants::NaN;

                try { lon = csv.getCell<double>(row, 4); }
                catch (...) { throw std::invalid_argument("Invalid longitude."); }

                try { lat = csv.getCell<double>(row, 5); }
                catch (...) { throw std::invalid_argument("Invalid latitude."); }

                switch (opType)
                {
                case OperationType::Arrival:
                    {
                        if (rwy.ArrivalRoutes.contains(rteName))
                        {
                            if (std::ranges::find(addedArrivalRoutes, rwy.ArrivalRoutes(rteName).get()) == addedArrivalRoutes.end())
                                throw GrapeException(std::format("Arrival route '{}' already exists in runway '{}' of airport '{}'.", rteName, rwyName, aptName));
                        }
                        else
                        {
                            auto& addedRte = study.Airports.addRouteArrivalE(rwy, Route::Type::Simple, rteName);
                            addedArrivalRoutes.emplace_back(&addedRte);
                        }

                        updateRte = rwy.ArrivalRoutes(rteName).get();
                        auto& rte = *updateRte;
                        RouteSimpleInserter rteIns(rte, lon, lat);
                        study.Airports.update(rte);
                        break;
                    }
                case OperationType::Departure:
                    {
                        if (rwy.DepartureRoutes.contains(rteName))
                        {
                            if (std::ranges::find(addedDepartureRoutes, rwy.DepartureRoutes(rteName).get()) == addedDepartureRoutes.end())
                                throw GrapeException(std::format("Departure route '{}' already exists in runway '{}' of airport '{}'.", rteName, rwyName, aptName));
                        }
                        else
                        {
                            auto& addedRte = study.Airports.addRouteDepartureE(rwy, Route::Type::Simple, rteName);
                            addedDepartureRoutes.emplace_back(&addedRte);
                        }

                        updateRte = rwy.DepartureRoutes(rteName).get();
                        auto& rte = *updateRte;
                        RouteSimpleInserter rteIns(rte, lon, lat);
                        study.Airports.update(rte);
                        break;
                    }
                default: GRAPE_ASSERT(false);
                    break;
                }
            }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing route simple point at row {}. {}", row + 2, err.what());
                if (updateRte)
                    study.Airports.update(*updateRte);
                csvImp.ErrorCount++;
            }
        }
    }

    namespace {
        struct RouteVectorInserter : RouteTypeVisitor {
            RouteVectorInserter(Route& Rte, RouteTypeVectors::VectorType VecType, double GroundDistance, double TurnRadius, double HeadingChange, RouteTypeVectors::Turn::Direction TurnDir) : m_VecType(VecType), m_GroundDistance(GroundDistance), m_TurnRadius(TurnRadius), m_HeadingChange(HeadingChange), m_TurnDirection(TurnDir) { Rte.accept(*this); }

            void visitVectors(RouteTypeVectors& Rte) override {
                switch (m_VecType)
                {
                case RouteTypeVectors::VectorType::Straight:
                    {
                        Rte.addStraightE(m_GroundDistance);
                        break;
                    }
                case RouteTypeVectors::VectorType::Turn:
                    {
                        Rte.addTurnE(m_TurnRadius, m_HeadingChange, m_TurnDirection);
                        break;
                    }
                default: GRAPE_ASSERT(false);
                    break;
                }
            }

        private:
            RouteTypeVectors::VectorType m_VecType;
            double m_GroundDistance;
            double m_TurnRadius;
            double m_HeadingChange;
            RouteTypeVectors::Turn::Direction m_TurnDirection;
        };
    }

    void importRoutesVectors(const std::string& CsvPath) {
        auto& study = Application::study();
        const Settings& set = Application::settings();

        CsvImport csvImp(CsvPath, "vector routes", 9);
        if (!csvImp.valid())
            return;
        auto& csv = csvImp.CsvImp;
        const auto columnNames = csv.columnNames();

        std::vector<const Route*> addedArrivalRoutes;
        std::vector<const Route*> addedDepartureRoutes;

        for (std::size_t row = 0; row < csv.rowCount(); row++)
        {
            Route* updateRte = nullptr;
            try
            {
                auto aptName = csv.getCell<std::string>(row, 0);
                if (aptName.empty())
                    throw GrapeException("Empty airport name not allowed.");
                if (!study.Airports().contains(aptName))
                    throw GrapeException(std::format("Airport '{}' does not exist in this study.", aptName));
                auto& apt = study.Airports()(aptName);

                auto rwyName = csv.getCell<std::string>(row, 1);
                if (rwyName.empty())
                    throw GrapeException("Empty runway name not allowed.");
                if (!apt.Runways.contains(rwyName))
                    throw GrapeException(std::format("Runway '{}' does not exist in airport '{}'.", rwyName, aptName));
                auto& rwy = apt.Runways(rwyName);

                auto opTypeStr = csv.getCell<std::string>(row, 2);
                if (!OperationTypes.contains(opTypeStr))
                    throw GrapeException(std::format("Invalid operation type '{}'.", opTypeStr));
                const OperationType opType = OperationTypes.fromString(opTypeStr);

                auto rteName = csv.getCell<std::string>(row, 3);
                if (rteName.empty())
                    throw GrapeException("Empty name not allowed.");

                auto vecTypeStr = csv.getCell<std::string>(row, 4);
                if (!RouteTypeVectors::VectorTypes.contains(vecTypeStr))
                    throw GrapeException(std::format("Invalid vector type '{}'.", vecTypeStr));
                const auto vecType = RouteTypeVectors::VectorTypes.fromString(vecTypeStr);

                double distance = Constants::NaN;
                double turnRadius = Constants::NaN;
                double heading = Constants::NaN;
                auto turnDir = RouteTypeVectors::Turn::Direction::Left;

                switch (vecType)
                {
                case RouteTypeVectors::VectorType::Straight:
                    {
                        try { distance = set.DistanceUnits.toSi(csv.getCell<double>(row, 5), columnNames.at(5)); }
                        catch (...) { throw std::invalid_argument("Invalid distance."); }
                        break;
                    }
                case RouteTypeVectors::VectorType::Turn:
                    {
                        try { turnRadius = set.DistanceUnits.toSi(csv.getCell<double>(row, 6), columnNames.at(6)); }
                        catch (...) { throw std::invalid_argument("Invalid turn radius."); }

                        try { heading = csv.getCell<double>(row, 7); }
                        catch (...) { throw std::invalid_argument("Invalid heading change."); }

                        auto turnDirStr = csv.getCell<std::string>(row, 8);
                        if (!RouteTypeVectors::Turn::Directions.contains(turnDirStr))
                            throw GrapeException(std::format("Invalid turn direction '{}'.", turnDirStr));
                        turnDir = RouteTypeVectors::Turn::Directions.fromString(turnDirStr);
                        break;
                    }
                default: GRAPE_ASSERT(false);
                }

                switch (opType)
                {
                case OperationType::Arrival:
                    {
                        if (rwy.ArrivalRoutes.contains(rteName))
                        {
                            if (std::ranges::find(addedArrivalRoutes, rwy.ArrivalRoutes(rteName).get()) == addedArrivalRoutes.end())
                                throw GrapeException(std::format("Arrival route '{}' already exists in runway '{}' of airport '{}'.", rteName, rwyName, aptName));
                        }
                        else
                        {
                            auto& addedRte = study.Airports.addRouteArrivalE(rwy, Route::Type::Vectors, rteName);
                            addedArrivalRoutes.emplace_back(&addedRte);
                        }

                        updateRte = rwy.ArrivalRoutes(rteName).get();
                        auto& rte = *updateRte;
                        RouteVectorInserter rteIns(rte, vecType, distance, turnRadius, heading, turnDir);
                        study.Airports.update(rte);
                        break;
                    }
                case OperationType::Departure:
                    {
                        if (rwy.DepartureRoutes.contains(rteName))
                        {
                            if (std::ranges::find(addedDepartureRoutes, rwy.DepartureRoutes(rteName).get()) == addedDepartureRoutes.end())
                                throw GrapeException(std::format("Departure route '{}' already exists in runway '{}' of airport '{}'.", rteName, rwyName, aptName));
                        }
                        else
                        {
                            auto& addedRte = study.Airports.addRouteDepartureE(rwy, Route::Type::Vectors, rteName);
                            addedDepartureRoutes.emplace_back(&addedRte);
                        }

                        updateRte = rwy.DepartureRoutes(rteName).get();
                        auto& rte = *updateRte;
                        RouteVectorInserter rteIns(rte, vecType, distance, turnRadius, heading, turnDir);
                        study.Airports.update(rte);
                        break;
                    }
                default: GRAPE_ASSERT(false);
                    break;
                }
            }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing route vector at row {}. {}", row + 2, err.what());
                if (updateRte)
                    study.Airports.update(*updateRte);
                csvImp.ErrorCount++;
            }
        }
    }

    namespace {
        struct RouteRnpInserter : RouteTypeVisitor {
            RouteRnpInserter(Route& Rte, RouteTypeRnp::StepType RnpStepType, double Longitude, double Latitude, double CenterLongitude, double CenterLatitude) : m_RnpStepType(RnpStepType), m_Longitude(Longitude), m_Latitude(Latitude), m_CenterLongitude(CenterLongitude), m_CenterLatitude(CenterLatitude) { Rte.accept(*this); }

            void visitRnp(RouteTypeRnp& Rte) override {
                switch (m_RnpStepType)
                {
                case RouteTypeRnp::StepType::TrackToFix:
                    {
                        Rte.addTrackToFixE(m_Longitude, m_Latitude);
                        break;
                    }
                case RouteTypeRnp::StepType::RadiusToFix:
                    {
                        Rte.addRadiusToFixE(m_Longitude, m_Latitude, m_CenterLongitude, m_CenterLatitude);
                        break;
                    }
                default: GRAPE_ASSERT(false);
                    break;
                }
            }

        private:
            RouteTypeRnp::StepType m_RnpStepType;
            double m_Longitude;
            double m_Latitude;
            double m_CenterLongitude;
            double m_CenterLatitude;
        };
    }

    void importRoutesRnp(const std::string& CsvPath) {
        auto& study = Application::study();

        CsvImport csvImp(CsvPath, "RNP routes", 9);
        if (!csvImp.valid())
            return;
        auto& csv = csvImp.CsvImp;
        const auto columnNames = csv.columnNames();

        std::vector<const Route*> addedArrivalRoutes;
        std::vector<const Route*> addedDepartureRoutes;

        for (std::size_t row = 0; row < csv.rowCount(); row++)
        {
            Route* updateRte = nullptr;
            try
            {
                auto aptName = csv.getCell<std::string>(row, 0);
                if (aptName.empty())
                    throw GrapeException("Empty airport name not allowed.");
                if (!study.Airports().contains(aptName))
                    throw GrapeException(std::format("Airport '{}' does not exist in this study.", aptName));
                auto& apt = study.Airports()(aptName);

                auto rwyName = csv.getCell<std::string>(row, 1);
                if (rwyName.empty())
                    throw GrapeException("Empty runway name not allowed.");
                if (!apt.Runways.contains(rwyName))
                    throw GrapeException(std::format("Runway '{}' does not exist in airport '{}'.", rwyName, aptName));
                auto& rwy = apt.Runways(rwyName);

                auto opTypeStr = csv.getCell<std::string>(row, 2);
                if (!OperationTypes.contains(opTypeStr))
                    throw GrapeException(std::format("Invalid operation type '{}'.", opTypeStr));
                const OperationType opType = OperationTypes.fromString(opTypeStr);

                auto rteName = csv.getCell<std::string>(row, 3);
                if (rteName.empty())
                    throw GrapeException("Empty name not allowed.");

                auto rnpStepTypeStr = csv.getCell<std::string>(row, 4);
                if (!RouteTypeRnp::StepTypes.contains(rnpStepTypeStr))
                    throw GrapeException(std::format("Invalid RNP step type '{}'.", rnpStepTypeStr));
                const RouteTypeRnp::StepType stepType = RouteTypeRnp::StepTypes.fromString(rnpStepTypeStr);

                double longitude = Constants::NaN;
                double latitude = Constants::NaN;
                double centerLongitude = Constants::NaN;
                double centerLatitude = Constants::NaN;
                try { longitude = csv.getCell<double>(row, 5); }
                catch (...) { throw std::invalid_argument("Invalid longitude."); }

                try { latitude = csv.getCell<double>(row, 6); }
                catch (...) { throw std::invalid_argument("Invalid latitude."); }

                if (stepType == RouteTypeRnp::StepType::RadiusToFix)
                {
                    try { centerLongitude = csv.getCell<double>(row, 7); }
                    catch (...) { throw std::invalid_argument("Invalid center longitude."); }

                    try { centerLatitude = csv.getCell<double>(row, 8); }
                    catch (...) { throw std::invalid_argument("Invalid center latitude."); }
                }

                switch (opType)
                {
                case OperationType::Arrival:
                    {
                        if (rwy.ArrivalRoutes.contains(rteName))
                        {
                            if (std::ranges::find(addedArrivalRoutes, rwy.ArrivalRoutes(rteName).get()) == addedArrivalRoutes.end())
                                throw GrapeException(std::format("Arrival route '{}' already exists in runway '{}' of airport '{}'.", rteName, rwyName, aptName));
                        }
                        else
                        {
                            auto& addedRte = study.Airports.addRouteArrivalE(rwy, Route::Type::Rnp, rteName);
                            addedArrivalRoutes.emplace_back(&addedRte);
                        }

                        updateRte = rwy.ArrivalRoutes(rteName).get();
                        auto& rte = *updateRte;
                        RouteRnpInserter rteIns(rte, stepType, longitude, latitude, centerLongitude, centerLatitude);
                        study.Airports.update(rte);
                        break;
                    }
                case OperationType::Departure:
                    {
                        if (rwy.DepartureRoutes.contains(rteName))
                        {
                            if (std::ranges::find(addedDepartureRoutes, rwy.DepartureRoutes(rteName).get()) == addedDepartureRoutes.end())
                                throw GrapeException(std::format("Departure route '{}' already exists in runway '{}' of airport '{}'.", rteName, rwyName, aptName));
                        }
                        else
                        {
                            auto& addedRte = study.Airports.addRouteDepartureE(rwy, Route::Type::Rnp, rteName);
                            addedDepartureRoutes.emplace_back(&addedRte);
                        }

                        updateRte = rwy.DepartureRoutes(rteName).get();
                        auto& rte = *updateRte;
                        RouteRnpInserter rteIns(rte, stepType, longitude, latitude, centerLongitude, centerLatitude);
                        study.Airports.update(rte);
                        break;
                    }
                default: GRAPE_ASSERT(false);
                    break;
                }
            }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing RNP route step at row {}. {}", row + 2, err.what());
                if (updateRte)
                    study.Airports.update(*updateRte);
                csvImp.ErrorCount++;
            }
        }
    }

    void importFlights(const std::string& CsvPath) {
        auto& study = Application::study();
        const Settings& set = Application::settings();

        CsvImport csvImp(CsvPath, "flights", 12);
        if (!csvImp.valid())
            return;
        auto& csv = csvImp.CsvImp;
        const auto columnNames = csv.columnNames();

        for (std::size_t row = 0; row < csv.rowCount(); row++)
        {
            try
            {
                auto opId = csv.getCell<std::string>(row, 0);

                auto opTypeStr = csv.getCell<std::string>(row, 1);
                if (!OperationTypes.contains(opTypeStr))
                    throw GrapeException(std::format("Invalid operation type '{}'.", opTypeStr));
                const OperationType opType = OperationTypes.fromString(opTypeStr);

                bool hasRoute = true;

                auto aptName = csv.getCell<std::string>(row, 2);
                if (aptName.empty())
                    hasRoute = false;
                if (hasRoute && !study.Airports().contains(aptName))
                    throw GrapeException(std::format("Airport '{}' does not exist in this study.", aptName));

                auto rwyName = csv.getCell<std::string>(row, 3);
                if (rwyName.empty())
                    hasRoute = false;
                if (hasRoute && !study.Airports(aptName).Runways.contains(rwyName))
                    throw GrapeException(std::format("Runway '{}' does not exist in airport '{}'.", rwyName, aptName));

                auto rteName = csv.getCell<std::string>(row, 4);
                if (rteName.empty())
                    hasRoute = false;

                // 5 = Time
                // 6 = Count

                auto fleetId = csv.getCell<std::string>(row, 7);
                if (!study.Aircrafts().contains(fleetId))
                    throw GrapeException(std::format("Aircraft '{}' does not exist in this study.", fleetId));
                const Aircraft& acft = study.Aircrafts(fleetId);

                // 8 = Weight

                auto doc29ProfName = csv.getCell<std::string>(row, 9);

                switch (opType)
                {
                case OperationType::Arrival:
                    {
                        auto& op = study.Operations.addArrivalFlightE(opId, acft);

                        try
                        {
                            if (hasRoute)
                            {
                                const auto& rwy = study.Airports(aptName).Runways(rwyName);
                                if (!rwy.ArrivalRoutes.contains(rteName))
                                    throw GrapeException(std::format("Arrival route '{}' does not exist in runway '{}' of airport '{}'.", rteName, rwyName, aptName));
                                op.setRoute(rwy.ArrivalRoutes(rteName).get());
                            }

                            op.setTime(csv.getCell<std::string>(row, 5));

                            try { op.setCount(csv.getCell<double>(row, 6)); }
                            catch (const GrapeException&) { throw; }
                            catch (...) { throw std::invalid_argument("Invalid count."); }

                            try { op.setWeight(set.WeightUnits.toSi(csv.getCell<double>(row, 8), columnNames.at(8))); }
                            catch (const GrapeException&) { throw; }
                            catch (...) { throw std::invalid_argument("Invalid weight."); }

                            if (!doc29ProfName.empty())
                            {
                                if (!acft.Doc29Acft->ArrivalProfiles.contains(doc29ProfName))
                                    throw GrapeException(std::format("Doc29 arrival profile '{}' does not exist in Doc29 performance '{}' associated with aircraft '{}'", doc29ProfName, acft.Doc29Acft->Name, fleetId));
                                study.Operations.setDoc29Profile(op, acft.Doc29Acft->ArrivalProfiles(doc29ProfName).get());
                            }

                            study.Operations.update(op);
                        }
                        catch (...)
                        {
                            study.Operations.erase(op);
                            throw;
                        }
                        break;
                    }
                case OperationType::Departure:
                    {
                        auto& op = study.Operations.addDepartureFlightE(opId, acft);

                        try
                        {
                            if (hasRoute)
                            {
                                const auto& rwy = study.Airports(aptName).Runways(rwyName);
                                if (!rwy.DepartureRoutes.contains(rteName))
                                    throw GrapeException(std::format("Departure route '{}' does not exist in runway '{}' of airport '{}'.", rteName, rwyName, aptName));
                                op.setRoute(rwy.DepartureRoutes(rteName).get());
                            }

                            op.setTime(csv.getCell<std::string>(row, 5));

                            try { op.setCount(csv.getCell<double>(row, 6)); }
                            catch (const GrapeException&) { throw; }
                            catch (...) { throw std::invalid_argument("Invalid count."); }

                            try { op.setWeight(set.WeightUnits.toSi(csv.getCell<double>(row, 8), columnNames.at(8))); }
                            catch (const GrapeException&) { throw; }
                            catch (...) { throw std::invalid_argument("Invalid weight."); }

                            if (!doc29ProfName.empty())
                            {
                                if (!acft.Doc29Acft->DepartureProfiles.contains(doc29ProfName))
                                    throw GrapeException(std::format("Doc29 departure profile '{}' does not exist in Doc29 performance '{}' associated with aircraft '{}'", doc29ProfName, acft.Doc29Acft->Name, fleetId));
                                study.Operations.setDoc29Profile(op, acft.Doc29Acft->DepartureProfiles(doc29ProfName).get());
                            }

                            if (!csv.getCell<std::string>(row, 10).empty())
                            {
                                try { op.setThrustPercentageTakeoff(csv.getCell<double>(row, 10)); }
                                catch (const GrapeException&) { throw; }
                                catch (...) { throw std::invalid_argument("Invalid thrust percentage for takeoff."); }
                            }

                            if (!csv.getCell<std::string>(row, 11).empty())
                            {
                                try { op.setThrustPercentageClimb(csv.getCell<double>(row, 11)); }
                                catch (const GrapeException&) { throw; }
                                catch (...) { throw std::invalid_argument("Invalid thrust percentage for climb."); }
                            }

                            study.Operations.update(op);
                        }
                        catch (...)
                        {
                            study.Operations.erase(op);
                            throw;
                        }
                        break;
                    }
                default: GRAPE_ASSERT(false);
                }
            }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing flight at row {}. {}", row + 2, err.what());
                csvImp.ErrorCount++;
            }
        }
    }

    void importTracks4d(const std::string& CsvPath) {
        auto& study = Application::study();

        CsvImport csvImp(CsvPath, "tracks 4D", 5);
        if (!csvImp.valid())
            return;
        auto& csv = csvImp.CsvImp;

        for (std::size_t row = 0; row < csv.rowCount(); row++)
        {
            try
            {
                auto opId = csv.getCell<std::string>(row, 0);

                auto opTypeStr = csv.getCell<std::string>(row, 1);
                if (!OperationTypes.contains(opTypeStr))
                    throw GrapeException(std::format("Invalid operation type '{}'.", opTypeStr));

                // 2 = Time
                // 3 = Count

                auto fleetId = csv.getCell<std::string>(row, 4);
                if (!study.Aircrafts().contains(fleetId))
                    throw GrapeException(std::format("Aircraft '{}' does not exist in this study.", fleetId));
                const Aircraft& acft = study.Aircrafts(fleetId);

                switch (OperationTypes.fromString(opTypeStr))
                {
                case OperationType::Arrival:
                    {
                        auto& op = study.Operations.addArrivalTrack4dE(opId, acft);

                        try
                        {
                            op.setTime(csv.getCell<std::string>(row, 2));

                            try { op.setCount(csv.getCell<double>(row, 3)); }
                            catch (const GrapeException&) { throw; }
                            catch (...) { throw std::invalid_argument("Invalid count."); }

                            study.Operations.update(op);
                        }
                        catch (...)
                        {
                            study.Operations.erase(op);
                            throw;
                        }
                        break;
                    }
                case OperationType::Departure:
                    {
                        auto& op = study.Operations.addDepartureTrack4dE(opId, acft);

                        try
                        {
                            op.setTime(csv.getCell<std::string>(row, 2));

                            try { op.setCount(csv.getCell<double>(row, 3)); }
                            catch (const GrapeException&) { throw; }
                            catch (...) { throw std::invalid_argument("Invalid count."); }

                            study.Operations.update(op);
                        }
                        catch (...)
                        {
                            study.Operations.erase(op);
                            throw;
                        }
                        break;
                    }
                default: GRAPE_ASSERT(false);
                    break;
                }
            }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing track 4D at row {}. {}", row + 2, err.what());
                csvImp.ErrorCount++;
            }
        }
    }

    void importTracks4dPoints(const std::string& CsvPath) {
        auto& study = Application::study();
        const Settings& set = Application::settings();

        CsvImport csvImp(CsvPath, "tracks 4D points", 10);
        if (!csvImp.valid())
            return;
        auto& csv = csvImp.CsvImp;
        const auto columnNames = csv.columnNames();

        for (std::size_t row = 0; row < csv.rowCount(); row++)
        {
            try
            {
                auto opId = csv.getCell<std::string>(row, 0);
                if (opId.empty())
                    throw GrapeException("Empty name not allowed.");

                auto opTypeStr = csv.getCell<std::string>(row, 1);
                if (!OperationTypes.contains(opTypeStr))
                    throw GrapeException(std::format("Invalid operation type '{}'.", opTypeStr));

                Track4d* op = nullptr;
                switch (OperationTypes.fromString(opTypeStr))
                {
                case OperationType::Arrival:
                    {
                        if (!study.Operations.track4dArrivals().contains(opId))
                            throw GrapeException(std::format("Track 4D arrival operation '{}' doesn't exist in this study.", opId));

                        op = &study.Operations.track4dArrivals().at(opId);
                        break;
                    }
                case OperationType::Departure:
                    {
                        if (!study.Operations.track4dDepartures().contains(opId))
                            throw GrapeException(std::format("Track 4D departure operation '{}' doesn't exist in this study.", opId));

                        op = &study.Operations.track4dDepartures().at(opId);
                        break;
                    }
                default: GRAPE_ASSERT(false);
                }

                Track4d::Point pt;
                auto flPhaseStr = csv.getCell<std::string>(row, 2);
                if (!FlightPhases.contains(flPhaseStr))
                    throw GrapeException(std::format("Invalid flight phase '{}'", flPhaseStr));
                pt.FlPhase = FlightPhases.fromString(flPhaseStr);

                try { pt.CumulativeGroundDistance = set.DistanceUnits.toSi(csv.getCell<double>(row, 3), columnNames.at(3)); }
                catch (...) { throw std::invalid_argument("Invalid cumulative ground distance."); }

                try { pt.setLongitude(csv.getCell<double>(row, 4)); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid longitude."); }

                try { pt.setLatitude(csv.getCell<double>(row, 5)); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid latitude."); }

                try { pt.AltitudeMsl = set.AltitudeUnits.toSi(csv.getCell<double>(row, 6), columnNames.at(6)); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid altitude MSL."); }

                try { pt.setTrueAirspeed(set.SpeedUnits.toSi(csv.getCell<double>(row, 7), columnNames.at(7))); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid true airspeed."); }

                try { pt.setGroundspeed(set.SpeedUnits.toSi(csv.getCell<double>(row, 8), columnNames.at(8))); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid groundspeed."); }

                try { pt.CorrNetThrustPerEng = set.ThrustUnits.toSi(csv.getCell<double>(row, 9), columnNames.at(9)); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid thrust."); }

                try { pt.setBankAngle(csv.getCell<double>(row, 10)); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid bank angle."); }

                try { pt.setFuelFlowPerEng(set.FuelFlowUnits.toSi(csv.getCell<double>(row, 11), columnNames.at(11))); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid fuel flow per engine."); }

                op->addPoint(pt);
                study.Operations.update(*op);
            }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing track 4D point at row {}. {}", row + 2, err.what());
                csvImp.ErrorCount++;
            }
        }
    }

    void importScenarios(const std::string& CsvPath) {
        auto& study = Application::study();

        CsvImport csvImp(CsvPath, "scenarios", 1);
        if (!csvImp.valid())
            return;
        auto& csv = csvImp.CsvImp;

        for (std::size_t row = 0; row < csv.rowCount(); row++)
        {
            try { study.Scenarios.addScenarioE(csv.getCell<std::string>(row, 0)); }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing scenario at row {}. {}", row + 2, err.what());
                csvImp.ErrorCount++;
            }
        }
    }

    void importScenariosOperations(const std::string& CsvPath) {
        auto& study = Application::study();

        CsvImport csvImp(CsvPath, "scenario operations", 4);
        if (!csvImp.valid())
            return;
        auto& csv = csvImp.CsvImp;

        for (std::size_t row = 0; row < csv.rowCount(); row++)
        {
            try
            {
                auto scenName = csv.getCell<std::string>(row, 0);
                auto& scen = study.Scenarios().contains(scenName) ? study.Scenarios().at(scenName) : study.Scenarios.addScenarioE(scenName);

                auto opName = csv.getCell<std::string>(row, 1);

                auto opTypeStr = csv.getCell<std::string>(row, 2);
                if (!OperationTypes.contains(opTypeStr))
                    throw GrapeException(std::format("Invalid operation '{}'.", opTypeStr));

                auto typeStr = csv.getCell<std::string>(row, 3);
                if (!Operation::Types.contains(typeStr))
                    throw GrapeException(std::format("Invalid operation type '{}'.", opTypeStr));

                switch (OperationTypes.fromString(opTypeStr))
                {
                case OperationType::Arrival:
                    {
                        switch (Operation::Types.fromString(typeStr))
                        {
                        case Operation::Type::Flight: study.Scenarios.addFlightArrivalE(scen, opName);
                            break;
                        case Operation::Type::Track4d: study.Scenarios.addTrack4dArrivalE(scen, opName);
                            break;
                        default: GRAPE_ASSERT(false);
                            break;
                        }
                        break;
                    }
                case OperationType::Departure:
                    {
                        switch (Operation::Types.fromString(typeStr))
                        {
                        case Operation::Type::Flight: study.Scenarios.addFlightDepartureE(scen, opName);
                            break;
                        case Operation::Type::Track4d: study.Scenarios.addTrack4dDepartureE(scen, opName);
                            break;
                        default: GRAPE_ASSERT(false);
                            break;
                        }
                        break;
                    }
                default: GRAPE_ASSERT(false);
                    break;
                }
            }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing scenario operation at row {}. {}", row + 2, err.what());
                csvImp.ErrorCount++;
            }
        }
    }

    void importPerformanceRuns(const std::string& CsvPath) {
        auto& study = Application::study();
        const Settings& set = Application::settings();

        CsvImport csvImp(CsvPath, "performance runs", 20);
        if (!csvImp.valid())
            return;
        auto& csv = csvImp.CsvImp;
        const auto columnNames = csv.columnNames();

        for (std::size_t row = 0; row < csv.rowCount(); row++)
        {
            const PerformanceRun* newPerfRunPtr = nullptr;
            try
            {
                auto scenName = csv.getCell<std::string>(row, 0);
                if (scenName.empty())
                    throw GrapeException("Empty scenario name not allowed.");
                if (!study.Scenarios().contains(scenName))
                    throw GrapeException(std::format("Scenario '{}' does not exist in this study.", scenName));
                auto& scen = study.Scenarios().at(scenName);

                auto perfRunName = csv.getCell<std::string>(row, 1);
                auto& newPerfRun = study.Scenarios.addPerformanceRunE(scen, perfRunName);
                newPerfRunPtr = &newPerfRun;

                auto csTypeStr = csv.getCell<std::string>(row, 2);
                if (!CoordinateSystem::Types.contains(csTypeStr))
                    throw GrapeException(std::format("Invalid coordinate system type '{}'.", csTypeStr));

                switch (CoordinateSystem::Types.fromString(csTypeStr))
                {
                case CoordinateSystem::Type::Geodesic: newPerfRun.PerfRunSpec.CoordSys = std::make_unique<Geodesic>();
                    break;
                case CoordinateSystem::Type::LocalCartesian:
                    {
                        auto cs = std::make_unique<LocalCartesian>(0.0, 0.0);
                        double lon0, lat0;
                        try { lon0 = csv.getCell<double>(row, 3); }
                        catch (...) { throw std::invalid_argument("Invalid longitude."); }
                        try { lat0 = csv.getCell<double>(row, 4); }
                        catch (...) { throw std::invalid_argument("Invalid latitude."); }

                        try { cs->resetE(lon0, lat0); }
                        catch (const GrapeException&) { throw; }

                        newPerfRun.PerfRunSpec.CoordSys = std::move(cs);
                        break;
                    }
                default: GRAPE_ASSERT(false);
                }

                auto filterMinAltStr = csv.getCell<std::string>(row, 5);
                if (!filterMinAltStr.empty())
                {
                    try { newPerfRun.PerfRunSpec.setFilterMinimumAltitude(set.AltitudeUnits.toSi(csv.getCell<double>(row, 5), columnNames.at(5))); }
                    catch (const GrapeException&) { throw; }
                    catch (...) { throw std::invalid_argument("Invalid minimum altitude."); }
                }

                auto filterMaxAltStr = csv.getCell<std::string>(row, 6);
                if (!filterMaxAltStr.empty())
                {
                    try { newPerfRun.PerfRunSpec.setFilterMaximumAltitude(set.AltitudeUnits.toSi(csv.getCell<double>(row, 6), columnNames.at(6))); }
                    catch (const GrapeException&) { throw; }
                    catch (...) { throw std::invalid_argument("Invalid maximum altitude."); }
                }

                auto filterMinCumGroundDistStr = csv.getCell<std::string>(row, 7);
                if (!filterMinCumGroundDistStr.empty())
                {
                    try { newPerfRun.PerfRunSpec.setFilterMinimumCumulativeGroundDistance(set.DistanceUnits.toSi(csv.getCell<double>(row, 7), columnNames.at(7))); }
                    catch (const GrapeException&) { throw; }
                    catch (...) { throw std::invalid_argument("Invalid minimum cumulative ground distance."); }
                }

                auto filterMaxCumGroundDistStr = csv.getCell<std::string>(row, 8);
                if (!filterMaxCumGroundDistStr.empty())
                {
                    try { newPerfRun.PerfRunSpec.setFilterMaximumCumulativeGroundDistance(set.DistanceUnits.toSi(csv.getCell<double>(row, 8), columnNames.at(8))); }
                    catch (const GrapeException&) { throw; }
                    catch (...) { throw std::invalid_argument("Invalid maximum cumulative ground distance."); }
                }

                auto filterGroundDistThrStr = csv.getCell<std::string>(row, 9);
                if (!filterGroundDistThrStr.empty())
                {
                    try { newPerfRun.PerfRunSpec.setFilterGroundDistanceThreshold(set.DistanceUnits.toSi(csv.getCell<double>(row, 9), columnNames.at(9))); }
                    catch (const GrapeException&) { throw; }
                    catch (...) { throw std::invalid_argument("Invalid ground distance filter threshold."); }
                }

                auto segmentationSpeedDeltaThrStr = csv.getCell<std::string>(row, 10);
                if (!segmentationSpeedDeltaThrStr.empty())
                {
                    try { newPerfRun.PerfRunSpec.setSegmentationSpeedDeltaThreshold(set.SpeedUnits.toSi(csv.getCell<double>(row, 10), columnNames.at(10))); }
                    catch (const GrapeException&) { throw; }
                    catch (...) { throw std::invalid_argument("Invalid speed delta segmentation threshold."); }
                }

                auto flightsPerfModelStr = csv.getCell<std::string>(row, 11);
                if (!PerformanceModelTypes.contains(flightsPerfModelStr))
                    throw GrapeException(std::format("Invalid performance model '{}'.", flightsPerfModelStr));
                newPerfRun.PerfRunSpec.FlightsPerformanceMdl = PerformanceModelTypes.fromString(flightsPerfModelStr);

                auto flightsDoc29SegmentationStr = csv.getCell<std::string>(row, 12);
                if (!flightsDoc29SegmentationStr.empty())
                {
                    try { newPerfRun.PerfRunSpec.FlightsDoc29Segmentation = static_cast<bool>(csv.getCell<int>(row, 12)); }
                    catch (...) { throw std::invalid_argument("Invalid flights Doc29 segmentation flag."); }
                }

                auto tracks4dCalculatePerformanceStr = csv.getCell<std::string>(row, 13);
                if (!tracks4dCalculatePerformanceStr.empty())
                {
                    try { newPerfRun.PerfRunSpec.Tracks4dCalculatePerformance = static_cast<bool>(csv.getCell<int>(row, 13)); }
                    catch (...) { throw std::invalid_argument("Invalid tracks 4D calculate performance flag."); }
                }

                auto tracks4dMinPointsStr = csv.getCell<std::string>(row, 14);
                if (!tracks4dMinPointsStr.empty())
                {
                    try { newPerfRun.PerfRunSpec.setTracks4dMinimumPoints(csv.getCell<int>(row, 14)); }
                    catch (const GrapeException&) { throw; }
                    catch (...) { throw std::invalid_argument("Invalid tracks 4D minimum points."); }
                }

                auto tracks4dRecalculateCumulativeGroundDistanceStr = csv.getCell<std::string>(row, 15);
                if (!tracks4dRecalculateCumulativeGroundDistanceStr.empty())
                {
                    try { newPerfRun.PerfRunSpec.Tracks4dRecalculateCumulativeGroundDistance = static_cast<bool>(csv.getCell<int>(row, 15)); }
                    catch (...) { throw std::invalid_argument("Invalid tracks 4D recalculate cumulative ground distance flag."); }
                }

                auto tracks4dRecalculateGroundspeedStr = csv.getCell<std::string>(row, 16);
                if (!tracks4dRecalculateGroundspeedStr.empty())
                {
                    try { newPerfRun.PerfRunSpec.Tracks4dRecalculateGroundspeed = static_cast<bool>(csv.getCell<int>(row, 16)); }
                    catch (...) { throw std::invalid_argument("Invalid tracks 4D recalculate groundspeed flag."); }
                }

                auto tracks4dRecalculateFuelFlowStr = csv.getCell<std::string>(row, 17);
                if (!tracks4dRecalculateFuelFlowStr.empty())
                {
                    try { newPerfRun.PerfRunSpec.Tracks4dRecalculateFuelFlow = static_cast<bool>(csv.getCell<int>(row, 17)); }
                    catch (...) { throw std::invalid_argument("Invalid tracks 4D recalculate fuel flow flag."); }
                }

                auto fuelFlowModelStr = csv.getCell<std::string>(row, 18);
                if (!FuelFlowModelTypes.contains(fuelFlowModelStr))
                    throw GrapeException(std::format("Invalid fuel flow model '{}'.", fuelFlowModelStr));
                newPerfRun.PerfRunSpec.FuelFlowMdl = FuelFlowModelTypes.fromString(fuelFlowModelStr);

                auto fuelFlowLTOAltitudeCorrectionStr = csv.getCell<std::string>(row, 19);
                if (!fuelFlowLTOAltitudeCorrectionStr.empty())
                {
                    try { newPerfRun.PerfRunSpec.FuelFlowLTOAltitudeCorrection = static_cast<bool>(csv.getCell<int>(row, 19)); }
                    catch (...) { throw std::invalid_argument("Invalid fuel flow LTO altitude correction flag."); }
                }

                study.Scenarios.update(newPerfRun);
            }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing performance run at row {}. {}", row + 2, err.what());
                if (newPerfRunPtr)
                    study.Scenarios.erase(*newPerfRunPtr);
                csvImp.ErrorCount++;
            }
        }
    }

    void importPerformanceRunsAtmospheres(const std::string& CsvPath) {
        auto& study = Application::study();
        const Settings& set = Application::settings();

        CsvImport csvImp(CsvPath, "performance run atmospheres", 8);
        if (!csvImp.valid())
            return;
        auto& csv = csvImp.CsvImp;
        const auto columnNames = csv.columnNames();

        for (std::size_t row = 0; row < csv.rowCount(); row++)
        {
            try
            {
                const auto scenName = csv.getCell<std::string>(row, 0);
                if (scenName.empty())
                    throw GrapeException("Empty scenario name not allowed.");
                if (!study.Scenarios().contains(scenName))
                    throw GrapeException(std::format("Scenario '{}' does not exist in this study.", scenName));
                auto& scen = study.Scenarios().at(scenName);

                const auto perfRunName = csv.getCell<std::string>(row, 1);
                if (perfRunName.empty())
                    throw GrapeException("Empty performance run name not allowed.");
                if (!scen.PerformanceRuns.contains(perfRunName))
                    throw GrapeException(std::format("Performance run '{}' does not exist in scenario '{}'.", perfRunName, scen.Name));
                auto& perfRun = scen.PerformanceRuns(perfRunName);

                const auto timeStr = csv.getCell<std::string>(row, 2);
                const auto timeOpt = utcStringToTime(timeStr);
                if (!timeOpt)
                    throw GrapeException(std::format("Invalid time '{}'", timeStr));

                Atmosphere atm;

                const auto temperatureDeltaStr = csv.getCell<std::string>(row, 3);
                if (!temperatureDeltaStr.empty())
                {
                    try { atm.setTemperatureDeltaE(set.TemperatureUnits.toSiDelta(csv.getCell<double>(row, 3), columnNames.at(3))); }
                    catch (const GrapeException&) { throw; }
                    catch (...) { throw std::invalid_argument("Invalid temperature delta."); }
                }

                const auto pressureDeltaStr = csv.getCell<std::string>(row, 4);
                if (!pressureDeltaStr.empty())
                {
                    try { atm.setPressureDeltaE(set.PressureUnits.toSiDelta(csv.getCell<double>(row, 4), columnNames.at(4))); }
                    catch (const GrapeException&) { throw; }
                    catch (...) { throw std::invalid_argument("Invalid pressure delta."); }
                }

                try { atm.setWindSpeed(set.SpeedUnits.toSi(csv.getCell<double>(row, 5), columnNames.at(5))); }
                catch (...) { throw std::invalid_argument("Invalid wind speed."); }

                const auto windDirStr = csv.getCell<std::string>(row, 6);
                if (!windDirStr.empty())
                {
                    try { atm.setWindDirectionE(csv.getCell<double>(row, 6)); }
                    catch (const GrapeException&) { throw; }
                    catch (...) { throw std::invalid_argument(std::format("Invalid wind direction '{}'.", windDirStr)); }
                }
                else
                {
                    atm.setConstantHeadwind(atm.windSpeed());
                }

                const auto relHumidityStr = csv.getCell<std::string>(row, 7);
                try { atm.setRelativeHumidityE(csv.getCell<double>(row, 7)); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument(std::format("Invalid relative humidity '{}'.", relHumidityStr)); }

                perfRun.PerfRunSpec.Atmospheres.addAtmosphereE(timeOpt.value(), atm);
                study.Scenarios.update(perfRun);
            }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing performance run atmosphere at row {}. {}", row + 2, err.what());
                ++csvImp.ErrorCount;
            }
        }
    }

    void importNoiseRuns(const std::string& CsvPath) {
        auto& study = Application::study();

        CsvImport csvImp(CsvPath, "noise runs", 7);
        if (!csvImp.valid())
            return;
        auto& csv = csvImp.CsvImp;
        const auto columnNames = csv.columnNames();

        for (std::size_t row = 0; row < csv.rowCount(); row++)
        {
            const NoiseRun* newNsRunPtr = nullptr;
            try
            {
                auto scenName = csv.getCell<std::string>(row, 0);
                if (scenName.empty())
                    throw GrapeException("Empty scenario name not allowed.");
                if (!study.Scenarios().contains(scenName))
                    throw GrapeException(std::format("Scenario '{}' does not exist in this study.", scenName));
                auto& scen = study.Scenarios().at(scenName);

                auto perfRunName = csv.getCell<std::string>(row, 1);
                if (perfRunName.empty())
                    throw GrapeException("Empty performance run name not allowed.");
                if (!scen.PerformanceRuns.contains(perfRunName))
                    throw GrapeException(std::format("Performance run '{}' does not exist in scenario '{}'.", perfRunName, scen.Name));
                auto& perfRun = scen.PerformanceRuns(perfRunName);

                auto nsRunName = csv.getCell<std::string>(row, 2);
                auto& newNsRun = study.Scenarios.addNoiseRunE(perfRun, nsRunName);
                newNsRunPtr = &newNsRun;

                auto nsModelStr = csv.getCell<std::string>(row, 3);
                if (!NoiseModelTypes.contains(nsModelStr))
                    throw GrapeException(std::format("Invalid noise model '{}'.", nsModelStr));
                newNsRun.NsRunSpec.NoiseMdl = NoiseModelTypes.fromString(nsModelStr);

                auto atmAbsStr = csv.getCell<std::string>(row, 4);
                if (!AtmosphericAbsorption::Types.contains(atmAbsStr))
                    throw GrapeException(std::format("Invalid atmospheric absorption type '{}'", atmAbsStr));
                newNsRun.NsRunSpec.AtmAbsorptionType = AtmosphericAbsorption::Types.fromString(atmAbsStr);

                auto receptorTypeStr = csv.getCell<std::string>(row, 5);
                if (!ReceptorSet::Types.contains(receptorTypeStr))
                    throw GrapeException(std::format("Invalid receptor set type '{}'.", receptorTypeStr));

                switch (ReceptorSet::Types.fromString(receptorTypeStr))
                {
                case ReceptorSet::Type::Grid: newNsRun.NsRunSpec.ReceptSet = std::make_unique<ReceptorGrid>();
                    break;
                case ReceptorSet::Type::Points: newNsRun.NsRunSpec.ReceptSet = std::make_unique<ReceptorPoints>();
                    break;
                default: GRAPE_ASSERT(false);
                }

                try { newNsRun.NsRunSpec.SaveSingleMetrics = static_cast<bool>(csv.getCell<int>(row, 6)); }
                catch (...) { throw std::invalid_argument("Invalid value for save single metrics."); }

                study.Scenarios.update(newNsRun);
            }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing noise run at row {}. {}", row + 2, err.what());
                if (newNsRunPtr)
                    study.Scenarios.erase(*newNsRunPtr);
                ++csvImp.ErrorCount;
            }
        }
    }

    void importNoiseRunsReceptorsGrids(const std::string& CsvPath) {
        auto& study = Application::study();
        const auto& set = Application::settings();

        CsvImport csvImp(CsvPath, "grid receptors", 12);
        if (!csvImp.valid())
            return;
        auto& csv = csvImp.CsvImp;
        const auto columnNames = csv.columnNames();

        for (std::size_t row = 0; row < csv.rowCount(); row++)
        {
            try
            {
                auto scenName = csv.getCell<std::string>(row, 0);
                if (scenName.empty())
                    throw GrapeException("Empty scenario name not allowed.");
                if (!study.Scenarios().contains(scenName))
                    throw GrapeException(std::format("Scenario '{}' does not exist in this study.", scenName));
                auto& scen = study.Scenarios().at(scenName);

                auto perfRunName = csv.getCell<std::string>(row, 1);
                if (perfRunName.empty())
                    throw GrapeException("Empty performance run name not allowed.");
                if (!scen.PerformanceRuns.contains(perfRunName))
                    throw GrapeException(std::format("Performance run '{}' does not exist in scenario '{}'.", perfRunName, scen.Name));
                auto& perfRun = scen.PerformanceRuns(perfRunName);

                auto nsRunName = csv.getCell<std::string>(row, 2);
                if (nsRunName.empty())
                    throw GrapeException("Empty noise run name not allowed.");
                if (!perfRun.NoiseRuns.contains(nsRunName))
                    throw GrapeException(std::format("Noise run '{}' does not exist in performance run '{}' of scenario '{}'.", nsRunName, perfRun.Name, scen.Name));
                auto& nsRun = perfRun.NoiseRuns(nsRunName);

                if (nsRun.NsRunSpec.ReceptSet->type() != ReceptorSet::Type::Grid)
                {
                    Log::io()->warn("Receptor set of noise run '{}' of performance run '{}' of scenario '{}' is not of grid type. It will be reset to grid type.", nsRun.Name, perfRun.Name, scen.Name);
                    nsRun.NsRunSpec.ReceptSet = std::make_unique<ReceptorGrid>();
                }
                const auto receptSetGrid = static_cast<ReceptorGrid*>(nsRun.NsRunSpec.ReceptSet.get());

                auto refLocation = csv.getCell<std::string>(row, 3);
                if (!ReceptorGrid::Locations.contains(refLocation))
                    throw GrapeException(std::format("Invalid reference location '{}'.", refLocation));
                receptSetGrid->RefLocation = ReceptorGrid::Locations.fromString(refLocation);

                try { receptSetGrid->setReferenceLongitude(csv.getCell<double>(row, 4)); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid reference longitude."); }

                try { receptSetGrid->setReferenceLatitude(csv.getCell<double>(row, 5)); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid reference latitude."); }

                try { receptSetGrid->RefAltitudeMsl = set.AltitudeUnits.toSi(csv.getCell<double>(row, 6), columnNames.at(6)); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid reference altitude MSL."); }

                try { receptSetGrid->setHorizontalSpacing(set.DistanceUnits.toSi(csv.getCell<double>(row, 7), columnNames.at(7))); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid horizontal spacing."); }

                try { receptSetGrid->setVerticalSpacing(set.DistanceUnits.toSi(csv.getCell<double>(row, 8), columnNames.at(8))); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid vertical spacing."); }

                try { receptSetGrid->setHorizontalCount(csv.getCell<std::size_t>(row, 9)); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid horizontal count."); }

                try { receptSetGrid->setVerticalCount(csv.getCell<std::size_t>(row, 10)); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid vertical count."); }

                try { receptSetGrid->setGridRotation(csv.getCell<double>(row, 11)); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid grid rotation."); }

                study.Scenarios.update(nsRun);
            }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing grid receptors at row {}. {}", row + 2, err.what());
                ++csvImp.ErrorCount;
            }
        }
    }

    void importNoiseRunsReceptorsPoints(const std::string& CsvPath) {
        auto& study = Application::study();
        const auto& set = Application::settings();

        CsvImport csvImp(CsvPath, "point receptors", 7);
        if (!csvImp.valid())
            return;
        auto& csv = csvImp.CsvImp;
        const auto columnNames = csv.columnNames();

        for (std::size_t row = 0; row < csv.rowCount(); row++)
        {
            try
            {
                auto scenName = csv.getCell<std::string>(row, 0);
                if (scenName.empty())
                    throw GrapeException("Empty scenario name not allowed.");
                if (!study.Scenarios().contains(scenName))
                    throw GrapeException(std::format("Scenario '{}' does not exist in this study.", scenName));
                auto& scen = study.Scenarios().at(scenName);

                auto perfRunName = csv.getCell<std::string>(row, 1);
                if (perfRunName.empty())
                    throw GrapeException("Empty performance run name not allowed.");
                if (!scen.PerformanceRuns.contains(perfRunName))
                    throw GrapeException(std::format("Performance run '{}' does not exist in scenario '{}'.", perfRunName, scen.Name));
                auto& perfRun = scen.PerformanceRuns(perfRunName);

                auto nsRunName = csv.getCell<std::string>(row, 2);
                if (nsRunName.empty())
                    throw GrapeException("Empty noise run name not allowed.");
                if (!perfRun.NoiseRuns.contains(nsRunName))
                    throw GrapeException(std::format("Noise run '{}' does not exist in performance run '{}' of scenario '{}'.", nsRunName, perfRun.Name, scen.Name));
                auto& nsRun = perfRun.NoiseRuns(nsRunName);

                if (nsRun.NsRunSpec.ReceptSet->type() != ReceptorSet::Type::Points)
                {
                    Log::io()->warn("Receptor set of noise run '{}' of performance run '{}' of scenario '{}' is not of points type. It will be reset to points type.", nsRun.Name, perfRun.Name, scen.Name);
                    nsRun.NsRunSpec.ReceptSet = std::make_unique<ReceptorPoints>();
                }
                const auto receptSetPts = static_cast<ReceptorPoints*>(nsRun.NsRunSpec.ReceptSet.get());

                auto receptId = csv.getCell<std::string>(row, 3);

                double longitude, latitude, altitudeMsl;
                try { longitude = csv.getCell<double>(row, 4); }
                catch (...) { throw std::invalid_argument("Invalid longitude."); }
                try { latitude = csv.getCell<double>(row, 5); }
                catch (...) { throw std::invalid_argument("Invalid latitude."); }
                try { altitudeMsl = set.AltitudeUnits.toSi(csv.getCell<double>(row, 6), columnNames.at(6)); }
                catch (...) { throw std::invalid_argument("Invalid altitude MSL."); }

                receptSetPts->addPointE(receptId, longitude, latitude, altitudeMsl);
                study.Scenarios.update(nsRun);
            }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing points receptors at row {}. {}", row + 2, err.what());
                ++csvImp.ErrorCount;
            }
        }
    }

    void importNoiseRunsCumulativeMetrics(const std::string& CsvPath) {
        auto& study = Application::study();

        CsvImport csvImp(CsvPath, "noise runs cumulative metrics", 9);
        if (!csvImp.valid())
            return;
        auto& csv = csvImp.CsvImp;

        for (std::size_t row = 0; row < csv.rowCount(); row++)
        {
            const NoiseCumulativeMetric* newNsCumMetricPtr = nullptr;
            try
            {
                auto scenName = csv.getCell<std::string>(row, 0);
                if (scenName.empty())
                    throw GrapeException("Empty scenario name not allowed.");
                if (!study.Scenarios().contains(scenName))
                    throw GrapeException(std::format("Scenario '{}' does not exist in this study.", scenName));
                auto& scen = study.Scenarios().at(scenName);

                auto perfRunName = csv.getCell<std::string>(row, 1);
                if (perfRunName.empty())
                    throw GrapeException("Empty performance run name not allowed.");
                if (!scen.PerformanceRuns.contains(perfRunName))
                    throw GrapeException(std::format("Performance run '{}' does not exist in scenario '{}'.", perfRunName, scen.Name));
                auto& perfRun = scen.PerformanceRuns(perfRunName);

                auto nsRunName = csv.getCell<std::string>(row, 2);
                if (nsRunName.empty())
                    throw GrapeException("Empty noise run name not allowed.");
                if (!perfRun.NoiseRuns.contains(nsRunName))
                    throw GrapeException(std::format("Noise run '{}' does not exist in performance run '{}' of scenario '{}'.", nsRunName, perfRun.Name, scen.Name));
                auto& nsRun = perfRun.NoiseRuns(nsRunName);

                auto& newNsCumMetric = study.Scenarios.addNoiseCumulativeMetricE(nsRun, csv.getCell<std::string>(row, 3));
                newNsCumMetricPtr = &newNsCumMetric;

                try { newNsCumMetric.setThreshold(csv.getCell<double>(row, 4)); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid threshold."); }

                try { newNsCumMetric.setAveragingTimeConstant(csv.getCell<double>(row, 5)); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid averaging time constant."); }

                newNsCumMetric.setStartTimePoint(csv.getCell<std::string>(row, 6));
                newNsCumMetric.setEndTimePoint(csv.getCell<std::string>(row, 7));

                const auto naThrStr = csv.getCell<std::string>(row, 8);
                if (!naThrStr.empty())
                {
                    std::size_t start = 0;
                    std::size_t end = naThrStr.find(" ");
                    do
                    {
                        try { newNsCumMetric.addNumberAboveThresholdE(std::stod(naThrStr.substr(start, end - start))); }
                        catch (const GrapeException&) { throw; }
                        catch (...) { throw std::invalid_argument("Invalid number above threshold."); }
                        start = end + 1;
                        end = naThrStr.find(" ", start);
                    }
                    while (end != std::string::npos);
                }

                study.Scenarios.update(newNsCumMetric);
            }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing noise run cumulative metric at row {}. {}", row + 2, err.what());
                if (newNsCumMetricPtr)
                    study.Scenarios.erase(*newNsCumMetricPtr);
                ++csvImp.ErrorCount;
            }
        }
    }

    void importNoiseRunsCumulativeMetricsWeights(const std::string& CsvPath) {
        auto& study = Application::study();

        CsvImport csvImp(CsvPath, "noise runs cumulative metrics weights", 6);
        if (!csvImp.valid())
            return;
        auto& csv = csvImp.CsvImp;

        for (std::size_t row = 0; row < csv.rowCount(); row++)
        {
            try
            {
                auto scenName = csv.getCell<std::string>(row, 0);
                if (scenName.empty())
                    throw GrapeException("Empty scenario name not allowed.");
                if (!study.Scenarios().contains(scenName))
                    throw GrapeException(std::format("Scenario '{}' does not exist in this study.", scenName));
                auto& scen = study.Scenarios().at(scenName);

                auto perfRunName = csv.getCell<std::string>(row, 1);
                if (perfRunName.empty())
                    throw GrapeException("Empty performance run name not allowed.");
                if (!scen.PerformanceRuns.contains(perfRunName))
                    throw GrapeException(std::format("Performance run '{}' does not exist in scenario '{}'.", perfRunName, scen.Name));
                auto& perfRun = scen.PerformanceRuns(perfRunName);

                auto nsRunName = csv.getCell<std::string>(row, 2);
                if (nsRunName.empty())
                    throw GrapeException("Empty noise run name not allowed.");
                if (!perfRun.NoiseRuns.contains(nsRunName))
                    throw GrapeException(std::format("Noise run '{}' does not exist in performance run '{}' of scenario '{}'.", nsRunName, perfRun.Name, scen.Name));
                auto& nsRun = perfRun.NoiseRuns(nsRunName);

                auto nsCumMetricName = csv.getCell<std::string>(row, 3);
                if (nsCumMetricName.empty())
                    throw GrapeException("Empty noise cumulative metric name not allowed.");
                if (!nsRun.CumulativeMetrics.contains(nsCumMetricName))
                    throw GrapeException(std::format("Noise cumulative metric '{}' does not exist in noise run '{}' of performance run '{}' of scenario '{}'.", nsCumMetricName, nsRunName, perfRun.Name, scen.Name));
                auto& nsCumMetric = nsRun.CumulativeMetrics(nsCumMetricName);

                auto time = Duration(0);
                double weight = Constants::NaN;

                try
                {
                    const auto timeStr = csv.getCell<std::string>(row, 4);
                    const auto timeOpt = stringToDuration(timeStr);
                    if (timeOpt.has_value())
                        time = timeOpt.value();
                    else
                        throw GrapeException(std::format("Invalid time of day '{}'", timeStr));
                }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid time of day."); }

                try { weight = csv.getCell<double>(row, 5); }
                catch (...) { throw std::invalid_argument("Invalid weight."); }

                if (time == Duration(0))
                    nsCumMetric.setBaseWeight(weight);
                else
                    nsCumMetric.addWeightE(time, weight);

                study.Scenarios.update(nsCumMetric);
            }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing noise run cumulative metric weight at row {}. {}", row + 2, err.what());
                ++csvImp.ErrorCount;
            }
        }
    }

    void importEmissionsRuns(const std::string& CsvPath) {
        auto& study = Application::study();
        const auto& set = Application::settings();

        CsvImport csvImp(CsvPath, "emissions runs", 23);
        if (!csvImp.valid())
            return;
        auto& csv = csvImp.CsvImp;
        const auto columnNames = csv.columnNames();

        for (std::size_t row = 0; row < csv.rowCount(); row++)
        {
            const EmissionsRun* newEmiRunPtr = nullptr;
            try
            {
                std::size_t col = 0;
                auto scenName = csv.getCell<std::string>(row, col++);
                if (scenName.empty())
                    throw GrapeException("Empty scenario name not allowed.");
                if (!study.Scenarios().contains(scenName))
                    throw GrapeException(std::format("Scenario '{}' does not exist in this study.", scenName));
                auto& scen = study.Scenarios().at(scenName);

                auto perfRunName = csv.getCell<std::string>(row, col++);
                if (perfRunName.empty())
                    throw GrapeException("Empty performance run name not allowed.");
                if (!scen.PerformanceRuns.contains(perfRunName))
                    throw GrapeException(std::format("Performance run '{}' does not exist in scenario '{}'.", perfRunName, scen.Name));
                auto& perfRun = scen.PerformanceRuns(perfRunName);

                auto emiRunName = csv.getCell<std::string>(row, col++);
                auto& newEmiRun = study.Scenarios.addEmissionsRunE(perfRun, emiRunName);
                newEmiRunPtr = &newEmiRun;

                try { newEmiRun.EmissionsRunSpec.CalculateGasEmissions = static_cast<bool>(csv.getCell<int>(row, col++)); }
                catch (...) { throw std::invalid_argument("Invalid calculate gas emissions flag."); }

                try { newEmiRun.EmissionsRunSpec.CalculateParticleEmissions = static_cast<bool>(csv.getCell<int>(row, col++)); }
                catch (...) { throw std::invalid_argument("Invalid calculate particle emissions flag."); }

                auto emiGasMdlStr = csv.getCell<std::string>(row, col++);
                if (!EmissionsModelTypes.contains(emiGasMdlStr))
                    throw GrapeException(std::format("Invalid emissions model type '{}'", emiGasMdlStr));
                newEmiRun.EmissionsRunSpec.EmissionsMdl = EmissionsModelTypes.fromString(emiGasMdlStr);

                try { newEmiRun.EmissionsRunSpec.BFFM2Model = static_cast<bool>(csv.getCell<int>(row, col++)); }
                catch (...) { throw std::invalid_argument("Invalid use BFFM 2 for gas pollutant EIs flag."); }

                auto emiPtMdlStr = csv.getCell<std::string>(row, col++);
                if (!EmissionsParticleSmokeNumberModelTypes.contains(emiPtMdlStr))
                    throw GrapeException(std::format("Invalid smoke number to particle emission index model type '{}'", emiPtMdlStr));
                newEmiRun.EmissionsRunSpec.ParticleSmokeNumberModel = EmissionsParticleSmokeNumberModelTypes.fromString(emiPtMdlStr);

                try { newEmiRun.EmissionsRunSpec.setLTOCycle(LTOPhase::Idle, csv.getCell<double>(row, col++)); }
                catch (...) { throw std::invalid_argument("Invalid LTO cycle time for idle phase."); }

                try { newEmiRun.EmissionsRunSpec.setLTOCycle(LTOPhase::Approach, csv.getCell<double>(row, col++)); }
                catch (...) { throw std::invalid_argument("Invalid LTO cycle time for approach phase."); }

                try { newEmiRun.EmissionsRunSpec.setLTOCycle(LTOPhase::ClimbOut, csv.getCell<double>(row, col++)); }
                catch (...) { throw std::invalid_argument("Invalid LTO cycle time for climb out phase."); }

                try { newEmiRun.EmissionsRunSpec.setLTOCycle(LTOPhase::Takeoff, csv.getCell<double>(row, col++)); }
                catch (...) { throw std::invalid_argument("Invalid LTO cycle time for takeoff phase."); }

                try { newEmiRun.EmissionsRunSpec.setParticleEffectiveDensity(csv.getCell<double>(row, col++)); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid particle effective density."); }

                try { newEmiRun.EmissionsRunSpec.setParticleGeometricStandardDeviation(csv.getCell<double>(row, col++)); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid particle geometric standard deviation."); }

                try { newEmiRun.EmissionsRunSpec.setParticleGeometricMeanDiameter(LTOPhase::Idle, csv.getCell<double>(row, col++) * 1e-9); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid particle geometric mead diameter for idle phase."); }

                try { newEmiRun.EmissionsRunSpec.setParticleGeometricMeanDiameter(LTOPhase::Approach, csv.getCell<double>(row, col++) * 1e-9); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid particle geometric mead diameter for approach phase."); }

                try { newEmiRun.EmissionsRunSpec.setParticleGeometricMeanDiameter(LTOPhase::ClimbOut, csv.getCell<double>(row, col++) * 1e-9); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid particle geometric mead diameter for climb out phase."); }

                try { newEmiRun.EmissionsRunSpec.setParticleGeometricMeanDiameter(LTOPhase::Takeoff, csv.getCell<double>(row, col++) * 1e-9); }
                catch (const GrapeException&) { throw; }
                catch (...) { throw std::invalid_argument("Invalid particle geometric mead diameter for takeoff phase."); }

                auto minAltStr = csv.getCell<std::string>(row, col++);
                if (!minAltStr.empty())
                {
                    try { newEmiRun.EmissionsRunSpec.setFilterMinimumAltitude(set.AltitudeUnits.toSi(csv.getCell<double>(row, col - 1), columnNames.at(col - 1))); }
                    catch (const GrapeException&) { throw; }
                    catch (...) { throw std::invalid_argument("Invalid minimum altitude."); }
                }

                auto maxAltStr = csv.getCell<std::string>(row, col++);
                if (!maxAltStr.empty())
                {
                    try { newEmiRun.EmissionsRunSpec.setFilterMaximumAltitude(set.AltitudeUnits.toSi(csv.getCell<double>(row, col - 1), columnNames.at(col - 1))); }
                    catch (const GrapeException&) { throw; }
                    catch (...) { throw std::invalid_argument("Invalid maximum altitude."); }
                }

                auto minCumGroundDist = csv.getCell<std::string>(row, col++);
                if (!minCumGroundDist.empty())
                {
                    try { newEmiRun.EmissionsRunSpec.setFilterMinimumCumulativeGroundDistance(set.DistanceUnits.toSi(csv.getCell<double>(row, col - 1), columnNames.at(col - 1))); }
                    catch (const GrapeException&) { throw; }
                    catch (...) { throw std::invalid_argument("Invalid minimum cumulative ground distance."); }
                }

                auto maxCumGroundDist = csv.getCell<std::string>(row, col++);
                if (!maxCumGroundDist.empty())
                {
                    try { newEmiRun.EmissionsRunSpec.setFilterMaximumCumulativeGroundDistance(set.DistanceUnits.toSi(csv.getCell<double>(row, col - 1), columnNames.at(col - 1))); }
                    catch (const GrapeException&) { throw; }
                    catch (...) { throw std::invalid_argument("Invalid maximum cumulative ground distance."); }
                }

                try { newEmiRun.EmissionsRunSpec.SaveSegmentResults = static_cast<bool>(csv.getCell<int>(row, col++)); }
                catch (...) { throw std::invalid_argument("Invalid save segment results flag."); }
            }
            catch (const std::exception& err)
            {
                Log::io()->error("Importing emissions run at row {}. {}", row + 2, err.what());
                if (newEmiRunPtr)
                    study.Scenarios.erase(*newEmiRunPtr);
                csvImp.ErrorCount++;
            }
        }
    }

    void importDoc29Files(const std::string& FolderPath) {
        {
            std::string filePath = std::format("{}/Doc29 Performance.csv", FolderPath);
            if (std::filesystem::exists(filePath))
                Application::get().queueAsyncTask([&, filePath] {
                importDoc29Performance(filePath);
                    }, std::format("Importing Doc29 Performance from '{}'", filePath));
            else
                Log::io()->warn("Doc29 Performance.csv not found in folder '{}'.", FolderPath);
        }
        {
            std::string filePath = std::format("{}/Doc29 Aerodynamic Coefficients.csv", FolderPath);
            if (std::filesystem::exists(filePath))
                Application::get().queueAsyncTask([&, filePath] {
                importDoc29PerformanceAerodynamicCoefficients(filePath);
                    }, std::format("Importing Doc29 aerodynamic coefficients from '{}'", filePath));
            else
                Log::io()->warn("Doc29 Aerodynamic Coefficients.csv not found in folder '{}'.", FolderPath);
        }
        {
            std::string filePath = std::format("{}/Doc29 Thrust Ratings.csv", FolderPath);
            if (std::filesystem::exists(filePath))
                Application::get().queueAsyncTask([&, filePath] {
                importDoc29PerformanceThrustRatings(filePath);
                    }, std::format("Importing Doc29 thrust ratings from '{}'", filePath));
            else
                Log::io()->warn("Doc29 Thrust Ratings.csv not found in folder '{}'.", FolderPath);
        }
        {
            std::string filePath = std::format("{}/Doc29 Thrust Ratings Propeller.csv", FolderPath);
            if (std::filesystem::exists(filePath))
                Application::get().queueAsyncTask([&, filePath] {
                importDoc29PerformanceThrustRatingsPropeller(filePath);
                    }, std::format("Importing Doc29 thrust propeller ratings from '{}'", filePath));
            else
                Log::io()->warn("Doc29 Thrust Ratings Propeller.csv not found in folder '{}'.", FolderPath);
        }
        {
            std::string filePath = std::format("{}/Doc29 Profiles Points.csv", FolderPath);
            if (std::filesystem::exists(filePath))
                Application::get().queueAsyncTask([&, filePath] {
                importDoc29PerformanceProfilesPoints(filePath);
                    }, std::format("Importing Doc29 point profiles from '{}'", filePath));
            else
                Log::io()->warn("Doc29 Profiles Points.csv not found in folder '{}'.", FolderPath);
        }
        {
            std::string filePath = std::format("{}/Doc29 Profiles Procedural Arrival.csv", FolderPath);
            if (std::filesystem::exists(filePath))
                Application::get().queueAsyncTask([&, filePath] {
                importDoc29PerformanceProfilesArrivalSteps(filePath);
                    }, std::format("Importing Doc29 arrival procedural profiles from '{}'", filePath));
            else
                Log::io()->warn("Doc29 Profiles Procedural Arrival.csv not found in folder '{}'.", FolderPath);
        }
        {
            std::string filePath = std::format("{}/Doc29 Profiles Procedural Departure.csv", FolderPath);
            if (std::filesystem::exists(filePath))
                Application::get().queueAsyncTask([&, filePath] {
                importDoc29PerformanceProfilesDepartureSteps(filePath);
                    }, std::format("Importing Doc29 departure procedural profiles from '{}'", filePath));
            else
                Log::io()->warn("Doc29 Profiles Procedural Departure.csv not found in folder '{}'.", FolderPath);
        }
        {
            std::string filePath = std::format("{}/Doc29 Noise.csv", FolderPath);
            if (std::filesystem::exists(filePath))
                Application::get().queueAsyncTask([&, filePath] {
                importDoc29Noise(filePath);
                    }, std::format("Importing Doc29 Noise from '{}'", filePath));
            else
                Log::io()->warn("Doc29 Noise.csv not found in folder '{}'.", FolderPath);
        }
        {
            std::string filePath = std::format("{}/Doc29 Noise NPD.csv", FolderPath);
            if (std::filesystem::exists(filePath))
                Application::get().queueAsyncTask([&, filePath] {
                importDoc29NoiseNpd(filePath);
                    }, std::format("Importing Doc29 NPD data from '{}'", filePath));
            else
                Log::io()->warn("Doc29 Noise NPD.csv not found in folder '{}'.", FolderPath);
        }
        {
            std::string filePath = std::format("{}/Doc29 Noise Spectrum.csv", FolderPath);
            if (std::filesystem::exists(filePath))
                Application::get().queueAsyncTask([&, filePath] {
                importDoc29NoiseSpectrum(filePath);
                    }, std::format("Importing Doc29 noise spectrums from '{}'", filePath));
            else
                Log::io()->warn("Doc29 Noise Spectrum.csv not found in folder '{}'.", FolderPath);
        }
    }

    void importDatasetFiles(const std::string& FolderPath) {
        importDoc29Files(FolderPath);
        {
            std::string filePath = std::format("{}/LTO Engines.csv", FolderPath);
            if (std::filesystem::exists(filePath))
                Application::get().queueAsyncTask([&, filePath] {
                importLTO(filePath);
                    }, std::format("Importing LTO engines from '{}'", filePath));
            else
                Log::io()->warn("LTO Engines.csv not found in folder '{}'.", FolderPath);
        }
        {
            std::string filePath = std::format("{}/SFI.csv", FolderPath);
            if (std::filesystem::exists(filePath))
                Application::get().queueAsyncTask([&, filePath] {
                importSFI(filePath);
                    }, std::format("Importing SFI coefficients from '{}'", filePath));
            else
                Log::io()->warn("SFI.csv not found in folder '{}'.", FolderPath);
        }
        {
            std::string filePath = std::format("{}/Fleet.csv", FolderPath);
            if (std::filesystem::exists(filePath))
                Application::get().queueAsyncTask([&, filePath] {
                importFleet(filePath);
                    }, std::format("Importing fleet from '{}'", filePath));
            else
                Log::io()->warn("Fleet.csv not found in folder '{}'.", FolderPath);
        }
    }

    void importInputDataFiles(const std::string& FolderPath) {
        {
            std::string filePath = std::format("{}/Airports.csv", FolderPath);
            if (std::filesystem::exists(filePath))
                Application::get().queueAsyncTask([&, filePath] {
                importAirports(filePath);
                    }, std::format("Importing airports from '{}'", filePath));
            else
                Log::io()->warn("Airports.csv not found in folder '{}'.", FolderPath);
        }
        {
            std::string filePath = std::format("{}/Runways.csv", FolderPath);
            if (std::filesystem::exists(filePath))
                Application::get().queueAsyncTask([&, filePath] {
                importRunways(filePath);
                    }, std::format("Importing runways from '{}'", filePath));
            else
                Log::io()->warn("Runways.csv not found in folder '{}'.", FolderPath);
        }
        {
            std::string filePath = std::format("{}/Routes Simple.csv", FolderPath);
            if (std::filesystem::exists(filePath))
                Application::get().queueAsyncTask([&, filePath] {
                importRoutesSimple(filePath);
                    }, std::format("Importing simple routes from '{}'", filePath));
            else
                Log::io()->warn("Routes Simple.csv not found in folder '{}'.", FolderPath);
        }
        {
            std::string filePath = std::format("{}/Routes Vector.csv", FolderPath);
            if (std::filesystem::exists(filePath))
                Application::get().queueAsyncTask([&, filePath] {
                importRoutesVectors(filePath);
                    }, std::format("Importing vector routes from '{}'", filePath));
            else
                Log::io()->warn("Routes Vector.csv not found in folder '{}'.", FolderPath);
        }
        {
            std::string filePath = std::format("{}/Routes RNP.csv", FolderPath);
            if (std::filesystem::exists(filePath))
                Application::get().queueAsyncTask([&, filePath] {
                importRoutesRnp(filePath);
                    }, std::format("Importing RNP routes from '{}'", filePath));
            else
                Log::io()->warn("Routes RNP.csv not found in folder '{}'.", FolderPath);
        }
        {
            std::string filePath = std::format("{}/Flights.csv", FolderPath);
            if (std::filesystem::exists(filePath))
                Application::get().queueAsyncTask([&, filePath] {
                importFlights(filePath);
                    }, std::format("Importing flights from '{}'", filePath));
            else
                Log::io()->warn("Flights.csv not found in folder '{}'.", FolderPath);
        }
        {
            std::string filePath = std::format("{}/Tracks 4D.csv", FolderPath);
            if (std::filesystem::exists(filePath))
                Application::get().queueAsyncTask([&, filePath] {
                importTracks4d(filePath);
                    }, std::format("Importing 4D tracks from '{}'", filePath));
            else
                Log::io()->warn("Tracks 4D.csv not found in folder '{}'.", FolderPath);
        }
        {
            std::string filePath = std::format("{}/Tracks 4D Points.csv", FolderPath);
            if (std::filesystem::exists(filePath))
                Application::get().queueAsyncTask([&, filePath] {
                importTracks4dPoints(filePath);
                    }, std::format("Importing 4D tracks points from '{}'", filePath));
            else
                Log::io()->warn("Tracks 4D Points.csv not found in folder '{}'.", FolderPath);
        }
        {
            std::string filePath = std::format("{}/Scenarios.csv", FolderPath);
            if (std::filesystem::exists(filePath))
                Application::get().queueAsyncTask([&, filePath] {
                importScenarios(filePath);
                    }, std::format("Importing scenarios from '{}'", filePath));
            else
                Log::io()->warn("Scenarios.csv not found in folder '{}'.", FolderPath);
        }
        {
            std::string filePath = std::format("{}/Scenarios Operations.csv", FolderPath);
            if (std::filesystem::exists(filePath))
                Application::get().queueAsyncTask([&, filePath] {
                importScenariosOperations(filePath);
                    }, std::format("Importing scenarios operations from '{}'", filePath));
            else
                Log::io()->warn("Scenarios Operations.csv not found in folder '{}'.", FolderPath);
        }
        {
            std::string filePath = std::format("{}/Performance Runs.csv", FolderPath);
            if (std::filesystem::exists(filePath))
                Application::get().queueAsyncTask([&, filePath] {
                importPerformanceRuns(filePath);
                    }, std::format("Importing performance runs from '{}'", filePath));
            else
                Log::io()->warn("Performance Runs.csv not found in folder '{}'.", FolderPath);
        }
        {
            std::string filePath = std::format("{}/Performance Runs Atmospheres.csv", FolderPath);
            if (std::filesystem::exists(filePath))
                Application::get().queueAsyncTask([&, filePath] {
                importPerformanceRunsAtmospheres(filePath);
                    }, std::format("Importing performance runs atmospheres from '{}'", filePath));
            else
                Log::io()->warn("Performance Runs Atmospheres.csv not found in folder '{}'.", FolderPath);
        }
        {
            std::string filePath = std::format("{}/Noise Runs.csv", FolderPath);
            if (std::filesystem::exists(filePath))
                Application::get().queueAsyncTask([&, filePath] {
                importNoiseRuns(filePath);
                    }, std::format("Importing noise runs from '{}'", filePath));
            else
                Log::io()->warn("Noise Runs.csv not found in folder '{}'.", FolderPath);
        }
        {
            std::string filePath = std::format("{}/Noise Runs Grid Receptors.csv", FolderPath);
            if (std::filesystem::exists(filePath))
                Application::get().queueAsyncTask([&, filePath] {
                importNoiseRunsReceptorsGrids(filePath);
                    }, std::format("Importing noise runs grid receptors from '{}'", filePath));
            else
                Log::io()->warn("Noise Runs Grid Receptors.csv not found in folder '{}'.", FolderPath);
        }
        {
            std::string filePath = std::format("{}/Noise Runs Point Receptors.csv", FolderPath);
            if (std::filesystem::exists(filePath))
                Application::get().queueAsyncTask([&, filePath] {
                importNoiseRunsReceptorsPoints(filePath);
                    }, std::format("Importing noise runs point receptors from '{}'", filePath));
            else
                Log::io()->warn("Noise Runs Point Receptors.csv not found in folder '{}'.", FolderPath);
        }
        {
            std::string filePath = std::format("{}/Noise Runs Cumulative Metrics.csv", FolderPath);
            if (std::filesystem::exists(filePath))
                Application::get().queueAsyncTask([&, filePath] {
                importNoiseRunsCumulativeMetrics(filePath);
                    }, std::format("Importing noise runs cumulative metrics from '{}'", filePath));
            else
                Log::io()->warn("Noise Runs Cumulative Metrics.csv not found in folder '{}'.", FolderPath);
        }
        {
            std::string filePath = std::format("{}/Noise Runs Cumulative Metrics Weights.csv", FolderPath);
            if (std::filesystem::exists(filePath))
                Application::get().queueAsyncTask([&, filePath] {
                importNoiseRunsCumulativeMetricsWeights(filePath);
                    }, std::format("Importing noise runs cumulative metrics weights from '{}'", filePath));
            else
                Log::io()->warn("Noise Runs Cumulative Metrics Weights.csv not found in folder '{}'.", FolderPath);
        }
        {
            std::string filePath = std::format("{}/Emissions Runs.csv", FolderPath);
            if (std::filesystem::exists(filePath))
                Application::get().queueAsyncTask([&, filePath] {
                importEmissionsRuns(filePath);
                    }, std::format("Importing emissions runs from '{}'", filePath));
            else
                Log::io()->warn("Emissions Runs.csv not found in folder '{}'.", FolderPath);
        }
    }

    void importAllFiles(const std::string& FolderPath) {
        importDatasetFiles(FolderPath);
        importInputDataFiles(FolderPath);
    }
}
