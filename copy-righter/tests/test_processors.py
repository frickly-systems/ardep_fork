from pathlib import Path
import sys
from datetime import datetime

PROJECT_ROOT = Path(__file__).resolve().parents[1]
project_root_str = str(PROJECT_ROOT)
if project_root_str not in sys.path:
    sys.path.insert(0, project_root_str)

from c import CProcessor  # type: ignore  # pylint: disable=import-error
from cmake import CmakeProcessor  # type: ignore  # pylint: disable=import-error
from devicetree import DevicetreeProcessor  # type: ignore  # pylint: disable=import-error
from python import PythonProcessor  # type: ignore  # pylint: disable=import-error
from util import Config, CopyrightStyle, License  # type: ignore  # pylint: disable=import-error


def get_default_config() -> Config:
    return Config(
        dry_run=False,
        verbose=False,
        copyright_style=CopyrightStyle.SPDX_YEAR,
    )


def _run_processor(
    processor_cls, content: str, suffix: str, config: Config, **kwargs
) -> str:
    tmp_path = PROJECT_ROOT / "tests" / f"_temp_test_file{suffix}"
    tmp_path.write_text(content, encoding="utf-8")
    processor = processor_cls(str(tmp_path), config, **kwargs)
    processor.run()
    result = tmp_path.read_text(encoding="utf-8")
    tmp_path.unlink()
    return result


def test_cmake_processor_formats_header():
    original = "\n".join(
        [
            "# Existing header",
            "#",
            "# SPDX-License-Identifier: Apache-2.0",
            "",
            "set(VAR 1)",
            "",
        ]
    )

    result = _run_processor(
        CmakeProcessor,
        original,
        ".cmake",
        get_default_config(),
        license_identifier=License("Apache-2.0"),
    )

    year = datetime.now().year
    expected = (
        f"# SPDX-FileCopyrightText: Copyright (C) {year} Frickly Systems GmbH\n"
        "#\n"
        "# SPDX-License-Identifier: Apache-2.0\n"
        "#\n"
        "# Existing header\n"
        "\n"
        "set(VAR 1)\n"
    )

    assert result == expected


def test_c_processor_formats_block_comment_header():
    original = "\n".join(
        [
            "/*",
            " * Test header",
            " *",
            " */",
            "int main(void) { return 0; }",
            "",
        ]
    )

    result = _run_processor(
        CProcessor,
        original,
        ".c",
        get_default_config(),
        license_identifier=License("Apache-2.0"),
    )

    year = datetime.now().year
    expected = (
        "/*\n"
        f" * SPDX-FileCopyrightText: Copyright (C) {year} Frickly Systems GmbH\n"
        " *\n"
        " * SPDX-License-Identifier: Apache-2.0\n"
        " *\n"
        " * Test header\n"
        " */\n"
        "\n"
        "int main(void) { return 0; }\n"
    )

    assert result == expected


def test_c_processor_formats_single_line_header():
    original = (
        """// Test header\n//\n// SPDX-License-Identifier: Apache-2.0\n\nint value;\n"""
    )

    result = _run_processor(
        CProcessor,
        original,
        ".cpp",
        get_default_config(),
    )

    year = datetime.now().year
    expected = (
        f"// SPDX-FileCopyrightText: Copyright (C) {year} Frickly Systems GmbH\n"
        "//\n"
        "// SPDX-License-Identifier: Apache-2.0\n"
        "//\n"
        "// Test header\n"
        "\n"
        "int value;\n"
    )

    assert result == expected


def test_python_processor_formats_header():
    original = """# Existing header\n"""

    result = _run_processor(
        PythonProcessor,
        original,
        ".py",
        get_default_config(),
        license_identifier=License("Apache-2.0"),
    )

    year = datetime.now().year
    expected = (
        f"# SPDX-FileCopyrightText: Copyright (C) {year} Frickly Systems GmbH\n"
        "#\n"
        "# SPDX-License-Identifier: Apache-2.0\n"
        "#\n"
        "# Existing header\n"
    )

    assert result == expected


def test_python_processor_handles_shebang():
    original = """#!/usr/bin/env python3\nprint('hi')\n"""

    result = _run_processor(
        PythonProcessor,
        original,
        ".py",
        get_default_config(),
        license_identifier=License("Apache-2.0"),
    )

    year = datetime.now().year
    expected = (
        "#!/usr/bin/env python3\n"
        "#\n"
        f"# SPDX-FileCopyrightText: Copyright (C) {year} Frickly Systems GmbH\n"
        "#\n"
        "# SPDX-License-Identifier: Apache-2.0\n"
        "\n"
        "print('hi')\n"
    )

    assert result == expected


def test_devicetree_processor_formats_block_comment():
    original = """/*\n * Test header\n */\n/dts-v1/;\n"""

    result = _run_processor(
        DevicetreeProcessor,
        original,
        ".dts",
        get_default_config(),
        license_identifier=License("Apache-2.0"),
    )

    year = datetime.now().year
    expected = (
        "/*\n"
        f" * SPDX-FileCopyrightText: Copyright (C) {year} Frickly Systems GmbH\n"
        " *\n"
        " * SPDX-License-Identifier: Apache-2.0\n"
        " *\n"
        " * Test header\n"
        " */\n"
        "\n"
        "/dts-v1/;\n"
    )

    assert result == expected


def test_processors_honor_custom_license_identifier():
    original_py = "# header\n"
    original_dts = "/dts-v1/;\n"

    result_py = _run_processor(
        PythonProcessor,
        original_py,
        ".py",
        license_identifier="MIT",
        config=get_default_config(),
    )
    result_dts = _run_processor(
        DevicetreeProcessor,
        original_dts,
        ".dts",
        license_identifier="MIT",
        config=get_default_config(),
    )

    assert "SPDX-License-Identifier: MIT" in result_py
    assert "SPDX-License-Identifier: MIT" in result_dts


def test_python_processor_preserves_script_metadata():
    original = (
        "#!/usr/bin/env -S uv run --script\n"
        "# /// script\n"
        "# dependencies = [\n"
        '#    "can-isotp==2.0.3",\n'
        '#    "udsoncan==1.21.2",\n'
        "# ]\n"
        "# ///\n"
        "#\n"
        "# Copyright (C) Frickly Systems GmbH\n"
        "# Copyright (C) MBition GmbH\n"
        "#\n"
        "# SPDX-License-Identifier: Apache-2.0\n"
    )

    result = _run_processor(PythonProcessor, original, ".py", get_default_config())

    assert result == original
