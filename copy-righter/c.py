from __future__ import annotations

from pathlib import Path

from header import Header
from util import CopyrightProcessor, Config, CopyrightStyle


class CommentStyle:
    SINGLE = "single"
    BLOCK = "block"


DEFAULT_COMPANIES = ["Frickly Systems GmbH"]


class CProcessor(CopyrightProcessor):
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
        original_lines = self._read_lines()
        processed_lines, changed = self._process_lines(original_lines)

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

    def _read_lines(self) -> list[str]:
        return Path(self.path).read_text(encoding="utf-8").splitlines(keepends=True)

    def _write_lines(self, lines: list[str]) -> None:
        Path(self.path).write_text("".join(lines), encoding="utf-8")

    def _process_lines(self, lines: list[str]) -> tuple[list[str], bool]:
        idx = 0
        shebang: str | None = None

        if idx < len(lines) and lines[idx].startswith("#!"):
            shebang = self._ensure_newline(lines[idx])
            idx += 1

        while idx < len(lines) and lines[idx].strip() == "":
            idx += 1

        if idx >= len(lines):
            comment_style = CommentStyle.BLOCK
            header_lines: list[str] = []
            header_end = idx
        else:
            stripped = lines[idx].lstrip()
            if stripped.startswith("//"):
                comment_style = CommentStyle.SINGLE
                header_lines, header_end = self._collect_single_comment(lines, idx)
            elif stripped.startswith("/*"):
                comment_style = CommentStyle.BLOCK
                header_lines, header_end = self._collect_block_comment(lines, idx)
            else:
                comment_style = CommentStyle.BLOCK
                header_lines = []
                header_end = idx

        header = Header(
            config=self.config,
            companies=self.companies,
            license_identifier=self.license_identifier,
        )
        if header_lines:
            if comment_style == CommentStyle.SINGLE:
                header.add_lines(
                    self._strip_single_comment_prefix(line) for line in header_lines
                )
            else:
                header.add_lines(self._strip_block_comment_lines(header_lines))

        formatted_header = header.get_formatted()
        rendered_header = self._render_header(formatted_header, comment_style)

        rest_lines = lines[header_end:]

        new_lines: list[str] = []
        if shebang:
            new_lines.append(shebang)
        new_lines.extend(rendered_header)
        if rest_lines and rest_lines[0].strip():
            new_lines.append("\n")
        new_lines.extend(rest_lines)

        if new_lines and not new_lines[-1].endswith("\n"):
            new_lines[-1] = f"{new_lines[-1]}\n"

        changed = new_lines != lines
        return new_lines, changed

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

    def _strip_single_comment_prefix(self, line: str) -> str:
        stripped = line.lstrip()
        if not stripped.startswith("//"):
            return ""
        content = stripped[2:]
        if content.startswith(" "):
            content = content[1:]
        return content.rstrip("\n")

    def _strip_block_comment_lines(self, lines: list[str]) -> list[str]:
        if not lines:
            return []

        result: list[str] = []
        first = lines[0]
        after = first.split("/*", 1)[1] if "/*" in first else ""

        if "*/" in after:
            inner = after.split("*/", 1)[0].strip()
            if inner:
                result.append(inner)
            return result

        initial = after.strip()
        if initial:
            result.append(initial)

        for body_line in lines[1:]:
            if "*/" in body_line:
                before = body_line.split("*/", 1)[0].strip()
                if before.startswith("*"):
                    before = before[1:].lstrip()
                if before:
                    result.append(before)
                break
            stripped = body_line.strip()
            if stripped.startswith("*"):
                stripped = stripped[1:].lstrip()
            result.append(stripped)

        return result

    def _render_header(self, entries: list[str], style: str) -> list[str]:
        if style == CommentStyle.SINGLE:
            rendered: list[str] = []
            for entry in entries:
                if entry:
                    rendered.append(f"// {entry}\n")
                else:
                    rendered.append("//\n")
            return rendered

        rendered = ["/*\n"]
        for entry in entries:
            if entry:
                rendered.append(f" * {entry}\n")
            else:
                rendered.append(" *\n")
        rendered.append(" */\n")
        return rendered

    def _ensure_newline(self, line: str) -> str:
        return line if line.endswith("\n") else f"{line}\n"
