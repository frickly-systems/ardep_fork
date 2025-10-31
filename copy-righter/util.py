from __future__ import annotations

from argparse import Namespace
from dataclasses import dataclass
from enum import Enum
from typing import Optional


class CopyrightStyle(Enum):
    SIMPLE = "simple"
    SIMPLE_YEAR = "year"
    SPDX = "spdx"
    SPDX_YEAR = "spdx-year"

    @classmethod
    def from_args(cls, args: Namespace) -> "CopyrightStyle":
        return cls(getattr(args, "copyright_style"))

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

    @classmethod
    def from_args(cls, args: Namespace) -> "Config":
        return cls(
            dry_run=args.dry_run,
            verbose=args.verbose,
            copyright_style=CopyrightStyle.from_args(args),
        )


class CopyrightProcessor:
    path: str
    config: Config

    def __init__(self, path: str, config: Config):
        self.path = path
        self.config = config

    def run(self):
        pass
