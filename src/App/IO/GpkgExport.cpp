// Copyright (C) 2023 Goncalo Soares Roque 

#include "GRAPE_pch.h"

#include "GpkgExport.h"

#include "Airport/RouteCalculator.h"
#include "Application.h"
#include "Embed/GrapeGeopackageSchema.embed"
#include "Schema/SchemaGpkg.h"

namespace GRAPE::IO::GPKG {
    namespace {
        constexpr int Srs = 4326;
        constexpr const char* DataType = "features";
        constexpr const char* GeometryColumn = "geometry";
        constexpr std::uint8_t G = 71;
        constexpr std::uint8_t P = 80;
        constexpr std::uint8_t EndianFlag = std::endian::native == std::endian::big ? 0 : 1;

        enum class GeometryType : int {
            Point = 1,
            LineString = 2,
        };
        constexpr EnumStrings<GeometryType> GeometryTypes{ "POINT", "LINESTRING" };

        enum WkbGeometryType : std::uint32_t {
            WkbPointZ = 1001,
            WkbLineStringZ = 1002,
        };

        std::optional<Database> createGeoPackage(const std::string& Path) {
            std::error_code sysErr;
            if (std::filesystem::exists(Path))
                Log::study()->warn("Creating geopackage file at '{}'. The file already exists and will be overwritten.", Path);

            Database db;
            if (!db.create(Path, g_GrapeGeopackageSchema, sizeof g_GrapeGeopackageSchema))
                return std::nullopt;


            return db;
        }

        void addHeaderToBlob(Blob& Blb) {
            Blb.add(G);
            Blb.add(P);
            Blb.add(static_cast<std::uint8_t>(0));
            Blb.add(EndianFlag);
            Blb.add(static_cast<std::uint32_t>(Srs));
            Blb.add(EndianFlag);
        }

        void addToContentsTable(const Database& Gpkg, std::string_view Name) {
            Gpkg.insert(Schema::GPKG::gpkg_contents, {}, std::make_tuple(
                std::string(Name),
                DataType,
                std::monostate(),
                std::monostate(),
                std::format("{0:%F}T{0:%T}", std::chrono::system_clock::now()),
                std::monostate(),
                std::monostate(),
                std::monostate(),
                std::monostate(),
                Srs)
            );
        }

        void addToGeometryColumnsTable(const Database& Gpkg, std::string_view Name, GeometryType GeoType, int Z = 1, int M = 0) {
            Gpkg.insert(Schema::GPKG::gpkg_geometry_columns, {}, std::make_tuple(
                std::string(Name),
                GeometryColumn,
                GeometryTypes.toString(GeoType),
                Srs,
                Z,
                M)
            );
        }
    }

