#!/usr/bin/env python3

"""Verify that local Markdown links and embedded images resolve."""

from pathlib import Path
import re
from typing import Optional
from urllib.parse import unquote


PROJECT_ROOT = Path(__file__).resolve().parents[1]
EXCLUDED_DIRECTORIES = {".git", ".pio", ".trash", "managed_components"}
MARKDOWN_LINK = re.compile(r"\[[^\]]*\]\(([^)]+)\)")
HTML_SOURCE = re.compile(r"\b(?:src|href)=[\"']([^\"']+)[\"']")


def markdown_files() -> list[Path]:
    """Returns project documentation while excluding generated trees."""
    # 返回项目文档，并排除生成目录。
    return sorted(
        path
        for path in PROJECT_ROOT.rglob("*.md")
        if not EXCLUDED_DIRECTORIES.intersection(path.relative_to(PROJECT_ROOT).parts)
    )


def local_target(source: Path, raw_target: str) -> Optional[Path]:
    """Resolves one repository-local link target."""
    # 解析一个仓库内链接目标。
    target = raw_target.strip().split()[0].strip("<>")
    if target.startswith(("#", "http://", "https://", "mailto:")):
        return None
    target = unquote(target.split("#", 1)[0].split("?", 1)[0])
    if not target:
        return None
    return (source.parent / target).resolve()


def main() -> None:
    failures: list[str] = []
    for source in markdown_files():
        content = source.read_text(encoding="utf-8")
        targets = MARKDOWN_LINK.findall(content) + HTML_SOURCE.findall(content)
        for raw_target in targets:
            target = local_target(source, raw_target)
            if target is not None and not target.exists():
                relative_source = source.relative_to(PROJECT_ROOT)
                failures.append(f"{relative_source}: missing {raw_target}")

    if failures:
        raise AssertionError("\n".join(failures))


if __name__ == "__main__":
    main()
