// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "CsvExport.h"

#include "Application.h"
#include "Csv.h"
#include "Performance/PerformanceOutput.h"

namespace GRAPE::IO::CSV {
    void exportDoc29Performance(const std::string& CsvPath) {
        const auto& study = Application::study();
        const auto& set = Application::settings();

        Csv csv;
        try { csv.setExport(CsvPath); }
        catch (const std::exception& err)
        {
            Log::io()->error("Exporting Doc29 Aircraft to '{}'. {}", CsvPath, err.what());
            return;
        }

        csv.setColumnNames(
            "ID",
            std::format("Maximum Sea Level Static Thrust ({})", set.ThrustUnits.shortName()),
            "Thrust Type",
            std::format("Engine Breakpoint Temperature ({})", set.TemperatureUnits.shortName())
        );

        std::size_t row = 0;
        for (const auto& doc29Acft : study.Doc29Aircrafts)
        {
            csv.setCell(row, 0, doc29Acft.Name);
            csv.setCell(row, 1, set.ThrustUnits.fromSi(doc29Acft.MaximumSeaLevelStaticThrust));
            csv.setCell(row, 2, Doc29Thrust::Types.toString(doc29Acft.thrust().type()));
            csv.setCell(row, 3, set.TemperatureUnits.fromSi(doc29Acft.EngineBreakpointTemperature));

            ++row;
        }

        if (row)
        {
            csv.write();
            Log::io()->info("Exported Doc29 Aircraft to '{}'.", CsvPath);
        }
    }

    void exportDoc29PerformanceAerodynamicCoefficients(const std::string& CsvPath) {
        const auto& study = Application::study();
        const auto& set = Application::settings();

        Csv csv;
        try { csv.setExport(CsvPath); }
        catch (const std::exception& err)
        {
            Log::io()->error("Exporting Doc29 aerodynamic coefficients to '{}'. {}", CsvPath, err.what());
            return;
        }

        csv.setColumnNames(
            "Doc29 Performance ID",
            "ID",
            "Type",
            "R",
            std::format("B ({})", set.Doc29AeroBUnits.shortName()),
            std::format("C ({})", set.Doc29AeroCDUnits.shortName()),
            std::format("D ({})", set.Doc29AeroCDUnits.shortName())
        );

        std::size_t row = 0;
        for (const auto& doc29Acft : study.Doc29Aircrafts)
        {
            for (const auto& [coeffsId, coeffs] : doc29Acft.AerodynamicCoefficients)
            {
                csv.setCell(row, 0, doc29Acft.Name);
                csv.setCell(row, 1, coeffsId);
                csv.setCell(row, 2, Doc29AerodynamicCoefficients::Types.toString(coeffs.CoefficientType));
                csv.setCell(row, 3, coeffs.R);
                if (coeffs.CoefficientType == Doc29AerodynamicCoefficients::Type::Takeoff)
                {
                    csv.setCell(row, 4, set.Doc29AeroBUnits.fromSi(coeffs.B));
                    csv.setCell(row, 5, set.Doc29AeroCDUnits.fromSi(coeffs.C));
                }
                if (coeffs.CoefficientType == Doc29AerodynamicCoefficients::Type::Land)
                {
                    csv.setCell(row, 6, set.Doc29AeroCDUnits.fromSi(coeffs.D));
                }
                ++row;
            }
        }

        if (row)
        {
            csv.write();
            Log::io()->info("Exported Doc29 aerodynamic coefficients to '{}'.", CsvPath);
        }
    }

    namespace {
        struct Doc29ThrustExporter : Doc29ThrustVisitor {
            Doc29ThrustExporter(Csv& CsvFile, std::size_t& Row, const Doc29Aircraft& Doc29Acft, const Doc29Thrust& Doc29Thr) : m_Csv(CsvFile), m_Row(Row), m_Doc29Acft(Doc29Acft) { Doc29Thr.accept(*this); }
            void visitDoc29ThrustRating(const Doc29ThrustRating& Doc29Thr) override;
            void visitDoc29ThrustPropeller(const Doc29ThrustRatingPropeller& Doc29Thr) override;
        private:
            Csv& m_Csv;
            std::size_t& m_Row;
            const Doc29Aircraft& m_Doc29Acft;
        };
    }

    void exportDoc29PerformanceThrustRatings(const std::string& CsvPath) {
        const auto& study = Application::study();
        const auto& set = Application::settings();

        Csv csv;
        try { csv.setExport(CsvPath); }
        catch (const std::exception& err)
        {
            Log::io()->error("Exporting Doc29 thrust ratings to '{}'. {}", CsvPath, err.what());
            return;
        }

        csv.setColumnNames(
            "Doc29 Aircraft ID",
            "Thrust Rating",
            std::format("E ({})", set.ThrustUnits.shortName()),
            std::format("F ({})", set.Doc29ThrustFUnits.shortName()),
            std::format("Ga ({})", set.Doc29ThrustGaUnits.shortName()),
            std::format("Gb ({})", set.Doc29ThrustGbUnits.shortName()),
            std::format("H ({})", set.Doc29ThrustHUnits.shortName())
        );

        std::size_t row = 0;
        for (const auto& doc29Acft : study.Doc29Aircrafts)
        {
            if (doc29Acft.thrust().type() == Doc29Thrust::Type::Rating)
                Doc29ThrustExporter(csv, row, doc29Acft, doc29Acft.thrust());
        }

        if (row)
        {
            csv.write();
            Log::io()->info("Exported Doc29 thrust ratings to '{}'.", CsvPath);
        }
    }
    void Doc29ThrustExporter::visitDoc29ThrustRating(const Doc29ThrustRating& Doc29Thr) {
        const auto& set = Application::settings();

        Csv& csv = m_Csv;
        std::size_t& row = m_Row;

        for (const auto& [rating, coeffs] : Doc29Thr)
        {
            csv.setCell(row, 0, m_Doc29Acft.Name);
            csv.setCell(row, 1, Doc29Thrust::Ratings.toString(rating));
            csv.setCell(row, 2, set.ThrustUnits.fromSi(coeffs.E));
            csv.setCell(row, 3, set.Doc29ThrustFUnits.fromSi(coeffs.F));
            csv.setCell(row, 4, set.Doc29ThrustGaUnits.fromSi(coeffs.Ga));
            csv.setCell(row, 5, set.Doc29ThrustGbUnits.fromSi(coeffs.Gb));
            csv.setCell(row, 6, set.Doc29ThrustHUnits.fromSi(coeffs.H));
            ++row;
        }
    }

    void exportDoc29PerformanceThrustRatingsPropeller(const std::string& CsvPath) {
        const auto& study = Application::study();
        const auto& set = Application::settings();

        Csv csv;
        try { csv.setExport(CsvPath); }
        catch (const std::exception& err)
        {
            Log::io()->error("Exporting Doc29 thrust propeller ratings to '{}'. {}", CsvPath, err.what());
            return;
        }

        csv.setColumnNames(
            "Doc29 Aircraft ID",
            "Thrust Rating",
            "Propeller Efficiency",
            std::format("Propeller Power ({})", set.PowerUnits.shortName())
        );

        std::size_t row = 0;
        for (const auto& doc29Acft : study.Doc29Aircrafts)
        {
            if (doc29Acft.thrust().type() == Doc29Thrust::Type::RatingPropeller)
                Doc29ThrustExporter(csv, row, doc29Acft, doc29Acft.thrust());
        }

        if (row)
        {
            csv.write();
            Log::io()->info("Exported Doc29 thrust propeller ratings to '{}'.", CsvPath);
        }
    }

    void Doc29ThrustExporter::visitDoc29ThrustPropeller(const Doc29ThrustRatingPropeller& Doc29Thr) {
        const auto& set = Application::settings();

        Csv& csv = m_Csv;
        std::size_t& row = m_Row;

        for (const auto& [rating, coeffs] : Doc29Thr)
        {
            csv.setCell(row, 0, m_Doc29Acft.Name);
            csv.setCell(row, 1, Doc29Thrust::Ratings.toString(rating));
            csv.setCell(row, 2, coeffs.Pe);
            csv.setCell(row, 3, set.PowerUnits.fromSi(coeffs.Pp));
            ++row;
        }
    }

    namespace {
        struct Doc29ProfileExporter : Doc29ProfileVisitor {
            Doc29ProfileExporter(Csv& CsvFile, std::size_t& Row, const Doc29Profile& Doc29Prof) : m_Csv(CsvFile), m_Row(Row) { Doc29Prof.accept(*this); }
            void visitDoc29ProfileArrivalPoints(const Doc29ProfileArrivalPoints& Doc29Prof) override;
            void visitDoc29ProfileDeparturePoints(const Doc29ProfileDeparturePoints& Doc29Prof) override;
            void visitDoc29ProfileArrivalProcedural(const Doc29ProfileArrivalProcedural& Doc29Prof) override;
            void visitDoc29ProfileDepartureProcedural(const Doc29ProfileDepartureProcedural& Doc29Prof) override;
        private:
            Csv& m_Csv;
            std::size_t& m_Row;
        };
    }

    void exportDoc29PerformanceProfilesPoints(const std::string& CsvPath) {
        const auto& study = Application::study();
        const auto& set = Application::settings();

        Csv csv;
        try { csv.setExport(CsvPath); }
        catch (const std::exception& err)
        {
            Log::io()->error("Exporting Doc29 point profiles to '{}'. {}", CsvPath, err.what());
            return;
        }

        csv.setColumnNames(
            "Doc29 Aircraft ID",
            "Operation",
            "Profile ID",
            std::format("Cumulative Ground Distance ({})", set.DistanceUnits.shortName()),
            std::format("Altitude ATE ({})", set.AltitudeUnits.shortName()),
            std::format("True Airspeed ({})", set.SpeedUnits.shortName()),
            std::format("Corrected Net Thrust per Engine ({})", set.ThrustUnits.shortName())
        );

        std::size_t row = 0;

        for (const auto& doc29Acft : study.Doc29Aircrafts)
        {
            for (const auto& [profName, arrProfPtr] : doc29Acft.ArrivalProfiles)
            {
                const auto& arrProf = *arrProfPtr;
                if (arrProf.type() == Doc29Profile::Type::Points)
                    Doc29ProfileExporter(csv, row, arrProf);
            }
            for (const auto& [profName, depProfPtr] : doc29Acft.DepartureProfiles)
            {
                const auto& depProf = *depProfPtr;
                if (depProf.type() == Doc29Profile::Type::Points)
                    Doc29ProfileExporter(csv, row, depProf);
            }
        }

        if (row)
        {
            csv.write();
            Log::io()->info("Exported Doc29 point profiles to '{}'.", CsvPath);
        }
    }

    void Doc29ProfileExporter::visitDoc29ProfileArrivalPoints(const Doc29ProfileArrivalPoints& Doc29Prof) {
        const auto& set = Application::settings();

        Csv& csv = m_Csv;
        std::size_t& row = m_Row;

        for (const auto& [cumDist, pt] : Doc29Prof)
        {
            csv.setCell(row, 0, Doc29Prof.parentDoc29Performance().Name);
            csv.setCell(row, 1, OperationTypes.toString(Doc29Prof.operationType()));
            csv.setCell(row, 2, Doc29Prof.Name);
            csv.setCell(row, 3, set.DistanceUnits.fromSi(cumDist));
            csv.setCell(row, 4, set.AltitudeUnits.fromSi(pt.AltitudeAfe));
            csv.setCell(row, 5, set.SpeedUnits.fromSi(pt.TrueAirspeed));
            csv.setCell(row, 6, set.ThrustUnits.fromSi(pt.CorrNetThrustPerEng));
            ++row;
        }
    }

    void Doc29ProfileExporter::visitDoc29ProfileDeparturePoints(const Doc29ProfileDeparturePoints& Doc29Prof) {
        const auto& set = Application::settings();

        Csv& csv = m_Csv;
        std::size_t& row = m_Row;

        for (const auto& [cumDist, pt] : Doc29Prof)
        {
            csv.setCell(row, 0, Doc29Prof.parentDoc29Performance().Name);
            csv.setCell(row, 1, OperationTypes.toString(Doc29Prof.operationType()));
            csv.setCell(row, 2, Doc29Prof.Name);
            csv.setCell(row, 3, set.DistanceUnits.fromSi(cumDist));
            csv.setCell(row, 4, set.AltitudeUnits.fromSi(pt.AltitudeAfe));
            csv.setCell(row, 5, set.SpeedUnits.fromSi(pt.TrueAirspeed));
            csv.setCell(row, 6, set.ThrustUnits.fromSi(pt.CorrNetThrustPerEng));
            ++row;
        }
    }

