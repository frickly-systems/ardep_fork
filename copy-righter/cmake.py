from __future__ import annotations

from pathlib import Path

from config import Config
from header import rewrite_header
from util import CopyrightProcessor


class CmakeProcessor(CopyrightProcessor):
    path: str

    def __init__(self, path: str):
        super().__init__(path)

    def run(self, config: Config):
        lines = self._read_lines()

        processed_lines = self._process_lines(lines)
        changed = processed_lines != lines

        if not changed:
            if config.verbose:
                print(f"[OK] {self.path}")
            return

        if config.dry_run:
            print(f"[DRY-RUN] Would update {self.path}")
            return

        self._write_lines(processed_lines)
        if config.verbose:
            print(f"[UPDATED] {self.path}")

    def _process_lines(self, lines: list[str]) -> list[str]:
        original = lines[:]
        working = lines[:]

        shebang: str | None = None
        if working and working[0].startswith("#!"):
            shebang = self._ensure_newline(working.pop(0))

        while working and not working[0].strip():
            working.pop(0)

        header_lines, rest_lines = self._split_header(working)
        plain_header = [self._strip_comment_prefix(line) for line in header_lines]
        rewritten_header, _ = rewrite_header(plain_header)
        rendered_header = [self._format_comment_line(entry) for entry in rewritten_header]

        rest_lines = self._strip_redundant_hash_header(rest_lines)

        final_lines: list[str] = []
        if shebang:
            final_lines.append(shebang)
            final_lines.append("#\n")

        final_lines.extend(rendered_header)

        gap_line = self._gap_line_for_rest(rest_lines)
        if gap_line:
            final_lines.append(gap_line)

        final_lines.extend(rest_lines)

        if final_lines and not final_lines[-1].endswith("\n"):
            final_lines[-1] = f"{final_lines[-1]}\n"

        return final_lines

    def _split_header(self, lines: list[str]) -> tuple[list[str], list[str]]:
        header: list[str] = []
        idx = 0
        limit = min(20, len(lines))
        while idx < len(lines) and idx < limit:
            stripped = lines[idx].strip()
            if not stripped:
                header.append(lines[idx])
                idx += 1
                continue
            if stripped.startswith("#"):
                header.append(lines[idx])
                idx += 1
                continue
            break
        return header, lines[idx:]

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

    def _gap_line_for_rest(self, rest_lines: list[str]) -> str | None:
        if not rest_lines:
            return None
        if rest_lines[0].strip() == "":
            return None

        first_idx = 0
        while first_idx < len(rest_lines) and rest_lines[first_idx].strip() == "":
            first_idx += 1
        if first_idx > 0:
            return None

        first_line = rest_lines[0].lstrip()
        if first_line.startswith("#"):
            return "#\n"
        return "\n"

    def _strip_redundant_hash_header(self, lines: list[str]) -> list[str]:
        idx = 0
        while idx < len(lines):
            stripped = lines[idx].strip()
            if stripped == "":
                idx += 1
                continue
            if stripped.startswith("#"):
                content = self._strip_comment_prefix(lines[idx])
                if not content:
                    idx += 1
                    continue
                if content.startswith("SPDX-License-Identifier:"):
                    idx += 1
                    continue
                if content.startswith("SPDX-FileCopyrightText:"):
                    idx += 1
                    continue
                if content.startswith("Copyright (C)"):
                    idx += 1
                    continue
            break
        return lines[idx:]

    def _ensure_newline(self, line: str) -> str:
        return line if line.endswith("\n") else f"{line}\n"

    def _read_lines(self) -> list[str]:
        return Path(self.path).read_text(encoding="utf-8").splitlines(keepends=True)

    def _write_lines(self, lines: list[str]) -> None:
        Path(self.path).write_text("".join(lines), encoding="utf-8")
