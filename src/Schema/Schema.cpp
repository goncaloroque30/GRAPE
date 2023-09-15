/**
* @file Schema.cpp
* File automatically generated with python script "GenerateSchemaSourceFiles.py".
*/

#include "GRAPE_pch.h"

#include "Database/Table.h"

namespace GRAPE::Schema {
    extern const Table airports_routes("airports_routes",
        {
            "airport_id",
            "runway_id",
            "operation",
            "id",
            "type",
        }
    );

    extern const Table airports_routes_simple("airports_routes_simple",
        {
            "airport_id",
            "runway_id",
            "operation",
            "route_id",
            "point_number",
            "longitude",
            "latitude",
        }
    );

    extern const Table airports_runways("airports_runways",
        {
            "airport_id",
            "id",
            "longitude",
            "latitude",
            "elevation",
            "length",
            "heading",
            "gradient",
        }
    );

    extern const Table doc29_noise("doc29_noise",
        {
            "id",
            "lateral_directivity",
            "start_of_roll_correction",
        }
    );

    extern const Table doc29_noise_npd_data("doc29_noise_npd_data",
        {
            "noise_id",
            "operation",
            "noise_metric",
            "thrust",
            "l_200_ft",
            "l_400_ft",
            "l_630_ft",
            "l_1000_ft",
            "l_2000_ft",
            "l_4000_ft",
            "l_6300_ft",
            "l_10000_ft",
            "l_16000_ft",
            "l_25000_ft",
        }
    );

    extern const Table doc29_noise_spectrum("doc29_noise_spectrum",
        {
            "noise_id",
            "operation",
            "l_50_hz",
            "l_63_hz",
            "l_80_hz",
            "l_100_hz",
            "l_125_hz",
            "l_160_hz",
            "l_200_hz",
            "l_250_hz",
            "l_315_hz",
            "l_400_hz",
            "l_500_hz",
            "l_630_hz",
            "l_800_hz",
            "l_1000_hz",
            "l_1250_hz",
            "l_1600_hz",
            "l_2000_hz",
            "l_2500_hz",
            "l_3150_hz",
            "l_4000_hz",
            "l_5000_hz",
            "l_6300_hz",
            "l_8000_hz",
            "l_10000_hz",
        }
    );

    extern const Table doc29_performance_profiles("doc29_performance_profiles",
        {
            "performance_id",
            "operation",
            "id",
            "type",
        }
    );

    extern const Table doc29_performance_thrust("doc29_performance_thrust",
        {
            "performance_id",
            "type",
        }
    );

    extern const Table doc29_performance_thrust_rating_coefficients("doc29_performance_thrust_rating_coefficients",
        {
            "performance_id",
            "thrust_rating",
            "e",
            "f",
            "ga",
            "gb",
            "h",
        }
    );

    extern const Table doc29_performance_thrust_rating_coefficients_propeller("doc29_performance_thrust_rating_coefficients_propeller",
        {
            "performance_id",
            "thrust_rating",
            "efficiency",
            "propulsive_power",
        }
    );

    extern const Table doc29_performance_thrust_ratings("doc29_performance_thrust_ratings",
        {
            "performance_id",
            "thrust_rating",
        }
    );

    extern const Table sfi_fuel("sfi_fuel",
        {
            "id",
            "a",
            "b1",
            "b2",
            "b3",
            "k1",
            "k2",
            "k3",
            "k4",
        }
    );

    extern const Table operations_flights("operations_flights",
        {
            "id",
            "airport_id",
            "runway_id",
            "operation",
            "route_id",
            "time",
            "count",
            "fleet_id",
            "weight",
        }
    );

    extern const Table operations_tracks_4d("operations_tracks_4d",
        {
            "id",
            "operation",
            "time",
            "count",
            "fleet_id",
        }
    );

    extern const Table scenarios("scenarios",
        {
            "id",
        }
    );

    extern const Table performance_run_output("performance_run_output",
        {
            "scenario_id",
            "performance_run_id",
            "operation_id",
            "operation",
            "operation_type",
        }
    );

    extern const Table noise_run_cumulative_metrics_weights("noise_run_cumulative_metrics_weights",
        {
            "scenario_id",
            "performance_run_id",
            "noise_run_id",
            "noise_run_cumulative_metric_id",
            "time_of_day",
            "weight",
        }
    );

    extern const Table noise_run_cumulative_metrics_number_above_thresholds("noise_run_cumulative_metrics_number_above_thresholds",
        {
            "scenario_id",
            "performance_run_id",
            "noise_run_id",
            "noise_run_cumulative_metric_id",
            "threshold",
        }
    );