    void exportDoc29PerformanceProfilesArrivalSteps(const std::string& CsvPath) {
        const auto& study = Application::study();
        const auto& set = Application::settings();

        Csv csv;
        try { csv.setExport(CsvPath); }
        catch (const std::exception& err)
        {
            Log::io()->error("Exporting Doc29 arrival procedural profiles to '{}'. {}", CsvPath, err.what());
            return;
        }

        csv.setColumnNames(
            "Doc29 Aircraft ID",
            "Profile ID",
            "Step Type",
            "Aerodynamic Coefficient ID",
            std::format("Start Altitude ATE ({})", set.AltitudeUnits.shortName()),
            "Descent Angle",
            std::format("Start Calibrated Airspeed ({})", set.SpeedUnits.shortName()),
            std::format("Ground Distance ({})", set.DistanceUnits.shortName()),
            "Descend Land - Descent Angle",
            std::format("Descend Land - Threshold Crossing Altitude ({})", set.AltitudeUnits.shortName()),
            std::format("Descend Land - Touchdown Roll ({})", set.DistanceUnits.shortName()),
            std::format("Ground Decelerate - Ground Distance ({})", set.DistanceUnits.shortName()),
            std::format("Ground Decelerate - Start Calibrated Airspeed ({})", set.SpeedUnits.shortName()),
            "Ground Decelerate -  Start Thrust Percentage"
        );

        std::size_t row = 0;
        for (const auto& doc29Acft : study.Doc29Aircrafts)
        {
            for (const auto& [profName, arrProfPtr] : doc29Acft.ArrivalProfiles)
            {
                const auto& arrProf = *arrProfPtr;
                if (arrProf.type() == Doc29Profile::Type::Procedural)
                    Doc29ProfileExporter(csv, row, arrProf);
            }
        }

        if (row)
        {
            csv.write();
            Log::io()->info("Exported Doc29 arrival procedural profiles to '{}'.", CsvPath);
        }
    }

    void Doc29ProfileExporter::visitDoc29ProfileArrivalProcedural(const Doc29ProfileArrivalProcedural& Doc29Prof) {
        const auto& set = Application::settings();

        Csv& csv = m_Csv;
        std::size_t& row = m_Row;

        for (const auto& step : Doc29Prof)
        {
            csv.setCell(row, 0, Doc29Prof.parentDoc29Performance().Name);
            csv.setCell(row, 1, Doc29Prof.Name);
            csv.setCell(row, 2, std::visit(Doc29ProfileArrivalProcedural::VisitorStepTypeString(), step));

            std::visit(Overload{
                [&](const Doc29ProfileArrivalProcedural::DescendDecelerate& Step) {
                    csv.setCell(row, 3, Step.Doc29AerodynamicCoefficients->Name);
                    csv.setCell(row, 4, set.AltitudeUnits.fromSi(Step.StartAltitudeAfe));
                    csv.setCell(row, 5, Step.DescentAngle);
                    csv.setCell(row, 6, set.SpeedUnits.fromSi(Step.StartCalibratedAirspeed));
                },
                [&](const Doc29ProfileArrivalProcedural::DescendIdle& Step) {
                    csv.setCell(row, 4, set.AltitudeUnits.fromSi(Step.StartAltitudeAfe));
                    csv.setCell(row, 5, Step.DescentAngle);
                    csv.setCell(row, 6, set.SpeedUnits.fromSi(Step.StartCalibratedAirspeed));
                },
                [&](const Doc29ProfileArrivalProcedural::Level& Step) {
                    csv.setCell(row, 3, Step.Doc29AerodynamicCoefficients->Name);
                    csv.setCell(row, 7, set.DistanceUnits.fromSi(Step.GroundDistance));
                },
                [&](const Doc29ProfileArrivalProcedural::LevelDecelerate& Step) {
                    csv.setCell(row, 3, Step.Doc29AerodynamicCoefficients->Name);
                    csv.setCell(row, 6, set.SpeedUnits.fromSi(Step.StartCalibratedAirspeed));
                    csv.setCell(row, 7, set.DistanceUnits.fromSi(Step.GroundDistance));
                },
                [&](const Doc29ProfileArrivalProcedural::LevelIdle& Step) {
                    csv.setCell(row, 6, set.SpeedUnits.fromSi(Step.StartCalibratedAirspeed));
                    csv.setCell(row, 7, set.DistanceUnits.fromSi(Step.GroundDistance));
                },
                [&](const Doc29ProfileArrivalProcedural::DescendLand& Step) {
                    csv.setCell(row, 3, Step.Doc29AerodynamicCoefficients->Name);
                    csv.setCell(row, 8, Step.DescentAngle);
                    csv.setCell(row, 9, set.AltitudeUnits.fromSi(Step.ThresholdCrossingAltitudeAfe));
                    csv.setCell(row, 10, set.DistanceUnits.fromSi(Step.TouchdownRoll));
                },
                [&](const Doc29ProfileArrivalProcedural::GroundDecelerate& Step) {
                    csv.setCell(row, 11, set.DistanceUnits.fromSi(Step.GroundDistance));
                    csv.setCell(row, 12, set.SpeedUnits.fromSi(Step.StartCalibratedAirspeed));
                    csv.setCell(row, 13, Step.StartThrustPercentage);
                },
                [&](const auto&) { GRAPE_ASSERT(false); }
                }, step);

            ++row;
        }
    }

    void exportDoc29PerformanceProfilesDepartureSteps(const std::string& CsvPath) {
        const auto& study = Application::study();
        const auto& set = Application::settings();

        Csv csv;
        try { csv.setExport(CsvPath); }
        catch (const std::exception& err)
        {
            Log::io()->error("Exporting Doc29 departure procedural profiles to '{}'. {}", CsvPath, err.what());
            return;
        }

        csv.setColumnNames(
            "Doc29 Aircraft ID",
            "Profile ID",
            "Step Type",
            "Thrust Cutback",
            "Aerodynamic Coefficient ID",
            std::format("End Altitude ATE ({})", set.AltitudeUnits.shortName()),
            std::format("End Calibrated Airspeed ({})", set.SpeedUnits.shortName()),
            std::format("Climb Rate ({})", set.DistanceUnits.shortName()),
            "Acceleration Percentage",
            std::format("Takeoff - Initial Calibrated Airspeed ({})", set.SpeedUnits.shortName())
        );

        std::size_t row = 0;
        for (const auto& doc29Acft : study.Doc29Aircrafts)
        {
            for (const auto& [profName, depProfPtr] : doc29Acft.DepartureProfiles)
            {
                const auto& depProf = *depProfPtr;
                if (depProf.type() == Doc29Profile::Type::Procedural)
                    Doc29ProfileExporter(csv, row, depProf);
            }
        }

        if (row)
        {
            csv.write();
            Log::io()->info("Exported Doc29 departure procedural profiles to '{}'.", CsvPath);
        }
    }

    void Doc29ProfileExporter::visitDoc29ProfileDepartureProcedural(const Doc29ProfileDepartureProcedural& Doc29Prof) {
        const auto& set = Application::settings();

        Csv& csv = m_Csv;
        std::size_t& row = m_Row;

        for (auto it = Doc29Prof.begin(); it != Doc29Prof.end(); ++it)
        {
            const auto index = std::distance(Doc29Prof.begin(), it);
            const auto& step = *it;
            csv.setCell(row, 0, Doc29Prof.parentDoc29Performance().Name);
            csv.setCell(row, 1, Doc29Prof.Name);
            csv.setCell(row, 2, std::visit(Doc29ProfileDepartureProcedural::VisitorStepTypeString(), step));
            if (index == Doc29Prof.thrustCutback())
                csv.setCell(row, 3, 1);
            std::visit(Overload{
                [&](const Doc29ProfileDepartureProcedural::Takeoff& Step) {
                    csv.setCell(row, 4, Step.Doc29AerodynamicCoefficients->Name);
                    csv.setCell(row, 9, set.SpeedUnits.fromSi(Step.InitialCalibratedAirspeed));
                },
                [&](const Doc29ProfileDepartureProcedural::Climb& Step) {
                    csv.setCell(row, 4, Step.Doc29AerodynamicCoefficients->Name);
                    csv.setCell(row, 5, set.AltitudeUnits.fromSi(Step.EndAltitudeAfe));
                },
                [&](const Doc29ProfileDepartureProcedural::ClimbAccelerate& Step) {
                    csv.setCell(row, 4, Step.Doc29AerodynamicCoefficients->Name);
                    csv.setCell(row, 6, set.SpeedUnits.fromSi(Step.EndCalibratedAirspeed));
                    csv.setCell(row, 7, set.VerticalSpeedUnits.fromSi(Step.ClimbParameter));
                },
                [&](const Doc29ProfileDepartureProcedural::ClimbAcceleratePercentage& Step) {
                    csv.setCell(row, 4, Step.Doc29AerodynamicCoefficients->Name);
                    csv.setCell(row, 6, set.SpeedUnits.fromSi(Step.EndCalibratedAirspeed));
                    csv.setCell(row, 8, Step.ClimbParameter);
                },
                [&](const auto&) { GRAPE_ASSERT(false); }
                }, step);
            ++row;
        }
    }

    void exportDoc29Noise(const std::string& CsvPath) {
        const auto& study = Application::study();

        Csv csv;
        try { csv.setExport(CsvPath); }
        catch (const std::exception& err)
        {
            Log::io()->error("Exporting Doc29 Noise to '{}'. {}", CsvPath, err.what());
            return;
        }

        csv.setColumnNames(
            "ID",
            "Lateral Directivity",
            "Start of Roll Correction"
        );

        std::size_t row = 0;
        for (const auto& doc29Ns : study.Doc29Noises)
        {
            csv.setCell(row, 0, doc29Ns.Name);
            csv.setCell(row, 1, Doc29Noise::LateralDirectivities.toString(doc29Ns.LateralDir));
            csv.setCell(row, 2, Doc29Noise::SORCorrections.toString(doc29Ns.SOR));
            ++row;
        }

        if (row)
        {
            csv.write();
            Log::io()->info("Exported Doc29 Noise to '{}'.", CsvPath);
        }
    }

    void exportDoc29NoiseNpd(const std::string& CsvPath) {
        const auto& study = Application::study();
        const auto& set = Application::settings();

        Csv csv;
        try { csv.setExport(CsvPath); }
        catch (const std::exception& err)
        {
            Log::io()->error("Exporting Doc29 Noise NPD data to '{}'. {}", CsvPath, err.what());
            return;
        }

        csv.setColumnNames(
            "Doc29 Noise ID",
            "Operation",
            "Noise Metric",
            std::format("Thrust ({})", set.ThrustUnits.shortName())
        );
        {
            std::size_t col = 3;
            for (const auto& dist : NpdStandardDistances)
                csv.setColumnName(++col, std::format("Level {:.0} ft", toFeet(dist)));
        }

        std::size_t row = 0;
        const auto addNpdData = [&](const std::string& Name, OperationType OpType, NoiseSingleMetric NsMetric, const NpdData& Npd) {
            const std::string opTypeStr = OperationTypes.toString(OpType);
            const std::string nsMetricStr = NoiseSingleMetrics.toString(NsMetric);

            for (const auto& [thr, levels] : Npd)
            {
                csv.setCell(row, 0, Name);
                csv.setCell(row, 1, opTypeStr);
                csv.setCell(row, 2, nsMetricStr);
                csv.setCell(row, 3, set.ThrustUnits.fromSi(thr));

                std::size_t col = 3;
                for (const auto& lvl : levels)
                    csv.setCell(row, ++col, lvl);
                ++row;
            }
            };
        for (const auto& doc29Ns : study.Doc29Noises)
        {
            addNpdData(doc29Ns.Name, OperationType::Arrival, NoiseSingleMetric::Lamax, doc29Ns.ArrivalLamax);
            addNpdData(doc29Ns.Name, OperationType::Arrival, NoiseSingleMetric::Sel, doc29Ns.ArrivalSel);
            addNpdData(doc29Ns.Name, OperationType::Departure, NoiseSingleMetric::Lamax, doc29Ns.DepartureLamax);
            addNpdData(doc29Ns.Name, OperationType::Departure, NoiseSingleMetric::Sel, doc29Ns.DepartureSel);
        }

        if (row)
        {
            csv.write();
            Log::io()->info("Exported Doc29 Noise NPD data to '{}'.", CsvPath);
        }
    }

