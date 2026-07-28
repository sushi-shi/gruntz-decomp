#!/usr/bin/env bash
# Build the record/replay pair with the period toolchain under wine.
#
#     recomp/replay/build.sh capture     -> SFMAN32.DLL (the recorder)
#     recomp/replay/build.sh replay      -> replay.exe  (the replayer)
#
# replay.exe links NO decompiled objects, and that is the point. It used to link
# build/objdiff/base/<unit>.obj, which resolved every global the function under
# test touches to replay.exe's own copy rather than the game's - so only bodies
# with zero relocations could be run at all. OUR code now arrives at RUN time as
# a pre-relocated blob from objbind.py, bound to `game_base + rva`. Linking
# nothing means there is no second copy of our code anywhere in the process, so
# "did it really run the module?" is not a question that can be got wrong.
#
# Two non-obvious things, both inherited from recomp/harness/build.sh:
#   * MSDIS100.DLL is missing from the packaged VC5 toolchain and link.exe
#     imports it, so we reuse scripts/gruntz/build/msdis_stub.py.
#   * /BASE - both binaries must live clear of the addresses they restore.
#     The DLL goes to 0x50000000 and replay.exe to 0x30000000; the recorded
#     regions of a wine 32-bit process sit far below that (see census.txt), and
#     the object module is baked at 0x58000000, between them and the arena.
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo="$(cd "$here/../.." && pwd)"
: "${MSVC_DIR:?run inside 'nix develop'}"
: "${WINEPREFIX:?run inside 'nix develop'}"

name="${1:?usage: build.sh capture|replay}"
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
    out="$here/replay.exe"
    rm -f "$out"
    wine "$MSVC_DIR/bin/cl.exe" /nologo /O2 /MT /W3 \
        "/Fe$(winepath -w "$out")" "$(winepath -w "$here/replay.cpp")" \
        /link /BASE:0x30000000 /INCREMENTAL:NO /SUBSYSTEM:CONSOLE \
        kernel32.lib || true
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
echo "replay: built $out"
