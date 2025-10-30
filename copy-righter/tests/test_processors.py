from pathlib import Path
import sys
from datetime import datetime

PROJECT_ROOT = Path(__file__).resolve().parents[1]
project_root_str = str(PROJECT_ROOT)
if project_root_str not in sys.path:
    sys.path.insert(0, project_root_str)

from c import CProcessor  # type: ignore  # pylint: disable=import-error
from cmake import CmakeProcessor  # type: ignore  # pylint: disable=import-error
from python import PythonProcessor  # type: ignore  # pylint: disable=import-error
from config import Config  # type: ignore  # pylint: disable=import-error


def _run_processor(processor_cls, content: str, suffix: str) -> str:
    tmp_path = PROJECT_ROOT / "tests" / f"_temp_test_file{suffix}"
    tmp_path.write_text(content, encoding="utf-8")
    processor = processor_cls(str(tmp_path))
    processor.run(Config(dry_run=False, verbose=False))
    result = tmp_path.read_text(encoding="utf-8")
    tmp_path.unlink()
    return result


def test_cmake_processor_formats_header():
    original = """# Existing header\n#\n# SPDX-License-Identifier: Apache-2.0\n\nset(VAR 1)\n"""

    result = _run_processor(CmakeProcessor, original, ".cmake")

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
    original = """/*\n * Test header\n *\n */\nint main(void) { return 0; }\n"""

    result = _run_processor(CProcessor, original, ".c")

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
    original = """// Test header\n//\n// SPDX-License-Identifier: Apache-2.0\n\nint value;\n"""

    result = _run_processor(CProcessor, original, ".cpp")

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

    result = _run_processor(PythonProcessor, original, ".py")

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

    result = _run_processor(PythonProcessor, original, ".py")

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