    void exportDoc29NoiseSpectrum(const std::string& CsvPath) {
        const auto& study = Application::study();

        Csv csv;
        try { csv.setExport(CsvPath); }
        catch (const std::exception& err)
        {
            Log::io()->error("Exporting Doc29 Noise spectrum to '{}'. {}", CsvPath, err.what());
            return;
        }

        csv.setColumnNames(
            "Doc29 Noise ID",
            "Operation"
        );
        {
            std::size_t col = 1;
            for (const auto& freq : OneThirdOctaveCenterFrequencies)
                csv.setColumnName(++col, std::format("Level {:.0} ft", freq));
        }

        std::size_t row = 0;
        const auto addSpectrum = [&](OperationType OpType, const Doc29Spectrum& Doc29Spec) {
            csv.setCell(row, 0, Doc29Spec.parentDoc29Noise().Name);
            csv.setCell(row, 1, OperationTypes.toString(OpType));

            std::size_t col = 1;
            for (const auto& lvl : Doc29Spec)
                csv.setCell(row, ++col, lvl);
            ++row;

            };

        for (const auto& doc29Ns : study.Doc29Noises)
        {
            addSpectrum(OperationType::Arrival, doc29Ns.ArrivalSpectrum);
            addSpectrum(OperationType::Departure, doc29Ns.DepartureSpectrum);
        }

        if (row)
        {
            csv.write();
            Log::io()->info("Exported Doc29 Noise spectrum to '{}'.", CsvPath);
        }
    }

    void exportLTO(const std::string& CsvPath) {
        const auto& study = Application::study();
        const auto& set = Application::settings();

        Csv csv;
        try { csv.setExport(CsvPath); }
        catch (const std::exception& err)
        {
            Log::io()->error("Exporting LTO engines to '{}'. {}", CsvPath, err.what());
            return;
        }

        csv.setColumnNames(
            "ID",
            std::format("Maximum Sea Level Static Thrust ({})", set.ThrustUnits.shortName()),
            std::format("Fuel Flow Idle ({})", set.FuelFlowUnits.shortName()),
            std::format("Fuel Flow Approach ({})", set.FuelFlowUnits.shortName()),
            std::format("Fuel Flow Climb Out ({})", set.FuelFlowUnits.shortName()),
            std::format("Fuel Flow Takeoff ({})", set.FuelFlowUnits.shortName()),
            "Fuel Flow Correction Factor Idle",
            "Fuel Flow Correction Factor Approach",
            "Fuel Flow Correction Factor Climb Out",
            "Fuel Flow Correction Factor Takeoff",
            std::format("EI HC Idle ({})", set.EmissionIndexUnits.shortName()),
            std::format("EI HC Approach ({})", set.EmissionIndexUnits.shortName()),
            std::format("EI HC Climb Out ({})", set.EmissionIndexUnits.shortName()),
            std::format("EI HC Takeoff ({})", set.EmissionIndexUnits.shortName()),
            std::format("EI CO Idle ({})", set.EmissionIndexUnits.shortName()),
            std::format("EI CO Approach ({})", set.EmissionIndexUnits.shortName()),
            std::format("EI CO Climb Out ({})", set.EmissionIndexUnits.shortName()),
            std::format("EI CO Takeoff ({})", set.EmissionIndexUnits.shortName()),
            std::format("EI NOx Idle ({})", set.EmissionIndexUnits.shortName()),
            std::format("EI NOx Approach ({})", set.EmissionIndexUnits.shortName()),
            std::format("EI NOx Climb Out ({})", set.EmissionIndexUnits.shortName()),
            std::format("EI NOx Takeoff ({})", set.EmissionIndexUnits.shortName()),
            "Mixed Nozzle Flag",
            "Bypass Ratio",
            "Air to Fuel Ratio Idle",
            "Air to Fuel Ratio Approach",
            "Air to Fuel Ratio Climb Out",
            "Air to Fuel Ratio Takeoff",
            "Smoke Number Idle",
            "Smoke Number Approach",
            "Smoke Number Climb Out",
            "Smoke Number Takeoff",
            "EI nvPM Idle (mg/kg)",
            "EI nvPM Approach (mg/kg)",
            "EI nvPM Climb Out (mg/kg)",
            "EI nvPM Takeoff (mg/kg)",
            "EI nvPM Number Idle",
            "EI nvPM Number Approach",
            "EI nvPM Number Climb Out",
            "EI nvPM Number Takeoff"
        );

        std::size_t row = 0;
        for (const auto& lto : study.LTOEngines)
        {
            std::size_t col = 0;

            // Name
            csv.setCell(row, col++, lto.Name);

            // Maximum Sea Level Static Thrust
            csv.setCell(row, col++, set.ThrustUnits.fromSi(lto.MaximumSeaLevelStaticThrust));

            // Fuel Flow
            for (const double fuelFlow : lto.FuelFlows)
                csv.setCell(row, col++, set.FuelFlowUnits.fromSi(fuelFlow));

            // Fuel Flow Correction Factors
            for (const double fuelFlowCorrFactor : lto.FuelFlowCorrectionFactors)
                csv.setCell(row, col++, fuelFlowCorrFactor);

            // HC
            for (const double emiHC : lto.EmissionIndexesHC)
                csv.setCell(row, col++, set.EmissionIndexUnits.fromSi(emiHC));

            // CO
            for (const double emiCO : lto.EmissionIndexesCO)
                csv.setCell(row, col++, set.EmissionIndexUnits.fromSi(emiCO));

            // NOx
            for (const double emiNOx : lto.EmissionIndexesNOx)
                csv.setCell(row, col++, set.EmissionIndexUnits.fromSi(emiNOx));

            // Mixed Nozzle & Bypass Ratio
            csv.setCell(row, col++, static_cast<int>(lto.MixedNozzle));
            csv.setCell(row, col++, lto.BypassRatio);

            // Air to Fuel Ratio
            for (const double afr : lto.AirFuelRatios)
                csv.setCell(row, col++, afr);

            // Smoke Number
            for (const double sn : lto.SmokeNumbers)
                if (!std::isnan(sn))
                    csv.setCell(row, col++, sn);

            // nvPM
            for (const double nvpm : lto.EmissionIndexesNVPM)
                if (!std::isnan(nvpm))
                    csv.setCell(row, col++, toMilligramsPerKilogram(nvpm));

            // nvPM Number
            for (const double nvpmNumber : lto.EmissionIndexesNVPMNumber)
                if (!std::isnan(nvpmNumber))
                    csv.setCell(row, col++, set.EmissionIndexUnits.fromSi(nvpmNumber));

            ++row;
        }

        if (row)
        {
            csv.write();
            Log::io()->info("Exported LTO engines to '{}'.", CsvPath);
        }
    }

    void exportSFI(const std::string& CsvPath) {
        const auto& study = Application::study();
        const auto& set = Application::settings();

        Csv csv;
        try { csv.setExport(CsvPath); }
        catch (const std::exception& err)
        {
            Log::io()->error("Exporting SFI coefficients to '{}'. {}", CsvPath, err.what());
            return;
        }

        csv.setColumnNames(
            "ID",
            std::format("Maximum Sea Level Static Thrust ({})", set.ThrustUnits.shortName()),
            "A",
            "B1",
            "B2",
            "B3",
            "K1",
            "K2",
            "K3",
            "K4"
        );

        std::size_t row = 0;
        for (const auto& sfi : study.SFIs)
        {
            csv.setCell(row, 0, sfi.Name);
            csv.setCell(row, 1, set.ThrustUnits.fromSi(sfi.MaximumSeaLevelStaticThrust));
            csv.setCell(row, 2, sfi.A);
            csv.setCell(row, 3, sfi.B1);
            csv.setCell(row, 4, sfi.B2);
            csv.setCell(row, 5, sfi.B3);
            csv.setCell(row, 6, sfi.K1);
            csv.setCell(row, 7, sfi.K2);
            csv.setCell(row, 8, sfi.K3);
            csv.setCell(row, 9, sfi.K4);
            ++row;
        }

        if (row)
        {
            csv.write();
            Log::io()->info("Exported SFI coefficients to '{}'.", CsvPath);
        }
    }

    void exportFleet(const std::string& CsvPath) {
        const auto& study = Application::study();
        const auto& set = Application::settings();

        Csv csv;
        try { csv.setExport(CsvPath); }
        catch (const std::exception& err)
        {
            Log::io()->error("Exporting fleet to '{}'. {}", CsvPath, err.what());
            return;
        }

        csv.setColumnNames(
            "ID",
            "Number of Engines",
            "Doc29 Aircraft ID",
            "SFI ID",
            "LTO ID",
            "Doc29 Noise ID",
            "Doc29 Noise Delta Arrival",
            "Doc29 Noise Delta Departure"
        );

        std::size_t row = 0;
        for (const auto& acft : study.Aircrafts)
        {
            csv.setCell(row, 0, acft.Name);
            csv.setCell(row, 1, acft.EngineCount);
            if (acft.validDoc29Performance())
                csv.setCell(row, 2, acft.Doc29Acft->Name);
            if (acft.validSFI())
                csv.setCell(row, 3, acft.SFIFuel->Name);
            if (acft.validLTOEngine())
                csv.setCell(row, 4, acft.LTOEng->Name);
            if (acft.validDoc29Noise())
                csv.setCell(row, 5, acft.Doc29Ns->Name);
            csv.setCell(row, 6, acft.Doc29NoiseDeltaArrivals);
            csv.setCell(row, 7, acft.Doc29NoiseDeltaDepartures);
            ++row;
        }

        if (row)
        {
            csv.write();
            Log::io()->info("Exported fleet to '{}'.", CsvPath);
        }
    }

    void exportAirports(const std::string& CsvPath) {
        const auto& study = Application::study();
        const auto& set = Application::settings();

        Csv csv;
        try { csv.setExport(CsvPath); }
        catch (const std::exception& err)
        {
            Log::io()->error("Exporting airports to '{}'. {}", CsvPath, err.what());
            return;
        }

        csv.setColumnNames(
            "ID",
            "Longitude",
            "Latitude",
            std::format("Elevation ({})", set.AltitudeUnits.shortName()),
            std::format("Reference Temperature ({})", set.TemperatureUnits.shortName()),
            std::format("Reference Pressure ({})", set.PressureUnits.shortName())
        );

        std::size_t row = 0;
        for (const auto& apt : study.Airports)
        {
            csv.setCell(row, 0, apt.Name);
            csv.setCell(row, 1, apt.Longitude);
            csv.setCell(row, 2, apt.Latitude);
            csv.setCell(row, 3, set.AltitudeUnits.fromSi(apt.Elevation));
            csv.setCell(row, 4, set.TemperatureUnits.fromSi(apt.ReferenceTemperature));
            csv.setCell(row, 5, set.PressureUnits.fromSi(apt.ReferenceSeaLevelPressure));
            ++row;
        }

        if (row)
        {
            csv.write();
            Log::io()->info("Exported airports to '{}'.", CsvPath);
        }
    }

    void exportRunways(const std::string& CsvPath) {
        const auto& study = Application::study();
        const auto& set = Application::settings();

        Csv csv;
        try { csv.setExport(CsvPath); }
        catch (const std::exception& err)
        {
            Log::io()->error("Exporting runways to '{}'. {}", CsvPath, err.what());
            return;
        }

        csv.setColumnNames(
            "Airport ID",
            "ID",
            "Longitude",
            "Latitude",
            std::format("Elevation ({})", set.AltitudeUnits.shortName()),
            std::format("Length ({})", set.DistanceUnits.shortName()),
            "Heading",
            "Gradient"
        );

        std::size_t row = 0;
        for (const auto& apt : study.Airports)
        {
            for (const auto& rwy : apt.Runways | std::views::values)
            {
                csv.setCell(row, 0, rwy.parentAirport().Name);
                csv.setCell(row, 1, rwy.Name);
                csv.setCell(row, 2, rwy.Longitude);
                csv.setCell(row, 3, rwy.Latitude);
                csv.setCell(row, 4, set.AltitudeUnits.fromSi(rwy.Elevation));
                csv.setCell(row, 5, set.DistanceUnits.fromSi(rwy.Length));
                csv.setCell(row, 6, rwy.Heading);
                csv.setCell(row, 7, rwy.Gradient);
                ++row;
            }
        }

        if (row)
        {
            csv.write();
            Log::io()->info("Exported runways to '{}'.", CsvPath);
        }
    }

