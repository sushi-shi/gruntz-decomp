"""gruntz.core.tsv - the one tracked-table convention.

A tracked TSV is: `#`-prefixed banner lines, one tab-separated header row,
then data rows. Fields never contain tabs; hex is lowercase 0x. This module
is the only reader/writer of that shape.
"""

from __future__ import annotations

from pathlib import Path


def read(path: Path | str) -> tuple[list[str], list[str], list[dict[str, str]]]:
    """(banner_lines, header_fields, rows-as-dicts). Raises on a missing
    header or a row whose field count disagrees with it."""
    banner: list[str] = []
    header: list[str] | None = None
    rows: list[dict[str, str]] = []
    for lineno, line in enumerate(Path(path).read_text().splitlines(), 1):
        if line.startswith("#"):
            banner.append(line)
            continue
        if not line.strip():
            continue
        fields = line.split("\t")
        if header is None:
            header = fields
            continue
        if len(fields) != len(header):
            raise ValueError(f"{path}:{lineno}: {len(fields)} fields, "
                             f"header has {len(header)}")
        rows.append(dict(zip(header, fields)))
    if header is None:
        raise ValueError(f"{path}: no header row")
    return banner, header, rows


def write(path: Path | str, banner: list[str], header: list[str],
          rows: list[dict[str, str]] | list[list[str]]) -> bool:
    """Write the table; returns True when the file content actually changed
    (write-if-different, so ninja's restat can prune downstream edges)."""
    out = list(banner) + ["\t".join(header)]
    for row in rows:
        fields = [row.get(h, "") for h in header] if isinstance(row, dict) else row
        out.append("\t".join(str(f) for f in fields))
    text = "\n".join(out) + "\n"
    path = Path(path)
    if path.is_file() and path.read_text() == text:
        return False
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text)
    return True


def rint(value: str) -> int:
    """Hex-or-decimal int (`0x...` or plain)."""
    value = value.strip()
    return int(value, 16) if value.lower().startswith("0x") else int(value)
