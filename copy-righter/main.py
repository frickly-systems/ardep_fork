from argparse import ArgumentParser, Namespace
from collections import defaultdict
from pathlib import Path
from subprocess import CalledProcessError, CompletedProcess, run
from cmake import CmakeProcessor
from c import CProcessor


from config import Config
from paths_to_process import PathsToProcess


def main():
    parser: ArgumentParser = ArgumentParser(description="Copy-righter tool")
    parser.add_argument("path", help="Path to process")
    parser.add_argument(
        "--dry-run", action="store_true", help="Run without making actual changes"
    )
    parser.add_argument("--verbose", action="store_true", help="Enable verbose output")
    parser.add_argument(
        "-c", "--config", choices=["zephyr", "python"], help="configuration"
    )

    args: Namespace = parser.parse_args()

    config: Config = Config.from_args(args)

    base_path = Path(args.path).resolve()

    paths: PathsToProcess = PathsToProcess(str(base_path))

    if config.verbose:
        print("Files to process:")
        for filter_type, file_list in paths.paths.items():
            print(f"\n{filter_type}:")
            for file_path in file_list:
                print(f"  {file_path}")

    for cmake_files in paths.paths.get("CMakeLists.txt", []):
        processor = CmakeProcessor(cmake_files)
        processor.run(config)

    for h_files in paths.paths.get(".h", []):
        processor = CProcessor(h_files)
        processor.run(config)


if __name__ == "__main__":
    main()
