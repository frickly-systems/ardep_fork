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
        self._lines.append(self._normalize_line(line))

        try:
            c = Copyright.from_string(line)
            if c is not None:
                self.copyrights.append(c)
        except ValueError:
            pass

        try:
            l = License.from_string(line)
            if l is not None and self.license_identifier is None:
                self.license_identifier = l
        except ValueError:
            pass

    def add_lines(self, lines: Iterable[str]) -> None:
        """Append multiple, prefix-stripped lines."""
        for line in lines:
            self.add_line(line)

    def has_license(self) -> bool:
        """Return True if an SPDX license identifier is present."""
        return self.license_identifier is not None

    def has_copyright(self) -> bool:
        """Return True if at least one copyright holder is present."""
        return self.copyrights != []

    def _coerce_companies(
        self, companies: Union[Iterable[str], str, None]
    ) -> list[str]:
        if companies is None:
            return [COMPANY_NAME]
        if isinstance(companies, str):
            return [companies]
        collected: list[str] = [company for company in companies if company]
        return collected or [COMPANY_NAME]

    def get_formatted(self) -> Tuple[list[str], bool]:
        """
        Return the normalized header lines and a flag indicating whether the
        content changed compared to the original input.
        """

        pass
