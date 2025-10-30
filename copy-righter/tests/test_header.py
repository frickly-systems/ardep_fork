from datetime import datetime
from pathlib import Path
import sys

PROJECT_ROOT = Path(__file__).resolve().parents[1]
project_root_str = str(PROJECT_ROOT)
if project_root_str not in sys.path:
    sys.path.insert(0, project_root_str)

from header import Header


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
    header = Header(companies=[company])
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
    header = Header(companies=[frickly])
    header.add_lines(
        [
            "Copyright (C) MBition GmbH",
            "",
            "SPDX-License-Identifier: Apache-2.0",
        ]
    )

    formatted, changed = header.get_formatted()

    expected_notice = f"Copyright (C) {frickly}"
    assert changed is True
    assert formatted == [
        "Copyright (C) MBition GmbH",
        expected_notice,
        "",
        "SPDX-License-Identifier: Apache-2.0",
    ]


def test_header_detects_license_and_holder():
    company = "Example Corp"
    header = Header(companies=[company])
    header.add_lines([f"Copyright (C) {company}", "SPDX-License-Identifier: Foo"])

    assert header.has_license() is True
    assert header.has_copyright() is True


def test_header_no_changes_when_complete():
    company = "Frickly Systems GmbH"
    header = Header(companies=[company])
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
    header = Header(license_identifier="MIT")
    header.add_lines(["", "Existing info"])

    formatted, changed = header.get_formatted()

    assert changed is True
    assert "SPDX-License-Identifier: MIT" in formatted


def test_header_adds_missing_companies_in_order():
    companies = ["Frickly Systems GmbH", "MBition GmbH", "Another Corp"]
    header = Header(companies=companies)
    header.add_lines(
        [
            "Copyright (C) MBition GmbH",
            "",
            "SPDX-License-Identifier: Apache-2.0",
        ]
    )

    assert header.has_copyright() is False

    formatted, changed = header.get_formatted()

    expected_spdx = "SPDX-License-Identifier: Apache-2.0"
    expected_notices = [
        "Copyright (C) MBition GmbH",
        _default_notice("Frickly Systems GmbH", style="simple"),
        _default_notice("Another Corp", style="simple"),
    ]

    assert changed is True
    assert formatted[:3] == expected_notices
    assert formatted[3] == ""
    assert formatted[4] == expected_spdx