    void exportAirports(const std::string& Path) {
        auto dbOpt = createGeoPackage(Path);
        if (!dbOpt.has_value())
            return;

        Database gpkg = dbOpt.value();

        const auto& study = Application::study();

        const Geodesic wgs84;
        RouteCalculator rteCalc(wgs84);

        addToContentsTable(gpkg, Schema::GPKG::grape_airports.name());
        addToContentsTable(gpkg, Schema::GPKG::grape_runways_points.name());
        addToContentsTable(gpkg, Schema::GPKG::grape_runways_lines.name());
        addToContentsTable(gpkg, Schema::GPKG::grape_routes.name());

        addToGeometryColumnsTable(gpkg, Schema::GPKG::grape_airports.name(), GeometryType::Point);
        addToGeometryColumnsTable(gpkg, Schema::GPKG::grape_runways_points.name(), GeometryType::Point);
        addToGeometryColumnsTable(gpkg, Schema::GPKG::grape_runways_lines.name(), GeometryType::LineString);
        addToGeometryColumnsTable(gpkg, Schema::GPKG::grape_routes.name(), GeometryType::LineString);

        for (const auto& apt : study.Airports)
        {
            Blob aptBlob;
            addHeaderToBlob(aptBlob);
            aptBlob.add(WkbPointZ);
            aptBlob.add(apt.Longitude);
            aptBlob.add(apt.Latitude);
            aptBlob.add(apt.Elevation);

            gpkg.insert(Schema::GPKG::grape_airports, {}, std::make_tuple(
                std::monostate(),
                aptBlob,
                apt.Name)
            );

            for (const auto& rwy : apt.Runways | std::views::values)
            {
                Blob rwyPtBlob;
                addHeaderToBlob(rwyPtBlob);
                rwyPtBlob.add(WkbPointZ);
                rwyPtBlob.add(rwy.Longitude);
                rwyPtBlob.add(rwy.Latitude);
                rwyPtBlob.add(rwy.Elevation);

                gpkg.insert(Schema::GPKG::grape_runways_points, {}, std::make_tuple(
                    std::monostate(),
                    rwyPtBlob,
                    rwy.parentAirport().Name,
                    rwy.Name)
                );

                Blob rwyLnBlob;
                auto [rwyEndLon, rwyEndLat] = wgs84.point(rwy.Longitude, rwy.Latitude, rwy.Length, rwy.Heading);
                addHeaderToBlob(rwyLnBlob);
                rwyLnBlob.add(WkbLineStringZ);
                rwyLnBlob.add(static_cast<std::uint32_t>(2));
                rwyLnBlob.add(rwy.Longitude);
                rwyLnBlob.add(rwy.Latitude);
                rwyLnBlob.add(rwy.Elevation);
                rwyLnBlob.add(rwyEndLon);
                rwyLnBlob.add(rwyEndLat);
                rwyLnBlob.add(rwy.elevationEnd());

                gpkg.insert(Schema::GPKG::grape_runways_lines, {}, std::make_tuple(
                    std::monostate(),
                    rwyLnBlob,
                    rwy.parentAirport().Name,
                    rwy.Name)
                );

                for (const auto& rte : rwy.ArrivalRoutes | std::views::values)
                {
                    const RouteOutput rteOut = rteCalc.calculate(*rte);

                    Blob rteBlob;
                    addHeaderToBlob(rteBlob);
                    rteBlob.add(WkbLineStringZ);
                    rteBlob.add(static_cast<std::uint32_t>(rteOut.size()));
                    for (const auto& pt : rteOut | std::views::values)
                    {
                        rteBlob.add(pt.Longitude);
                        rteBlob.add(pt.Latitude);
                        rteBlob.add(rte->parentRunway().Elevation);
                    }

                    gpkg.insert(Schema::GPKG::grape_routes, {}, std::make_tuple(
                        std::monostate(),
                        rteBlob,
                        rte->parentAirport().Name,
                        rte->parentRunway().Name,
                        rte->Name,
                        OperationTypes.toString(rte->operationType()),
                        Route::Types.toString(rte->type()))
                    );
                }

                for (const auto& rte : rwy.DepartureRoutes | std::views::values)
                {
                    const RouteOutput rteOut = rteCalc.calculate(*rte);

                    Blob rteBlob;
                    addHeaderToBlob(rteBlob);
                    rteBlob.add(WkbLineStringZ);
                    rteBlob.add(static_cast<std::uint32_t>(rteOut.size()));
                    for (const auto& pt : rteOut | std::views::values)
                    {
                        rteBlob.add(pt.Longitude);
                        rteBlob.add(pt.Latitude);
                        rteBlob.add(rte->parentRunway().Elevation);
                    }

                    gpkg.insert(Schema::GPKG::grape_routes, {}, std::make_tuple(
                        std::monostate(),
                        rteBlob,
                        rte->parentAirport().Name,
                        rte->parentRunway().Name,
                        rte->Name,
                        OperationTypes.toString(rte->operationType()),
                        Route::Types.toString(rte->type()))
                    );
                }
            }
        }
    }

