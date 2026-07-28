#!/usr/bin/env bash
# Build recomp/harness/pidrun.exe with the period toolchain under wine.
#
# Run inside `nix develop` (MSVC_DIR / WINEPREFIX come from the shell).
#
# Two things here are not obvious:
#
#   * MSDIS100.DLL is missing from the packaged VC5 toolchain, and link.exe
#     imports it, so it will not even load under wine. The repo already solves
#     this for `ninja candidate`; we reuse the same helper
#     (scripts/gruntz/build/msdis_stub.py) rather than re-inventing it.
#   * /BASE:0x10000000 - pidrun maps retail GRUNTZ.EXE at its preferred base
#     0x00400000 and calls straight into it, so the harness itself must not be
#     living there.
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo="$(cd "$here/../.." && pwd)"
: "${MSVC_DIR:?run inside 'nix develop'}"
: "${WINEPREFIX:?run inside 'nix develop'}"

export PYTHONPATH="$repo/scripts${PYTHONPATH:+:$PYTHONPATH}"
export WINEDEBUG="${WINEDEBUG:-fixme-all,err-all}"

python3 - "$WINEPREFIX" "$MSVC_DIR" <<'PY'
import sys
from gruntz.build.msdis_stub import ensure_msdis
ensure_msdis(sys.argv[1], sys.argv[2], verbose=True)
PY

cd "$here"
out="$here/pidrun.exe"
rm -f "$out" ./*.obj

wine "$MSVC_DIR/bin/cl.exe" /nologo /O2 /MT /W3 \
    "/Fe$(winepath -w "$out")" "$(winepath -w "$here/pidrun.c")" \
    /link /BASE:0x10000000 /INCREMENTAL:NO /SUBSYSTEM:CONSOLE \
    kernel32.lib || true

# cl leaves intermediates next to the source; they are not artefacts.
rm -f ./*.obj

# wine returns odd exit codes and spews unrelated noise, so the real success
# signal is "the .EXE exists" - the same rule cc_wrap.py uses.
test -f "$out" || { echo "recomp: cl/link produced no pidrun.exe" >&2; exit 1; }
echo "recomp: built $out"
