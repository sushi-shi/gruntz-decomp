"""gruntz.tool.cl - the era compiler.

    from gruntz.tool import cl
    cl.compile(src, obj, ["/nologo", "/c", "/O2", "/MT"])

ninja rule form (same function):
    python3 -m gruntz.tool.cl --out <obj> --src <src> -- /nologo /c /O2 /MT

System headers/libs come from the wine registry INCLUDE/LIB (init_prefix);
repo-local include/ and vendor/<sdk>/ dirs are passed as /I so
`#include <Module/Foo.h>` and the vendored SDK headers resolve. Wine spews
unrelated noise and can return a non-cl exit code, so the success signal is
the produced .obj, never the return code alone.
"""

from __future__ import annotations

from pathlib import Path

from gruntz.core.paths import INCLUDE, VENDOR
from gruntz.tool import ToolError
from gruntz.tool.wine import era_tool, run, winepath


def repo_include_flags() -> list[str]:
    dirs = []
    if INCLUDE.is_dir():
        dirs.append(INCLUDE)
    if VENDOR.is_dir():
        dirs += sorted(d for d in VENDOR.iterdir() if d.is_dir())
    return [f"/I{winepath(d)}" for d in dirs]


def compile(src: Path | str, out: Path | str, flags: list[str], *,
            extra_includes: list[Path] = (), timeout: float | None = None) -> str:
    """Compile one TU; return cl's output. Raises ToolError without an .obj."""
    src, out = Path(src).resolve(), Path(out).resolve()
    if not src.exists():
        raise ToolError(f"source missing: {src}")
    out.parent.mkdir(parents=True, exist_ok=True)
    out.unlink(missing_ok=True)

    cl_exe = era_tool("cl.exe")
    argv = ["wine", str(cl_exe), *repo_include_flags(),
            *[f"/I{winepath(d)}" for d in extra_includes],
            *flags, f"/Fo{winepath(out)}", winepath(src)]
    output, rc = run(argv, cwd=out.parent, timeout=timeout, success=out)
    if not out.exists():
        tail = "\n".join(output.strip().splitlines()[-12:])
        raise ToolError(f"cl produced no object for {src.name} (rc={rc}):\n{tail}")
    return output


def main() -> int:
    import argparse
    import sys
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out", required=True)
    ap.add_argument("--src", required=True)
    ap.add_argument("flags", nargs=argparse.REMAINDER)
    a = ap.parse_args()
    flags = a.flags[1:] if a.flags and a.flags[0] == "--" else a.flags
    try:
        compile(a.src, a.out, flags)
    except ToolError as e:
        print(f"[cl] {e}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
