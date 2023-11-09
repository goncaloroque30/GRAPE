#pragma once

namespace GRAPE::Schema::Elevator11 {
    constexpr std::string_view g_lto_fuel_emissions = R"(
CREATE TEMP TABLE grape_table AS SELECT * FROM lto_fuel_emissions;

DROP TABLE lto_fuel_emissions;

CREATE TABLE lto_fuel_emissions (
    id                                    TEXT    NOT NULL,
    fuel_flow_idle                        REAL    CHECK (fuel_flow_idle >= 0.0) 
                                                  DEFAULT (0.0) 
                                                  NOT NULL,
    fuel_flow_approach                    REAL    CHECK (fuel_flow_approach >= 0.0) 
                                                  DEFAULT (0.0) 
                                                  NOT NULL,
    fuel_flow_climb_out                   REAL    CHECK (fuel_flow_climb_out >= 0.0) 
                                                  DEFAULT (0.0) 
                                                  NOT NULL,
    fuel_flow_takeoff                     REAL    CHECK (fuel_flow_takeoff >= 0.0) 
                                                  DEFAULT (0.0) 
                                                  NOT NULL,
    fuel_flow_correction_factor_idle      REAL    CHECK (fuel_flow_correction_factor_idle >= 0.0) 
                                                  DEFAULT (1.1) 
                                                  NOT NULL,
    fuel_flow_correction_factor_approach  REAL    CHECK (fuel_flow_correction_factor_approach >= 0.0) 
                                                  DEFAULT (1.02) 
                                                  NOT NULL,
    fuel_flow_correction_factor_climb_out REAL    CHECK (fuel_flow_correction_factor_climb_out >= 0.0) 
                                                  DEFAULT (1.013) 
                                                  NOT NULL,
    fuel_flow_correction_factor_takeoff   REAL    CHECK (fuel_flow_correction_factor_takeoff >= 0.0) 
                                                  DEFAULT (1.01) 
                                                  NOT NULL,
    emission_index_hc_idle                REAL    CHECK (emission_index_hc_idle >= 0.0) 
                                                  DEFAULT (0.0) 
                                                  NOT NULL,
    emission_index_hc_approach            REAL    CHECK (emission_index_hc_approach >= 0.0) 
                                                  DEFAULT (0.0) 
                                                  NOT NULL,
    emission_index_hc_climb_out           REAL    CHECK (emission_index_hc_climb_out >= 0.0) 
                                                  DEFAULT (0.0) 
                                                  NOT NULL,
    emission_index_hc_takeoff             REAL    CHECK (emission_index_hc_takeoff >= 0.0) 
                                                  DEFAULT (0.0) 
                                                  NOT NULL,
    emission_index_co_idle                REAL    NOT NULL
                                                  CHECK (emission_index_co_idle >= 0.0) 
                                                  DEFAULT (0.0),
    emission_index_co_approach            REAL    CHECK (emission_index_co_approach >= 0.0) 
                                                  DEFAULT (0.0) 
                                                  NOT NULL,
    emission_index_co_climb_out           REAL    CHECK (emission_index_co_climb_out >= 0.0) 
                                                  DEFAULT (0.0) 
                                                  NOT NULL,
    emission_index_co_takeoff             REAL    CHECK (emission_index_co_takeoff >= 0.0) 
                                                  DEFAULT (0.0) 
                                                  NOT NULL,
    emission_index_nox_idle               REAL    NOT NULL
                                                  CHECK (emission_index_nox_idle >= 0.0) 
                                                  DEFAULT (0.0),
    emission_index_nox_approach           REAL    NOT NULL
                                                  CHECK (emission_index_nox_approach >= 0.0) 
                                                  DEFAULT (0.0),
    emission_index_nox_climb_out          REAL    NOT NULL
                                                  CHECK (emission_index_nox_climb_out >= 0.0) 
                                                  DEFAULT (0.0),
    emission_index_nox_takeoff            REAL    NOT NULL
                                                  CHECK (emission_index_nox_takeoff >= 0.0) 
                                                  DEFAULT (0.0),
    mixed_nozzle                          INTEGER CHECK (mixed_nozzle IN (0, 1) ) 
                                                  NOT NULL
                                                  DEFAULT (1),
    bypass_ratio                          REAL    NOT NULL
                                                  CHECK (bypass_ratio >= 0.0) 
                                                  DEFAULT (0.0),
    air_fuel_ratio_idle                   REAL    CHECK (air_fuel_ratio_idle >= 0.0) 
                                                  NOT NULL
                                                  DEFAULT (106.0),
    air_fuel_ratio_approach               REAL    CHECK (air_fuel_ratio_approach >= 0.0) 
                                                  NOT NULL
                                                  DEFAULT (83.0),
    air_fuel_ratio_climb_out              REAL    CHECK (air_fuel_ratio_climb_out >= 0.0) 
                                                  NOT NULL
                                                  DEFAULT (51.0),
    air_fuel_ratio_takeoff                REAL    CHECK (air_fuel_ratio_takeoff >= 0.0) 
                                                  NOT NULL
                                                  DEFAULT (45.0),
    smoke_number_idle                     REAL    CHECK (smoke_number_idle >= 0.0),
    smoke_number_approach                 REAL    CHECK (smoke_number_approach >= 0.0),
    smoke_number_climb_out                REAL    CHECK (smoke_number_climb_out >= 0.0),
    smoke_number_takeoff                  REAL    CHECK (smoke_number_takeoff >= 0.0),
    emission_index_nvpm_idle              REAL    CHECK (emission_index_nvpm_idle >= 0.0),
    emission_index_nvpm_approach          REAL    CHECK (emission_index_nvpm_approach >= 0.0),
    emission_index_nvpm_climb_out         REAL    CHECK (emission_index_nvpm_climb_out >= 0.0),
    emission_index_nvpm_takeoff           REAL    CHECK (emission_index_nvpm_takeoff >= 0.0),
    emission_index_nvpm_number_idle       REAL    CHECK (emission_index_nvpm_number_idle >= 0.0),
    emission_index_nvpm_number_approach   REAL    CHECK (emission_index_nvpm_number_approach >= 0.0),
    emission_index_nvpm_number_climb_out  REAL    CHECK (emission_index_nvpm_number_climb_out >= 0.0),
    emission_index_nvpm_number_takeoff    REAL    CHECK (emission_index_nvpm_number_takeoff >= 0.0),
    PRIMARY KEY (
        id
    )
);

INSERT INTO lto_fuel_emissions (
    id,
    fuel_flow_idle,
    fuel_flow_approach,
    fuel_flow_climb_out,
    fuel_flow_takeoff,
    fuel_flow_correction_factor_idle,
    fuel_flow_correction_factor_approach,
    fuel_flow_correction_factor_climb_out,
    fuel_flow_correction_factor_takeoff,
    emission_index_hc_idle,
    emission_index_hc_approach,
    emission_index_hc_climb_out,
    emission_index_hc_takeoff,
    emission_index_co_idle,
    emission_index_co_approach,
    emission_index_co_climb_out,
    emission_index_co_takeoff,
    emission_index_nox_idle,
    emission_index_nox_approach,
    emission_index_nox_climb_out,
    emission_index_nox_takeoff
) SELECT
    id,
    fuel_flow_idle,
    fuel_flow_approach,
    fuel_flow_climb_out,
    fuel_flow_takeoff,
    fuel_flow_correction_factor_idle,
    fuel_flow_correction_factor_approach,
    fuel_flow_correction_factor_climb_out,
    fuel_flow_correction_factor_takeoff,
    emission_index_hc_idle,
    emission_index_hc_approach,
    emission_index_hc_climb_out,
    emission_index_hc_takeoff,
    emission_index_co_idle,
    emission_index_co_approach,
    emission_index_co_climb_out,
    emission_index_co_takeoff,
    emission_index_nox_idle,
    emission_index_nox_approach,
    emission_index_nox_climb_out,
    emission_index_nox_takeoff
FROM temp.grape_table;

DROP TABLE temp.grape_table;
)";

    constexpr std::string_view g_operations_flights = R"(
