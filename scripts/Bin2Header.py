import argparse
import binascii
import os
from pathlib import Path


def file_header(file_name: str, var_name: str) -> str:
    script_file = os.path.basename(__file__)
    return f"""\
/**
* @file {file_name}
* File automatically generated with python script "{script_file}".
*/

const std::uint8_t g_{var_name}[] =
{{
"""


def file_footer() -> str:
    return f"""\
}};
"""


def main():
    # -- Parse command line arguments --
    parser = argparse.ArgumentParser(
        prog="Binary to C array converter",
        description="Creates a header file with one C style array from a binary file.")

    parser.add_argument("filename", help="The file to be converted.")
    parser.add_argument("output_file", help="The output filename. Will be used to name the variable.")

    args = parser.parse_args()
    input_filename = args.filename
    output_filename = args.output_file
    output_filepath = Path(output_filename)
    output_filepath.parent.mkdir(parents=True, exist_ok=True)

    # -- Read binary data --
    with open(input_filename, "rb") as f:
        input_content = f.read()

    # -- Write file header --
    output_file = open(output_filename, "w")
    output_file.write(file_header(output_filepath.name, output_filepath.stem))

    # -- Write hex string --
    hex_str = binascii.hexlify(input_content).decode("UTF-8")  # single string with no prefix, e.g. 00123f53...
    hex_array = ["0x" + hex_str[i:i+2] for i in range(0, len(hex_str), 2)]  # array with prefix, [0x00, 0x12, 0x3f, ...
    hex_array = [hex_array[i:i+16] for i in range(0, len(hex_array), 16)]  # array of arrays, each with 16 elements

    for array in hex_array:
        output_file.write("    ")  # indentation
        output_file.write(
            ", ".join(array) + ",\n"
        )

    # -- Write file footer --
    output_file.write(file_footer())

    # -- Close --
    output_file.close()


if __name__ == "__main__":
    main()
