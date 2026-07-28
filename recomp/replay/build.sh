#!/usr/bin/env bash
# Build the record/replay pair with the period toolchain under wine.
#
#     recomp/replay/build.sh capture              -> SFMAN32.DLL (the recorder)
#     recomp/replay/build.sh replay [unit ...]    -> replay.exe   (the replayer)
#
# [unit] links OUR compiled object out of build/objdiff/base/<unit>.obj, exactly
# as recomp/harness/build.sh does - that is what makes the replay call the bytes
# we ship rather than a transcription of our source.
#
# Three non-obvious things, all inherited from recomp/harness/build.sh:
#   * MSDIS100.DLL is missing from the packaged VC5 toolchain and link.exe
#     imports it, so we reuse scripts/gruntz/build/msdis_stub.py.
#   * /BASE - both binaries must live clear of the addresses they restore.
#     The DLL goes to 0x50000000 and replay.exe to 0x30000000; the recorded
#     regions of a wine 32-bit process sit far below that (see census.txt).
#   * /NODEFAULTLIB:nafxcw.lib - an MFC TU's object carries a default-lib
#     directive for nafxcw, whose operator new/delete then collide with
#     LIBCMT's. We never call MFC, so the library is simply not linked.
#   * /FORCE:UNRESOLVED - a decompiled unit's object carries every external its
#     TU references. `gruntz.audit.iat_tiers` says whether the function under
#     test can actually reach one; if it does, the replay faults loudly.
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo="$(cd "$here/../.." && pwd)"
: "${MSVC_DIR:?run inside 'nix develop'}"
: "${WINEPREFIX:?run inside 'nix develop'}"

name="${1:?usage: build.sh capture|replay [unit ...]}"
shift || true

export PYTHONPATH="$repo/scripts${PYTHONPATH:+:$PYTHONPATH}"
export WINEDEBUG="${WINEDEBUG:-fixme-all,err-all}"

python3 - "$WINEPREFIX" "$MSVC_DIR" <<'PY'
import sys
from gruntz.build.msdis_stub import ensure_msdis
ensure_msdis(sys.argv[1], sys.argv[2], verbose=False)
PY

cd "$here"
rm -f ./*.obj

case "$name" in
capture)
    out="$here/SFMAN32.DLL"
    rm -f "$out"
    wine "$MSVC_DIR/bin/cl.exe" /nologo /O2 /MT /W3 /LD \
        "/Fe$(winepath -w "$out")" "$(winepath -w "$here/capture.c")" \
        /link /BASE:0x50000000 /INCREMENTAL:NO \
        kernel32.lib user32.lib || true
    ;;
replay)
    objs=()
    for unit in "$@"; do
        obj="$repo/build/objdiff/base/$unit.obj"
        [ -f "$obj" ] || { echo "build.sh: $obj missing - run 'gruntz build' first" >&2; exit 1; }
        objs+=("$(winepath -w "$obj")")
    done
    out="$here/replay.exe"
    rm -f "$out"
    wine "$MSVC_DIR/bin/cl.exe" /nologo /O2 /MT /W3 \
        "/I$(winepath -w "$repo/include")" \
        "/Fe$(winepath -w "$out")" "$(winepath -w "$here/replay.cpp")" \
        /link /BASE:0x30000000 /INCREMENTAL:NO /SUBSYSTEM:CONSOLE \
        /FORCE:UNRESOLVED /FORCE:MULTIPLE /NODEFAULTLIB:nafxcw.lib \
        kernel32.lib ${objs[@]+"${objs[@]}"} || true
    ;;
*)
    echo "build.sh: unknown target '$name' (capture|replay)" >&2
    exit 1
    ;;
esac

rm -f ./*.obj ./*.exp ./*.lib
# wine returns odd exit codes and spews unrelated noise, so the real success
# signal is "the artefact exists" - the same rule cc_wrap.py uses.
test -f "$out" || { echo "build.sh: cl/link produced no $out" >&2; exit 1; }
echo "replay: built $out${*:+  (linked: $*)}"