CREATE TEMP TABLE grape_table AS SELECT * FROM operations_flights;

DROP TABLE operations_flights;

CREATE TABLE operations_flights (
    id         TEXT NOT NULL,
    operation  TEXT NOT NULL,
    airport_id TEXT,
    runway_id  TEXT,
    route_id   TEXT,
    time       TEXT NOT NULL
                    DEFAULT (datetime() ),
    count      REAL NOT NULL
                    CHECK (count >= 0.0) 
                    DEFAULT (1),
    fleet_id   TEXT NOT NULL,
    weight     REAL NOT NULL
                    CHECK (weight >= 0.000001) 
                    DEFAULT (10),
    PRIMARY KEY (
        id,
        operation
    ),
    CONSTRAINT fk_route FOREIGN KEY (
        airport_id,
        runway_id,
        operation,
        route_id
    )
    REFERENCES airports_routes (airport_id,
    runway_id,
    operation,
    id) ON DELETE CASCADE
        ON UPDATE CASCADE,
    CONSTRAINT fk_fleet FOREIGN KEY (
        fleet_id
    )
    REFERENCES fleet (id) ON DELETE CASCADE
                          ON UPDATE CASCADE
);

INSERT INTO operations_flights (id, operation, airport_id, runway_id, route_id, time, count, fleet_id, weight)
SELECT id, operation, airport_id, runway_id, route_id, time, count, fleet_id, weight FROM temp.grape_table;