    extern const Table noise_run_output_receptors("noise_run_output_receptors",
        {
            "scenario_id",
            "performance_run_id",
            "noise_run_id",
            "id",
            "longitude",
            "latitude",
            "altitude_msl",
        }
    );

    extern const Table noise_run_receptor_points("noise_run_receptor_points",
        {
            "scenario_id",
            "performance_run_id",
            "noise_run_id",
            "id",
            "longitude",
            "latitude",
            "altitude_msl",
        }
    );

    extern const Table doc29_performance("doc29_performance",
        {
            "id",
            "type",
        }
    );

    extern const Table noise_run("noise_run",
        {
            "scenario_id",
            "performance_run_id",
            "id",
            "noise_model",
            "atmospheric_absorption",
            "receptor_set_type",
            "save_single_event_metrics",
        }
    );

    extern const Table doc29_performance_aerodynamic_coefficients("doc29_performance_aerodynamic_coefficients",
        {
            "performance_id",
            "flap_id",
            "type",
            "r",
            "b",
            "c",
            "d",
        }
    );

    extern const Table doc29_performance_profiles_departure_procedural("doc29_performance_profiles_departure_procedural",
        {
            "performance_id",
            "operation",
            "profile_id",
            "step_number",
            "step_type",
            "thrust_cutback",
            "flap_id",
            "parameter_1",
            "parameter_2",
        }
    );

    extern const Table airports_routes_rnp("airports_routes_rnp",
        {
            "airport_id",
            "runway_id",
            "operation",
            "route_id",
            "step_number",
            "step_type",
            "longitude",
            "latitude",
            "center_longitude",
            "center_latitude",
        }
    );

    extern const Table lto_fuel_emissions("lto_fuel_emissions",
        {
            "id",
            "fuel_flow_idle",
            "fuel_flow_approach",
            "fuel_flow_climb_out",
            "fuel_flow_takeoff",
            "fuel_flow_correction_factor_idle",
            "fuel_flow_correction_factor_approach",
            "fuel_flow_correction_factor_climb_out",
            "fuel_flow_correction_factor_takeoff",
            "emission_index_hc_idle",
            "emission_index_hc_approach",
            "emission_index_hc_climb_out",
            "emission_index_hc_takeoff",
            "emission_index_co_idle",
            "emission_index_co_approach",
            "emission_index_co_climb_out",
            "emission_index_co_takeoff",
            "emission_index_nox_idle",
            "emission_index_nox_approach",
            "emission_index_nox_climb_out",
            "emission_index_nox_takeoff",
        }
    );

    extern const Table noise_run_cumulative_metrics("noise_run_cumulative_metrics",
        {
            "scenario_id",
            "performance_run_id",
            "noise_run_id",
            "id",
            "threshold_db",
            "averaging_time_constant_db",
            "start_time",
            "end_time",
        }
    );

    extern const Table noise_run_output_single_event("noise_run_output_single_event",
        {
            "scenario_id",
            "performance_run_id",
            "noise_run_id",
            "receptor_id",
            "operation_id",
            "operation",
            "operation_type",
            "maximum_db",
            "exposure_db",
        }
    );

    extern const Table noise_run_output_cumulative_number_above("noise_run_output_cumulative_number_above",
        {
            "scenario_id",
            "performance_run_id",
            "noise_run_id",
            "noise_run_cumulative_metric_id",
            "threshold_db",
            "receptor_id",
            "number_above",
        }
    );

    extern const Table noise_run_receptor_grid("noise_run_receptor_grid",
        {
            "scenario_id",
            "performance_run_id",
            "noise_run_id",
            "reference_location",
            "reference_longitude",
            "reference_latitude",
            "reference_altitude_msl",
            "horizontal_spacing",
            "vertical_spacing",
            "horizontal_count",
            "vertical_count",
            "grid_rotation",
        }
    );

    extern const Table operations_flights_arrival("operations_flights_arrival",
        {
            "operation_id",
            "operation",
            "doc29_profile_id",
        }
    );

    extern const Table scenarios_flights("scenarios_flights",
        {
            "scenario_id",
            "operation_id",
            "operation",
        }
    );

    extern const Table scenarios_tracks_4d("scenarios_tracks_4d",
        {
            "scenario_id",
            "operation_id",
            "operation",
        }
    );

    extern const Table doc29_performance_profiles_points("doc29_performance_profiles_points",
        {
            "performance_id",
            "operation",
            "profile_id",
            "cumulative_ground_distance",
            "altitude_ate",
            "true_airspeed",
            "corrected_net_thrust_per_engine",
        }
    );

    extern const Table airports_routes_vectors("airports_routes_vectors",
        {
            "airport_id",
            "runway_id",
            "operation",
            "route_id",
            "step_number",
            "type",
            "distance",
            "turn_radius",
            "heading_change",
            "turn_direction",
        }
    );