    namespace {
        struct RouteExporter : RouteTypeVisitor {
            RouteExporter(Csv& CsvFile, std::size_t& Row, const Route& Rte) : m_Csv(CsvFile), m_Row(Row) { Rte.accept(*this); }
            void visitSimple(const RouteTypeSimple& Rte) override;
            void visitVectors(const RouteTypeVectors& Rte) override;
            void visitRnp(const RouteTypeRnp& Rte) override;

        private:
            Csv& m_Csv;
            std::size_t& m_Row;
        };
    }

    void exportRoutesSimple(const std::string& CsvPath) {
        const auto& study = Application::study();

        Csv csv;
        try { csv.setExport(CsvPath); }
        catch (const std::exception& err)
        {
            Log::io()->error("Exporting simple routes to '{}'. {}", CsvPath, err.what());
            return;
        }

        csv.setColumnNames(
            "Airport ID",
            "Runway ID",
            "Operation",
            "Route ID",
            "Longitude",
            "Latitude"
        );

        std::size_t row = 0;
        for (const auto& apt : study.Airports)
        {
            for (const auto& rwy : apt.Runways | std::views::values)
            {
                for (const auto& rte : rwy.ArrivalRoutes | std::views::values)
                {
                    if (rte->type() == Route::Type::Simple)
                        RouteExporter(csv, row, *rte);
                }

                for (const auto& rte : rwy.DepartureRoutes | std::views::values)
                {
                    if (rte->type() == Route::Type::Simple)
                        RouteExporter(csv, row, *rte);
                }
            }
        }

        if (row)
        {
            csv.write();
            Log::io()->info("Exported simple routes to '{}'.", CsvPath);
        }
    }

    void RouteExporter::visitSimple(const RouteTypeSimple& Rte) {
        Csv& csv = m_Csv;
        std::size_t& row = m_Row;

        for (const auto& pt : Rte)
        {
            csv.setCell(row, 0, Rte.parentAirport().Name);
            csv.setCell(row, 1, Rte.parentRunway().Name);
            csv.setCell(row, 2, OperationTypes.toString(Rte.operationType()));
            csv.setCell(row, 3, Rte.Name);
            csv.setCell(row, 4, pt.Longitude);
            csv.setCell(row, 5, pt.Latitude);
            ++row;
        }
    }

    void exportRoutesVectors(const std::string& CsvPath) {
        const auto& study = Application::study();
        const auto& set = Application::settings();

        Csv csv;
        try { csv.setExport(CsvPath); }
        catch (const std::exception& err)
        {
            Log::io()->error("Exporting vector routes to '{}'. {}", CsvPath, err.what());
            return;
        }

        csv.setColumnNames(
            "Airport ID",
            "Runway ID",
            "Operation",
            "Route ID",
            "Vector Type",
            std::format("Distance ({})", set.DistanceUnits.shortName()),
            std::format("Turn Radius ({})", set.DistanceUnits.shortName()),
            "Heading",
            "Turn Direction"
        );

        std::size_t row = 0;
        for (const auto& apt : study.Airports)
        {
            for (const auto& rwy : apt.Runways | std::views::values)
            {
                for (const auto& rte : rwy.ArrivalRoutes | std::views::values)
                {
                    if (rte->type() == Route::Type::Vectors)
                        RouteExporter(csv, row, *rte);
                }

                for (const auto& rte : rwy.DepartureRoutes | std::views::values)
                {
                    if (rte->type() == Route::Type::Vectors)
                        RouteExporter(csv, row, *rte);
                }
            }
        }

        if (row)
        {
            csv.write();
            Log::io()->info("Exported vector routes to '{}'.", CsvPath);
        }
    }

    void RouteExporter::visitVectors(const RouteTypeVectors& Rte) {
        const auto& set = Application::settings();

        Csv& csv = m_Csv;
        std::size_t& row = m_Row;

        for (const auto& vec : Rte)
        {
            csv.setCell(row, 0, Rte.parentAirport().Name);
            csv.setCell(row, 1, Rte.parentRunway().Name);
            csv.setCell(row, 2, OperationTypes.toString(Rte.operationType()));
            csv.setCell(row, 3, Rte.Name);
            csv.setCell(row, 4, std::visit(RouteTypeVectors::VisitorVectorTypeString(), vec));
            std::visit(Overload{
                           [&](const RouteTypeVectors::Straight& Vec) { csv.setCell(row, 5, set.DistanceUnits.fromSi(Vec.Distance)); },
                           [&](const RouteTypeVectors::Turn& Vec) {
                               csv.setCell(row, 6, set.DistanceUnits.fromSi(Vec.TurnRadius));
                               csv.setCell(row, 7, Vec.HeadingChange);
                               csv.setCell(row, 8, RouteTypeVectors::Turn::Directions.toString(Vec.TurnDirection));
                           },
                           [&](const auto&) { GRAPE_ASSERT(false); },
                }, vec);
            ++row;
        }
    }

    void exportRoutesRnp(const std::string& CsvPath) {
        const auto& study = Application::study();

        Csv csv;
        try { csv.setExport(CsvPath); }
        catch (const std::exception& err)
        {
            Log::io()->error("Exporting RNP routes to '{}'. {}", CsvPath, err.what());
            return;
        }

        csv.setColumnNames(
            "Airport ID",
            "Runway ID",
            "Operation",
            "Route ID",
            "RNP Step Type",
            "Longitude",
            "Latitude",
            "Center Longitude",
            "Center Latitude"
        );

        std::size_t row = 0;
        for (const auto& apt : study.Airports)
        {
            for (const auto& rwy : apt.Runways | std::views::values)
            {
                for (const auto& rte : rwy.ArrivalRoutes | std::views::values)
                {
                    if (rte->type() == Route::Type::Rnp)
                        RouteExporter(csv, row, *rte);
                }

                for (const auto& rte : rwy.DepartureRoutes | std::views::values)
                {
                    if (rte->type() == Route::Type::Rnp)
                        RouteExporter(csv, row, *rte);
                }
            }
        }

        if (row)
        {
            csv.write();
            Log::io()->info("Exported RNP routes to '{}'.", CsvPath);
        }
    }

    void RouteExporter::visitRnp(const RouteTypeRnp& Rte) {
        Csv& csv = m_Csv;
        std::size_t& row = m_Row;

        for (const auto& step : Rte)
        {
            csv.setCell(row, 0, Rte.parentAirport().Name);
            csv.setCell(row, 1, Rte.parentRunway().Name);
            csv.setCell(row, 2, OperationTypes.toString(Rte.operationType()));
            csv.setCell(row, 3, Rte.Name);
            csv.setCell(row, 4, std::visit(RouteTypeRnp::VisitorRnpStepTypeString(), step));
            std::visit(Overload{
                           [&](const RouteTypeRnp::TrackToFix& Step) {
                               csv.setCell(row, 5, Step.Longitude);
                               csv.setCell(row, 6, Step.Latitude);
                           },
                           [&](const RouteTypeRnp::RadiusToFix& Step) {
                               csv.setCell(row, 5, Step.Longitude);
                               csv.setCell(row, 6, Step.Latitude);
                               csv.setCell(row, 7, Step.CenterLongitude);
                               csv.setCell(row, 8, Step.CenterLatitude);
                           },
                           [&](const auto&) { GRAPE_ASSERT(false); },
                }, step);
            ++row;
        }
    }

    void exportFlights(const std::string& CsvPath) {
        const auto& study = Application::study();
        const auto& set = Application::settings();

        Csv csv;
        try { csv.setExport(CsvPath); }
        catch (const std::exception& err)
        {
            Log::io()->error("Exporting flights to '{}'. {}", CsvPath, err.what());
            return;
        }

        csv.setColumnNames(
            "ID",
            "Airport ID",
            "Runway ID",
            "Operation",
            "Route ID",
            "Time",
            "Count",
            "Fleet ID",
            std::format("Weight ({})", set.WeightUnits.shortName()),
            "Doc29 Profile ID",
            "Thrust % Takeoff",
            "Thrust % Climb"
        );

        std::size_t row = 0;

        for (const auto& arrFl : study.Operations.flightArrivals() | std::views::values)
        {
            csv.setCell(row, 0, arrFl.Name);
            csv.setCell(row, 1, OperationTypes.toString(arrFl.operationType()));
            if (arrFl.hasRoute())
            {
                csv.setCell(row, 2, arrFl.route().parentAirport().Name);
                csv.setCell(row, 3, arrFl.route().parentRunway().Name);
                csv.setCell(row, 4, arrFl.route().Name);
            }
            csv.setCell(row, 5, timeToUtcString(arrFl.Time));
            csv.setCell(row, 6, arrFl.Count);
            csv.setCell(row, 7, arrFl.aircraft().Name);
            csv.setCell(row, 8, set.WeightUnits.fromSi(arrFl.Weight));
            if (arrFl.hasDoc29Profile())
                csv.setCell(row, 9, arrFl.doc29Profile()->Name);
            ++row;
        }

        for (const auto& depFl : study.Operations.flightDepartures() | std::views::values)
        {
            csv.setCell(row, 0, depFl.Name);
            csv.setCell(row, 1, OperationTypes.toString(depFl.operationType()));
            if (depFl.hasRoute())
            {
                csv.setCell(row, 2, depFl.route().parentAirport().Name);
                csv.setCell(row, 3, depFl.route().parentRunway().Name);
                csv.setCell(row, 4, depFl.route().Name);
            }
            csv.setCell(row, 5, timeToUtcString(depFl.Time));
            csv.setCell(row, 6, depFl.Count);
            csv.setCell(row, 7, depFl.aircraft().Name);
            csv.setCell(row, 8, set.WeightUnits.fromSi(depFl.Weight));
            if (depFl.hasDoc29Profile())
                csv.setCell(row, 9, depFl.doc29Profile()->Name);
            csv.setCell(row, 10, depFl.ThrustPercentageTakeoff);
            csv.setCell(row, 11, depFl.ThrustPercentageClimb);
            ++row;
        }

        if (row)
        {
            csv.write();
            Log::io()->info("Exported flights to '{}'.", CsvPath);
        }
    }

    void exportTracks4d(const std::string& CsvPath) {
        const auto& study = Application::study();

        Csv csv;
        try { csv.setExport(CsvPath); }
        catch (const std::exception& err)
        {
            Log::io()->error("Exporting tracks 4D to '{}'. {}", CsvPath, err.what());
            return;
        }

        csv.setColumnNames(
            "ID",
            "Operation",
            "Time",
            "Count",
            "Fleet ID"
        );

        std::size_t row = 0;

        for (const auto& arrTrack4d : study.Operations.track4dArrivals() | std::views::values)
        {
            csv.setCell(row, 0, arrTrack4d.Name);
            csv.setCell(row, 1, OperationTypes.toString(arrTrack4d.operationType()));
            csv.setCell(row, 2, timeToUtcString(arrTrack4d.Time));
            csv.setCell(row, 3, arrTrack4d.Count);
            csv.setCell(row, 4, arrTrack4d.aircraft().Name);
            ++row;
        }

        for (const auto& depTrack4d : study.Operations.track4dDepartures() | std::views::values)
        {
            csv.setCell(row, 0, depTrack4d.Name);
            csv.setCell(row, 1, OperationTypes.toString(depTrack4d.operationType()));
            csv.setCell(row, 2, timeToUtcString(depTrack4d.Time));
            csv.setCell(row, 3, depTrack4d.Count);
            csv.setCell(row, 4, depTrack4d.aircraft().Name);
            ++row;
        }

        if (row)
        {
            csv.write();
            Log::io()->info("Exported 4D tracks to '{}'.", CsvPath);
        }
    }