DROP TABLE temp.grape_table;
)";

    constexpr std::string_view g_performance_run = R"(
CREATE TEMP TABLE grape_table AS SELECT * FROM performance_run;

DROP TABLE performance_run;

CREATE TABLE performance_run (
    scenario_id                                      TEXT    NOT NULL,
    id                                               TEXT    NOT NULL,
    coordinate_system_type                           TEXT    CHECK (coordinate_system_type IN ('Geodesic WGS84', 'Local Cartesian') ) 
                                                             DEFAULT ('Geodesic WGS84') 
                                                             NOT NULL,
    coordinate_system_longitude_0                    REAL    CHECK (coordinate_system_longitude_0 BETWEEN -180.0 AND 180.0) 
                                                             DEFAULT (0.0),
    coordinate_system_latitude_0                     REAL    CHECK (coordinate_system_latitude_0 BETWEEN -90.0 AND 90.0) 
                                                             DEFAULT (0.0),
    filter_minimum_altitude                          REAL,
    filter_maximum_altitude                          REAL,
    filter_minimum_cumulative_ground_distance        REAL,
    filter_maximum_cumulative_ground_distance        REAL,
    filter_ground_distance_threshold                 REAL    CHECK (filter_ground_distance_threshold >= 0.0),
    segmentation_speed_delta_threshold               REAL    CHECK (segmentation_speed_delta_threshold > 0.0),
    flights_performance_model                        TEXT    CHECK (flights_performance_model IN ('None', 'Doc29') ) 
                                                             NOT NULL
                                                             DEFAULT ('Doc29'),
    flights_doc29_low_altitude_segmentation          INTEGER DEFAULT (1) 
                                                             CHECK (flights_doc29_low_altitude_segmentation IN (0, 1) ),
    tracks_4d_calculate_performance                  INTEGER DEFAULT (1) 
                                                             CHECK (tracks_4d_calculate_performance IN (0, 1) ),
    tracks_4d_minimum_points                         INTEGER CHECK (tracks_4d_minimum_points >= 0) 
                                                             DEFAULT (1),
    tracks_4d_recalculate_cumulative_ground_distance INTEGER DEFAULT (0) 
                                                             CHECK (tracks_4d_recalculate_cumulative_ground_distance IN (0, 1) ),
    tracks_4d_recalculate_groundspeed                INTEGER DEFAULT (0) 
                                                             CHECK (tracks_4d_recalculate_groundspeed IN (0, 1) ),
    tracks_4d_recalculate_fuel_flow                  INTEGER DEFAULT (0) 
                                                             CHECK (tracks_4d_recalculate_fuel_flow IN (0, 1) ),
    fuel_flow_model                                  TEXT    CHECK (fuel_flow_model IN ('None', 'LTO', 'LTO Doc9889', 'SFI') ) 
                                                             NOT NULL
                                                             DEFAULT ('None'),
    fuel_flow_lto_altitude_correction                INTEGER DEFAULT (1) 
                                                             CHECK (fuel_flow_lto_altitude_correction IN (0, 1) ),
    PRIMARY KEY (
        scenario_id,
        id
    ),
    CONSTRAINT fk_scenario FOREIGN KEY (
        scenario_id
    )
    REFERENCES scenarios (id) ON DELETE CASCADE
                              ON UPDATE CASCADE
);

