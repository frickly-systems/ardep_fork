from __future__ import annotations

from datetime import datetime
from typing import Iterable, Optional, Sequence, Tuple, Union
from util import Config, CopyrightStyle, License, Copyright

COMPANY_NAME = "Frickly Systems GmbH"
LICENSE_TOKEN = "SPDX-License-Identifier: "
DEFAULT_LICENSE = "Apache-2.0"


class Header:
    """
    Helper that collects plain (comment-prefix stripped) header lines and can
    emit a normalized version containing the SPDX license identifier and the
    company's copyright notice.
    """

    config: Config
    companies: list[str]
    license_identifier: Optional[License]
    copyrights: list[Copyright]

    _lines: list[str]
    _license_if_has_none: Optional[License]

    def __init__(
        self,
        config: Config,
        *,
        companies: Union[Iterable[str], str, None] = None,
        license_identifier: Optional[License] = None,
        lines: Optional[Iterable[str]] = None,
    ):
        self.config = config
        self.companies = self._coerce_companies(companies)
        self.license_identifier = None
        self.copyrights = []

        self._license_if_has_none = license_identifier
        self._lines: list[str] = []
        if lines is not None:
            self.add_lines(lines)

    def add_line(self, line: str) -> None:
        """Append a single, prefix-stripped line."""

        try:
            c = Copyright.from_string(line)
            if c is not None:
                self.copyrights.append(c)
            return
        except ValueError:
            pass

        try:
            l = License.from_string(line)
            if l is not None and self.license_identifier is None:
                self.license_identifier = l
            return
        except ValueError:
            pass

        self._lines.append(self._normalize_line(line))

    def add_lines(self, lines: Iterable[str]) -> None:
        """Append multiple, prefix-stripped lines."""
        for line in lines:
            self.add_line(line)

    def has_license(self) -> bool:
        """Return True if an SPDX license identifier is present."""
        return (
            self.license_identifier is not None or self._license_if_has_none is not None
        )

    def has_copyright(self) -> bool:
        """Return True if at least one copyright holder is present."""
        return bool(self.copyrights) or bool(self.companies)

    def _coerce_companies(
        self, companies: Union[Iterable[str], str, None]
    ) -> list[str]:
        if companies is None:
            return [COMPANY_NAME]
        if isinstance(companies, str):
            return [companies]
        collected: list[str] = [company for company in companies if company]
        return collected or [COMPANY_NAME]

    def _normalize_line(self, line: str) -> str:
        return line.rstrip("\n")

    def get_formatted(self) -> list[str]:
        """
        Return the normalized header lines and a flag indicating whether the
        content changed compared to the original input.
        """

        lines: list[str] = []

        copyrights: list[Copyright] = self.copyrights.copy()

        for c in self.companies:
            if c not in [a.holder for a in copyrights]:
                copyrights.append(
                    Copyright(
                        c,
                        style=self.config.copyright_style,
                    )
                )

        for c in copyrights:
            lines.append(c.to_string())

        l: Optional[License] = self.license_identifier or self._license_if_has_none

        if l is not None:
            if lines:
                lines.append("")
            lines.append(l.to_string())

        if lines:
            lines.append("")

        lines.extend(self._lines)

        return lines

    #     original_lines = list(self._lines)

    #     license_idx: Optional[int] = None
    #     notice_entries: list[tuple[int, Copyright, str]] = []
    #     for idx, line in enumerate(original_lines):
    #         if license_idx is None and self._is_license_line(line):
    #             license_idx = idx
    #         try:
    #             notice = Copyright.from_string(line)
    #         except ValueError:
    #             continue
    #         notice_entries.append((idx, notice, line))

    #     notice_indexes = {idx for idx, _, _ in notice_entries}

    #     resolved_license = self._resolve_license()
    #     self.license_identifier = resolved_license
    #     license_line = f"{LICENSE_TOKEN}{resolved_license.identifier}"

    #     existing_holder_keys: set[str] = set()
    #     existing_notice_lines: list[str] = []
    #     for _, notice, original in notice_entries:
    #         existing_holder_keys.add(notice.holder.strip().lower())
    #         if self.config.update_copyrights:
    #             formatted = self._format_notice(notice)
    #             existing_notice_lines.append(formatted)
    #         else:
    #             existing_notice_lines.append(original)

    #     new_notice_lines: list[str] = []
    #     if self._companies_provided:
    #         seen = set(existing_holder_keys)
    #         for company in self.companies:
    #             key = company.strip().lower()
    #             if key in seen:
    #                 continue
    #             new_notice_lines.append(self._build_notice(company))
    #             seen.add(key)

    #     tail_lines: list[str] = []
    #     for idx, line in enumerate(original_lines):
    #         if license_idx is not None and idx == license_idx:
    #             continue
    #         if idx in notice_indexes:
    #             continue
    #         if license_idx is not None and idx < license_idx and not line.strip():
    #             continue
    #         tail_lines.append(line)

    #     formatted: list[str] = []
    #     formatted.extend(existing_notice_lines)
    #     formatted.extend(new_notice_lines)

    #     if license_line:
    #         if formatted and formatted[-1] != "":
    #             formatted.append("")
    #         formatted.append(license_line)

    #     if tail_lines:
    #         if license_line and tail_lines[0] != "":
    #             formatted.append("")
    #         formatted.extend(tail_lines)

    #     changed = formatted != original_lines
    #     return formatted, changed

    # def _normalize_line(self, line: str) -> str:
    #     return line.rstrip("\n")

    # def _is_license_line(self, line: str) -> bool:
    #     return line.startswith(LICENSE_TOKEN)

    # def _resolve_license(self) -> License:
    #     if self.license_identifier is not None:
    #         return self.license_identifier
    #     if isinstance(self._license_if_has_none, License):
    #         return self._license_if_has_none
    #     if isinstance(self._license_if_has_none, str):
    #         return License(self._license_if_has_none)
    #     return License(DEFAULT_LICENSE)

    # def _build_notice(self, holder: str) -> str:
    #     style = self.config.copyright_style
    #     year: Optional[int] = None
    #     if style in (CopyrightStyle.SIMPLE_YEAR, CopyrightStyle.SPDX_YEAR):
    #         year = datetime.now().year
    #     notice = Copyright(holder=holder, year=year)
    #     return notice.to_string(style)

    # def _format_notice(self, notice: Copyright) -> str:
    #     style = self.config.copyright_style
    #     year = notice.year
    #     if style in (CopyrightStyle.SIMPLE_YEAR, CopyrightStyle.SPDX_YEAR):
    #         if year is None:
    #             year = datetime.now().year
    #     else:
    #         year = None
    #     normalized = Copyright(holder=notice.holder, year=year)
    #     return normalized.to_string(style)