    void exportTracks4dPoints(const std::string& CsvPath) {
        auto& study = Application::study();
        const auto& set = Application::settings();

        Csv csv;
        try { csv.setExport(CsvPath); }
        catch (const std::exception& err)
        {
            Log::io()->error("Exporting tracks 4D points to '{}'. {}", CsvPath, err.what());
            return;
        }

        csv.setColumnNames(
            "ID",
            "Operation",
            "Flight Phase",
            std::format("Cumulative Ground Distance ({})", set.DistanceUnits.shortName()),
            "Longitude",
            "Latitude",
            std::format("Altitude Msl ({})", set.AltitudeUnits.shortName()),
            std::format("True Airspeed ({})", set.SpeedUnits.shortName()),
            std::format("Groundspeed ({})", set.SpeedUnits.shortName()),
            std::format("Corrected Net Thrust per Engine ({})", set.ThrustUnits.shortName()),
            "Bank Angle",
            std::format("Fuel Flow per Engine ({})", set.FuelFlowUnits.shortName())
        );

        std::size_t row = 0;
        for (const auto& op : study.Operations.track4dArrivals() | std::views::values)
        {
            study.Operations.loadArr(op);
            for (const auto& pt : op)
            {
                csv.setCell(row, 0, op.Name);
                csv.setCell(row, 1, OperationTypes.toString(op.operationType()));
                csv.setCell(row, 2, FlightPhases.toString(pt.FlPhase));
                csv.setCell(row, 3, set.DistanceUnits.fromSi(pt.CumulativeGroundDistance));
                csv.setCell(row, 4, pt.Longitude);
                csv.setCell(row, 5, pt.Latitude);
                csv.setCell(row, 6, set.AltitudeUnits.fromSi(pt.AltitudeMsl));
                csv.setCell(row, 7, set.SpeedUnits.fromSi(pt.TrueAirspeed));
                csv.setCell(row, 8, set.SpeedUnits.fromSi(pt.Groundspeed));
                csv.setCell(row, 9, set.ThrustUnits.fromSi(pt.CorrNetThrustPerEng));
                csv.setCell(row, 10, pt.BankAngle);
                csv.setCell(row, 11, set.FuelFlowUnits.fromSi(pt.FuelFlowPerEng));
                ++row;
            }
        }

        for (const auto& op : study.Operations.track4dDepartures() | std::views::values)
        {
            study.Operations.loadDep(op);
            for (const auto& pt : op)
            {
                csv.setCell(row, 0, op.Name);
                csv.setCell(row, 1, OperationTypes.toString(op.operationType()));
                csv.setCell(row, 2, FlightPhases.toString(pt.FlPhase));
                csv.setCell(row, 3, set.DistanceUnits.fromSi(pt.CumulativeGroundDistance));
                csv.setCell(row, 4, pt.Longitude);
                csv.setCell(row, 5, pt.Latitude);
                csv.setCell(row, 6, set.AltitudeUnits.fromSi(pt.AltitudeMsl));
                csv.setCell(row, 7, set.SpeedUnits.fromSi(pt.TrueAirspeed));
                csv.setCell(row, 8, set.SpeedUnits.fromSi(pt.Groundspeed));
                csv.setCell(row, 9, set.ThrustUnits.fromSi(pt.CorrNetThrustPerEng));
                csv.setCell(row, 10, pt.BankAngle);
                csv.setCell(row, 11, set.FuelFlowUnits.fromSi(pt.FuelFlowPerEng));
                ++row;
            }
        }

        if (row)
        {
            csv.write();
            Log::io()->info("Exported 4D tracks points to '{}'.", CsvPath);
        }
    }

    void exportScenarios(const std::string& CsvPath) {
        const auto& study = Application::study();

        Csv csv;
        try { csv.setExport(CsvPath); }
        catch (const std::exception& err)
        {
            Log::io()->error("Exporting scenarios to '{}'. {}", CsvPath, err.what());
            return;
        }

        csv.setColumnNames(
            "ID",
            "# Operations",
            "# Arrivals",
            "# Departures",
            "# Flights",
            "# Tracks 4D",
            "# Arrival Flights",
            "# Departure Flights",
            "# Arrival Tracks 4D",
            "# Departure Tracks 4D",
            "Start Time",
            "End Time"
        );

        std::size_t row = 0;
        for (const auto& scen : study.Scenarios)
        {
            csv.setCell(row, 0, scen.Name);
            csv.setCell(row, 1, static_cast<int>(scen.size()));
            csv.setCell(row, 2, static_cast<int>(scen.arrivalsSize()));
            csv.setCell(row, 3, static_cast<int>(scen.departuresSize()));
            csv.setCell(row, 4, static_cast<int>(scen.flightsSize()));
            csv.setCell(row, 5, static_cast<int>(scen.tracks4dSize()));
            csv.setCell(row, 6, static_cast<int>(scen.FlightArrivals.size()));
            csv.setCell(row, 7, static_cast<int>(scen.FlightDepartures.size()));
            csv.setCell(row, 8, static_cast<int>(scen.Track4dArrivals.size()));
            csv.setCell(row, 9, static_cast<int>(scen.Track4dDepartures.size()));
            const auto [startTime, endTime] = scen.timeSpan();
            if (startTime != std::chrono::tai_seconds::max())
                csv.setCell(row, 10, timeToUtcString(startTime));
            if (endTime != std::chrono::tai_seconds::min())
                csv.setCell(row, 11, timeToUtcString(endTime));

            ++row;
        }

        if (row)
        {
            csv.write();
            Log::io()->info("Exported scenarios to '{}'.", CsvPath);
        }
    }

    void exportScenariosOperations(const std::string& CsvPath) {
        const auto& study = Application::study();

        Csv csv;
        try { csv.setExport(CsvPath); }
        catch (const std::exception& err)
        {
            Log::io()->error("Exporting scenarios operations to '{}'. {}", CsvPath, err.what());
            return;
        }

        csv.setColumnNames(
            "Scenario ID",
            "ID",
            "Operation",
            "Type"
        );

        std::size_t row = 0;

        for (const auto& scen : study.Scenarios)
        {
            for (const auto& opRef : scen.FlightArrivals)
            {
                const auto& op = opRef.get();
                csv.setCell(row, 0, scen.Name);
                csv.setCell(row, 1, op.Name);
                csv.setCell(row, 2, OperationTypes.toString(op.operationType()));
                csv.setCell(row, 3, Operation::Types.toString(op.type()));
                ++row;
            }

            for (const auto& opRef : scen.FlightDepartures)
            {
                const auto& op = opRef.get();
                csv.setCell(row, 0, scen.Name);
                csv.setCell(row, 1, op.Name);
                csv.setCell(row, 2, OperationTypes.toString(op.operationType()));
                csv.setCell(row, 3, Operation::Types.toString(op.type()));
                ++row;
            }

            for (const auto& opRef : scen.Track4dArrivals)
            {
                const auto& op = opRef.get();
                csv.setCell(row, 0, scen.Name);
                csv.setCell(row, 1, op.Name);
                csv.setCell(row, 2, OperationTypes.toString(op.operationType()));
                csv.setCell(row, 3, Operation::Types.toString(op.type()));
                ++row;
            }

            for (const auto& opRef : scen.Track4dDepartures)
            {
                const auto& op = opRef.get();
                csv.setCell(row, 0, scen.Name);
                csv.setCell(row, 1, op.Name);
                csv.setCell(row, 2, OperationTypes.toString(op.operationType()));
                csv.setCell(row, 3, Operation::Types.toString(op.type()));
                ++row;
            }
        }

        if (row)
        {
            csv.write();
            Log::io()->info("Exported scenarios operations to '{}'.", CsvPath);
        }
    }

    void exportPerformanceRuns(const std::string& CsvPath) {
        const auto& study = Application::study();
        const auto& set = Application::settings();

        const auto path = std::filesystem::path(CsvPath);

        Csv csv;
        try { csv.setExport(CsvPath); }
        catch (const std::exception& err)
        {
            Log::io()->error("Exporting performance runs to '{}'. {}", CsvPath, err.what());
            return;
        }

        csv.setColumnNames(
            "Scenario ID",
            "ID",
            "Coordinate System Type",
            "Coordinate System Longitude 0",
            "Coordinate System Latitude 0",
            std::format("Filter Minimum Altitude ({})", set.AltitudeUnits.shortName()),
            std::format("Filter Maximum Altitude ({})", set.AltitudeUnits.shortName()),
            std::format("Filter Minimum Cumulative Ground Distance ({})", set.DistanceUnits.shortName()),
            std::format("Filter Maximum Cumulative Ground Distance ({})", set.DistanceUnits.shortName()),
            std::format("Filter Ground Distance Threshold ({})", set.DistanceUnits.shortName()),
            std::format("Segmentation Speed Delta Threshold ({})", set.SpeedUnits.shortName()),
            "Flights Performance Model",
            "Flights Doc29 Segmentation",
            "Tracks 4D Calculate Performance",
            "Tracks 4D Minimum Points",
            "Tracks 4D Recalculate Cumulative Ground Distance",
            "Tracks 4D Recalculate Groundspeed",
            "Tracks 4D Recalculate Fuel Flow",
            "Fuel Flow Model"
            "Fuel Flow Model LTO Altitude Correction"
        );

        std::size_t row = 0;

        for (const auto& scen : study.Scenarios)
        {
            for (const auto& perfRun : scen.PerformanceRuns | std::views::values)
            {
                const auto& spec = perfRun.PerfRunSpec;

                csv.setCell(row, 0, scen.Name);
                csv.setCell(row, 1, perfRun.Name);

                const auto csType = spec.CoordSys->type();
                csv.setCell(row, 2, CoordinateSystem::Types.toString(spec.CoordSys->type()));
                if (csType == CoordinateSystem::Type::LocalCartesian)
                {
                    const auto cs = static_cast<const LocalCartesian*>(spec.CoordSys.get());
                    const auto [lon0, lat0] = cs->origin();
                    csv.setCell(row, 3, lon0);
                    csv.setCell(row, 4, lat0);
                }

                if (!std::isinf(spec.FilterMinimumAltitude))
                    csv.setCell(row, 5, set.AltitudeUnits.fromSi(spec.FilterMinimumAltitude));
                if (!std::isinf(spec.FilterMaximumAltitude))
                    csv.setCell(row, 6, set.AltitudeUnits.fromSi(spec.FilterMaximumAltitude));
                if (!std::isinf(spec.FilterMinimumCumulativeGroundDistance))
                    csv.setCell(row, 7, set.DistanceUnits.fromSi(spec.FilterMinimumCumulativeGroundDistance));
                if (!std::isinf(spec.FilterMaximumCumulativeGroundDistance))
                    csv.setCell(row, 8, set.DistanceUnits.fromSi(spec.FilterMaximumCumulativeGroundDistance));
                if (!std::isnan(spec.FilterGroundDistanceThreshold))
                    csv.setCell(row, 9, set.DistanceUnits.fromSi(spec.FilterGroundDistanceThreshold));
                if (!std::isnan(spec.SpeedDeltaSegmentationThreshold))
                    csv.setCell(row, 10, set.SpeedUnits.fromSi(spec.SpeedDeltaSegmentationThreshold));

                csv.setCell(row, 11, PerformanceModelTypes.toString(spec.FlightsPerformanceMdl));
                csv.setCell(row, 12, static_cast<int>(spec.FlightsDoc29Segmentation));

                csv.setCell(row, 13, static_cast<int>(spec.Tracks4dCalculatePerformance));
                csv.setCell(row, 14, spec.Tracks4dMinimumPoints);
                csv.setCell(row, 15, static_cast<int>(spec.Tracks4dRecalculateCumulativeGroundDistance));
                csv.setCell(row, 16, static_cast<int>(spec.Tracks4dRecalculateGroundspeed));
                csv.setCell(row, 17, static_cast<int>(spec.Tracks4dRecalculateFuelFlow));

                csv.setCell(row, 18, FuelFlowModelTypes.toString(spec.FuelFlowMdl));
                csv.setCell(row, 19, static_cast<int>(spec.FuelFlowLTOAltitudeCorrection));

                ++row;
            }
        }

        if (row)
        {
            csv.write();
            Log::io()->info("Exported performance runs to '{}'.", CsvPath);
        }
    }