INSERT INTO performance_run (
    scenario_id,
    id,
    coordinate_system_type,
    coordinate_system_longitude_0,
    coordinate_system_latitude_0,
    filter_minimum_altitude,
    filter_maximum_altitude,
    filter_minimum_cumulative_ground_distance,
    filter_maximum_cumulative_ground_distance,
    filter_ground_distance_threshold,
    segmentation_speed_delta_threshold,
    flights_performance_model,
    flights_doc29_low_altitude_segmentation,
    tracks_4d_minimum_points,
    tracks_4d_recalculate_cumulative_ground_distance,
    tracks_4d_recalculate_groundspeed,
    tracks_4d_recalculate_fuel_flow,
    fuel_flow_model
)
SELECT
    scenario_id,
    id,
    coordinate_system_type,
    coordinate_system_longitude_0,
    coordinate_system_latitude_0,
    filter_minimum_altitude,
    filter_maximum_altitude,
    filter_minimum_cumulative_ground_distance,
    filter_maximum_cumulative_ground_distance,
    filter_ground_distance_threshold,
    segmentation_speed_delta_threshold,
    flights_performance_model,
    flights_doc29_low_altitude_segmentation,
    tracks_4d_minimum_points,
    tracks_4d_recalculate_cumulative_ground_distance,
    tracks_4d_recalculate_groundspeed,
    tracks_4d_recalculate_fuel_flow,
    fuel_flow_model
FROM temp.grape_table;

DROP TABLE temp.grape_table;
)";

    constexpr std::string_view g_emissions_run = R"(
CREATE TEMP TABLE grape_table AS SELECT * FROM emissions_run;

UPDATE temp.grape_table SET emissions_model = 'Segments';

DROP TABLE emissions_run;

