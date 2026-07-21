"""gruntz.core.ir - clang front-end helpers shared across packages: compile a TU
to textual LLVM IR under the MSVC-compat flag set, and read the clangd compdb's
per-TU flags. Carved out of build/labels.py (the labels pipeline still imports
from here) so cleanliness/caller_callee needs no build/ import.
"""
import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
INC = str(REPO / "include")   # repo-local headers (mirror src/) live here; on the clang -I path
# Vendored third-party SDK headers live one dir deep under vendor/<sdk>/ (e.g.
# vendor/miles/Mss32.h, vendor/smacker/smack.h) so `#include <Mss32.h>` resolves
# like the original toolchain's SDK include dirs.
VENDOR_INCS = sorted(str(d) for d in (REPO / "vendor").iterdir() if d.is_dir()) \
    if (REPO / "vendor").is_dir() else []
# DirectX 6 headers sit one level deeper (vendor/directx6/Include), like cc_wrap.
if (REPO / "vendor" / "directx6" / "Include").is_dir():
    VENDOR_INCS.append(str(REPO / "vendor" / "directx6" / "Include"))
INC_CL = [f"/I{p}" for p in (INC, *VENDOR_INCS)]   # clang-cl driver (/I)
INC_GCC = [f"-I{p}" for p in (INC, *VENDOR_INCS)]  # plain clang driver (-I)

TARGET = "i686-pc-windows-msvc"
MSC_COMPAT = "1100"
MS_FLAGS = [f"--target={TARGET}", f"-fms-compatibility-version={MSC_COMPAT}",
            "-fms-extensions"]


def _log(msg):
    print(f"[ir] {msg}", file=sys.stderr)


def emit_ir(clang, tu, flags, cl_flags=None):
    """Compile a TU to textual LLVM IR.

    `-emit-llvm` stops at a fatal error (unlike `-ast-dump`, which recovers), so
    the system headers MUST resolve - the IR compile uses the clangd compdb's
    per-TU clang-cl flags (`/imsvc` lowercase-mirror includes + defines) when
    available, falling back to the bare MS_FLAGS otherwise.

    In clang-cl driver mode `-S`/`-o -` are rejected, and `-Xclang -emit-llvm`
    writes textual IR only to a real `-o` file - so write to a temp file and read
    it back. The plain driver streams to stdout.
    """
    if cl_flags is not None:
        # Retry once: under heavy parallel label-regen the temp .ll can go missing
        # (clang left nothing / a tmp race), which is transient - a re-run succeeds.
        res = None
        for _attempt in range(2):
            with tempfile.NamedTemporaryFile(suffix=".ll", delete=False) as tf:
                ll = tf.name
            try:
                cmd = [clang, "--driver-mode=cl", "/c", "/DGRUNTZ_EMIT_META",
                       *cl_flags, *INC_CL, "-Xclang", "-emit-llvm", "-o", ll, tu]
                res = subprocess.run(cmd, capture_output=True, text=True)
                # exists-guard: getsize() FileNotFounds on a vanished temp; treat as no-IR.
                ir = Path(ll).read_text() if (os.path.exists(ll) and os.path.getsize(ll)) else ""
            finally:
                try:
                    os.unlink(ll)
                except OSError:
                    pass
            if ir:
                return ir
        _log(f"ERROR {tu}: clang -emit-llvm produced no IR\n"
             f"{(res.stderr[:400] if res else '')}")
        return None
    cmd = [clang, "-DGRUNTZ_EMIT_META", *MS_FLAGS, *flags, *INC_GCC,
           "-S", "-emit-llvm", "-o", "-", tu]
    res = subprocess.run(cmd, capture_output=True, text=True)
    if not res.stdout:
        _log(f"ERROR {tu}: clang -emit-llvm produced no IR\n{res.stderr[:400]}")
        return None
    return res.stdout


def load_compdb(path):
    """compile_commands.json -> {realpath(source): [clang-cl flags]}.

    The clangd compdb carries the per-TU MS flags + `/imsvc` lowercase-mirror
    include dirs that let clang's header lookup succeed on case-sensitive Linux.
    We drop the driver (clang-cl), the `/c`, and the source file - emit_ir/clang_ast
    re-add their own driver mode, action, and the TU.
    """
    try:
        db = json.loads(Path(path).read_text())
    except (OSError, json.JSONDecodeError):
        return {}
    out = {}
    for e in db:
        args = e.get("arguments") or []
        if not args:
            continue
        src = e.get("file")
        flags = [a for a in args[1:] if a != "/c" and a != src]
        if src:
            out[os.path.realpath(src)] = flags
    return out
