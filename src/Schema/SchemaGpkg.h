/**
* @file SchemaGpkg.h
* File automatically generated with python script "GenerateSchemaSourceFiles.py".
*/

#pragma once

template<std::size_t>      
class GRAPE::Table;

namespace GRAPE::Schema::GPKG {
    extern const Table<6> gpkg_spatial_ref_sys;

    extern const Table<6> gpkg_geometry_columns;

    extern const Table<10> gpkg_contents;

    extern const Table<3> grape_airports;

    extern const Table<7> grape_routes;

    extern const Table<4> grape_runways_points;

    extern const Table<4> grape_runways_lines;

    extern const Table<8> grape_performance_run;

    extern const Table<8> grape_noise_run_cumulative_noise;

    extern const Table<5> grape_noise_run_cumulative_number_above;

    extern const Table<2> grape_noise_run_receptors;
}
    