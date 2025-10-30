from __future__ import annotations

from datetime import datetime

COMPANY_NAME = "Frickly Systems GmbH"
LICENSE_IDENTIFIER = "SPDX-License-Identifier: Apache-2.0"


def rewrite_header(
    lines: list[str],
    *,
    company: str = COMPANY_NAME,
    license_identifier: str = LICENSE_IDENTIFIER,
) -> tuple[list[str], bool]:
    """
    Normalize a header made of plain (prefix-stripped) lines.

    The function keeps existing holders, inserts the company's notice when
    missing, guarantees the SPDX license identifier is present, and moves any
    additional lines below the license separated by a blank entry.
    """
    normalized = [line.rstrip("\n") for line in lines]

    holders: list[str] = []
    other: list[str] = []
    license_present = False

    for entry in normalized:
        stripped = entry.strip()
        if not stripped:
            other.append("")
            continue
        if is_license_line(stripped):
            license_present = True
            continue
        if _style_from_comment(stripped) is not None:
            holders.append(stripped)
            continue
        other.append(stripped)

    if not any(company in holder for holder in holders):
        style = _determine_notice_style(holders)
        holders.append(_build_notice(style, company))

    result: list[str] = []
    result.extend(holders)
    if result:
        result.append("")

    result.append(license_identifier if license_present else license_identifier)

    cleaned_other = _trim_blank_edges(other)
    if cleaned_other:
        result.append("")
        result.extend(cleaned_other)

    result = _squash_blanks(result)
    if result and result[-1] == "":
        result.pop()

    changed = result != normalized
    return result, changed


def is_license_line(line: str) -> bool:
    return "SPDX-License-Identifier:" in line


def is_holder_line(line: str) -> bool:
    return _style_from_comment(line.strip()) is not None


def _determine_notice_style(holders: list[str]) -> str:
    for holder in holders:
        style = _style_from_comment(holder)
        if style is not None:
            return style
    return "spdx_year"


def _style_from_comment(comment: str) -> str | None:
    if comment.startswith("SPDX-FileCopyrightText:"):
        return "spdx_year" if _contains_year(comment) else "spdx"
    if comment.startswith("Copyright (C)"):
        return "simple_year" if _contains_year(comment) else "simple"
    return None


def _contains_year(text: str) -> bool:
    return any(part.isdigit() and len(part) == 4 for part in text.replace("-", " ").split())


def _build_notice(style: str, company: str) -> str:
    year = datetime.now().year
    if style == "simple":
        return f"Copyright (C) {company}"
    if style == "simple_year":
        return f"Copyright (C) {year} {company}"
    if style == "spdx":
        return f"SPDX-FileCopyrightText: Copyright (C) {company}"
    return f"SPDX-FileCopyrightText: Copyright (C) {year} {company}"


def _trim_blank_edges(entries: list[str]) -> list[str]:
    if not entries:
        return []
    start = 0
    end = len(entries)
    while start < end and not entries[start].strip():
        start += 1
    while end > start and not entries[end - 1].strip():
        end -= 1
    return entries[start:end]


def _squash_blanks(entries: list[str]) -> list[str]:
    squashed: list[str] = []
    previous_blank = False
    for entry in entries:
        is_blank = not entry.strip()
        if is_blank and previous_blank:
            continue
        squashed.append("" if is_blank else entry)
        previous_blank = is_blank
    return squashed
