#!/usr/bin/env bash
# Build one recomp harness with the period toolchain under wine.
#
#     recomp/harness/build.sh <name> [unit ...]
#
#   <name>   the harness source stem: <name>.c or <name>.cpp in this directory.
#   [unit]   zero or more decompiled units whose COMPILED object is linked in,
#            e.g. `shadetablecache` -> build/objdiff/base/shadetablecache.obj.
#            That is what lets a harness call OUR bytes rather than a
#            transcription of our source; see recomp.h.
#
# Examples:
#     recomp/harness/build.sh pidrun
#     recomp/harness/build.sh colorrun shadetablecache
#
# Three things here are not obvious:
#
#   * MSDIS100.DLL is missing from the packaged VC5 toolchain, and link.exe
#     imports it, so it will not even load under wine. The repo already solves
#     this for `ninja candidate`; we reuse the same helper
#     (scripts/gruntz/build/msdis_stub.py) rather than re-inventing it.
#   * /BASE:0x10000000 - a harness maps retail GRUNTZ.EXE at its preferred base
#     0x00400000 and calls straight into it, so it must not live there itself.
#   * /FORCE:UNRESOLVED - a decompiled unit's object carries every external its
#     TU references (typically ~30: the CRT, MFC, sibling TUs). We link none of
#     that. The reachability audit is exactly the promise that the function
#     under test never reaches one; if it does the harness faults immediately
#     and loudly, which is the right outcome rather than a silent wrong answer.
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo="$(cd "$here/../.." && pwd)"
: "${MSVC_DIR:?run inside 'nix develop'}"
: "${WINEPREFIX:?run inside 'nix develop'}"

name="${1:?usage: build.sh <name> [unit ...]}"
shift || true

src="$here/$name.c"
[ -f "$src" ] || src="$here/$name.cpp"
[ -f "$src" ] || { echo "build.sh: no $name.c or $name.cpp in $here" >&2; exit 1; }

objs=()
for unit in "$@"; do
    obj="$repo/build/objdiff/base/$unit.obj"
    [ -f "$obj" ] || { echo "build.sh: $obj missing - run 'gruntz build' first" >&2; exit 1; }
    objs+=("$(winepath -w "$obj")")
done

export PYTHONPATH="$repo/scripts${PYTHONPATH:+:$PYTHONPATH}"
export WINEDEBUG="${WINEDEBUG:-fixme-all,err-all}"

python3 - "$WINEPREFIX" "$MSVC_DIR" <<'PY'
import sys
from gruntz.build.msdis_stub import ensure_msdis
ensure_msdis(sys.argv[1], sys.argv[2], verbose=False)
PY

cd "$here"
out="$here/$name.exe"
rm -f "$out" ./*.obj

# /O2 /MT matches how the decompiled units are compiled, so a linked-in object
# meets the CRT model it was built against.
wine "$MSVC_DIR/bin/cl.exe" /nologo /O2 /MT /W3 \
    "/Fe$(winepath -w "$out")" "$(winepath -w "$src")" \
    /link /BASE:0x10000000 /INCREMENTAL:NO /SUBSYSTEM:CONSOLE \
    /FORCE:UNRESOLVED kernel32.lib ${objs[@]+"${objs[@]}"} || true

# cl leaves intermediates next to the source; they are not artefacts.
rm -f ./*.obj

# wine returns odd exit codes and spews unrelated noise, so the real success
# signal is "the .EXE exists" - the same rule cc_wrap.py uses.
test -f "$out" || { echo "build.sh: cl/link produced no $name.exe" >&2; exit 1; }
echo "recomp: built $out${*:+  (linked: $*)}"
