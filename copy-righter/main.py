from argparse import ArgumentParser, Namespace
from collections import defaultdict
from pathlib import Path
from subprocess import CalledProcessError, CompletedProcess, run
from cmake import CmakeProcessor
from c import CProcessor
from python import PythonProcessor
from devicetree import DevicetreeProcessor


from config import Config
from paths_to_process import PathsToProcess


def main():
    parser: ArgumentParser = ArgumentParser(description="Copy-righter tool")
    parser.add_argument("path", help="Path to process")
    parser.add_argument(
        "--dry-run", action="store_true", help="Run without making actual changes"
    )
    parser.add_argument("--verbose", action="store_true", help="Enable verbose output")
    parser.add_argument("--config", choices=["zephyr", "python"], help="configuration")
    parser.add_argument(
        "-c",
        "--copyright",
        action="append",
        dest="copyrights",
        help="Add a copyright holder (repeatable)",
    )
    parser.add_argument(
        "-l",
        "--license",
        dest="license_identifier",
        help="Override the license identifier",
    )

    args: Namespace = parser.parse_args()

    config: Config = Config.from_args(args)

    base_path = Path(args.path).resolve()

    paths: PathsToProcess = PathsToProcess(str(base_path))

    companies = args.copyrights or ["Frickly Systems GmbH"]
    license_identifier = (
        args.license_identifier or "SPDX-License-Identifier: Apache-2.0"
    )

    if config.verbose:
        print("Files to process:")
        for filter_type, file_list in paths.paths.items():
            print(f"\n{filter_type}:")
            for file_path in file_list:
                print(f"  {file_path}")

    for cmake_file in paths.paths.get("CMakeLists.txt", []):
        CmakeProcessor(
            cmake_file,
            companies=companies,
            license_identifier=license_identifier,
        ).run(config)

    for c_suffix in (".h", ".hpp", ".c", ".cpp"):
        for c_file in paths.paths.get(c_suffix, []):
            CProcessor(
                c_file,
                companies=companies,
                license_identifier=license_identifier,
            ).run(config)

    for python_file in paths.paths.get(".py", []):
        PythonProcessor(
            python_file,
            companies=companies,
            license_identifier=license_identifier,
        ).run(config)

    for dts_suffix in (".dts", ".dtsi", ".overlay"):
        for dt_file in paths.paths.get(dts_suffix, []):
            DevicetreeProcessor(
                dt_file,
                companies=companies,
                license_identifier=license_identifier,
            ).run(config)


if __name__ == "__main__":
    main()
