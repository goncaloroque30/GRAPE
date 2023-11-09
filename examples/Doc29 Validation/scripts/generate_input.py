from __future__ import annotations

from argparse import ArgumentParser
from datetime import datetime
from math import sqrt
from pathlib import Path

import numpy as np
import pandas as pd
from pyproj import Proj

airport_id = "Reference Airport"
scenario_id = "Reference Scenario"
performance_id = "Performance"
noise_id_points = "Noise Points"
noise_id_grid = "Noise Grid"


def main() -> None:
    # Parse arguments from the command line
    parser = ArgumentParser(
        prog="GRAPE Doc29 Validation Generator",
        description="Generates GRAPE input files with data from the Doc29 validation excell"
    )

    parser.add_argument("doc29_validation_excell", type=Path)
    parser.add_argument("output_folder", type=Path)

    args = parser.parse_args()

    generate_input(args.doc29_validation_excell, args.output_folder)


def generate_input(doc29_path: Path, output_folder: Path) -> None:
    # -- Create GRAPE tables
    # airports
    airports = pd.DataFrame({
        "id": [airport_id],
        "longitude": [0.0],
        "latitude": [0.0],
        "elevation": [0.0],
        "reference_temperature": [288.15],
        "reference_sea_level_pressure": [101325.0]
    })

    # runways
    doc29_runways = pd.read_excel(
        doc29_path,
        sheet_name="A-10_Runway",
        usecols=[
            "Runway Identifier",
            "SOR X coordinate (m)",
            "SOR Y coordinate (m)",
            "End X coordinate (m)",
            "End Y coordinate (m)",
        ])
    runways = doc29_runways

    runways = runways.rename(columns={
        "Runway Identifier": "id",
        "SOR X coordinate (m)": "start_x",
        "SOR Y coordinate (m)": "start_y",
        "End X coordinate (m)": "end_x",
        "End Y coordinate (m)": "end_y",
    })
    runways["id"] = "RWY " + runways["id"].astype(str)
    runway = runways.iloc[0, :].copy()  # Consider only first runway
    ortho_proj = Proj(proj="ortho",
                      lon_0=runway["start_x"],
                      lat_0=runway["start_y"],
                      ellps="WGS84",
                      always_xy=True)
    runway["longitude"], runway["latitude"] = ortho_proj(
        runway["start_x"], runway["start_y"], inverse=True
    )
    runway["length"] = sqrt(
        (runway["end_x"] - runway["start_x"]) ** 2 +
        (runway["end_y"] - runway["start_y"]) ** 2
    )
    rwy_vector = (runway["end_x"] - runway["start_x"],
                  runway["end_y"] - runway["start_y"])
    north_vector = (0, 1)

    def angle(x, y):
        res = np.degrees(np.arctan2(np.cross(x, y), np.dot(x, y)))
        return res if res > 0 else res + 360.0

    runway["heading"] = round(angle(rwy_vector, north_vector), 2)
    runway["airport_id"] = airport_id
    runway["elevation"] = 1.0
    runway["gradient"] = 0.0

    runway = runway[[
        "airport_id",
        "id",
        "longitude",
        "latitude",
        "elevation",
        "length",
        "heading",
        "gradient",
    ]]
    runway = pd.DataFrame(runway).T

    # routes
    runway_id = runway["id"].iloc[0]
    doc29_routes = pd.read_excel(
        doc29_path,
        sheet_name="A-11_Routes",
        usecols=[
            "route_ID",
            "Track Points",
            "X coordinate (m)",
            "Y coordinate (m)",
        ])
    routes_simple = doc29_routes
    routes_simple = routes_simple.rename(columns={
        "route_ID": "route_id",
        "Track Points": "point_number",
        "X coordinate (m)": "x",
        "Y coordinate (m)": "y",
    })
    routes_simple["longitude"], routes_simple["latitude"] = ortho_proj(
        routes_simple["x"], routes_simple["y"], inverse=True
    )
    routes_simple["operation"] = routes_simple.apply(
        lambda r: "Arrival" if r["route_id"].startswith("A") else "Departure",
        axis="columns"
    )
    routes_simple = routes_simple[~(
            (routes_simple["operation"] == "Departure") &
            (routes_simple["x"] == 0.0) &
            (routes_simple["y"] == 0.0)
    )]  # Deletes departure route points at the runway threshold
    routes_simple = routes_simple[~(
            (routes_simple["operation"] == "Arrival") &
            (routes_simple["x"] >= 0.0)
    )]  # Deletes arrival route points after the runway threshold
    routes_simple = routes_simple.assign(
        airport_id=airport_id,
        runway_id=runway_id,
    )
    routes_simple = routes_simple[[
        "airport_id",
        "runway_id",
        "operation",
        "route_id",
        "longitude",
        "latitude",
    ]]

    routes = routes_simple[["operation", "route_id"]].drop_duplicates()
    routes = routes.rename(columns={"route_id": "id"})
    routes = routes.assign(
        airport_id=airport_id,
        runway_id=runway_id,
        type="Simple",
    )
    routes = routes[[
        "airport_id",
        "runway_id",
        "operation",
        "id",
        "type",
    ]]

    # fleet
    doc29_aircraft = pd.read_excel(
        doc29_path,
        sheet_name="A-1_Aircraft",
    )
    doc29_aircraft.insert(5, "Owner Category", "NA")
    doc29_aircraft.insert(10, "Noise Chapter", "NA")
    doc29_aircraft.loc[
        doc29_aircraft["Power Parameter"] == "Corrected_Net_Thrust_(lb)",
        "Power Parameter"] = "CNT (lb)"
    fleet = doc29_aircraft[[
        "Aircraft Identifier",
        "Number of Engines",
        "Maximum Sea Level Static Thrust (lb)",
        "NPD Identifier",
        "Power Parameter",
    ]]
    fleet = fleet[fleet["Power Parameter"] != "Shaft_Horse_Power_(%)"]
    fleet = fleet.drop(columns=["Power Parameter"])
    fleet = fleet.rename(columns={
        "Aircraft Identifier": "id",
        "Number of Engines": "engine_count",
        "Maximum Sea Level Static Thrust (lb)":
            "maximum_sea_level_static_thrust",
        "NPD Identifier": "doc29_noise_id"
    })
    fleet["maximum_sea_level_static_thrust"] *= 9.80665 / 2.204623  # lbf to N
    fleet = fleet.assign(
        engine_breakpoint_temperature=303.15,  # 30°C
        doc29_performance_id=fleet["id"],
        sfi_id=None,
        lto_engine_id=None,
        doc29_noise_arrival_delta_db=0.0,
        doc29_noise_departure_delta_db=0.0,
    )
    fleet = fleet[[
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
    ]]

    # Profiles
    doc29_profiles = pd.read_excel(
        doc29_path,
        sheet_name="A-6_Fixed_Point_Profiles",
    )
    doc29_profiles["Aircraft Identifier"] = (
        doc29_profiles["Aircraft Identifier"].str.strip())
    doc29_profiles["Distance (m)"] /= 0.3048  # m 2 ft
    doc29_profiles["Altitude (m)"] /= 0.3048  # m 2 ft
    doc29_profiles["Altitude (m)"] = round(doc29_profiles["Altitude (m)"])
    doc29_profiles["True Airspeed (m/s)"] /= 1852.0 / 3600.0  # m/s 2 kts
    doc29_profiles = doc29_profiles.rename(columns={
        "Distance (m)": "Distance (ft)",
        "Altitude (m)": "Altitude (ft)",
        "True Airspeed (m/s)": "True Airspeed (kts)",
    })
    profiles = doc29_profiles[[
        "Aircraft Identifier",
        "Operation mode",
        "Profile identifier",
        "Stage Length",
    ]].drop_duplicates()
    profiles = profiles.rename(columns={
        "Aircraft Identifier": "fleet_id",
        "Operation mode": "operation",
        "Profile identifier": "doc29_profile_id",
    })
    profiles["doc29_profile_id"] += " " + profiles["Stage Length"].astype(str)
    profiles = profiles.drop(columns="Stage Length")
    profiles.loc[profiles["operation"] == "A", "operation"] = "Arrival"
    profiles.loc[profiles["operation"] == "D", "operation"] = "Departure"

    # Scenarios
    scenarios = pd.DataFrame({"id": [scenario_id]})

    # Performance Run
    performance_run = pd.DataFrame({
        "scenario_id": [scenario_id],
        "id": [performance_id],
        "coordinate_system_type": ["Local Cartesian"],
        "coordinate_system_longitude_0": runway["longitude"],
        "coordinate_system_latitude_0": runway["latitude"],
        "filter_minimum_altitude": [None],
        "filter_maximum_altitude": [None],
        "filter_minimum_cumulative_ground_distance": [None],
        "filter_maximum_cumulative_ground_distance": [None],
        "filter_ground_distance_threshold": [None],
        "segmentation_speed_delta_threshold": [20.0 * 1852.0 / 3600.0],  # 20 kts 2 m/s
        "flights_performance_model": ["Doc29"],
        "flights_doc29_low_altitude_segmentation": [1],
        "tracks_4d_calculate_performance": [True],
        "tracks_4d_minimum_points": [None],
        "tracks_4d_recalculate_cumulative_ground_distance": [0],
        "tracks_4d_recalculate_groundspeed": [0],
        "tracks_4d_recalculate_fuel_flow": [0],
        "fuel_flow_model": ["None"],
        "fuel_flow_lto_altitude_correction": [1],
    })

    # atmospheric conditions relative humidity flag is ignored
    doc29_atmosphere = pd.read_excel(
        doc29_path,
        sheet_name="A-9_Meteorological",
        usecols=[
            "Temperature (degC)",
            "Pressure (mmHg)",
            "Headwind (m/s)",
            "Elevation (m)",
            "Humidity (%)",
        ]).iloc[0]
    temp_delta = doc29_atmosphere["Temperature (degC)"] - 15.0
    press_delta = round(
        doc29_atmosphere["Pressure (mmHg)"] * 133.322387415 - 101325,
        -2
    )

    performance_run_atmosphere = pd.DataFrame({
        "scenario_id": [scenario_id],
        "performance_run_id": [performance_id],
        "time": datetime.now(tz=None).strftime("%Y-%m-%d %H:%M:%S"),
        "temperature_delta": [temp_delta],
        "pressure_delta": [press_delta],
        "wind_speed": [doc29_atmosphere["Headwind (m/s)"]],
        "wind_direction": [None],
        "relative_humidity": [doc29_atmosphere["Humidity (%)"] / 100.0],
    })

    # Noise Run
    noise_run = pd.DataFrame({
        "scenario_id": [scenario_id, scenario_id],
        "performance_run_id": [performance_id, performance_id],
        "id": [noise_id_points, noise_id_grid],
        "noise_model": ["Doc29", "Doc29"],
        "atmospheric_absorption": ["None", "None"],
        "receptor_set_type": ["Points", "Grid"],
        "save_single_event_metrics": [1, 1],
    })

    # Noise Run Points
    doc29_receptors = pd.read_excel(
        doc29_path,
        sheet_name="A-12_Receptors",
        usecols=[
            "receptor_ID",
            "X coordinate (m)",
            "Y coordinate (m)",
            "Height (m)",
        ])
    receptors = doc29_receptors
    receptors = receptors.rename(columns={
        "receptor_ID": "id",
        "X coordinate (m)": "x",
        "Y coordinate (m)": "y",
        "Height (m)": "altitude_msl",
    })
    receptors["longitude"], receptors["latitude"] = ortho_proj(
        receptors["x"], receptors["y"], inverse=True
    )
    receptors = receptors.assign(
        scenario_id=scenario_id,
        performance_run_id=performance_id,
        noise_run_id=noise_id_points
    )
    receptors = receptors[[
        "scenario_id",
        "performance_run_id",
        "noise_run_id",
        "id",
        "longitude",
        "latitude",
        "altitude_msl"
    ]]

    # Noise Run Grid
    g_lon, g_lat = ortho_proj(-27000, -12000, inverse=True)
    grid = pd.DataFrame({
        "scenario_id": [scenario_id],
        "performance_run_id": [performance_id],
        "noise_run_id": [noise_id_grid],
        "reference_location": ["Bottom Left"],
        "reference_longitude": [g_lon],
        "reference_latitude": [g_lat],
        "reference_altitude_msl": [0.0],
        "horizontal_spacing": [100.0],
        "vertical_spacing": [100.0],
        "horizontal_count": [471],
        "vertical_count": [141],
        "grid_rotation": [0.0],
    })

    # Operations
    flights = pd.merge(routes[["operation", "id"]],
                       fleet["id"],
                       how="cross",
                       on=None,
                       suffixes=("_route", "_fleet")
                       )
    flights = flights.rename(columns={
        "id_route": "route_id",
        "id_fleet": "fleet_id",
    })
    flights = flights.assign(
        id=flights["fleet_id"] + flights["route_id"],
        airport_id=airport_id,
        runway_id=runway_id,
        time=datetime.now(tz=None).strftime("%Y-%m-%d %H:%M:%S"),
        count=1,
        takeoff_thrust=1.0,
        climb_thrust=1.0,
    )
    doc29_weights = pd.read_excel(
        doc29_path,
        sheet_name="A-5_Default_Weights",
    )
    weights = doc29_weights[[
        "Aircraft Identifier",
        "Operation",
        "Weight (lb)",
    ]]
    weights.loc[weights["Operation"] == "A", "Operation"] = "Arrival"
    weights.loc[weights["Operation"] == "D", "Operation"] = "Departure"
    flights = flights.merge(
        weights,
        how="left",
        left_on=["fleet_id", "operation"],
        right_on=["Aircraft Identifier", "Operation"],
    )
    flights = flights.rename(columns={"Weight (lb)": "weight"})
    flights["weight"] /= 2.204623  # lb to kg
    flights = flights.merge(profiles, how="left", on=["fleet_id", "operation"])
    flights = flights[[
        "id",
        "airport_id",
        "runway_id",
        "operation",
        "route_id",
        "time",
        "count",
        "fleet_id",
        "weight",
        "doc29_profile_id",
        "takeoff_thrust",
        "climb_thrust",
    ]]

    # Scenarios Operations
    scenarios_operations = flights[["id", "operation"]]
    scenarios_operations = scenarios_operations.assign(
        scenario_id=scenario_id,
        type="Flight",
    )
    scenarios_operations = scenarios_operations.rename(columns={"id": "operation_id"})
    scenarios_operations = scenarios_operations[[
        "scenario_id",
        "operation_id",
        "operation",
        "type",
    ]]

    # -- Create ANP tables
    anp_path = Path(output_folder, "ANP")
    anp_path.mkdir(parents=True, exist_ok=True)
    doc29_aircraft.to_csv(f"{anp_path}/Aircraft.csv", index=False)
    pd.read_excel(
        doc29_path,
        sheet_name="A-2_Jet_Coefficients",
    ).to_csv(f"{anp_path}/Jet_engine_coefficients.csv", index=False)
    pd.read_excel(
        doc29_path,
        sheet_name="A-3_Propeller_Coefficients",
    ).to_csv(f"{anp_path}/Propeller_engine_coefficients.csv", index=False)
    pd.read_excel(
        doc29_path,
        sheet_name="A-4_Aerodynamic_Coefficients",
    ).to_csv(f"{anp_path}/Aerodynamic_coefficients.csv")
    doc29_weights.to_csv(f"{anp_path}/Default_weights.csv", index=False)
    doc29_profiles.to_csv(
        f"{anp_path}/Default_fixed_point_profiles.csv", index=False)
    pd.DataFrame(columns=[
        "ACFT_ID",
        "Profile_ID",
        "Step Number",
        "Step Type",
        "Flap_ID",
        "Start Altitude(ft)",
        "Start CAS (kt)",
        "Descent Angle (deg)",
        "Touchdown Roll (ft)",
        "Distance (ft)",
        "Start Thrust",
    ]).to_csv(f"{anp_path}/Default_approach_procedural_steps.csv", index=False)
    pd.DataFrame(columns=[
        "ACFT_ID",
        "Profile_ID",
        "Stage Length",
        "Step Number",
        "Step Type",
        "Thrust Rating",
        "Flap_ID",
        "End Point Altitude (ft)",
        "Rate of Climb (ft/min)",
        "End Point CAS (kt)",
        "Accel Percentage (%)",
    ]).to_csv(f"{anp_path}/Default_departure_procedural_steps.csv", index=False)
    pd.read_excel(
        doc29_path,
        sheet_name="A-7_NPD_Curves",
    ).to_csv(f"{anp_path}/NPD_data.csv", index=False)
    doc29_spectral_classes = pd.read_excel(
        doc29_path,
        sheet_name="A-8_Spectral_Class",
    )
    doc29_spectral_classes["Operation Mode"] = (
        doc29_spectral_classes["Operation Mode"].str.strip())
    doc29_spectral_classes.to_csv(
        f"{anp_path}/Spectral_classes.csv", index=False)

    # -- Create Input Tables --
    # Tables
    grape_tables = {
        "Airports": airports,
        "Runways": runway,
        "Routes": routes,
        "Routes Simple": routes_simple,
        "Flights": flights,
        "Scenarios": scenarios,
        "Scenarios Operations": scenarios_operations,
        "Performance Runs": performance_run,
        "Performance Runs Atmospheres": performance_run_atmosphere,
        "Noise Runs": noise_run,
        "Noise Runs Point Receptors": receptors,
        "Noise Runs Grid Receptors": grid,
    }

    input_path = Path(output_folder, "Input Tables")
    input_path.mkdir(parents=True, exist_ok=True)
    [tbl.to_csv(input_path / f"{tbl_name}.csv", index=False)
     for tbl_name, tbl in grape_tables.items()]


if __name__ == "__main__":
    main()
