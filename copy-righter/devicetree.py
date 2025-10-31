from __future__ import annotations

from pathlib import Path

from header import Header
from util import CopyrightProcessor, Config, CopyrightStyle


DEFAULT_COMPANIES = ["Frickly Systems GmbH"]


class DevicetreeProcessor(CopyrightProcessor):
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
        self.companies = companies or DEFAULT_COMPANIES
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
        comment_style = "block"

        header_lines: list[str] = []

        while idx < len(lines):
            stripped = lines[idx].lstrip()
            if stripped.startswith("/*"):
                comment_style = "block"
                header_lines, idx = self._collect_block_comment(lines, idx)
                break
            if stripped.startswith("//"):
                comment_style = "single"
                header_lines, idx = self._collect_single_comment(lines, idx)
                break
            # Devicetree files use C-style comments; stop header collection at the first
            # non-comment (e.g. #include) directive.
            if stripped.startswith("#"):
                break
            if stripped.strip():
                break
            header_lines.append(lines[idx])
            idx += 1

        header = Header(
            config=self.config,
            companies=self.companies,
            license_identifier=self.license_identifier,
        )

        if comment_style == "block":
            header.add_lines(self._strip_block_comment_lines(header_lines))
        elif comment_style == "single":
            header.add_lines(self._strip_single_comment_lines(header_lines))
        else:
            header.add_lines([line.rstrip("\n") for line in header_lines])

        formatted_header, _ = header.get_formatted()
        rendered_header = self._render_header(formatted_header, comment_style)

        rest_lines = lines[idx:]

        new_lines: list[str] = []
        new_lines.extend(rendered_header)

        if rest_lines:
            if rest_lines[0].strip():
                new_lines.append("\n")
        new_lines.extend(rest_lines)

        if new_lines and not new_lines[-1].endswith("\n"):
            new_lines[-1] = f"{new_lines[-1]}\n"

        return new_lines

    def _collect_block_comment(
        self, lines: list[str], start: int
    ) -> tuple[list[str], int]:
        collected: list[str] = []
        idx = start
        while idx < len(lines):
            collected.append(lines[idx])
            if "*/" in lines[idx]:
                idx += 1
                break
            idx += 1
        return collected, idx

    def _collect_single_comment(
        self, lines: list[str], start: int
    ) -> tuple[list[str], int]:
        collected: list[str] = []
        idx = start
        while idx < len(lines):
            stripped = lines[idx].lstrip()
            if not stripped.startswith("//"):
                break
            collected.append(lines[idx])
            idx += 1
        return collected, idx

    def _strip_block_comment_lines(self, lines: list[str]) -> list[str]:
        if not lines:
            return []

        content: list[str] = []
        for idx, line in enumerate(lines):
            stripped = line.rstrip("\n").lstrip()
            if idx == 0:
                stripped = stripped[2:].strip()
                if stripped.endswith("*/"):
                    stripped = stripped[:-2].strip()
                if stripped:
                    content.append(stripped)
                continue
            if "*/" in stripped:
                stripped = stripped.split("*/", 1)[0].strip()
                if stripped.startswith("*"):
                    stripped = stripped[1:].lstrip()
                if stripped:
                    content.append(stripped)
                break
            if stripped.startswith("*"):
                stripped = stripped[1:].lstrip()
            content.append(stripped)
        return content

    def _strip_single_comment_prefix(self, line: str) -> str:
        stripped = line.lstrip()[2:]
        if stripped.startswith(" "):
            stripped = stripped[1:]
        return stripped.rstrip("\n")

    def _strip_single_comment_lines(self, lines: list[str]) -> list[str]:
        return [self._strip_single_comment_prefix(line) for line in lines]

    def _strip_hash_comment_lines(self, lines: list[str]) -> list[str]:
        content: list[str] = []
        for line in lines:
            stripped = line.lstrip()[1:]
            if stripped.startswith(" "):
                stripped = stripped[1:]
            content.append(stripped.rstrip("\n"))
        return content

    def _render_header(self, entries: list[str], style: str) -> list[str]:
        if style == "single":
            rendered: list[str] = []
            for entry in entries:
                if entry:
                    rendered.append(f"// {entry}\n")
                else:
                    rendered.append("//\n")
            return rendered

        if style == "hash":
            rendered: list[str] = []
            for entry in entries:
                if entry:
                    rendered.append(f"# {entry}\n")
                else:
                    rendered.append("#\n")
            return rendered

        rendered = ["/*\n"]
        for entry in entries:
            if entry:
                rendered.append(f" * {entry}\n")
            else:
                rendered.append(" *\n")
        rendered.append(" */\n")
        return rendered

    def _read_lines(self) -> list[str]:
        return Path(self.path).read_text(encoding="utf-8").splitlines(keepends=True)

    def _write_lines(self, lines: list[str]) -> None:
        Path(self.path).write_text("".join(lines), encoding="utf-8")
