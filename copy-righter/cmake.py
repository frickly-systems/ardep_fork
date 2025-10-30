from __future__ import annotations

from dataclasses import dataclass
from datetime import datetime
from pathlib import Path

from util import CopyrightProcessor
from config import Config


LICENSE_TEXT = "# SPDX-License-Identifier: Apache-2.0\n"
COMPANY = "Frickly Systems GmbH"


class Style:
    SIMPLE = "simple"
    SIMPLE_YEAR = "simple_year"
    SPDX = "spdx"
    SPDX_YEAR = "spdx_year"


class CmakeProcessor(CopyrightProcessor):
    path: str

    def __init__(self, path: str):
        super().__init__(path)

    def run(self, config: Config):
        lines = self._read_lines()

        lines, license_changed = self._ensure_license(lines)
        lines, copyright_changed = self._ensure_copyright(lines)

        changed = license_changed or copyright_changed

        if not changed:
            if config.verbose:
                print(f"[OK] {self.path}")
            return

        if config.dry_run:
            print(f"[DRY-RUN] Would update {self.path}")
            return

        self._write_lines(lines)
        if config.verbose:
            print(f"[UPDATED] {self.path}")

    def _read_lines(self) -> list[str]:
        return Path(self.path).read_text(encoding="utf-8").splitlines(keepends=True)

    def _write_lines(self, lines: list[str]) -> None:
        Path(self.path).write_text("".join(lines), encoding="utf-8")

    def _ensure_license(self, lines: list[str]) -> tuple[list[str], bool]:
        license_index = self._find_license_index(lines)
        if license_index is not None:
            return lines, False

        insert_at = self._header_end_index(lines)
        if self._requires_comment_gap(lines, insert_at):
            lines.insert(insert_at, "#\n")
            insert_at += 1

        lines.insert(insert_at, LICENSE_TEXT)
        insert_at += 1

        if insert_at >= len(lines) or lines[insert_at].strip():
            lines.insert(insert_at, "\n")

        return lines, True

    def _ensure_copyright(self, lines: list[str]) -> tuple[list[str], bool]:
        holders = self._collect_holders(lines)
        if any(COMPANY in holder.line for holder in holders):
            return lines, False

        style = self._determine_style(holders)
        new_line = self._build_notice(style)

        if holders:
            insert_at = holders[-1].index + 1
        else:
            license_idx = self._find_license_index(lines)
            if license_idx is not None:
                insert_at = license_idx
            else:
                insert_at = self._header_end_index(lines)

        lines.insert(insert_at, new_line)

        subsequent_index = insert_at + 1
        if subsequent_index < len(lines) and self._is_license_line(
            lines[subsequent_index]
        ):
            if subsequent_index >= len(lines) or lines[subsequent_index].strip():
                lines.insert(subsequent_index, "\n")

        return lines, True

    def _find_license_index(self, lines: list[str]) -> int | None:
        limit = min(20, len(lines))
        for idx in range(limit):
            if self._is_license_line(lines[idx]):
                return idx
        return None

    def _is_license_line(self, line: str) -> bool:
        return "SPDX-License-Identifier:" in line

    def _header_end_index(self, lines: list[str]) -> int:
        limit = min(20, len(lines))
        for idx in range(limit):
            stripped = lines[idx].strip()
            if not stripped:
                continue
            if stripped.startswith("#"):
                continue
            return idx
        return limit

    def _requires_comment_gap(self, lines: list[str], insert_at: int) -> bool:
        if insert_at == 0:
            return False
        previous = lines[insert_at - 1].strip()
        if not previous:
            return False
        if previous == "#":
            return False
        return previous.startswith("#")

    def _collect_holders(self, lines: list[str]) -> list["Holder"]:
        limit = min(20, len(lines))
        holders: list[Holder] = []
        for idx in range(limit):
            line = lines[idx]
            stripped = line.strip()
            if not stripped.startswith("#"):
                continue
            comment = stripped.lstrip("#").strip()
            if self._is_license_line(comment):
                continue
            if "Copyright" in comment:
                holders.append(Holder(index=idx, line=line, comment=comment))
        return holders

    def _determine_style(self, holders: list["Holder"]) -> str:
        for holder in holders:
            style = self._style_from_comment(holder.comment)
            if style is not None:
                return style
        return Style.SPDX_YEAR

    def _style_from_comment(self, comment: str) -> str | None:
        if comment.startswith("SPDX-FileCopyrightText:"):
            return Style.SPDX_YEAR if self._contains_year(comment) else Style.SPDX
        if comment.startswith("Copyright (C)"):
            return Style.SIMPLE_YEAR if self._contains_year(comment) else Style.SIMPLE
        return None

    def _contains_year(self, text: str) -> bool:
        return any(
            part.isdigit() and len(part) == 4 for part in text.replace("-", " ").split()
        )

    def _build_notice(self, style: str) -> str:
        year = datetime.now().year
        if style == Style.SIMPLE:
            return f"# Copyright (C) {COMPANY}\n"
        if style == Style.SIMPLE_YEAR:
            return f"# Copyright (C) {year} {COMPANY}\n"
        if style == Style.SPDX:
            return f"# SPDX-FileCopyrightText: Copyright (C) {COMPANY}\n"
        return f"# SPDX-FileCopyrightText: Copyright (C) {year} {COMPANY}\n"


@dataclass
class Holder:
    index: int
    line: str
    comment: str
