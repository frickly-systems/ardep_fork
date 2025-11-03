from __future__ import annotations

from argparse import Namespace
from dataclasses import dataclass
from datetime import datetime
from enum import Enum
from typing import Optional


class CopyrightStyle(Enum):
    SIMPLE = "simple"
    SIMPLE_YEAR = "year"
    SPDX = "spdx"
    SPDX_YEAR = "spdx-year"

    @classmethod
    def from_args(cls, args: Namespace) -> "CopyrightStyle":
        style = getattr(args, "copyright_style", None)
        return cls.from_string(style) or cls.SPDX_YEAR

    @classmethod
    def from_string(cls, style_str: Optional[str]) -> Optional["CopyrightStyle"]:
        if not style_str:
            return None
        mapping = {
            "simple": cls.SIMPLE,
            "year": cls.SIMPLE_YEAR,
            "spdx": cls.SPDX,
            "spdx-year": cls.SPDX_YEAR,
        }
        key = style_str.lower()
        if key not in mapping:
            raise ValueError(f"Invalid copyright style: {style_str}")
        return mapping[key]


@dataclass
class Config:
    dry_run: bool
    verbose: bool
    copyright_style: CopyrightStyle
    update_copyrights: bool = False

    @classmethod
    def from_args(cls, args: Namespace) -> "Config":
        return cls(
            dry_run=getattr(args, "dry_run", False),
            verbose=getattr(args, "verbose", False),
            copyright_style=CopyrightStyle.from_args(args),
            update_copyrights=getattr(args, "update_copyrights", False),
        )


class CopyrightProcessor:
    path: str
    config: Config

    def __init__(self, path: str, config: Config):
        self.path = path
        self.config = config

    def run(self):
        pass


class License:
    identifier: str

    def __init__(self, identifier: str):
        self.identifier = identifier

    @classmethod
    def from_string(cls, text: str) -> "License":
        text = text.strip()
        if not text.startswith("SPDX-License-Identifier:"):
            raise ValueError(f"Invalid license line: {text}")
        identifier = text[len("SPDX-License-Identifier:") :].strip()
        return cls(identifier=identifier)

    def to_string(self) -> str:
        return f"SPDX-License-Identifier: {self.identifier}"


class Copyright:
    holder: str
    year: Optional[int]

    style: Optional[CopyrightStyle]
    lowercase_c: bool

    def __init__(
        self,
        holder: str,
        year: Optional[int] = datetime.now().year,
        style: Optional[CopyrightStyle] = None,
        lowercase_c: bool = False,
    ):
        self.holder = holder
        self.year = year
        self.style = style
        self.lowercase_c = lowercase_c

    @classmethod
    def from_string(cls, text: str) -> "Copyright":
        text = text.strip()
        year = None
        holder = ""
        style = None
        lowercase_c = "(c)" in text and "(C)" not in text

        if text.startswith("SPDX-FileCopyrightText:"):
            style = (
                CopyrightStyle.SPDX_YEAR
                if cls._contains_year(text)
                else CopyrightStyle.SPDX
            )
            content = text[len("SPDX-FileCopyrightText: Copyright (C)") :].strip()
        elif text.lower().startswith("copyright (c)"):
            style = (
                CopyrightStyle.SIMPLE_YEAR
                if cls._contains_year(text)
                else CopyrightStyle.SIMPLE
            )
            content = text[len("Copyright (C)") :].strip()
        else:
            raise ValueError(f"Invalid copyright line: {text}")

        parts = content.split()
        if parts and parts[0].isdigit() and len(parts[0]) == 4:
            year = int(parts[0])
            holder = " ".join(parts[1:])
        else:
            holder = " ".join(parts)

        return cls(holder=holder, year=year, style=style, lowercase_c=lowercase_c)

    def to_string(self, style: Optional[CopyrightStyle] = None) -> str:
        if style is None:
            style = self.style

        copyright_casing = "(c)" if self.lowercase_c else "(C)"

        if style == CopyrightStyle.SIMPLE or (
            style == CopyrightStyle.SIMPLE_YEAR and not self.year
        ):
            return f"Copyright {copyright_casing} {self.holder}"
        elif style == CopyrightStyle.SIMPLE_YEAR:
            return f"Copyright {copyright_casing} {self.year} {self.holder}"
        elif style == CopyrightStyle.SPDX or (
            style == CopyrightStyle.SPDX_YEAR and not self.year
        ):
            return f"SPDX-FileCopyrightText: Copyright {copyright_casing} {self.holder}"
        elif style == CopyrightStyle.SPDX_YEAR and self.year:
            return f"SPDX-FileCopyrightText: Copyright {copyright_casing} {self.year} {self.holder}"

        # Fallback (should not reach here)
        if self.year:
            return f"SPDX-FileCopyrightText: Copyright {copyright_casing} {self.year} {self.holder}"
        return f"SPDX-FileCopyrightText: Copyright {copyright_casing} {self.holder}"

    @classmethod
    def _contains_year(self, text: str) -> bool:
        return any(
            part.isdigit() and len(part) == 4 for part in text.replace("-", " ").split()
        )
