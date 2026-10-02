#!/usr/bin/env python3

import argparse
import re
import sys
from pathlib import Path


CONFIG_SUFFIX = ".workshopc.yaml"


def cpp_identifier(config_path):
    name = config_path.name[:-len(CONFIG_SUFFIX)]
    return "config_" + re.sub(r"[^A-Za-z0-9_]", "_", name)


def escape_cpp_bytes(value):
    escaped = []

    for byte in value:
        if byte == 0x22:
            escaped.append(r"\"")
        elif byte == 0x5C:
            escaped.append(r"\\")
        elif byte == 0x0A:
            escaped.append(r"\n")
        elif byte == 0x0D:
            escaped.append(r"\r")
        elif byte == 0x09:
            escaped.append(r"\t")
        elif 0x20 <= byte <= 0x7E:
            escaped.append(chr(byte))
        else:
            escaped.append(f"\\{byte:03o}")

    return "".join(escaped)


def render_header(config_path, contents):
    chunks = [contents[index:index + 32] for index in range(0, len(contents), 32)]
    if not chunks:
        chunks = [b""]

    literals = "\n".join(
        f'    "{escape_cpp_bytes(chunk)}"'
        for chunk in chunks
    )

    return (
        "#pragma once\n\n"
        "#include <string_view>\n\n"
        "namespace workshopc::configs {\n"
        f"inline constexpr std::string_view {cpp_identifier(config_path)} =\n"
        f"{literals};\n"
        "}\n"
    )


def read_config(path):
    with path.open("r", encoding="utf-8", newline="") as config_file:
        return config_file.read().encode("utf-8")


def write_config(config_path, output_path):
    contents = read_config(config_path)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(render_header(config_path, contents), encoding="utf-8", newline="")
    print(f"{config_path} -> {output_path}")


def convert_file(parser, source, destination):
    if not source.name.endswith(CONFIG_SUFFIX):
        parser.error(f"source file must end with {CONFIG_SUFFIX}: {source}")

    if destination is not None and destination.exists() and destination.is_dir():
        parser.error("source file and destination folder cannot be used together")

    if destination is None:
        output_name = source.name[:-len(CONFIG_SUFFIX)] + ".config.hpp"
        destination = source.with_name(output_name)

    if source.resolve() == destination.resolve():
        parser.error("destination must not overwrite the source config")

    try:
        write_config(source, destination)
    except (OSError, UnicodeError) as error:
        parser.error(f"could not convert {source}: {error}")


def convert_folder(parser, source, destination):
    if destination is not None and destination.exists() and not destination.is_dir():
        parser.error("source folder and destination file cannot be used together")

    destination = destination or source
    config_files = sorted(
        path for path in source.rglob(f"*{CONFIG_SUFFIX}")
        if path.is_file()
    )

    if not config_files:
        parser.error(f"no files ending with {CONFIG_SUFFIX} found in {source}")

    try:
        for config_path in config_files:
            relative_path = config_path.relative_to(source)
            output_name = config_path.name[:-len(CONFIG_SUFFIX)] + ".config.hpp"
            output_path = destination / relative_path.parent / output_name
            write_config(config_path, output_path)
    except (OSError, UnicodeError) as error:
        parser.error(f"could not convert config files: {error}")


def main():
    parser = argparse.ArgumentParser(
        description="Convert WorkshopC YAML configs to constexpr C++ string_view headers."
    )
    parser.add_argument("source", type=Path, help="a .workshopc.yaml file or a folder")
    parser.add_argument(
        "destination",
        type=Path,
        nargs="?",
        help="an output .hpp file or destination folder",
    )
    args = parser.parse_args()

    if not args.source.exists():
        parser.error(f"source does not exist: {args.source}")

    if args.source.is_file():
        convert_file(parser, args.source, args.destination)
    elif args.source.is_dir():
        convert_folder(parser, args.source, args.destination)
    else:
        parser.error(f"source must be a file or folder: {args.source}")


if __name__ == "__main__":
    main()