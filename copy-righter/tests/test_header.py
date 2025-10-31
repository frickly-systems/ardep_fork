from datetime import datetime
from pathlib import Path
import sys

PROJECT_ROOT = Path(__file__).resolve().parents[1]
project_root_str = str(PROJECT_ROOT)
if project_root_str not in sys.path:
    sys.path.insert(0, project_root_str)

from header import Header
from util import Config, CopyrightStyle


def get_default_config() -> Config:
    return Config(
        dry_run=False,
        verbose=False,
        copyright_style=CopyrightStyle.SPDX_YEAR,
    )


def _default_notice(company: str, style: str = "spdx_year") -> str:
    year = datetime.now().year
    if style == "spdx":
        return f"SPDX-FileCopyrightText: Copyright (C) {company}"
    if style == "spdx_year":
        return f"SPDX-FileCopyrightText: Copyright (C) {year} {company}"
    if style == "simple_year":
        return f"Copyright (C) {year} {company}"
    return f"Copyright (C) {company}"


def test_header_adds_company_and_license_when_missing():
    company = "Frickly Systems GmbH"
    header = Header(config=get_default_config(), companies=[company])
    header.add_lines(["", "Project Foo"])

    formatted, changed = header.get_formatted()

    expected_notice = _default_notice(company)
    assert changed is True
    assert formatted == [
        expected_notice,
        "",
        "SPDX-License-Identifier: Apache-2.0",
        "",
        "Project Foo",
    ]


def test_header_preserves_existing_holder_and_license():
    frickly = "Frickly Systems GmbH"
    header = Header(config=get_default_config(), companies=[frickly])
    header.add_lines(
        [
            "Copyright (C) MBition GmbH",
            "",
            "SPDX-License-Identifier: Apache-2.0",
        ]
    )

    formatted, changed = header.get_formatted()

    # The config uses SPDX_YEAR style for new notices
    assert changed is True
    assert formatted == [
        "Copyright (C) MBition GmbH",
        _default_notice(frickly),
        "",
        "SPDX-License-Identifier: Apache-2.0",
    ]


def test_header_detects_license_and_holder():
    company = "Example Corp"
    header = Header(config=get_default_config(), companies=[company])
    header.add_lines([f"Copyright (C) {company}", "SPDX-License-Identifier: Foo"])

    assert header.has_license() is True
    assert header.has_copyright() is True


def test_header_no_changes_when_complete():
    company = "Frickly Systems GmbH"
    header = Header(config=get_default_config(), companies=[company])
    notice = _default_notice(company)
    header.add_lines(
        [
            notice,
            "",
            "SPDX-License-Identifier: Apache-2.0",
        ]
    )

    formatted, changed = header.get_formatted()

    assert changed is False

    assert formatted == [
        notice,
        "",
        "SPDX-License-Identifier: Apache-2.0",
    ]


def test_header_respects_explicit_license_override():
    header = Header(config=get_default_config(), license_identifier="MIT")
    header.add_lines(["", "Existing info"])

    formatted, changed = header.get_formatted()

    assert changed is True
    assert formatted == [
        "SPDX-License-Identifier: MIT",
        "",
        "Existing info",
    ]


def test_header_respects_requested_notice_style():
    company = "Custom Corp"

    config_simple = Config(
        dry_run=False, verbose=False, copyright_style=CopyrightStyle.SIMPLE
    )
    header_simple = Header(config=config_simple, companies=[company])
    header_simple.add_lines(["", "Body"])
    formatted_simple, _ = header_simple.get_formatted()
    assert formatted_simple[0] == f"Copyright (C) {company}"

    config_spdx = Config(
        dry_run=False, verbose=False, copyright_style=CopyrightStyle.SPDX
    )
    header_spdx = Header(config=config_spdx, companies=[company])
    header_spdx.add_lines(["", "Body"])
    formatted_spdx, _ = header_spdx.get_formatted()
    assert formatted_spdx[0] == f"SPDX-FileCopyrightText: Copyright (C) {company}"

    config_year = Config(
        dry_run=False, verbose=False, copyright_style=CopyrightStyle.SIMPLE_YEAR
    )
    header_year = Header(config=config_year, companies=[company])
    header_year.add_lines(["", "Body"])
    formatted_year, _ = header_year.get_formatted()
    year = datetime.now().year
    assert formatted_year[0] == f"Copyright (C) {year} {company}"

    config_spdx_year = Config(
        dry_run=False, verbose=False, copyright_style=CopyrightStyle.SPDX_YEAR
    )
    header_spdx = Header(config=config_spdx_year, companies=[company])
    header_spdx.add_lines(["", "Body"])
    formatted_spdx, _ = header_spdx.get_formatted()
    assert formatted_spdx[0] == f"SPDX-FileCopyrightText: Copyright (C) {company}"


def test_header_preserves_lowercase_c_holders():
    companies = ["Frickly Systems GmbH", "MBition GmbH"]
    header = Header(config=get_default_config(), companies=companies)
    header.add_lines(
        [
            "Copyright (c) 2019 STMicroelectronics.",
            "Copyright (C) Frickly Systems GmbH",
            "",
            "SPDX-License-Identifier: Apache-2.0",
        ]
    )

    formatted, changed = header.get_formatted()

    assert changed is True

    assert formatted == [
        "Copyright (c) 2019 STMicroelectronics.",
        "Copyright (C) Frickly Systems GmbH",
        _default_notice(company="MBition GmbH"),
        "",
        "SPDX-License-Identifier: Apache-2.0",
    ]


def test_header_adds_missing_companies_in_order():
    companies = ["Frickly Systems GmbH", "MBition GmbH", "Another Corp"]
    header = Header(config=get_default_config(), companies=companies)
    header.add_lines(
        [
            "Copyright (C) MBition GmbH",
            "",
            "SPDX-License-Identifier: Apache-2.0",
        ]
    )

    assert header.has_copyright() is False

    formatted, changed = header.get_formatted()

    assert changed is True

    assert formatted == [
        "Copyright (C) MBition GmbH",
        _default_notice("Frickly Systems GmbH"),
        _default_notice("Another Corp"),
        "",
        "SPDX-License-Identifier: Apache-2.0",
    ]