    extern const Table fleet("fleet",
        {
            "id",
            "engine_count",
            "maximum_sea_level_static_thrust",
            "engine_breakpoint_temperature",
            "doc29_performance_id",
            "sfi_id",
            "lto_engine_id",
            "doc29_noise_id",
            "doc29_noise_arrival_delta_db",
            "doc29_noise_departure_delta_db",
        }
    );

    extern const Table airports("airports",
        {
            "id",
            "longitude",
            "latitude",
            "elevation",
            "reference_temperature",
            "reference_sea_level_pressure",
        }
    );

    extern const Table emissions_run_output("emissions_run_output",
        {
            "scenario_id",
            "performance_run_id",
            "emissions_run_id",
            "fuel",
            "hc",
            "co",
            "nox",
        }
    );

    extern const Table emissions_run("emissions_run",
        {
            "scenario_id",
            "performance_run_id",
            "id",
            "emissions_model",
            "emissions_maximum_altitude",
            "emissions_minimum_altitude",
            "emissions_maximum_cumulative_ground_distance",
            "emissions_minimum_cumulative_ground_distance",
            "save_segment_results",
        }
    );

    extern const Table emissions_run_output_operations("emissions_run_output_operations",
        {
            "scenario_id",
            "performance_run_id",
            "emissions_run_id",
            "operation_id",
            "operation",
            "operation_type",
            "fuel",
            "hc",
            "co",
            "nox",
        }
    );

    extern const Table emissions_run_output_segments("emissions_run_output_segments",
        {
            "scenario_id",
            "performance_run_id",
            "emissions_run_id",
            "operation_id",
            "operation",
            "operation_type",
            "segment_number",
            "fuel",
            "hc",
            "co",
            "nox",
        }
    );

    extern const Table noise_run_output_cumulative("noise_run_output_cumulative",
        {
            "scenario_id",
            "performance_run_id",
            "noise_run_id",
            "noise_run_cumulative_metric_id",
            "receptor_id",
            "count",
            "count_weighted",
            "maximum_absolute_db",
            "maximum_average_db",
            "exposure_db",
        }
    );

    extern const Table performance_run_atmospheres("performance_run_atmospheres",
        {
            "scenario_id",
            "performance_run_id",
            "time",
            "temperature_delta",
            "pressure_delta",
            "wind_speed",
            "wind_direction",
            "relative_humidity",
        }
    );

    extern const Table operations_flights_departure("operations_flights_departure",
        {
            "operation_id",
            "operation",
            "doc29_profile_id",
            "thrust_percentage_takeoff",
            "thrust_percentage_climb",
        }
    );

    extern const Table performance_run_output_points("performance_run_output_points",
        {
            "scenario_id",
            "performance_run_id",
            "operation_id",
            "operation",
            "operation_type",
            "point_number",
            "point_origin",
            "flight_phase",
            "cumulative_ground_distance",
            "longitude",
            "latitude",
            "altitude_msl",
            "true_airspeed",
            "ground_speed",
            "corrected_net_thrust_per_engine",
            "bank_angle",
            "fuel_flow_per_engine",
        }
    );

    extern const Table doc29_performance_profiles_arrival_procedural("doc29_performance_profiles_arrival_procedural",
        {
            "performance_id",
            "operation",
            "profile_id",
            "step_number",
            "step_type",
            "flap_id",
            "parameter_1",
            "parameter_2",
            "parameter_3",
        }
    );

    extern const Table operations_tracks_4d_points("operations_tracks_4d_points",
        {
            "operation_id",
            "operation",
            "point_number",
            "flight_phase",
            "cumulative_ground_distance",
            "longitude",
            "latitude",
            "altitude_msl",
            "true_airspeed",
            "groundspeed",
            "corrected_net_thrust_per_engine",
            "bank_angle",
            "fuel_flow_per_engine",
        }
    );

    extern const Table performance_run("performance_run",
        {
            "scenario_id",
            "id",
            "coordinate_system_type",
            "coordinate_system_longitude_0",
            "coordinate_system_latitude_0",
            "filter_minimum_altitude",
            "filter_maximum_altitude",
            "filter_minimum_cumulative_ground_distance",
            "filter_maximum_cumulative_ground_distance",
            "filter_ground_distance_threshold",
            "segmentation_speed_delta_threshold",
            "flights_performance_model",
            "flights_doc29_low_altitude_segmentation",
            "tracks_4d_minimum_points",
            "tracks_4d_recalculate_cumulative_ground_distance",
            "tracks_4d_recalculate_groundspeed",
            "tracks_4d_recalculate_fuel_flow",
            "fuel_flow_model",
        }
    );
}
    