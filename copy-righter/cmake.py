from __future__ import annotations

from pathlib import Path

from header import Header
from util import CopyrightProcessor, Config, CopyrightStyle


class CmakeProcessor(CopyrightProcessor):
    path: str

    def __init__(
        self,
        path: str,
        config: Config,
        *,
        companies: list[str] | None = None,
        license_identifier: str | None = None,
    ):
        super().__init__(path, config)
        self.companies = companies or ["Frickly Systems GmbH"]
        self.license_identifier = license_identifier

    def run(self):
        lines = self._read_lines()

        processed_lines = self._process_lines(lines)
        changed = processed_lines != lines

        if not changed:
            if self.config.verbose:
                print(f"[OK] {self.path}")
            return

        if self.config.dry_run:
            print(f"[DRY-RUN] Would update {self.path}")
            return

        self._write_lines(processed_lines)
        if self.config.verbose:
            print(f"[UPDATED] {self.path}")

    def _process_lines(self, lines: list[str]) -> list[str]:
        idx = 0
        shebang: str | None = None
        if idx < len(lines) and lines[idx].startswith("#!"):
            shebang = self._ensure_newline(lines[idx])
            idx += 1

        header_start = idx
        header_lines: list[str] = []
        while idx < len(lines):
            stripped = lines[idx].lstrip()
            if stripped.startswith("#"):
                header_lines.append(lines[idx])
                idx += 1
            else:
                break

        rest_lines = lines[idx:]

        header = Header(
            config=self.config,
            companies=self.companies,
            license_identifier=self.license_identifier,
        )
        if header_lines:
            for raw in header_lines:
                header.add_line(self._strip_comment_prefix(raw))
        formatted_header, _ = header.get_formatted()
        rendered_header = [
            self._format_comment_line(entry) for entry in formatted_header
        ]

        new_lines: list[str] = []
        if shebang:
            new_lines.append(shebang)
            if rendered_header:
                new_lines.append("#\n")

        new_lines.extend(rendered_header)

        if rest_lines:
            if rest_lines[0].strip():
                new_lines.append("\n")
        new_lines.extend(rest_lines)

        if new_lines and not new_lines[-1].endswith("\n"):
            new_lines[-1] = f"{new_lines[-1]}\n"

        return new_lines

    def _strip_comment_prefix(self, line: str) -> str:
        stripped = line.lstrip()
        if not stripped.startswith("#"):
            return ""
        content = stripped[1:]
        if content.startswith(" "):
            content = content[1:]
        return content.rstrip("\n")

    def _format_comment_line(self, entry: str) -> str:
        if entry:
            return f"# {entry}\n"
        return "#\n"

    def _ensure_newline(self, line: str) -> str:
        return line if line.endswith("\n") else f"{line}\n"

    def _read_lines(self) -> list[str]:
        return Path(self.path).read_text(encoding="utf-8").splitlines(keepends=True)

    def _write_lines(self, lines: list[str]) -> None:
        Path(self.path).write_text("".join(lines), encoding="utf-8")
