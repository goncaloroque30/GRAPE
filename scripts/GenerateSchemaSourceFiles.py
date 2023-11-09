import argparse
import os
import sqlite3


def h_file_header(file_name: str, namespace: str) -> str:
    script_file = os.path.basename(__file__)
    return f"""\
/**
* @file {file_name}
* File automatically generated with python script "{script_file}".
*/

#pragma once

template<std::size_t>      
class GRAPE::Table;

namespace {namespace} {{
    """


def cpp_file_header(file_name: str, namespace: str) -> str:
    script_file = os.path.basename(__file__)
    return f"""\
/**
* @file {file_name}
* File automatically generated with python script "{script_file}".
*/

#include "GRAPE_pch.h"

#include "Database/Table.h"

namespace {namespace} {{
    """


def file_footer() -> str:
    return "}"


def table_declaration(table_name: str, table_size: int) -> str:
    return f"""\
extern const Table<{table_size}> {table_name};

    """


def table_header(table_name: str) -> str:
    return f"""\
extern const Table {table_name}("{table_name}",
        {{
    """


def table_footer() -> str:
    return f"""\
    }}
    );

    """


def var_string(var_name: str) -> str:
    var_name.strip("'()'")
    return f"""        "{var_name}",
    """


def main():
    # -- Parse command line arguments --
    parser = argparse.ArgumentParser(
        prog="GRAPE schema source files creator",
        description="Creates Table variables (see src/Database/Table.h) from a sqlite file. "
                    "Saves the declarations as extern in a .h file and the contents in a -cpp file.")

    parser.add_argument("grp", help="The sqlite file.", )
    parser.add_argument("path", help="The folder to which the source files will be written.")
    parser.add_argument("filename", help="The files <FileName>.h and <FileName>.cpp will be created.")
    parser.add_argument("namespace", help="The namespace under which the tables will be created.")

    args = parser.parse_args()
    grp_path = args.grp
    output_folder = args.path
    namespace = args.namespace
    file_name = args.filename

    h_file_name = f"{file_name}.h"
    cpp_file_name = f"{file_name}.cpp"
    h_file_path = f"{output_folder}/{file_name}.h"
    cpp_file_path = f"{output_folder}/{file_name}.cpp"

    h_file = open(h_file_path, "w")
    cpp_file = open(cpp_file_path, "w")

    # -- Read sqlite file --
    con = sqlite3.connect(grp_path)
    cur = con.cursor()

    cur.execute("SELECT * FROM sqlite_schema")
    schema_tables = [table[2] for table in cur.fetchall() if table[0] == "table"]

    # -- Write File Headers --
    h_file.write(h_file_header(h_file_name, namespace))
    cpp_file.write(cpp_file_header(cpp_file_name, namespace))

    # -- Write Tables --
    for schema_table in schema_tables:
        if schema_table.startswith("sqlite"):
            continue
        cur.execute(f"PRAGMA table_info({schema_table})")
        table_variables = [variableRow[1] for variableRow in cur.fetchall()]
        table_size = len(table_variables)

        h_file.write(table_declaration(schema_table, table_size))
        cpp_file.write(table_header(schema_table))
        for var in table_variables:
            cpp_file.write(var_string(var))
        cpp_file.write(table_footer())

    # -- Write File Footer --
    h_file.seek(h_file.tell() - 6)  # Go back clearing spaces and new line
    h_file.write(file_footer())
    cpp_file.seek(cpp_file.tell() - 6)  # Go back clearing spaces and new line
    cpp_file.write(file_footer())


if __name__ == "__main__":
    main()