    void exportPerformanceRunOutput(const PerformanceRun& PerfRun, const std::string& Path) {
        auto dbOpt = createGeoPackage(Path);
        if (!dbOpt.has_value())
            return;

        Database gpkg = dbOpt.value();

        addToContentsTable(gpkg, Schema::GPKG::grape_performance_run.name());
        addToGeometryColumnsTable(gpkg, Schema::GPKG::grape_performance_run.name(), GeometryType::LineString);

        for (const auto& opRef : PerfRun.output().arrivalOutputs())
        {
            const auto& op = opRef.get();
            const PerformanceOutput perfOut = PerfRun.output().arrivalOutput(op);

            Blob opBlob;
            addHeaderToBlob(opBlob);
            opBlob.add(WkbLineStringZ);
            opBlob.add(static_cast<std::uint32_t>(perfOut.size()));

            for (const auto& pt : perfOut | std::views::values)
            {
                opBlob.add(pt.Longitude);
                opBlob.add(pt.Latitude);
                opBlob.add(pt.AltitudeMsl);
            }

            gpkg.insert(Schema::GPKG::grape_performance_run, {}, std::make_tuple(
                std::monostate(),
                opBlob,
                op.Name,
                OperationTypes.toString(op.operationType()),
                Operation::Types.toString(op.type()),
                timeToUtcString(op.Time),
                op.Count,
                op.aircraft().Name)
            );
        }

        for (const auto& opRef : PerfRun.output().departureOutputs())
        {
            const auto& op = opRef.get();
            const PerformanceOutput perfOut = PerfRun.output().departureOutput(op);

            Blob opBlob;
            addHeaderToBlob(opBlob);
            opBlob.add(WkbLineStringZ);
            opBlob.add(static_cast<std::uint32_t>(perfOut.size()));

            for (const auto& pt : perfOut | std::views::values)
            {
                opBlob.add(pt.Longitude);
                opBlob.add(pt.Latitude);
                opBlob.add(pt.AltitudeMsl);
            }

            gpkg.insert(Schema::GPKG::grape_performance_run, {}, std::make_tuple(
                std::monostate(),
                opBlob,
                op.Name,
                OperationTypes.toString(op.operationType()),
                Operation::Types.toString(op.type()),
                timeToUtcString(op.Time),
                op.Count,
                op.aircraft().Name)
            );
        }
    }

    void exportNoiseRunOutput(const NoiseRun& NsRun, const std::string& Path) {
        auto dbOpt = createGeoPackage(Path);
        if (!dbOpt.has_value())
            return;

        Database gpkg = dbOpt.value();

        addToContentsTable(gpkg, Schema::GPKG::grape_noise_run_receptors.name());
        addToContentsTable(gpkg, Schema::GPKG::grape_noise_run_cumulative_noise.name());
        addToContentsTable(gpkg, Schema::GPKG::grape_noise_run_cumulative_number_above.name());

        addToGeometryColumnsTable(gpkg, Schema::GPKG::grape_noise_run_receptors.name(), GeometryType::Point);
        addToGeometryColumnsTable(gpkg, Schema::GPKG::grape_noise_run_cumulative_noise.name(), GeometryType::Point);
        addToGeometryColumnsTable(gpkg, Schema::GPKG::grape_noise_run_cumulative_number_above.name(), GeometryType::Point);

        const auto& receptors = NsRun.output().receptors();

        // Receptor Grid
        gpkg.beginTransaction();
        std::ranges::for_each(receptors, [&gpkg, &NsRun](const Receptor& Recept) {
            Blob rBlob;
            addHeaderToBlob(rBlob);
            rBlob.add(WkbPointZ);
            rBlob.add(Recept.Longitude);
            rBlob.add(Recept.Latitude);
            rBlob.add(Recept.Elevation);
            gpkg.insert(Schema::GPKG::grape_noise_run_receptors, {}, std::make_tuple(
                std::monostate(),
                rBlob,
                NsRun.parentScenario().Name,
                NsRun.parentPerformanceRun().Name,
                NsRun.Name)
            );
            });
        gpkg.commitTransaction();

        // Cumulative Output
        for (const auto& [metric, output] : NsRun.output().cumulativeOutputs())
        {
            gpkg.beginTransaction();
            for (std::size_t i = 0; i < receptors.size(); ++i)
            {
                const auto& recept = receptors(i);
                Blob rBlob;
                addHeaderToBlob(rBlob);
                rBlob.add(WkbPointZ);
                rBlob.add(recept.Longitude);
                rBlob.add(recept.Latitude);
                rBlob.add(recept.Elevation);

                gpkg.insert(Schema::GPKG::grape_noise_run_cumulative_noise, {}, std::make_tuple(
                    std::monostate(),
                    rBlob,
                    NsRun.Name,
                    metric->Name,
                    output.Count.at(i),
                    output.CountWeighted.at(i),
                    output.MaximumAbsolute.at(i),
                    output.MaximumAverage.at(i),
                    output.Exposure.at(i))
                );

                for (std::size_t j = 0; j < metric->numberAboveThresholds().size(); ++j)
                    gpkg.insert(Schema::GPKG::grape_noise_run_cumulative_number_above, {}, std::make_tuple(
                        std::monostate(),
                        rBlob,
                        NsRun.Name,
                        metric->Name,
                        metric->numberAboveThresholds().at(j),
                        output.NumberAboveThresholds.at(j).at(i))
                    );
            }
            gpkg.commitTransaction();
        }
    }
}
