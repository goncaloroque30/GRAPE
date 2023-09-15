/**
* @file SchemaGpkg.cpp
* File automatically generated with python script "GenerateSchemaSourceFiles.py".
*/

#include "GRAPE_pch.h"

#include "Database/Table.h"

namespace GRAPE::Schema::GPKG {
    extern const Table gpkg_spatial_ref_sys("gpkg_spatial_ref_sys",
        {
            "srs_name",
            "srs_id",
            "organization",
            "organization_coordsys_id",
            "definition",
            "description",
        }
    );

    extern const Table gpkg_geometry_columns("gpkg_geometry_columns",
        {
            "table_name",
            "column_name",
            "geometry_type_name",
            "srs_id",
            "z",
            "m",
        }
    );

    extern const Table gpkg_contents("gpkg_contents",
        {
            "table_name",
            "data_type",
            "identifier",
            "description",
            "last_change",
            "min_x",
            "min_y",
            "max_x",
            "max_y",
            "srs_id",
        }
    );

    extern const Table grape_airports("grape_airports",
        {
            "id",
            "geometry",
            "airport",
        }
    );

    extern const Table grape_routes("grape_routes",
        {
            "id",
            "geometry",
            "airport",
            "runway",
            "route",
            "operation",
            "type",
        }
    );

    extern const Table grape_runways_points("grape_runways_points",
        {
            "id",
            "geometry",
            "airport",
            "runway",
        }
    );

    extern const Table grape_runways_lines("grape_runways_lines",
        {
            "id",
            "geometry",
            "airport",
            "runway",
        }
    );

    extern const Table grape_performance_run("grape_performance_run",
        {
            "id",
            "geometry",
            "name",
            "operation",
            "type",
            "time",
            "count",
            "fleet_id",
        }
    );

    extern const Table grape_noise_run_cumulative_noise("grape_noise_run_cumulative_noise",
        {
            "id",
            "geometry",
            "cumulative_metric",
            "count",
            "count_weighted",
            "maximum_absolute_db",
            "maximum_average_db",
            "exposure_db",
        }
    );

    extern const Table grape_noise_run_cumulative_number_above("grape_noise_run_cumulative_number_above",
        {
            "id",
            "geometry",
            "cumulative_metric",
            "threshold",
            "number",
        }
    );

    extern const Table grape_noise_run_receptors("grape_noise_run_receptors",
        {
            "id",
            "geometry",
        }
    );
}
    