from pathlib import Path
import sys

PROJECT_ROOT = Path(__file__).resolve().parents[1]
project_root_str = str(PROJECT_ROOT)
if project_root_str not in sys.path:
    sys.path.insert(0, project_root_str)

from argparse import Namespace
import pytest

from util import Config, CopyrightStyle  # type: ignore  # pylint: disable=import-error


def test_copyright_style_from_string():
    assert CopyrightStyle.from_string("simple") == CopyrightStyle.SIMPLE
    assert CopyrightStyle.from_string("year") == CopyrightStyle.SIMPLE_YEAR
    assert CopyrightStyle.from_string("spdx") == CopyrightStyle.SPDX
    assert CopyrightStyle.from_string("spdx-year") == CopyrightStyle.SPDX_YEAR
    assert CopyrightStyle.from_string(None) is None


def test_copyright_style_from_string_case_insensitive():
    assert CopyrightStyle.from_string("SIMPLE") == CopyrightStyle.SIMPLE
    assert CopyrightStyle.from_string("Year") == CopyrightStyle.SIMPLE_YEAR
    assert CopyrightStyle.from_string("SPDX-YEAR") == CopyrightStyle.SPDX_YEAR


def test_copyright_style_from_string_invalid():
    with pytest.raises(ValueError, match="Invalid copyright style"):
        CopyrightStyle.from_string("invalid")


def test_config_from_args_with_style():
    args = Namespace(dry_run=True, verbose=False, copyright_style="spdx")
    config = Config.from_args(args)

    assert config.dry_run is True
    assert config.verbose is False
    assert config.copyright_style == CopyrightStyle.SPDX


def test_config_from_args_without_style():
    args = Namespace(dry_run=False, verbose=True)
    config = Config.from_args(args)

    assert config.dry_run is False
    assert config.verbose is True
    assert config.copyright_style is CopyrightStyle.SPDX_YEAR


def test_config_from_args_with_different_styles():
    for style_str, expected_enum in [
        ("simple", CopyrightStyle.SIMPLE),
        ("year", CopyrightStyle.SIMPLE_YEAR),
        ("spdx", CopyrightStyle.SPDX),
        ("spdx-year", CopyrightStyle.SPDX_YEAR),
    ]:
        args = Namespace(dry_run=False, verbose=False, copyright_style=style_str)
        config = Config.from_args(args)
        assert config.copyright_style == expected_enum
