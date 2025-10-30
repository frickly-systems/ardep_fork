from copy_righter.header import Header


def test_header_adds_company_and_license_when_missing():
    header = Header(company="Frickly Systems GmbH")
    header.add_lines(["", "Project Foo"])

    formatted, changed = header.get_formatted()

    assert changed is True
    assert formatted == [
        "SPDX-FileCopyrightText: Copyright (C) 2025 Frickly Systems GmbH",
        "",
        "SPDX-License-Identifier: Apache-2.0",
        "",
        "Project Foo",
    ]


def test_header_preserves_existing_holder_and_license():
    header = Header(company="Frickly Systems GmbH")
    header.add_lines(
        [
            "Copyright (C) MBition GmbH",
            "",
            "SPDX-License-Identifier: Apache-2.0",
        ]
    )

    formatted, changed = header.get_formatted()

    assert changed is True
    assert formatted == [
        "Copyright (C) MBition GmbH",
        "Copyright (C) Frickly Systems GmbH",
        "",
        "SPDX-License-Identifier: Apache-2.0",
    ]


def test_header_detects_license_and_holder():
    header = Header()
    header.add_lines(["Copyright (C) Test", "SPDX-License-Identifier: Foo"])

    assert header.has_license() is True
    assert header.has_copyright() is True


def test_header_no_changes_when_complete():
    header = Header(company="Frickly Systems GmbH")
    header.add_lines(
        [
            "SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH",
            "",
            "SPDX-License-Identifier: Apache-2.0",
        ]
    )

    formatted, changed = header.get_formatted()

    assert changed is False
    assert formatted == [
        "SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH",
        "",
        "SPDX-License-Identifier: Apache-2.0",
    ]
