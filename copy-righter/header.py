from __future__ import annotations

from datetime import datetime
from typing import Iterable, Sequence, Tuple, Union

COMPANY_NAME = "Frickly Systems GmbH"
LICENSE_TOKEN = "SPDX-License-Identifier: "
DEFAULT_LICENSE = "Apache-2.0"


class Header:
    """
    Helper that collects plain (comment-prefix stripped) header lines and can
    emit a normalized version containing the SPDX license identifier and the
    company's copyright notice.
    """

    def __init__(
        self,
        *,
        companies: Union[Iterable[str], str, None] = None,
        license_identifier: str | None = None,
        lines: Iterable[str] | None = None,
    ):
        self.companies = self._coerce_companies(companies)
        self._explicit_license = license_identifier is not None
        self.license_identifier = (license_identifier or DEFAULT_LICENSE).strip()
        self._lines: list[str] = []
        if lines is not None:
            self.add_lines(lines)

    def add_line(self, line: str) -> None:
        """Append a single, prefix-stripped line."""
        self._lines.append(self._normalize_line(line))

    def add_lines(self, lines: Iterable[str]) -> None:
        """Append multiple, prefix-stripped lines."""
        for line in lines:
            self.add_line(line)

    def has_license(self) -> bool:
        """Return True if an SPDX license identifier is present."""
        _, _, _, has_license = self._analyse()
        return has_license

    def has_copyright(self) -> bool:
        """Return True if at least one copyright holder is present."""
        _, holders, _, _ = self._analyse()
        return all(
            any(self._company_in_holder(holder, company) for holder in holders)
            for company in self.companies
        )

    def get_formatted(self) -> Tuple[list[str], bool]:
        """
        Return the normalized header lines and a flag indicating whether the
        content changed compared to the original input.
        """
        normalized, holders, other, has_license = self._analyse()

        new_holders = holders[:]
        style = self._determine_notice_style(new_holders)
        for company in self._missing_companies(new_holders):
            new_holders.append(self._build_notice(style, company))

        result: list[str] = []
        if new_holders:
            result.extend(new_holders)
            result.append("")

        result.append(f"{LICENSE_TOKEN}{self.license_identifier}")

        cleaned_other = self._trim_blank_edges(other)
        if cleaned_other:
            result.append("")
            result.extend(cleaned_other)

        result = self._squash_blanks(result)
        if result and result[-1] == "":
            result.pop()

        changed = result != normalized
        return result, changed

    def _analyse(self) -> Tuple[list[str], list[str], list[str], bool]:
        normalized = [self._normalize_line(line) for line in self._lines]
        holders: list[str] = []
        other: list[str] = []
        has_license = False

        for entry in normalized:
            stripped = entry.strip()
            if not stripped:
                other.append("")
                continue
            if self.is_license_line(stripped):
                has_license = True
                if not self._explicit_license:
                    extracted = self._extract_license_value(stripped)
                    if extracted:
                        self.license_identifier = extracted
                continue
            if self._style_from_comment(stripped) is not None:
                holders.append(stripped)
                continue
            other.append(stripped)

        return normalized, holders, other, has_license

    def _normalize_line(self, line: str) -> str:
        return line.rstrip("\n")

    def _missing_companies(self, holders: Sequence[str]) -> list[str]:
        missing: list[str] = []
        for company in self.companies:
            if not any(self._company_in_holder(holder, company) for holder in holders):
                missing.append(company)
        return missing

    def _company_in_holder(self, holder: str, company: str) -> bool:
        return company in holder

    def _coerce_companies(
        self, companies: Union[Iterable[str], str, None]
    ) -> list[str]:
        if companies is None:
            return [COMPANY_NAME]
        if isinstance(companies, str):
            return [companies]
        collected = [company for company in companies if company]
        return collected or [COMPANY_NAME]

    def is_license_line(self, line: str) -> bool:
        return line.startswith(LICENSE_TOKEN)

    def _extract_license_value(self, line: str) -> str:
        if not self.is_license_line(line):
            return ""
        return line[len(LICENSE_TOKEN):].strip()

    def is_holder_line(self, line: str) -> bool:
        return self._style_from_comment(line.strip()) is not None

    def _determine_notice_style(self, holders: list[str]) -> str:
        for holder in holders:
            style = self._style_from_comment(holder)
            if style is not None:
                return style
        return "spdx_year"

    def _style_from_comment(self, comment: str) -> str | None:
        if comment.startswith("SPDX-FileCopyrightText:"):
            return "spdx_year" if self._contains_year(comment) else "spdx"
        if comment.startswith("Copyright (C)"):
            return "simple_year" if self._contains_year(comment) else "simple"
        return None

    def _contains_year(self, text: str) -> bool:
        return any(
            part.isdigit() and len(part) == 4 for part in text.replace("-", " ").split()
        )

    def _build_notice(self, style: str, company: str) -> str:
        year = datetime.now().year
        if style == "simple":
            return f"Copyright (C) {company}"
        if style == "simple_year":
            return f"Copyright (C) {year} {company}"
        if style == "spdx":
            return f"SPDX-FileCopyrightText: Copyright (C) {company}"
        return f"SPDX-FileCopyrightText: Copyright (C) {year} {company}"

    def _trim_blank_edges(self, entries: list[str]) -> list[str]:
        if not entries:
            return []
        start = 0
        end = len(entries)
        while start < end and not entries[start].strip():
            start += 1
        while end > start and not entries[end - 1].strip():
            end -= 1
        return entries[start:end]

    def _squash_blanks(self, entries: list[str]) -> list[str]:
        squashed: list[str] = []
        previous_blank = False
        for entry in entries:
            is_blank = not entry.strip()
            if is_blank and previous_blank:
                continue
            squashed.append("" if is_blank else entry)
            previous_blank = is_blank
        return squashed
