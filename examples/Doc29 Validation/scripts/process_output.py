from __future__ import annotations

from argparse import ArgumentParser
from pathlib import Path
import sqlite3

from pyproj import Proj
import pandas as pd

airport_id = "Reference Airport"
scenario_id = "Reference Scenario"
performance_id = "Performance"
noise_id_points = "Noise Points"
noise_id_grid = "Noise Grid"


def main() -> None:
    # Parse arguments from the command line
    parser = ArgumentParser(
        prog="GRAPE Doc29 Validation Processor",
        description="Generates validation tables from the output of GRAPE Doc29 validation",
    )

    parser.add_argument("doc29_validation_excell", type=Path)
    parser.add_argument("grape_study_path", type=Path)
    parser.add_argument("output_excell_path", type=Path)

    args = parser.parse_args()

    process_output(args.doc29_validation_excell, args.grape_study_path, args.output_excell_path)


def process_output(doc29_path: Path, grape_study_path: Path, output_excell_path: Path) -> None:
    ortho_00 = Proj(proj="ortho",
                    lat_0=0,
                    lon_0=0,
                    ellps="WGS84",
                    always_xy=True)

    ex_writer = pd.ExcelWriter(output_excell_path, engine="xlsxwriter")
    ex_workbook = ex_writer.book

    # Read data from Doc29 validation excell
    doc29_points = pd.read_excel(
        doc29_path,
        sheet_name="A-12_Receptors",
        usecols=[
            "receptor_ID",
            "X coordinate (m)",
            "Y coordinate (m)",
            "Height (m)",
        ]
    )
    doc29_points = doc29_points.rename(columns={
        "receptor_ID": "receptor",
        "X coordinate (m)": "x",
        "Y coordinate (m)": "y",
        "Height (m)": "altitude_msl",
    })
    doc29_segments = pd.read_excel(
        doc29_path,
        sheet_name="B-2_Segment_Results",
        usecols=["case_ID",
                 "receptor_ID",
                 "segment_ID",
                 "segment_start_x(ft)",
                 "segment_start_y(ft)",
                 "segment_start_z(ft)",
                 "segment_end_x(ft)",
                 "segment_end_y(ft)",
                 "segment_end_z(ft)",
                 "segment_length(ft)"])
    doc29_segments = doc29_segments.rename(columns={
        "case_ID": "operation",
        "receptor_ID": "receptor",
        "segment_ID": "segment_index",
        "segment_start_x(ft)": "x",
        "segment_start_y(ft)": "y",
        "segment_start_z(ft)": "altitude",
        "segment_length(ft)": "distance",
        "segment_end_x(ft)": "x_end",
        "segment_end_y(ft)": "y_end",
        "segment_end_z(ft)": "altitude_end",
    })

    doc29_sel_points = pd.read_excel(
        doc29_path,
        sheet_name="B-1_SEL_Results",
    )
    doc29_sel_points = doc29_sel_points.rename(columns={
        "case_ID": "operation",
        "receptor_ID": "receptor",
        "SEL(dB)": "sel",
    })
    doc29_sel_grid = pd.read_excel(
        doc29_path,
        sheet_name="B-3_Grid_Results",
    )
    doc29_sel_grid = doc29_sel_grid.rename(columns={
        "case_ID": "operation",
        "grid_x(m)": "x",
        "grid_y(m)": "y",
        "SEL(dB)": "sel",
    })

    # Read data from .grp file
    con = sqlite3.connect(grape_study_path)
    grape_perf = pd.read_sql(
        f"SELECT * FROM performance_run_output_points WHERE "
        f"scenario_id = '{scenario_id}' AND "
        f"performance_run_id = '{performance_id}'",
        con=con)

    grape_receptors = pd.read_sql(
        f"SELECT * FROM noise_run_output_receptors WHERE "
        f"scenario_id = '{scenario_id}' AND "
        f"performance_run_id = '{performance_id}'",
        con=con)
    grape_receptors = grape_receptors[["noise_run_id",
                                       "id",
                                       "longitude",
                                       "latitude",
                                       "altitude_msl"]]
    grape_receptors["x"], grape_receptors["y"] = ortho_00(
        grape_receptors["longitude"],
        grape_receptors["latitude"],
    )
    grape_receptors["x"] = grape_receptors["x"].round(-2)
    grape_receptors["y"] = grape_receptors["y"].round(-2)
    grape_points = grape_receptors.loc[
        grape_receptors["noise_run_id"] == noise_id_points
        ]
    grape_grid = grape_receptors.loc[
        grape_receptors["noise_run_id"] == noise_id_grid
        ]

    grape_noise = pd.read_sql(
        f"SELECT * FROM noise_run_output_single_event WHERE "
        f"scenario_id = '{scenario_id}' AND "
        f"performance_run_id = '{performance_id}'",
        con=con)

    # Point locations validation
    points = pd.merge(doc29_points, grape_points,
                      how="left",
                      left_on="receptor",
                      right_on="id",
                      suffixes=("", " grape"))
    points = points[[
        "receptor",
        "x",
        "y",
        "altitude_msl",
        "longitude",
        "latitude",
        "x grape",
        "y grape",
        "altitude_msl grape"
    ]]
    points.to_excel(ex_writer, sheet_name="Points", index=False)

    # Validation of each operation
    cases = {
        "JETFAC": "R02",
        "JETFAS": "R02",
        "JETFDC": "R01",
        "JETFDS": "R01"
    }

    for operation, receptor in cases.items():
        #  Performance Data
        d_perf = doc29_segments.query(
            "operation == @operation & receptor == @receptor"
        )
        d_last = d_perf.tail(1)
        d_perf = d_perf.drop(columns=["receptor",
                                      "segment_index",
                                      "x_end",
                                      "y_end",
                                      "altitude_end"])
        d_perf = pd.concat([d_perf, pd.DataFrame({
            "operation": [operation],
            "x": [d_last["x_end"].iat[0]],
            "y": [d_last["y_end"].iat[0]],
            "altitude": [d_last["altitude_end"].iat[0]],
            "distance": ["0.0"]
        })], ignore_index=True)

        num_cols = ["x",
                    "y",
                    "altitude",
                    "distance"]
        for col in num_cols:
            d_perf[col] = pd.to_numeric(d_perf[col])

        d_perf["x"] *= 0.3048  # ft 2 m
        d_perf["y"] *= 0.3048  # ft 2 m
        d_perf["altitude"] *= 0.3048  # ft 2 m
        d_perf["distance"] *= 0.3048  # ft 2 m

        d_perf["cumulative_ground_distance"] = d_perf["distance"].cumsum()
        d_perf["cumulative_ground_distance"] = pd.concat(
            [pd.Series([0]), d_perf["cumulative_ground_distance"].iloc[:-1]],
            ignore_index=True
        )
        d_perf["cumulative_ground_distance"] -= (
            d_perf["cumulative_ground_distance"].loc[
                (d_perf["x"] == 0) & (d_perf["y"] == 0)].iat[0]
        )
        d_perf = d_perf.drop(columns="distance")

        d_perf["longitude"], d_perf["latitude"] = ortho_00(d_perf["x"],
                                                           d_perf["y"],
                                                           inverse=True)
        d_perf = d_perf[["cumulative_ground_distance",
                         "x",
                         "y",
                         "longitude",
                         "latitude",
                         "altitude",
                         ]]
        col_d_perf = 0
        d_perf.to_excel(ex_writer, sheet_name=f"{operation}", index=False)

        g_perf = grape_perf.loc[
            grape_perf["operation_id"] == f"{operation}",
            ["point_origin",
             "flight_phase",
             "cumulative_ground_distance",
             "longitude",
             "latitude",
             "altitude_msl",
             ]
        ].copy()
        g_x, g_y = ortho_00(g_perf["longitude"], g_perf["latitude"])
        g_perf["x grape"] = g_x.round(2)
        g_perf["y grape"] = g_y.round(2)
        g_perf = g_perf.rename(columns={
            "cumulative_ground_distance": "cumulative_ground_distance grape",
            "longitude": "longitude grape",
            "latitude": "latitude grape",
            "altitude_msl": "altitude grape",
        })
        g_perf = g_perf[["cumulative_ground_distance grape",
                         "x grape",
                         "y grape",
                         "longitude grape",
                         "latitude grape",
                         "altitude grape",
                         "point_origin",
                         "flight_phase",
                         ]]
        col_g_perf = len(d_perf.columns) + 1
        g_perf.to_excel(ex_writer,
                        sheet_name=f"{operation}",
                        index=False,
                        startcol=col_g_perf)

        # Noise Points Data
        d_noise_pts = doc29_sel_points.query("operation == @operation")
        g_noise_pts = grape_noise.loc[
            (grape_noise["noise_run_id"] == f"{noise_id_points}") &
            (grape_noise["operation_id"] == f"{operation}")
            ].copy()
        n_pts = pd.merge(d_noise_pts, g_noise_pts,
                         how="left",
                         left_on=["operation", "receptor"],
                         right_on=["operation_id", "receptor_id"],
                         suffixes=["_d", "_g"])
        n_pts = n_pts[[
            "receptor",
            "sel",
            "exposure_db",
        ]]
        n_pts = n_pts.rename(columns={
            "sel": "sel doc29",
            "exposure_db": "sel grape",
        })
        n_pts["diff"] = n_pts["sel grape"] - n_pts["sel doc29"]
        col_n_pts = col_g_perf + len(g_perf.columns) + 1
        n_pts.to_excel(ex_writer,
                       sheet_name=f"{operation}",
                       index=False,
                       startcol=col_n_pts)

        # Noise grid data
        d_noise_grd = doc29_sel_grid.query("operation == @operation")
        g_noise_grd = grape_noise.loc[
            (grape_noise["noise_run_id"] == f"{noise_id_grid}") &
            (grape_noise["operation_id"] == f"{operation}"),
            ["receptor_id", "exposure_db"]
        ].copy()
        g_noise_grd = g_noise_grd.merge(
            grape_grid[["id", "longitude", "latitude", "x", "y"]],
            how="inner",
            left_on="receptor_id",
            right_on="id",
        )
        n_grd = pd.merge(d_noise_grd, g_noise_grd,
                         how="left",
                         on=["x", "y"],
                         suffixes=("", " GRAPE"))
        n_grd = n_grd[[
            "id",
            "x",
            "y",
            "longitude",
            "latitude",
            "sel",
            "exposure_db"
        ]]
        n_grd = n_grd.rename(columns={
            "sel": "sel doc29",
            "exposure_db": "sel grape",
        })
        n_grd["diff"] = n_grd["sel grape"] - n_grd["sel doc29"]
        col_n_grd = col_n_pts + len(n_pts.columns) + 1
        n_grd.to_excel(ex_writer,
                       sheet_name=f"{operation}",
                       index=False,
                       startcol=col_n_grd)
        n_grd_rmse = pd.DataFrame({
            "type": ["points", "grid"],
            "root mean squared error": [
                (n_pts["diff"] ** 2).mean() ** 0.5,
                (n_grd["diff"] ** 2).mean() ** 0.5
            ]
        }).set_index("type")
        col_n_grd_rmse = col_n_grd + len(n_grd.columns) + 1
        n_grd_rmse.to_excel(ex_writer,
                            sheet_name=f"{operation}",
                            startcol=col_n_grd_rmse)

        #  Charts
        ex_worksheet = ex_writer.sheets[f"{operation}"]

        # Track Chart
        ex_chart_t = ex_workbook.add_chart({
            "type": "scatter",
            "name": "Track",
        })
        ex_chart_t.set_size({
            "x_scale": 2.0,
            "y_scale": 1.5,
        })
        ex_chart_t.add_series({
            "name": "Doc29",
            "categories": [f"{operation}", 1, col_d_perf + 1, 100, col_d_perf + 1],
            "values": [f"{operation}", 1, col_d_perf + 2, 100, col_d_perf + 2],
            "marker": {
                "type": "circle",
                "size": 7,
            }
        })
        ex_chart_t.add_series({
            "name": "GRAPE",
            "categories": [f"{operation}", 1, col_g_perf + 1, 100, col_g_perf + 1],
            "values": [f"{operation}", 1, col_g_perf + 2, 100, col_g_perf + 2],
            "marker": {
                "type": "x",
                "size": 5,
            }
        })
        ex_chart_t.set_title({"name": "Track"})
        ex_chart_t.set_x_axis({
            "name": "X",
        })
        ex_chart_t.set_y_axis({
            "name": "Y",
        })
        ex_chart_t.set_legend({"position": "bottom"})
        ex_worksheet.insert_chart(50, 0, ex_chart_t)

        # Profile Chart
        ex_chart_p = ex_workbook.add_chart({
            "type": "scatter",
            "name": "Profile",
        })
        ex_chart_p.set_size({
            "x_scale": 2.0,
            "y_scale": 1.5,
        })
        ex_chart_p.add_series({
            "name": "Doc29",
            "categories": [f"{operation}", 1, col_d_perf, 100, col_d_perf],
            "values": [f"{operation}", 1, col_d_perf + 5, 100, col_d_perf + 5],
            "marker": {
                "type": "circle",
                "size": 7,
            }
        })
        ex_chart_p.add_series({
            "name": "GRAPE",
            "categories": [f"{operation}", 1, col_g_perf, 100, col_g_perf],
            "values": [f"{operation}", 1, col_g_perf + 5, 100, col_g_perf + 5],
            "marker": {
                "type": "x",
                "size": 5,
            }
        })
        ex_chart_p.set_title({"name": "Profile"})
        ex_chart_p.set_x_axis({
            "name": "Cumulative Ground Distance",
        })
        ex_chart_p.set_y_axis({
            "name": "Altitude",
        })
        ex_chart_p.set_legend({"position": "bottom"})
        ex_worksheet.insert_chart(50, 16, ex_chart_p)

        # Noise Chart
        ex_chart_n = ex_workbook.add_chart({
            "type": "line",
            "name": "SEL",
        })
        ex_chart_n.set_size({
            "x_scale": 2.0,
            "y_scale": 1.5,
        })
        ex_chart_n.add_series({
            "name": "Doc29",
            "categories": [f"{operation}", 1, col_n_pts, len(n_pts), col_n_pts],
            "values": [f"{operation}", 1, col_n_pts + 1, len(n_pts), col_n_pts + 1],
            'line': {'none': True},
            "marker": {
                "type": "circle",
                "size": 7,
            }
        })
        ex_chart_n.add_series({
            "name": "GRAPE",
            "categories": [f"{operation}", 1, col_n_pts, len(n_pts), col_n_pts],
            "values": [f"{operation}", 1, col_n_pts + 2, len(n_pts), col_n_pts + 2],
            'line': {'none': True},
            "marker": {
                "type": "x",
                "size": 5,
            }
        })
        ex_chart_n.set_title({"name": "SEL Points"})
        ex_chart_n.set_y_axis({
            "name": "Noise (dB)",
        })
        ex_chart_n.set_legend({"position": "bottom"})
        ex_worksheet.insert_chart(50, 32, ex_chart_n)
    ex_writer.close()


if __name__ == "__main__":
    main()