CREATE TABLE emissions_run (
    scenario_id                                  TEXT    NOT NULL,
    performance_run_id                           TEXT    NOT NULL,
    id                                           TEXT    NOT NULL,
    calculate_gas_emissions                      INTEGER CHECK (calculate_gas_emissions IN (0, 1) ) 
                                                         NOT NULL
                                                         DEFAULT (1),
    calculate_particle_emissions                 INTEGER CHECK (calculate_particle_emissions IN (0, 1) ) 
                                                         NOT NULL
                                                         DEFAULT (1),
    emissions_model                              TEXT    NOT NULL
                                                         CHECK (emissions_model IN ('LTO Cycle', 'Segments') ) 
                                                         DEFAULT ('Segments'),
    bffm2_gas_emission_indexes                   INTEGER CHECK (bffm2_gas_emission_indexes IN (0, 1) ) 
                                                         NOT NULL
                                                         DEFAULT (1),
    emissions_model_particles_smoke_number       TEXT    NOT NULL
                                                         CHECK (emissions_model_particles_smoke_number IN ('None', 'FOA 3', 'FOA 4') ) 
                                                         DEFAULT ('FOA 4'),
    lto_cycle_idle                               REAL    NOT NULL
                                                         CHECK (lto_cycle_idle >= 0) 
                                                         DEFAULT (1560),
    lto_cycle_approach                           REAL    DEFAULT (240) 
                                                         NOT NULL
                                                         CHECK (lto_cycle_approach >= 0),
    lto_cycle_climb                              REAL    NOT NULL
                                                         CHECK (lto_cycle_climb >= 0) 
                                                         DEFAULT (132),
    lto_cycle_takeoff                            REAL    CHECK (lto_cycle_takeoff >= 0) 
                                                         NOT NULL
                                                         DEFAULT (42),
    particle_effective_density                   REAL    CHECK (particle_effective_density > 0.0) 
                                                         NOT NULL
                                                         DEFAULT (1000.0),
    particle_geometric_standard_deviation        REAL    CHECK (particle_geometric_standard_deviation > 0.0) 
                                                         NOT NULL
                                                         DEFAULT (1.8),
    particle_geometric_mean_diameter_idle        REAL    CHECK (particle_geometric_mean_diameter_idle > 0.0) 
                                                         NOT NULL
                                                         DEFAULT (0.00000004),
    particle_geometric_mean_diameter_approach    REAL    CHECK (particle_geometric_mean_diameter_approach > 0.0) 
                                                         NOT NULL
                                                         DEFAULT (0.00000004),
    particle_geometric_mean_diameter_climb_out   REAL    CHECK (particle_geometric_mean_diameter_climb_out > 0.0) 
                                                         NOT NULL
                                                         DEFAULT (0.00000002),
    particle_geometric_mean_diameter_takeoff     REAL    NOT NULL
                                                         DEFAULT (0.00000002) 
                                                         CHECK (particle_geometric_mean_diameter_takeoff > 0.0),
    emissions_minimum_altitude                   REAL,
    emissions_maximum_altitude                   REAL,
    emissions_minimum_cumulative_ground_distance REAL,
    emissions_maximum_cumulative_ground_distance REAL,
    save_segment_results                         INTEGER CHECK (save_segment_results IN (0, 1) ) 
                                                         NOT NULL
                                                         DEFAULT (0),
    PRIMARY KEY (
        scenario_id,
        performance_run_id,
        id
    ),
    CONSTRAINT fk_performance_run FOREIGN KEY (
        scenario_id,
        performance_run_id
    )
    REFERENCES performance_run (scenario_id,
    id) ON DELETE CASCADE
        ON UPDATE CASCADE
);

INSERT INTO emissions_run (
    scenario_id,
    performance_run_id,
    id,
    emissions_model,
    emissions_minimum_altitude,
    emissions_maximum_altitude,
    emissions_minimum_cumulative_ground_distance,
    emissions_maximum_cumulative_ground_distance,
    save_segment_results,
    calculate_particle_emissions
)
SELECT
    scenario_id,
    performance_run_id,
    id,
    emissions_model,
    emissions_maximum_altitude,
    emissions_minimum_altitude,
    emissions_maximum_cumulative_ground_distance,
    emissions_minimum_cumulative_ground_distance,
    save_segment_results,
    0
FROM temp.grape_table;

DROP TABLE temp.grape_table;
)";

    constexpr std::string_view g_emissions_run_output = R"(
CREATE TEMP TABLE grape_table AS SELECT * FROM emissions_run_output;

DROP TABLE emissions_run_output;

CREATE TABLE emissions_run_output (
    scenario_id        TEXT NOT NULL,
    performance_run_id TEXT NOT NULL,
    emissions_run_id   TEXT NOT NULL,
    fuel               REAL NOT NULL,
    hc                 REAL NOT NULL,
    co                 REAL NOT NULL,
    nox                REAL NOT NULL,
    nvpm               REAL NOT NULL,
    nvpm_number        REAL NOT NULL,
    PRIMARY KEY (
        scenario_id,
        performance_run_id,
        emissions_run_id
    ),
    CONSTRAINT fk_emissions_run FOREIGN KEY (
        scenario_id,
        performance_run_id,
        emissions_run_id
    )
    REFERENCES emissions_run (scenario_id,
    performance_run_id,
    id) ON DELETE CASCADE
        ON UPDATE CASCADE
);

INSERT INTO emissions_run_output (
    scenario_id,
    performance_run_id,
    emissions_run_id,
    fuel,
    hc,
    co,
    nox,
    nvpm,
    nvpm_number
)
SELECT
    scenario_id,
    performance_run_id,
    emissions_run_id,
    fuel,
    hc,
    co,
    nox,
    0.0,
    0.0
FROM temp.grape_table;

DROP TABLE temp.grape_table;
)";

    constexpr std::string_view g_emissions_run_output_operations = R"(