    void exportPerformanceRunsAtmospheres(const std::string& CsvPath) {
        const auto& study = Application::study();
        const auto& set = Application::settings();

        Csv csv;
        try { csv.setExport(CsvPath); }
        catch (const std::exception& err)
        {
            Log::io()->error("Exporting performance runs atmospheres to '{}'. {}", CsvPath, err.what());
            return;
        }

        csv.setColumnNames(
            "Scenario ID",
            "Performance Run ID",
            "Time",
            std::format("Temperature Delta ({})", set.TemperatureUnits.shortName()),
            std::format("Pressure Delta ({})", set.PressureUnits.shortName()),
            std::format("Wind Speed ({})", set.SpeedUnits.shortName()),
            "Wind Direction",
            "Relative Humidity"
        );

        std::size_t row = 0;

        for (const auto& scen : study.Scenarios)
        {
            for (const auto& perfRun : scen.PerformanceRuns | std::views::values)
            {
                for (const auto& [time, atm] : perfRun.PerfRunSpec.Atmospheres)
                {
                    csv.setCell(row, 0, scen.Name);
                    csv.setCell(row, 1, perfRun.Name);
                    csv.setCell(row, 2, timeToUtcString(time));
                    csv.setCell(row, 3, set.TemperatureUnits.fromSiDelta(atm.temperatureDelta()));
                    csv.setCell(row, 4, set.PressureUnits.fromSiDelta(atm.pressureDelta()));
                    csv.setCell(row, 5, set.SpeedUnits.fromSi(atm.windSpeed()));
                    if (!atm.isHeadwind())
                        csv.setCell(row, 6, atm.windDirection());
                    csv.setCell(row, 7, atm.relativeHumidity());

                    ++row;
                }
            }
        }

        if (row)
        {
            csv.write();
            Log::io()->info("Exported performance runs atmospheres to '{}'.", CsvPath);
        }
    }

    void exportNoiseRuns(const std::string& CsvPath) {
        const auto& study = Application::study();

        Csv csv;
        try { csv.setExport(CsvPath); }
        catch (const std::exception& err)
        {
            Log::io()->error("Exporting noise runs to '{}'. {}", CsvPath, err.what());
            return;
        }

        csv.setColumnNames(
            "Scenario ID",
            "Performance Run ID",
            "ID",
            "Noise Model",
            "Atmospheric Absorption",
            "Receptor Set Type",
            "Save Single Event Metrics"
        );

        std::size_t row = 0;
        for (const auto& scen : study.Scenarios)
        {
            for (const auto& perfRun : scen.PerformanceRuns | std::views::values)
            {
                for (const auto& nsRun : perfRun.NoiseRuns | std::views::values)
                {
                    const auto& spec = nsRun.NsRunSpec;
                    csv.setCell(row, 0, scen.Name);
                    csv.setCell(row, 1, perfRun.Name);
                    csv.setCell(row, 2, nsRun.Name);
                    csv.setCell(row, 3, NoiseModelTypes.toString(spec.NoiseMdl));
                    csv.setCell(row, 4, AtmosphericAbsorption::Types.toString(spec.AtmAbsorptionType));
                    csv.setCell(row, 5, ReceptorSet::Types.toString(spec.ReceptSet->type()));
                    csv.setCell(row, 6, static_cast<int>(spec.SaveSingleMetrics));

                    ++row;
                }
            }
        }

        if (row)
        {
            csv.write();
            Log::io()->info("Exported noise runs to '{}'.", CsvPath);
        }
    }

    void exportNoiseRunsReceptorsPoints(const std::string& CsvPath) {
        const auto& study = Application::study();
        const auto& set = Application::settings();

        Csv csv;
        try { csv.setExport(CsvPath); }
        catch (const std::exception& err)
        {
            Log::io()->error("Exporting point receptors to '{}'. {}", CsvPath, err.what());
            return;
        }

        csv.setColumnNames(
            "Scenario ID",
            "Performance Run ID",
            "Noise Run ID",
            "ID",
            "Longitude",
            "Latitude",
            std::format("Elevation ({})", set.AltitudeUnits.shortName())
        );

        std::size_t row = 0;
        for (const auto& scen : study.Scenarios)
        {
            for (const auto& perfRun : scen.PerformanceRuns | std::views::values)
            {
                for (const auto& nsRun : perfRun.NoiseRuns | std::views::values)
                {
                    if (nsRun.NsRunSpec.ReceptSet->type() != ReceptorSet::Type::Points)
                        continue;

                    const auto receptPts = static_cast<const ReceptorPoints*>(nsRun.NsRunSpec.ReceptSet.get());
                    for (const auto& [name, pt] : *receptPts)
                    {
                        csv.setCell(row, 0, scen.Name);
                        csv.setCell(row, 1, perfRun.Name);
                        csv.setCell(row, 2, nsRun.Name);
                        csv.setCell(row, 3, name);
                        csv.setCell(row, 4, pt.Longitude);
                        csv.setCell(row, 5, pt.Latitude);
                        csv.setCell(row, 6, set.AltitudeUnits.fromSi(pt.Elevation));
                        ++row;
                    }
                }
            }
        }

        if (row)
        {
            csv.write();
            Log::io()->info("Exported noise runs point receptors to '{}'.", CsvPath);
        }
    }

    void exportNoiseRunsReceptorsGrids(const std::string& CsvPath) {
        const auto& study = Application::study();
        const auto& set = Application::settings();

        Csv csv;
        try { csv.setExport(CsvPath); }
        catch (const std::exception& err)
        {
            Log::io()->error("Exporting grid receptors to '{}'. {}", CsvPath, err.what());
            return;
        }

        csv.setColumnNames(
            "Scenario ID",
            "Performance Run ID",
            "Noise Run ID",
            "Reference Location",
            "Reference Longitude",
            "Reference Latitude",
            std::format("Reference Altitude MSL ({})", set.AltitudeUnits.shortName()),
            std::format("Horizontal Spacing ({})", set.DistanceUnits.shortName()),
            std::format("Vertical Spacing ({})", set.DistanceUnits.shortName()),
            "Horizontal Count",
            "Vertical Count",
            "Grid Rotation"
        );

        std::size_t row = 0;
        for (const auto& scen : study.Scenarios)
        {
            for (const auto& perfRun : scen.PerformanceRuns | std::views::values)
            {
                for (const auto& nsRun : perfRun.NoiseRuns | std::views::values)
                {
                    if (nsRun.NsRunSpec.ReceptSet->type() != ReceptorSet::Type::Grid)
                        continue;

                    const auto receptGridPtr = static_cast<const ReceptorGrid*>(nsRun.NsRunSpec.ReceptSet.get());
                    const auto& receptGrid = *receptGridPtr;

                    csv.setCell(row, 0, scen.Name);
                    csv.setCell(row, 1, perfRun.Name);
                    csv.setCell(row, 2, nsRun.Name);
                    csv.setCell(row, 3, ReceptorGrid::Locations.toString(receptGrid.RefLocation));
                    csv.setCell(row, 4, receptGrid.RefLongitude);
                    csv.setCell(row, 5, receptGrid.RefLatitude);
                    csv.setCell(row, 6, set.AltitudeUnits.fromSi(receptGrid.RefAltitudeMsl));
                    csv.setCell(row, 7, set.DistanceUnits.fromSi(receptGrid.HorizontalSpacing));
                    csv.setCell(row, 8, set.DistanceUnits.fromSi(receptGrid.VerticalSpacing));
                    csv.setCell(row, 9, static_cast<int>(receptGrid.HorizontalCount));
                    csv.setCell(row, 10, static_cast<int>(receptGrid.VerticalCount));
                    csv.setCell(row, 11, receptGrid.GridRotation);
                    ++row;
                }
            }
        }

        if (row)
        {
            csv.write();
            Log::io()->info("Exported noise runs grid receptors to '{}'.", CsvPath);
        }
    }

    void exportNoiseRunsCumulativeMetrics(const std::string& CsvPath) {
        const auto& study = Application::study();

        Csv csv;
        try { csv.setExport(CsvPath); }
        catch (const std::exception& err)
        {
            Log::io()->error("Exporting noise runs cumulative metrics to '{}'. {}", CsvPath, err.what());
            return;
        }

        csv.setColumnNames(
            "Scenario ID",
            "Performance Run ID",
            "Noise Run ID",
            "ID",
            "Threshold (dB)",
            "Averaging Time Constant (dB)",
            "Start Time",
            "End Time",
            "Number Above Thresholds (dB)"
        );

        std::size_t row = 0;
        for (const auto& scen : study.Scenarios)
        {
            for (const auto& perfRun : scen.PerformanceRuns | std::views::values)
            {
                for (const auto& nsRun : perfRun.NoiseRuns | std::views::values)
                {
                    for (const auto& cumMetric : nsRun.CumulativeMetrics | std::views::values)
                    {
                        csv.setCell(row, 0, scen.Name);
                        csv.setCell(row, 1, perfRun.Name);
                        csv.setCell(row, 2, nsRun.Name);
                        csv.setCell(row, 3, cumMetric.Name);
                        csv.setCell(row, 4, cumMetric.Threshold);
                        csv.setCell(row, 5, cumMetric.AveragingTimeConstant);
                        csv.setCell(row, 6, timeToUtcString(cumMetric.StartTimePoint));
                        csv.setCell(row, 7, timeToUtcString(cumMetric.EndTimePoint));

                        if (!cumMetric.numberAboveThresholds().empty())
                        {
                            std::stringstream naThrSs;
                            for (const auto& naThr : cumMetric.numberAboveThresholds())
                                naThrSs << naThr << " ";
                            std::string naThrStr = naThrSs.str();
                            naThrStr.pop_back();
                            csv.setCell(row, 8, naThrStr);
                        }
                        ++row;
                    }
                }
            }
        }

        if (row)
        {
            csv.write();
            Log::io()->info("Exported noise runs cumulative metrics to '{}'.", CsvPath);
        }
    }

    void exportNoiseRunsCumulativeMetricsWeights(const std::string& CsvPath) {
        const auto& study = Application::study();

        Csv csv;
        try { csv.setExport(CsvPath); }
        catch (const std::exception& err)
        {
            Log::io()->error("Exporting noise runs cumulative metrics weights to '{}'. {}", CsvPath, err.what());
            return;
        }

        csv.setColumnNames(
            "Scenario ID",
            "Performance Run ID",
            "Noise Run ID",
            "Noise Run Cumulative Metric ID",
            "Time of Day",
            "Weight"
        );

        std::size_t row = 0;
        for (const auto& scen : study.Scenarios)
        {
            for (const auto& perfRun : scen.PerformanceRuns | std::views::values)
            {
                for (const auto& nsRun : perfRun.NoiseRuns | std::views::values)
                {
                    for (const auto& cumMetric : nsRun.CumulativeMetrics | std::views::values)
                    {
                        for (const auto& [time, weight] : cumMetric.weights())
                        {
                            csv.setCell(row, 0, scen.Name);
                            csv.setCell(row, 1, perfRun.Name);
                            csv.setCell(row, 2, nsRun.Name);
                            csv.setCell(row, 3, cumMetric.Name);
                            csv.setCell(row, 4, durationToString(time));
                            csv.setCell(row, 5, weight);
                            ++row;
                        }
                    }
                }
            }
        }

        if (row)
        {
            csv.write();
            Log::io()->info("Exported noise runs cumulative metrics weights to '{}'.", CsvPath);
        }
    }

