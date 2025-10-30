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
        lines, normalized_changed = self._normalize_header(lines)

        changed = license_changed or copyright_changed or normalized_changed

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
        changed = False
        license_index = self._find_license_index(lines)

        if license_index is None:
            insert_at = self._header_end_index(lines)
            lines.insert(insert_at, LICENSE_TEXT)
            changed = True

        return lines, changed

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

    def _ensure_license_spacer(
        self, lines: list[str], license_idx: int
    ) -> tuple[int, bool]:
        license_idx, trimmed = self._trim_blank_before(lines, license_idx)
        if license_idx == 0:
            return license_idx, trimmed

        prev_line = lines[license_idx - 1]
        if prev_line.strip() == "#":
            license_idx, deduped = self._dedupe_comment_spacers(lines, license_idx)
            return license_idx, trimmed or deduped

        lines.insert(license_idx, "#\n")
        return license_idx + 1, True

    def _trim_blank_before(self, lines: list[str], index: int) -> tuple[int, bool]:
        changed = False
        while index > 0 and not lines[index - 1].strip():
            del lines[index - 1]
            index -= 1
            changed = True
        return index, changed

    def _dedupe_comment_spacers(
        self, lines: list[str], license_idx: int
    ) -> tuple[int, bool]:
        changed = False
        while license_idx > 1 and lines[license_idx - 2].strip() == "#":
            del lines[license_idx - 2]
            license_idx -= 1
            changed = True
        return license_idx, changed

    def _normalize_header(self, lines: list[str]) -> tuple[list[str], bool]:
        original = lines[:]

        body_lines = lines[:]
        shebang_line: str | None = None
        shebang_idx: int | None = None
        for idx, entry in enumerate(body_lines):
            if entry.startswith("#!"):
                shebang_idx = idx
                break
        if shebang_idx is not None:
            shebang_line = body_lines.pop(shebang_idx)
            if shebang_idx < len(body_lines) and body_lines[shebang_idx].strip() == "#":
                body_lines.pop(shebang_idx)

        license_idx = self._find_license_index(body_lines)
        if license_idx is None:
            license_idx = self._find_license_index(lines)
            if license_idx is None:
                return lines, False

        header_end = self._header_end_index(body_lines)
        header = body_lines[:header_end]

        holders: list[str] = []
        other: list[str] = []
        license_line = LICENSE_TEXT

        for line in header:
            stripped = line.strip()
            if not stripped:
                continue
            if stripped.startswith("#"):
                content = stripped.lstrip("#").strip()
                if self._is_license_line(content):
                    license_line = line if line.endswith("\n") else f"{line}\n"
                elif self._style_from_comment(content) is not None:
                    holders.append(line if line.endswith("\n") else f"{line}\n")
                elif content:
                    other.append(line if line.endswith("\n") else f"{line}\n")
            else:
                other.append(line)

        new_header: list[str] = []
        new_header.extend(holders)
        if holders:
            new_header.append("#\n")
        new_header.append(license_line)

        rebuilt_body = new_header + other + body_lines[header_end:]

        final_lines: list[str] = []
        if shebang_line is not None:
            if not shebang_line.endswith("\n"):
                shebang_line = f"{shebang_line}\n"
            final_lines.append(shebang_line)
            final_lines.append("#\n")
        final_lines.extend(rebuilt_body)

        changed = final_lines != original

        if changed:
            lines[:] = final_lines

        license_idx = self._find_license_index(lines)
        if license_idx is None:
            return lines, changed

        license_idx, spacer_changed = self._ensure_license_spacer(lines, license_idx)
        post_changed = self._ensure_post_license_gap(lines, license_idx)
        return lines, changed or spacer_changed or post_changed

    def _ensure_post_license_gap(self, lines: list[str], license_idx: int) -> bool:
        gap_start = license_idx + 1
        next_content_idx = gap_start
        while next_content_idx < len(lines) and lines[next_content_idx].strip() in {"", "#"}:
            next_content_idx += 1

        next_is_comment = (
            next_content_idx < len(lines)
            and lines[next_content_idx].lstrip().startswith("#")
        )

        desired_gap = "#\n" if next_is_comment else "\n"
        changed = False

        if gap_start >= len(lines):
            lines.append(desired_gap)
            return True

        if gap_start == next_content_idx:
            lines.insert(gap_start, desired_gap)
            return True

        if lines[gap_start] != desired_gap:
            lines[gap_start] = desired_gap
            changed = True

        if next_content_idx - gap_start > 1:
            del lines[gap_start + 1 : next_content_idx]
            changed = True

        return changed

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