CREATE TEMP TABLE grape_table AS SELECT * FROM emissions_run_output_operations;

DROP TABLE emissions_run_output_operations;

CREATE TABLE emissions_run_output_operations (
    scenario_id        TEXT NOT NULL,
    performance_run_id TEXT NOT NULL,
    emissions_run_id   TEXT NOT NULL,
    operation_id       TEXT NOT NULL,
    operation          TEXT NOT NULL,
    operation_type     TEXT NOT NULL,
    fuel               REAL NOT NULL,
    hc                 REAL NOT NULL,
    co                 REAL NOT NULL,
    nox                REAL NOT NULL,
    nvpm               REAL NOT NULL,
    nvpm_number        REAL NOT NULL,
    PRIMARY KEY (
        scenario_id,
        performance_run_id,
        emissions_run_id,
        operation_id,
        operation,
        operation_type
    ),
    CONSTRAINT fk_emissions_run_output FOREIGN KEY (
        scenario_id,
        performance_run_id,
        emissions_run_id
    )
    REFERENCES emissions_run_output (scenario_id,
    performance_run_id,
    emissions_run_id) ON DELETE CASCADE
                      ON UPDATE CASCADE,
    CONSTRAINT fk_performance_run_output FOREIGN KEY (
        scenario_id,
        performance_run_id,
        operation_id,
        operation,
        operation_type
    )
    REFERENCES performance_run_output (scenario_id,
    performance_run_id,
    operation_id,
    operation,
    operation_type) 
);

INSERT INTO emissions_run_output_operations (
    scenario_id,
    performance_run_id,
    emissions_run_id,
    operation_id,
    operation,
    operation_type,
    fuel,
    hc,
    co,
    nox,
    nvpm,
    nvpm_number
)
SELECT
    scenario_id,
    performance_run_id,
    emissions_run_id,
    operation_id,
    operation,
    operation_type,
    fuel,
    hc,
    co,
    nox,
    0.0,
    0.0
FROM temp.grape_table;

DROP TABLE temp.grape_table;
)";

    constexpr std::string_view g_emissions_run_output_segments = R"(
CREATE TEMP TABLE grape_table AS SELECT * FROM emissions_run_output_segments;

DROP TABLE emissions_run_output_segments;

CREATE TABLE emissions_run_output_segments (
    scenario_id        TEXT    NOT NULL,
    performance_run_id TEXT    NOT NULL,
    emissions_run_id   TEXT    NOT NULL,
    operation_id       TEXT    NOT NULL,
    operation          TEXT    NOT NULL,
    operation_type     TEXT    NOT NULL,
    segment_number     INTEGER NOT NULL,
    fuel               REAL    NOT NULL,
    hc                 REAL    NOT NULL,
    co                 REAL    NOT NULL,
    nox                REAL    NOT NULL,
    nvpm               REAL    NOT NULL,
    nvpm_number        REAL    NOT NULL,
    PRIMARY KEY (
        scenario_id,
        performance_run_id,
        emissions_run_id,
        operation_id,
        operation,
        operation_type,
        segment_number
    ),
    CONSTRAINT fk_emissions_run_output_operations FOREIGN KEY (
        scenario_id,
        performance_run_id,
        emissions_run_id,
        operation_id,
        operation,
        operation_type
    )
    REFERENCES emissions_run_output_operations (scenario_id,
    performance_run_id,
    emissions_run_id,
    operation_id,
    operation,
    operation_type) ON DELETE CASCADE
                    ON UPDATE CASCADE
);

INSERT INTO emissions_run_output_segments (
    scenario_id,
    performance_run_id,
    emissions_run_id,
    operation_id,
    operation,
    operation_type,
    segment_number,
    fuel,
    hc,
    co,
    nox,
    nvpm,
    nvpm_number
)
SELECT
    scenario_id,
    performance_run_id,
    emissions_run_id,
    operation_id,
    operation,
    operation_type,
    segment_number,
    fuel,
    hc,
    co,
    nox,
    0.0,
    0.0
FROM temp.grape_table;

DROP TABLE temp.grape_table;
)";
}