    void exportEmissionsRuns(const std::string& CsvPath) {
        const auto& study = Application::study();
        const auto& set = Application::settings();

        Csv csv;
        try { csv.setExport(CsvPath); }
        catch (const std::exception& err)
        {
            Log::io()->error("Exporting emissions runs to '{}'. {}", CsvPath, err.what());
            return;
        }

        csv.setColumnNames(
            "Scenario ID",
            "Performance Run ID",
            "ID",
            "Calculate Gas Emissions",
            "Calculate Particle Emissions",
            "Emissions Model",
            "Use BFFM 2 for gas pollutant EIs"
            "Smoke Number to nvPM EI Model",
            "LTO Cycle Time Idle",
            "LTO Cycle Time Approach",
            "LTO Cycle Time Climb Out",
            "LTO Cycle Time Takeoff",
            "Particle Effective Density (kg/m3)",
            "Particle Geometric Standard Deviation",
            "Particle Geometric Mean Diameter Idle (nm)",
            "Particle Geometric Mean Diameter Approach (nm)",
            "Particle Geometric Mean Diameter Climb Out (nm)",
            "Particle Geometric Mean Diameter Takeoff (nm)",
            std::format("Minimum Altitude ({})", set.AltitudeUnits.shortName()),
            std::format("Maximum Altitude ({})", set.AltitudeUnits.shortName()),
            std::format("Minimum Cumulative Ground Distance ({})", set.DistanceUnits.shortName()),
            std::format("Maximum Cumulative Ground Distance ({})", set.DistanceUnits.shortName()),
            "Save Segment Results"
        );

        std::size_t row = 0;
        for (const auto& scen : study.Scenarios)
        {
            for (const auto& perfRun : scen.PerformanceRuns | std::views::values)
            {
                for (const auto& emiRun : perfRun.EmissionsRuns | std::views::values)
                {
                    std::size_t col = 0;
                    const auto& spec = emiRun.EmissionsRunSpec;
                    csv.setCell(row, col++, scen.Name);
                    csv.setCell(row, col++, perfRun.Name);
                    csv.setCell(row, col++, emiRun.Name);
                    csv.setCell(row, col++, static_cast<int>(spec.CalculateGasEmissions));
                    csv.setCell(row, col++, static_cast<int>(spec.CalculateParticleEmissions));
                    csv.setCell(row, col++, EmissionsModelTypes.toString(spec.EmissionsMdl));
                    csv.setCell(row, col++, static_cast<int>(spec.BFFM2Model));
                    csv.setCell(row, col++, EmissionsParticleSmokeNumberModelTypes.toString(spec.ParticleSmokeNumberModel));
                    csv.setCell(row, col++, spec.LTOCycle.at(0));
                    csv.setCell(row, col++, spec.LTOCycle.at(1));
                    csv.setCell(row, col++, spec.LTOCycle.at(2));
                    csv.setCell(row, col++, spec.LTOCycle.at(3));
                    csv.setCell(row, col++, spec.ParticleEffectiveDensity);
                    csv.setCell(row, col++, spec.ParticleGeometricStandardDeviation);
                    csv.setCell(row, col++, spec.ParticleGeometricMeanDiameter.at(0) * 1e9);
                    csv.setCell(row, col++, spec.ParticleGeometricMeanDiameter.at(1) * 1e9);
                    csv.setCell(row, col++, spec.ParticleGeometricMeanDiameter.at(2) * 1e9);
                    csv.setCell(row, col++, spec.ParticleGeometricMeanDiameter.at(3) * 1e9);
                    csv.setCell(row, col++, set.AltitudeUnits.fromSi(spec.FilterMinimumAltitude));
                    csv.setCell(row, col++, set.AltitudeUnits.fromSi(spec.FilterMaximumAltitude));
                    csv.setCell(row, col++, set.DistanceUnits.fromSi(spec.FilterMinimumCumulativeGroundDistance));
                    csv.setCell(row, col++, set.DistanceUnits.fromSi(spec.FilterMaximumCumulativeGroundDistance));
                    csv.setCell(row, col++, static_cast<int>(spec.SaveSegmentResults));
                    ++row;
                }
            }
        }

        if (row)
        {
            csv.write();
            Log::io()->info("Exported emissions runs to '{}'.", CsvPath);
        }
    }

    void exportDoc29Files(const std::string& FolderPath) {
        Application::get().queueAsyncTask([=] { exportDoc29Performance(std::format("{}/Doc29 Performance.csv", FolderPath)); }, "Exporting Doc29 Performance");

        Application::get().queueAsyncTask([=] { exportDoc29PerformanceAerodynamicCoefficients(std::format("{}/Doc29 Aerodynamic Coefficients.csv", FolderPath)); }, "Exporting Doc29 aerodynamic coefficients");

        Application::get().queueAsyncTask([=] { exportDoc29PerformanceThrustRatings(std::format("{}/Doc29 Thrust Ratings.csv", FolderPath)); }, "Exporting Doc29 thrust ratings");

        Application::get().queueAsyncTask([=] { exportDoc29PerformanceThrustRatingsPropeller(std::format("{}/Doc29 Thrust Ratings Propeller.csv", FolderPath)); }, "Exporting Doc29 thrust propeller ratings");

        Application::get().queueAsyncTask([=] { exportDoc29PerformanceProfilesPoints(std::format("{}/Doc29 Profiles Points.csv", FolderPath)); }, "Exporting Doc29 point profiles");

        Application::get().queueAsyncTask([=] { exportDoc29PerformanceProfilesArrivalSteps(std::format("{}/Doc29 Profiles Procedural Arrival.csv", FolderPath)); }, "Exporting Doc29 arrival procedural profiles");

        Application::get().queueAsyncTask([=] { exportDoc29PerformanceProfilesDepartureSteps(std::format("{}/Doc29 Profiles Procedural Departure.csv", FolderPath)); }, "Exporting Doc29 departure procedural profiles");

        Application::get().queueAsyncTask([=] { exportDoc29Noise(std::format("{}/Doc29 Noise.csv", FolderPath)); }, "Exporting Doc29 Noise");

        Application::get().queueAsyncTask([=] { exportDoc29NoiseNpd(std::format("{}/Doc29 Noise NPD.csv", FolderPath)); }, "Exporting Doc29 NPD data");

        Application::get().queueAsyncTask([=] { exportDoc29NoiseSpectrum(std::format("{}/Doc29 Noise Spectrum.csv", FolderPath)); }, "Exporting Doc29 Noise spectrum");
    }

    void exportDatasetFiles(const std::string& FolderPath) {
        exportDoc29Files(FolderPath);

        Application::get().queueAsyncTask([=] { exportLTO(std::format("{}/LTO Engines.csv", FolderPath)); }, "Exporting LTO engines");

        Application::get().queueAsyncTask([=] { exportSFI(std::format("{}/SFI.csv", FolderPath)); }, "Exporting SFI coefficients");

        Application::get().queueAsyncTask([=] { exportFleet(std::format("{}/Fleet.csv", FolderPath)); }, "Exporting fleet");
    }

    void exportInputDataFiles(const std::string& FolderPath) {
        Application::get().queueAsyncTask([=] { exportAirports(std::format("{}/Airports.csv", FolderPath)); }, "Exporting airports");

        Application::get().queueAsyncTask([=] { exportRunways(std::format("{}/Runways.csv", FolderPath)); }, "Exporting runways");

        Application::get().queueAsyncTask([=] { exportRoutesSimple(std::format("{}/Routes Simple.csv", FolderPath)); }, "Exporting simple routes");

        Application::get().queueAsyncTask([=] { exportRoutesVectors(std::format("{}/Routes Vector.csv", FolderPath)); }, "Exporting vector routes");

        Application::get().queueAsyncTask([=] { exportRoutesRnp(std::format("{}/Routes RNP.csv", FolderPath)); }, "Exporting RNP routes");

        Application::get().queueAsyncTask([=] { exportFlights(std::format("{}/Flights.csv", FolderPath)); }, "Exporting flights");

        Application::get().queueAsyncTask([=] { exportTracks4d(std::format("{}/Tracks 4D.csv", FolderPath)); }, "Exporting 4D tracks");

        Application::get().queueAsyncTask([=] { exportTracks4dPoints(std::format("{}/Tracks 4D Points.csv", FolderPath)); }, "Exporting tracks 4D points");

        Application::get().queueAsyncTask([=] { exportScenarios(std::format("{}/Scenarios.csv", FolderPath)); }, "Exporting scenarios");

        Application::get().queueAsyncTask([=] { exportScenariosOperations(std::format("{}/Scenarios Operations.csv", FolderPath)); }, "Exporting scenarios operations");

        Application::get().queueAsyncTask([=] { exportPerformanceRuns(std::format("{}/Performance Runs.csv", FolderPath)); }, "Exporting performance runs");

        Application::get().queueAsyncTask([=] { exportPerformanceRunsAtmospheres(std::format("{}/Performance Runs Atmospheres.csv", FolderPath)); }, "Exporting performance runs");

        Application::get().queueAsyncTask([=] { exportNoiseRuns(std::format("{}/Noise Runs.csv", FolderPath)); }, "Exporting noise runs");

        Application::get().queueAsyncTask([=] { exportNoiseRunsReceptorsGrids(std::format("{}/Noise Runs Grid Receptors.csv", FolderPath)); }, "Exporting noise runs grid receptors");

        Application::get().queueAsyncTask([=] { exportNoiseRunsReceptorsPoints(std::format("{}/Noise Runs Point Receptors.csv", FolderPath)); }, "Exporting noise runs point receptors");

        Application::get().queueAsyncTask([=] { exportNoiseRunsCumulativeMetrics(std::format("{}/Noise Runs Cumulative Metrics.csv", FolderPath)); }, "Exporting noise runs cumulative metrics");

        Application::get().queueAsyncTask([=] { exportNoiseRunsCumulativeMetricsWeights(std::format("{}/Noise Runs Cumulative Metrics Weights.csv", FolderPath)); }, "Exporting noise runs cumulative metrics weights");

        Application::get().queueAsyncTask([=] { exportEmissionsRuns(std::format("{}/Emissions Runs.csv", FolderPath)); }, "Exporting emissions runs");
    }

    void exportAllFiles(const std::string& FolderPath) {
        exportDatasetFiles(FolderPath);
        exportInputDataFiles(FolderPath);
    }

    void exportPerformanceOutput(const PerformanceOutput& PerfOut, const std::string& CsvPath) {
        const auto& set = Application::settings();

        Csv csv;
        try { csv.setExport(CsvPath); }
        catch (const std::exception& err)
        {
            Log::io()->error("Exporting performance output to '{}'. {}", CsvPath, err.what());
            return;
        }

        csv.setColumnNames(
            "Point Number",
            "Point Origin",
            "Flight Phase",
            std::format("Cumulative Ground Distance ({})", set.DistanceUnits.shortName()),
            "Longitude",
            "Latitude",
            std::format("Altitude MSL ({})", set.AltitudeUnits.shortName()),
            std::format("True Airspeed ({})", set.SpeedUnits.shortName()),
            std::format("Ground Speed ({})", set.SpeedUnits.shortName()),
            std::format("Corrected Net Thrust per Engine ({})", set.ThrustUnits.shortName()),
            "Bank Angle",
            std::format("Fuel Flow per Engine ({})", set.FuelFlowUnits.shortName())
        );

        std::size_t row = 0;
        for (auto it = PerfOut.begin(); it != PerfOut.end(); ++it)
        {
            const auto& [cumGroundDist, pt] = *it;
            csv.setCell(row, 0, static_cast<int>(std::distance(PerfOut.begin(), it)));
            csv.setCell(row, 1, PerformanceOutput::Origins.toString(pt.PtOrigin));
            csv.setCell(row, 2, FlightPhases.toString(pt.FlPhase));
            csv.setCell(row, 3, set.DistanceUnits.fromSi(cumGroundDist));
            csv.setCell(row, 4, pt.Longitude);
            csv.setCell(row, 5, pt.Latitude);
            csv.setCell(row, 6, set.AltitudeUnits.fromSi(pt.AltitudeMsl));
            csv.setCell(row, 7, set.SpeedUnits.fromSi(pt.TrueAirspeed));
            csv.setCell(row, 8, set.SpeedUnits.fromSi(pt.Groundspeed));
            csv.setCell(row, 9, set.ThrustUnits.fromSi(pt.CorrNetThrustPerEng));
            csv.setCell(row, 10, set.FuelFlowUnits.fromSi(pt.FuelFlowPerEng));
            ++row;
        }

        csv.write();
    }

