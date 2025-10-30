from collections import defaultdict
from pathlib import Path
from subprocess import CalledProcessError, CompletedProcess, run


class PathsToProcess:
    paths: dict[str, list[str]] = {}

    def __init__(self, root_path: str):

        base_path = Path(root_path).resolve()
        git_root = self._git_root(base_path)

        try:
            relative_target = base_path.relative_to(git_root)
        except ValueError:  # pragma: no cover - only if outside repo, caught earlier
            relative_target = base_path

        # Recursively find all files under args.path that are not ignored by gitignore
        candidate_files: list[Path] = self._git_tracked_files(git_root, relative_target)

        allowed_suffixes = {
            ".c",
            ".h",
            ".cpp",
            ".hpp",
            ".dts",
            ".dtsi",
            ".yaml",
            ".yml",
            ".py",
            ".overlay",
        }
        excluded_yaml_names = {
            "sample.yml",
            "sample.yaml",
            "testcase.yml",
            "testcase.yaml",
        }

        filtered: list[Path] = []
        for file_path in candidate_files:
            name = file_path.name
            suffix = file_path.suffix

            if name == "CMakeLists.txt" or name.startswith("Kconfig"):
                filtered.append(file_path)
                continue

            if suffix in allowed_suffixes:
                if name in excluded_yaml_names:
                    continue
                filtered.append(file_path)

        # Structure the file list by their filter type
        buckets: dict[str, list[str]] = defaultdict(list)
        for path in filtered:
            name = path.name
            suffix = path.suffix

            if name == "CMakeLists.txt":
                buckets["CMakeLists.txt"].append(str(path))
            elif name.startswith("Kconfig"):
                buckets["Kconfig"].append(str(path))
            else:
                buckets[suffix].append(str(path))

        self.paths: dict[str, list[str]] = dict(buckets)

    def _git_root(self, path: Path) -> Path:
        """Locate the repository root for a path."""
        candidate = path if path.is_dir() else path.parent
        try:
            result: CompletedProcess[str] = run(
                ["git", "-C", str(candidate), "rev-parse", "--show-toplevel"],
                check=True,
                text=True,
                capture_output=True,
            )
        except CalledProcessError as exc:  # pragma: no cover - surfaced to caller
            raise RuntimeError("Provided path is not inside a git repository") from exc

        return Path(result.stdout.strip())

    def _git_tracked_files(self, root: Path, target: Path) -> list[Path]:
        """Return files under target that are not ignored by git."""
        try:
            result: CompletedProcess[str] = run(
                [
                    "git",
                    "-C",
                    str(root),
                    "ls-files",
                    "--cached",
                    "--others",
                    "--exclude-standard",
                    "--",
                    str(target),
                ],
                check=True,
                text=True,
                capture_output=True,
            )
        except CalledProcessError as exc:  # pragma: no cover - surfaced to caller
            raise RuntimeError("Failed to list files via git") from exc

        lines = [line for line in result.stdout.splitlines() if line.strip()]
        return [root / line for line in lines]
