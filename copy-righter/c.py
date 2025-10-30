from __future__ import annotations

from pathlib import Path

from config import Config
from header import is_holder_line, is_license_line, rewrite_header
from util import CopyrightProcessor


class CommentStyle:
    SINGLE = "single"
    BLOCK = "block"


class CProcessor(CopyrightProcessor):
    path: str

    def __init__(self, path: str):
        super().__init__(path)

    def run(self, config: Config):
        original_lines = self._read_lines()
        processed_lines, changed = self._process_lines(original_lines)

        if not changed:
            if config.verbose:
                print(f"[OK] {self.path}")
            return

        if config.dry_run:
            print(f"[DRY-RUN] Would update {self.path}")
            return

        self._write_lines(processed_lines)
        if config.verbose:
            print(f"[UPDATED] {self.path}")

    def _read_lines(self) -> list[str]:
        return Path(self.path).read_text(encoding="utf-8").splitlines(keepends=True)

    def _write_lines(self, lines: list[str]) -> None:
        Path(self.path).write_text("".join(lines), encoding="utf-8")

    def _process_lines(self, lines: list[str]) -> tuple[list[str], bool]:
        original = lines[:]
        working: list[str] = lines[:]

        if not working:
            working = []

        shebang: str | None = None
        if working and working[0].startswith("#!"):
            shebang = self._ensure_trailing_newline(working.pop(0))

        while working and not working[0].strip():
            working.pop(0)
        self._consume_legacy_hash_header(working)
        while working and not working[0].strip():
            working.pop(0)

        comment_style = self._detect_comment_style(working)
        header_lines, rest_lines = self._split_header(working, comment_style)
        rest_lines = self._strip_legacy_hash_header(rest_lines)
        rest_lines = self._strip_legacy_comment_header(rest_lines, comment_style)
        header_content = self._parse_header_content(comment_style, header_lines)
        rewritten_content, _ = rewrite_header(header_content)
        if shebang and comment_style == CommentStyle.BLOCK:
            if not rewritten_content or rewritten_content[0].strip():
                rewritten_content = [""] + rewritten_content
        rendered_header = self._render_header(comment_style, rewritten_content)

        final_lines: list[str] = []
        if shebang:
            final_lines.append(shebang)
            if comment_style == CommentStyle.SINGLE:
                final_lines.append("//\n")

        final_lines.extend(rendered_header)

        gap_line = self._gap_line_for_rest(rest_lines, comment_style)
        if gap_line:
            final_lines.append(gap_line)

        final_lines.extend(rest_lines)
        if final_lines and not final_lines[-1].endswith("\n"):
            final_lines[-1] = f"{final_lines[-1]}\n"

        changed = final_lines != original
        return final_lines, changed

    def _detect_comment_style(self, lines: list[str]) -> str:
        for line in lines[:20]:
            stripped = line.strip()
            if not stripped:
                continue
            if stripped.startswith("//"):
                return CommentStyle.SINGLE
            if stripped.startswith("/*") or stripped.startswith("*") or stripped.startswith("*/"):
                return CommentStyle.BLOCK
        return CommentStyle.BLOCK

    def _split_header(self, lines: list[str], style: str) -> tuple[list[str], list[str]]:
        if not lines:
            return [], []

        if style == CommentStyle.SINGLE:
            header: list[str] = []
            idx = 0
            while idx < len(lines) and lines[idx].lstrip().startswith("//"):
                header.append(lines[idx])
                idx += 1
            return header, lines[idx:]

        idx = 0
        if not lines[0].lstrip().startswith("/*"):
            return [], lines
        header: list[str] = []
        while idx < len(lines):
            header.append(lines[idx])
            if "*/" in lines[idx]:
                idx += 1
                break
            idx += 1
        return header, lines[idx:]

    def _parse_header_content(self, style: str, header_lines: list[str]) -> list[str]:
        if not header_lines:
            return []

        if style == CommentStyle.SINGLE:
            content: list[str] = []
            for line in header_lines:
                stripped = line.lstrip()
                if not stripped.startswith("//"):
                    continue
                text = stripped[2:]
                if text.startswith(" "):
                    text = text[1:]
                content.append(text.rstrip("\n"))
            return content

        content: list[str] = []
        for idx, line in enumerate(header_lines):
            stripped = line.rstrip("\n")
            if idx == 0:
                after = stripped.split("/*", 1)[1] if "/*" in stripped else ""
                after = after.strip().rstrip("*/").strip()
                if after:
                    content.append(after)
                continue
            if "*/" in stripped:
                before = stripped.split("*/", 1)[0].strip()
                if before.startswith("*"):
                    before = before[1:].lstrip()
                if before:
                    content.append(before)
                break
            stripped = stripped.strip()
            if stripped.startswith("*"):
                text = stripped[1:].lstrip()
                content.append(text)
            else:
                content.append(stripped)
        return content

    def _render_header(self, style: str, content: list[str]) -> list[str]:
        if style == CommentStyle.SINGLE:
            lines: list[str] = []
            for entry in content:
                text = entry.rstrip()
                if text:
                    lines.append(f"// {text}\n")
                else:
                    lines.append("//\n")
            return lines

        lines: list[str] = ["/*\n"]
        for entry in content:
            text = entry.rstrip()
            if text:
                lines.append(f" * {text}\n")
            else:
                lines.append(" *\n")
        lines.append(" */\n")
        return lines

    def _gap_line_for_rest(self, rest_lines: list[str], style: str) -> str | None:
        if not rest_lines:
            return None
        if rest_lines[0].strip() == "":
            return None

        first_idx = 0
        while first_idx < len(rest_lines) and rest_lines[first_idx].strip() == "":
            first_idx += 1
        if first_idx > 0:
            return None

        first_line = rest_lines[0].lstrip()
        if style == CommentStyle.SINGLE and (
            first_line.startswith("//") or first_line.startswith("/*")
        ):
            return "//\n"
        return "\n"

    def _strip_legacy_hash_header(self, lines: list[str]) -> list[str]:
        idx = 0
        while idx < len(lines):
            stripped = lines[idx].strip()
            if stripped == "":
                idx += 1
                continue
            if stripped == "#":
                idx += 1
                continue
            if stripped.startswith("# SPDX-License-Identifier:"):
                idx += 1
                continue
            if stripped.startswith("# SPDX-FileCopyrightText:"):
                idx += 1
                continue
            break
        return lines[idx:]

    def _consume_legacy_hash_header(self, lines: list[str]) -> None:
        while lines:
            stripped = lines[0].strip()
            if stripped == "":
                lines.pop(0)
                continue
            if stripped == "#":
                lines.pop(0)
                continue
            if stripped.startswith("# SPDX-License-Identifier:"):
                lines.pop(0)
                continue
            if stripped.startswith("# SPDX-FileCopyrightText:"):
                lines.pop(0)
                continue
            break

    def _strip_legacy_comment_header(
        self, lines: list[str], default_style: str
    ) -> list[str]:
        if not lines:
            return lines

        stripped = lines[0].lstrip()
        if stripped.startswith("/*"):
            header, remainder = self._split_header(lines, CommentStyle.BLOCK)
            if header and self._is_redundant_header(CommentStyle.BLOCK, header):
                return self._strip_leading_blank(remainder)
            return lines

        if stripped.startswith("//"):
            header, remainder = self._split_header(lines, CommentStyle.SINGLE)
            if header and self._is_redundant_header(CommentStyle.SINGLE, header):
                return self._strip_leading_blank(remainder)
            return lines

        if default_style == CommentStyle.SINGLE and stripped.startswith("#"):
            # Treat legacy hash header already handled
            return lines

        return lines

    def _is_redundant_header(self, style: str, header_lines: list[str]) -> bool:
        content = self._parse_header_content(style, header_lines)
        if not content:
            return True
        for entry in content:
            stripped = entry.strip()
            if stripped == "":
                continue
            if is_license_line(stripped):
                continue
            if is_holder_line(stripped):
                continue
            return False
        return True

    def _strip_leading_blank(self, lines: list[str]) -> list[str]:
        idx = 0
        while idx < len(lines) and lines[idx].strip() == "":
            idx += 1
        return lines[idx:]

    def _ensure_trailing_newline(self, line: str) -> str:
        return line if line.endswith("\n") else f"{line}\n"