    void exportPerformanceRunOutput(const PerformanceRunOutput& PerfRunOut, const std::string& CsvPath) {
        const auto& set = Application::settings();

        Csv csv;
        try { csv.setExport(CsvPath); }
        catch (const std::exception& err)
        {
            Log::io()->error("Exporting performance run output to '{}'. {}", CsvPath, err.what());
            return;
        }

        csv.setColumnNames(
            "Name",
            "Operation",
            "Type",
            "Point Number",
            "Point Origin",
            "Flight Phase",
            std::format("Cumulative Ground Distance ({})", set.DistanceUnits.shortName()),
            "Longitude",
            "Latitude",
            std::format("Altitude MSL ({})", set.AltitudeUnits.shortName()),
            std::format("True Airspeed ({})", set.SpeedUnits.shortName()),
            std::format("Ground Speed ({})", set.SpeedUnits.shortName()),
            std::format("Corrected Net Thrust per Engine ({})", set.ThrustUnits.shortName()),
            "Bank Angle",
            std::format("Fuel Flow per Engine ({})", set.FuelFlowUnits.shortName())
        );

        std::size_t row = 0;
        for (const auto& opRef : PerfRunOut.arrivalOutputs())
        {
            const auto& op = opRef.get();
            PerformanceOutput perfOut = PerfRunOut.arrivalOutput(op);
            for (auto it = perfOut.begin(); it != perfOut.end(); ++it)
            {
                csv.setCell(row, 0, op.Name);
                csv.setCell(row, 1, OperationTypes.toString(op.operationType()));
                csv.setCell(row, 2, Operation::Types.toString(op.type()));

                const auto& [cumGroundDist, pt] = *it;
                csv.setCell(row, 3, static_cast<int>(std::distance(perfOut.begin(), it)));
                csv.setCell(row, 4, PerformanceOutput::Origins.toString(pt.PtOrigin));
                csv.setCell(row, 5, FlightPhases.toString(pt.FlPhase));
                csv.setCell(row, 6, set.DistanceUnits.fromSi(cumGroundDist));
                csv.setCell(row, 7, pt.Longitude);
                csv.setCell(row, 8, pt.Latitude);
                csv.setCell(row, 9, set.AltitudeUnits.fromSi(pt.AltitudeMsl));
                csv.setCell(row, 10, set.SpeedUnits.fromSi(pt.TrueAirspeed));
                csv.setCell(row, 11, set.SpeedUnits.fromSi(pt.Groundspeed));
                csv.setCell(row, 12, set.ThrustUnits.fromSi(pt.CorrNetThrustPerEng));
                csv.setCell(row, 13, pt.BankAngle);
                csv.setCell(row, 14, set.FuelFlowUnits.fromSi(pt.FuelFlowPerEng));
                ++row;
            }
        }

        for (const auto& opRef : PerfRunOut.departureOutputs())
        {
            const auto& op = opRef.get();
            PerformanceOutput perfOut = PerfRunOut.departureOutput(op);
            for (auto it = perfOut.begin(); it != perfOut.end(); ++it)
            {
                csv.setCell(row, 0, op.Name);
                csv.setCell(row, 1, OperationTypes.toString(op.operationType()));
                csv.setCell(row, 2, Operation::Types.toString(op.type()));

                const auto& [cumGroundDist, pt] = *it;
                csv.setCell(row, 3, static_cast<int>(std::distance(perfOut.begin(), it)));
                csv.setCell(row, 4, PerformanceOutput::Origins.toString(pt.PtOrigin));
                csv.setCell(row, 5, FlightPhases.toString(pt.FlPhase));
                csv.setCell(row, 6, set.DistanceUnits.fromSi(cumGroundDist));
                csv.setCell(row, 7, pt.Longitude);
                csv.setCell(row, 8, pt.Latitude);
                csv.setCell(row, 9, set.AltitudeUnits.fromSi(pt.AltitudeMsl));
                csv.setCell(row, 10, set.SpeedUnits.fromSi(pt.TrueAirspeed));
                csv.setCell(row, 11, set.SpeedUnits.fromSi(pt.Groundspeed));
                csv.setCell(row, 12, set.ThrustUnits.fromSi(pt.CorrNetThrustPerEng));
                csv.setCell(row, 13, pt.BankAngle);
                csv.setCell(row, 14, set.FuelFlowUnits.fromSi(pt.FuelFlowPerEng));
                ++row;
            }
        }
        csv.write();
    }

    void exportNoiseSingleEventOutput(const NoiseSingleEventOutput& NsSingleEventOutput, const ReceptorOutput& ReceptOut, const std::string& CsvPath) {
        GRAPE_ASSERT(NsSingleEventOutput.size() == ReceptOut.size());

        const Settings& set = Application::settings();

        Csv csv;
        try { csv.setExport(CsvPath); }
        catch (const std::exception& err)
        {
            Log::io()->error("Exporting noise single event output to '{}'. {}", CsvPath, err.what());
            return;
        }

        csv.setColumnNames(
            "Receptor ID",
            "Longitude",
            "Latitude",
            std::format("Elevation ({})", set.AltitudeUnits.shortName()),
            "Maximum (dB)",
            "Exposure (dB)"
        );

        for (std::size_t row = 0; row < NsSingleEventOutput.size(); ++row)
        {
            const auto& recept = ReceptOut.receptor(row);
            csv.setCell(row, 0, recept.Name);
            csv.setCell(row, 1, recept.Longitude);
            csv.setCell(row, 2, recept.Latitude);
            csv.setCell(row, 3, set.AltitudeUnits.fromSi(recept.Elevation));

            const auto& [lamax, sel] = NsSingleEventOutput.values(row);
            csv.setCell(row, 4, lamax);
            csv.setCell(row, 5, sel);
        }
        csv.write();
    }

    void exportNoiseCumulativeMetricOutput(const NoiseCumulativeMetric& NsCumMetric, const NoiseCumulativeOutput& NsCumMetricOut, const ReceptorOutput& ReceptOut, const std::string& CsvPath) {
        GRAPE_ASSERT(ReceptOut.size() == NsCumMetricOut.Exposure.size());

        const Settings& set = Application::settings();

        Csv csv;
        try { csv.setExport(CsvPath); }
        catch (const std::exception& err)
        {
            Log::io()->error("Exporting noise cumulative metric output to '{}'. {}", CsvPath, err.what());
            return;
        }

        csv.setColumnNames(
            "Receptor ID",
            "Longitude",
            "Latitude",
            std::format("Elevation ({})", set.AltitudeUnits.shortName()),
            "Count",
            "Weighted Count"
            "Maximum Absolute (dB)",
            "Maximum Average (dB)",
            "Exposure (dB)"
        );

        std::size_t column = 9;
        for (const auto& naThr : NsCumMetric.numberAboveThresholds())
            csv.setColumnName(++column, std::format("# Above {:.2f}", naThr));

        for (std::size_t row = 0; row < ReceptOut.size(); ++row)
        {
            const auto& recept = ReceptOut.receptor(row);
            csv.setCell(row, 0, recept.Name);
            csv.setCell(row, 1, recept.Longitude);
            csv.setCell(row, 2, recept.Latitude);
            csv.setCell(row, 3, set.AltitudeUnits.fromSi(recept.Elevation));
            csv.setCell(row, 4, NsCumMetricOut.Count.at(row));
            csv.setCell(row, 5, NsCumMetricOut.CountWeighted.at(row));
            csv.setCell(row, 6, NsCumMetricOut.MaximumAbsolute.at(row));
            csv.setCell(row, 7, NsCumMetricOut.MaximumAverage.at(row));
            csv.setCell(row, 8, NsCumMetricOut.Exposure.at(row));

            std::size_t col = 8;
            for (const auto& naVec : NsCumMetricOut.NumberAboveThresholds)
                csv.setCell(row, ++col, naVec.at(row));
        }
        csv.write();
    }

    void exportEmissionsSegmentOutput(const EmissionsOperationOutput& EmiOpOut, const std::string& CsvPath) {
        const Settings& set = Application::settings();

        Csv csv;
        try { csv.setExport(CsvPath); }
        catch (const std::exception& err)
        {
            Log::io()->error("Exporting emissions segment output to '{}'. {}", CsvPath, err.what());
            return;
        }

        csv.setColumnNames(
            "Segment Index",
            std::format("Fuel ({})", set.EmissionsWeightUnits.shortName()),
            std::format("HC ({})", set.EmissionsWeightUnits.shortName()),
            std::format("CO ({})", set.EmissionsWeightUnits.shortName()),
            std::format("NOx ({})", set.EmissionsWeightUnits.shortName()),
            "nvPM (mg/kg)",
            "nvPM Number"
        );

        std::size_t row = 0;
        csv.setCell(row, 0, std::string("Total")); // rapidcsv does not implement const char*
        csv.setCell(row, 1, set.EmissionsWeightUnits.fromSi(EmiOpOut.totalFuel()));
        csv.setCell(row, 2, set.EmissionsWeightUnits.fromSi(EmiOpOut.totalEmissions().HC));
        csv.setCell(row, 3, set.EmissionsWeightUnits.fromSi(EmiOpOut.totalEmissions().CO));
        csv.setCell(row, 4, set.EmissionsWeightUnits.fromSi(EmiOpOut.totalEmissions().NOx));
        csv.setCell(row, 5, toMilligramsPerKilogram(EmiOpOut.totalEmissions().nvPM));
        csv.setCell(row, 6, EmiOpOut.totalEmissions().nvPMNumber);

        ++row;
        for (const auto& segOut : EmiOpOut.segmentOutput())
        {
            csv.setCell(row, 0, segOut.Index);
            csv.setCell(row, 1, set.EmissionsWeightUnits.fromSi(segOut.Fuel));
            csv.setCell(row, 2, set.EmissionsWeightUnits.fromSi(segOut.Emissions.HC));
            csv.setCell(row, 3, set.EmissionsWeightUnits.fromSi(segOut.Emissions.CO));
            csv.setCell(row, 4, set.EmissionsWeightUnits.fromSi(segOut.Emissions.NOx));
            csv.setCell(row, 5, toMilligramsPerKilogram(segOut.Emissions.nvPM));
            csv.setCell(row, 6, segOut.Emissions.nvPMNumber);
            ++row;
        }
        csv.write();
    }

    void exportEmissionsRunOutput(const EmissionsRunOutput& EmiRunOutput, const std::string& CsvPath) {
        const Settings& set = Application::settings();

        Csv csv;
        try { csv.setExport(CsvPath); }
        catch (const std::exception& err)
        {
            Log::io()->error("Exporting emissions run output to '{}'. {}", CsvPath, err.what());
            return;
        }

        csv.setColumnNames(
            "Name",
            "Operation",
            "Type",
            std::format("Fuel ({})", set.EmissionsWeightUnits.shortName()),
            std::format("HC ({})", set.EmissionsWeightUnits.shortName()),
            std::format("CO ({})", set.EmissionsWeightUnits.shortName()),
            std::format("NOx ({})", set.EmissionsWeightUnits.shortName()),
            "nvPM (mg/kg)",
            "nvPM Number"
        );

        std::size_t row = 0;
        csv.setCell(row, 0, std::string("Total")); // rapidcsv does not implement const char*
        csv.setCell(row, 3, set.EmissionsWeightUnits.fromSi(EmiRunOutput.totalFuel()));
        csv.setCell(row, 4, set.EmissionsWeightUnits.fromSi(EmiRunOutput.totalEmissions().HC));
        csv.setCell(row, 5, set.EmissionsWeightUnits.fromSi(EmiRunOutput.totalEmissions().CO));
        csv.setCell(row, 6, set.EmissionsWeightUnits.fromSi(EmiRunOutput.totalEmissions().NOx));
        csv.setCell(row, 7, toMilligramsPerKilogram(EmiRunOutput.totalEmissions().nvPM));
        csv.setCell(row, 8, EmiRunOutput.totalEmissions().nvPMNumber);

        ++row;
        for (const auto& [op, opFlEmiOut] : EmiRunOutput)
        {
            csv.setCell(row, 0, op->Name);
            csv.setCell(row, 1, OperationTypes.toString(op->operationType()));
            csv.setCell(row, 2, Operation::Types.toString(op->type()));
            csv.setCell(row, 3, set.EmissionsWeightUnits.fromSi(opFlEmiOut.totalFuel()));
            csv.setCell(row, 4, set.EmissionsWeightUnits.fromSi(opFlEmiOut.totalEmissions().HC));
            csv.setCell(row, 5, set.EmissionsWeightUnits.fromSi(opFlEmiOut.totalEmissions().CO));
            csv.setCell(row, 6, set.EmissionsWeightUnits.fromSi(opFlEmiOut.totalEmissions().NOx));
            csv.setCell(row, 7, toMilligramsPerKilogram(opFlEmiOut.totalEmissions().nvPM));
            csv.setCell(row, 8, opFlEmiOut.totalEmissions().nvPMNumber);
            ++row;
        }
        csv.write();
    }
}
