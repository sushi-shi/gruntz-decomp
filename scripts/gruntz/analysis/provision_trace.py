#!/usr/bin/env python3
r"""gruntz.analysis.provision_trace - provision THIS checkout's dynamic this/ecx trace runtime.

Idempotent, re-runnable setup for the Frida `this`-tracer documented end-to-end in
`docs/dynamic-analysis.md`. Run from any checkout inside `nix develop .#build`:

    python -m gruntz.analysis.provision_trace            # retail GRUNTZ.EXE (default)
    python -m gruntz.analysis.provision_trace --play     # provision, then launch the game
    python -m gruntz.analysis.provision_trace --demo     # fallback: on-disk demo, no fetch

Targets the **retail** GRUNTZ.EXE (RVA-exact with all the Ghidra tooling). The retail
REZ/VRZ/fonts/DLLs are auto-fetched from the same archive.org ISO the flake pulls
GRUNTZ.EXE from; the EXE itself comes from build/exe/GRUNTZ.EXE (flake-fetched).

The game is launched WINDOWED in a SINGLE wine-generated virtual desktop with
**32-bit software GL** (Mesa llvmpipe). Both are load-bearing on this host — see
`docs/dynamic-analysis.md` "rendering" for why (32-bit GL ABI + one desktop).

Everything lands under THIS checkout's gitignored `build/` only (never src/, never
the shared Ghidra export). What it builds:

  build/game/<mode>/        game runtime cwd: the EXE + REZ/VRZ/fonts + MSS32/
                            SMACKW32 + SFMAN32.DLL (the Frida gadget) + the config +
                            gruntz_trace.js. This is the dir you `wine <EXE>` from.
  build/game/retail-assets/ the fetched retail REZ/VRZ/fonts/DLLs (kept out of the
                            runtime dir so re-provisioning never re-downloads).
  build/game/cd/            tiny <L>:\GAME\GRUNTZ.EXE tree the wine D: cdrom points at,
                            so the CD check (IsGruntzCDInAnyDrive) passes.
  build/wineprefix-game/    a dedicated wine prefix for PLAYING (separate from the
                            build prefix): virtual desktop, D:=cdrom, SoundFonts off.
  build/trace/cc_all.csv    per-function calling-convention dump (READ-ONLY Ghidra
                            cc dump; NEVER overwrites the shared functions.csv).
  build/trace/thiscall_discovery.csv  the hook set: cc in {__thiscall,unknown} minus
                            config/library_labels.csv (MFC/CRT/zlib).
  build/trace/cache/        the downloaded+decompressed frida gadget (pinned).

The Ghidra cc dump runs `dump_cc.py` under the PyGhidra driver with --no-analyze
and writes ONLY to build/trace/cc_all.csv via $GRUNTZ_CC_OUT. It does NOT run
export.py, so build/ghidra-enrich/exports/functions.csv is never touched.

After a play session, turn the trace into a class-membership worklist (these write
TRACKED source/docs, so they are intentionally NOT part of provisioning):

    python -m gruntz.analysis.this_cluster build/game/<mode>/gruntz_edges.csv \\
        --json build/trace/this_clusters.json
    python -m gruntz.analysis.tie_classes build/game/<mode>/gruntz_edges.csv \\
        --out docs/this-pointer-classes.csv      # regenerates build/trace/labels.csv too
    python -m gruntz.analysis.gen_class_stubs    # -> include/Stub/discovered.h, src/Stub/Discovered.cpp
    gruntz build
"""
import argparse
import csv
import os
import shutil
import subprocess
import sys
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
SCRIPTS = REPO / "scripts"

# --- pinned Frida gadget (windows-x86, 32-bit) -----------------------------
# GRUNTZ.EXE / GruntDem.exe are 32-bit PEs, so this is the windows-x86 build.
# Pinned by version so a fresh provision is reproducible. Bump deliberately.
FRIDA_VERSION = "17.15.3"
FRIDA_GADGET_URL = (
    f"https://github.com/frida/frida/releases/download/{FRIDA_VERSION}/"
    f"frida-gadget-{FRIDA_VERSION}-windows-x86.dll.xz"
)
FRIDA_GADGET_MD5 = "b61bd0e04c69e829a336edd07cfdd655"   # decompressed .dll

# --- retail assets (same archive.org ISO the flake pulls GRUNTZ.EXE from) ---
# Files INSIDE the ISO are served at <ISO_BASE><url-encoded path-in-iso>. The EXE
# is NOT here (it comes from the flake -> build/exe/GRUNTZ.EXE); everything else
# the runtime needs is fetched on demand.
ISO_BASE = "https://archive.org/download/gruntz-pc/Gruntz.iso/"
RETAIL_ASSETS = {
    "Gruntz.REZ":   "DATA%2FGRUNTZ.REZ",     # the EXE opens "Gruntz.REZ"
    "GRUNTZ.VRZ":   "GAME%2FGRUNTZ.VRZ",
    "MSS32.DLL":    "GAME%2FMSS32.DLL",
    "SMACKW32.DLL": "GAME%2FSMACKW32.DLL",
    "LARGE.FNT":    "GAME%2FLARGE.FNT",
    "MEDIUM.FNT":   "GAME%2FMEDIUM.FNT",
    "SMALL.FNT":    "GAME%2FSMALL.FNT",
    "TINY.FNT":     "GAME%2FTINY.FNT",
}

# The single wine virtual-desktop size. 1024x768 = the game's MAX in-game
# resolution (Options->Video offers 640x480 / 800x600 / 1024x768), so a
# 640x480->1024x768 mode change stays inside this desktop.
DESKTOP_SIZE = "1024x768"

# Paths (all under this checkout's build/).
BUILD = REPO / "build"
TRACE = BUILD / "trace"
CACHE = TRACE / "cache"
GAME = BUILD / "game"
CD = GAME / "cd"
RETAIL_ASSET_DIR = GAME / "retail-assets"
WINEPREFIX_GAME = BUILD / "wineprefix-game"
RETAIL_EXE = BUILD / "exe" / "GRUNTZ.EXE"                # stable retail copy (gruntz init/build)
GHIDRA_PROJECT_DIR = BUILD / "ghidra-named"
GHIDRA_PROJECT = "gruntz"
GHIDRA_DRIVER = SCRIPTS / "gruntz" / "ghidra" / "ghidra_metadata_apply.py"
DUMP_CC = SCRIPTS / "gruntz" / "ghidra" / "scripts" / "dump_cc.py"
LIBRARY_LABELS = REPO / "config" / "library_labels.csv"
# $GRUNTZ_DATA overrides where retail assets live/are fetched to.
RETAIL_ASSET_DIR_ENV = "GRUNTZ_DATA"


def log(m):
    print(f"[provision-trace] {m}", flush=True)


def die(m):
    print(f"[provision-trace] ERROR: {m}", file=sys.stderr)
    sys.exit(1)


def run(cmd, **kw):
    log("$ " + " ".join(str(c) for c in cmd))
    kw.setdefault("check", True)
    return subprocess.run([str(c) for c in cmd], **kw)


def _pkg_env():
    env = dict(os.environ)
    if str(SCRIPTS) not in env.get("PYTHONPATH", "").split(os.pathsep):
        env["PYTHONPATH"] = os.pathsep.join(
            p for p in (str(SCRIPTS), env.get("PYTHONPATH", "")) if p)
    return env


def _demo_dir():
    """Locate the demo game dir (GruntDem.exe + REZ/VRZ) for the --demo fallback. It
    sits beside the MAIN checkout, so resolve from the git common-dir's parent (the
    main repo root) when REPO is a worktree. Override with $GRUNTZ_DEMO_DIR."""
    env = os.environ.get("GRUNTZ_DEMO_DIR")
    cands = [Path(env)] if env else []
    try:
        common = Path(subprocess.check_output(
            ["git", "-C", str(REPO), "rev-parse", "--git-common-dir"],
            text=True).strip())
        if not common.is_absolute():
            common = (REPO / common).resolve()
        cands.append(common.parent.parent / "runtime")   # <main>/../runtime
    except Exception:
        pass
    cands.append(REPO.parent / "runtime")                 # fallback: beside this checkout
    for c in cands:
        if c.is_dir() and (c / "GruntDem.exe").exists():
            return c
    return cands[-1]


# --- 1. Frida gadget -------------------------------------------------------
def fetch_gadget():
    """Download + decompress the pinned frida gadget into build/trace/cache. Idempotent."""
    import hashlib
    CACHE.mkdir(parents=True, exist_ok=True)
    dll = CACHE / "frida-gadget.dll"
    if dll.exists() and hashlib.md5(dll.read_bytes()).hexdigest() == FRIDA_GADGET_MD5:
        log(f"frida gadget present + verified ({dll}).")
        return dll
    xz = CACHE / "frida-gadget.dll.xz"
    log(f"fetching frida gadget {FRIDA_VERSION} (windows-x86) ...")
    run(["curl", "-fsSL", "--retry", "3", "-o", str(xz), FRIDA_GADGET_URL])
    run(["xz", "-dkf", str(xz)])   # keep the .xz, write the .dll alongside
    got = hashlib.md5(dll.read_bytes()).hexdigest()
    if got != FRIDA_GADGET_MD5:
        die(f"frida gadget md5 mismatch: got {got}, expected {FRIDA_GADGET_MD5}. "
            "The pinned release may have moved; verify the URL + update FRIDA_GADGET_MD5.")
    log(f"frida gadget ready ({dll}, md5 {got}).")
    return dll


# --- 2. retail assets ------------------------------------------------------
def fetch_retail_assets(asset_dir):
    """Fetch the retail REZ/VRZ/fonts/DLLs from the archive.org Gruntz ISO into
    asset_dir (idempotent; skips files already present & non-empty). The retail
    GRUNTZ.EXE itself is NOT fetched here - it comes from build/exe/GRUNTZ.EXE."""
    asset_dir.mkdir(parents=True, exist_ok=True)
    for name, iso_path in RETAIL_ASSETS.items():
        dst = asset_dir / name
        if dst.exists() and dst.stat().st_size > 0:
            continue
        url = ISO_BASE + iso_path
        log(f"fetching retail asset {name} ...")
        run(["curl", "-fSL", "--retry", "3", "-o", str(dst), url])
        if not dst.exists() or dst.stat().st_size == 0:
            die(f"failed to fetch {name} from {url} (archive.org down or path moved).")
    log(f"retail assets ready in {asset_dir}.")
    return asset_dir


# --- 3. Ghidra cc dump (READ-ONLY) -----------------------------------------
def dump_cc():
    """READ-ONLY per-function calling-convention dump -> build/trace/cc_all.csv.

    Runs dump_cc.py as the ONLY GhidraScript under the PyGhidra driver with
    --no-analyze (re-opens the already-analyzed build/ghidra-named DB without
    re-analysis) and routes its output to build/trace/cc_all.csv via $GRUNTZ_CC_OUT.
    export.py is NOT run, so the shared functions.csv is never rewritten. Idempotent
    (skips if cc_all.csv already covers the DB)."""
    TRACE.mkdir(parents=True, exist_ok=True)
    cc_all = TRACE / "cc_all.csv"
    if cc_all.exists() and sum(1 for _ in cc_all.open()) > 1000:
        log(f"cc dump present ({cc_all}); skipping (delete it to redo).")
        return cc_all
    if not GHIDRA_PROJECT_DIR.exists() or not any(GHIDRA_PROJECT_DIR.glob("*.rep")):
        die(f"no Ghidra project at {GHIDRA_PROJECT_DIR} - run `gruntz init` in this "
            "checkout first (the cc dump needs the analyzed DB).")
    if not RETAIL_EXE.exists():
        die(f"no {RETAIL_EXE} - run `gruntz init` first.")
    env = _pkg_env()
    env["GRUNTZ_CC_OUT"] = str(cc_all)
    log("cc dump: dump_cc.py under PyGhidra (--no-analyze, READ-ONLY) ...")
    run([sys.executable, str(GHIDRA_DRIVER), str(RETAIL_EXE),
         str(GHIDRA_PROJECT_DIR), GHIDRA_PROJECT, str(DUMP_CC), "--no-analyze"],
        env=env)
    if not cc_all.exists():
        die(f"cc dump produced no {cc_all} - check the PyGhidra output above.")
    log(f"cc dump -> {cc_all} ({sum(1 for _ in cc_all.open()) - 1} functions).")
    return cc_all


# --- 4. the hook set -------------------------------------------------------
def _rint(s):
    s = str(s).strip().strip('"')
    return int(s, 16) if s.lower().startswith("0x") else int(s)


def build_hookset(cc_all):
    """thiscall_discovery.csv = cc in {__thiscall,unknown} minus library_labels.csv.

    The named __thiscall functions are class anchors; the cc=unknown FUN_ bodies are
    the discovery targets. MFC/CRT/zlib RVAs (config/library_labels.csv) are dropped
    so we only hook engine code. gen_frida_script then drops jmp/padding starts + the
    MFC window/dialog band, leaving the real engine bodies."""
    lib_rvas = set()
    if LIBRARY_LABELS.exists():
        with LIBRARY_LABELS.open() as f:
            for r in csv.DictReader(f):
                try:
                    lib_rvas.add(_rint(r["rva"]))
                except (KeyError, ValueError):
                    pass
    out = TRACE / "thiscall_discovery.csv"
    kept = 0
    with cc_all.open() as fi, out.open("w", newline="") as fo:
        rd = csv.DictReader(fi)
        w = csv.writer(fo)
        w.writerow(["rva", "name"])
        for r in rd:
            try:
                rva = _rint(r["entry_rva"])
            except (KeyError, ValueError):
                continue
            cc = (r.get("cc") or "").strip()
            if cc not in ("__thiscall", "unknown"):
                continue
            if rva in lib_rvas:
                continue
            w.writerow(["0x%x" % rva, r.get("name", "")])
            kept += 1
    log(f"hook set -> {out} ({kept} fns; cc in thiscall/unknown minus "
        f"{len(lib_rvas)} library rvas).")
    return out


# --- 5. game runtime dir ---------------------------------------------------
def _link_or_copy(src, dst):
    """Symlink src->dst (cheap, leaves big assets in place); copy if symlink fails."""
    src, dst = Path(src), Path(dst)
    if dst.exists() or dst.is_symlink():
        dst.unlink()
    try:
        dst.symlink_to(src.resolve())
    except OSError:
        shutil.copyfile(src, dst)


def provision_game(mode, gadget, asset_dir):
    """Lay out build/game/<mode>/: EXE + REZ/VRZ/fonts + DLLs + SFMAN32 gadget + config."""
    gdir = GAME / mode
    gdir.mkdir(parents=True, exist_ok=True)

    if mode == "retail":
        if not RETAIL_EXE.exists():
            die(f"no {RETAIL_EXE} - run `gruntz init` first.")
        assets = ["Gruntz.REZ", "GRUNTZ.VRZ", "MSS32.DLL", "SMACKW32.DLL",
                  "LARGE.FNT", "MEDIUM.FNT", "SMALL.FNT", "TINY.FNT"]
        for a in assets:
            src = asset_dir / a
            if not src.exists():
                die(f"retail asset missing: {src} (fetch step should have produced it).")
            _link_or_copy(src, gdir / a)
        _link_or_copy(RETAIL_EXE, gdir / "GRUNTZ.EXE")
        game_exe = "GRUNTZ.EXE"
    else:  # demo fallback
        demo_dir = _demo_dir()
        if not (demo_dir.is_dir() and (demo_dir / "GruntDem.exe").exists()):
            die(f"demo assets not found at {demo_dir} (expected GruntDem.exe + "
                "GRUNTDEM.REZ/VRZ + MSS32/SMACKW32 + fonts). Set $GRUNTZ_DEMO_DIR.")
        assets = ["GRUNTDEM.REZ", "GRUNTDEM.VRZ", "MSS32.DLL", "SMACKW32.DLL",
                  "large.fnt", "medium.fnt", "small.fnt", "tiny.fnt"]
        for a in assets:
            src = demo_dir / a
            if not src.exists():
                die(f"demo asset missing: {src}")
            _link_or_copy(src, gdir / a)
        _link_or_copy(demo_dir / "GruntDem.exe", gdir / "GruntDem.exe")
        game_exe = "GruntDem.exe"

    # The Frida gadget masquerades as SFMAN32.DLL: the game LoadLibrary's it (the
    # optional SoundFont DLL) and its DllMain starts Frida in-process. The gadget
    # reads its config from "<dll-without-ext>.config" NEXT TO THE RESOLVED DLL, so
    # the gadget must be a REAL copy in the game dir (a symlink resolves the config
    # search to the link target's dir and the auto-run never fires). The script path
    # is ABSOLUTE so it resolves regardless of cwd. gruntz_trace.js is written later.
    import json as _json
    gadget_dst = gdir / "SFMAN32.DLL"
    if gadget_dst.exists() or gadget_dst.is_symlink():
        gadget_dst.unlink()
    shutil.copyfile(gadget, gadget_dst)
    script_abs = str((gdir / "gruntz_trace.js").resolve())
    (gdir / "SFMAN32.config").write_text(_json.dumps({
        "interaction": {"type": "script", "path": script_abs, "on_change": "ignore"}
    }, indent=2) + "\n")
    log(f"game runtime -> {gdir} (mode={mode}, exe={game_exe}; gadget copied, "
        f"script={script_abs}).")
    return gdir, game_exe


def provision_cd(mode, game_exe):
    """Tiny <L>:\\GAME\\GRUNTZ.EXE tree for the CD check (no volume-label check).

    IsGruntzCDInAnyDrive scans for a DRIVE_CDROM drive holding GAME\\GRUNTZ.EXE. We
    point the wine D: cdrom at build/game/cd, which has GAME/GRUNTZ.EXE (the exe is
    enough; a real REZ isn't checked)."""
    game = CD / "GAME"
    game.mkdir(parents=True, exist_ok=True)
    exe_src = GAME / mode / game_exe
    _link_or_copy(exe_src, game / "GRUNTZ.EXE")
    log(f"cd tree -> {CD} (D:\\GAME\\GRUNTZ.EXE).")
    return CD


# --- 6. wine prefix for playing --------------------------------------------
def _swrast_dri_dir():
    """Locate a **32-bit** Mesa swrast_dri.so so we can force software GL for the
    32-BIT game process. GRUNTZ.EXE / GruntDem.exe are 32-bit PEs -> wine runs them
    32-bit -> they load 32-bit Linux GL drivers. On NixOS those live in
    /run/opengl-driver-32/lib/dri; pointing LIBGL_DRIVERS_PATH at the 64-bit tree
    (/run/opengl-driver/lib/dri) is the WRONG ABI -> the 32-bit loader can't load
    it, falls back to the (broken) hardware driver, and the game shows a blue
    screen. THE measured fix. Override the dir with $GRUNTZ_DRI_DIR."""
    env = os.environ.get("GRUNTZ_DRI_DIR")
    if env and (Path(env) / "swrast_dri.so").exists():
        return env
    import glob
    # 32-bit first (the game is a 32-bit PE); 64-bit/nix-store only as last resort.
    cands = ["/run/opengl-driver-32/lib/dri/swrast_dri.so"]
    cands += glob.glob("/usr/lib/i386-linux-gnu/dri/swrast_dri.so")
    cands += glob.glob("/run/opengl-driver/lib/dri/swrast_dri.so")
    cands += glob.glob("/nix/store/*/lib/dri/swrast_dri.so")
    for cand in cands:
        if Path(cand).exists():
            return str(Path(cand).parent)
    return None


def _wine_env(prefix):
    """Wine env for PLAYING: 32-bit software GL (Mesa llvmpipe) + $DISPLAY.

    Software rendering is the host fix: wine's DirectDraw goes through OpenGL, and
    forcing Mesa llvmpipe (with the 32-bit swrast driver, see _swrast_dri_dir) keeps
    the game off any flaky hardware GL path. The 640x480 game renders fine on the
    CPU. Paired with the single virtual desktop (provision_wineprefix) the game
    reaches its menu in one window."""
    env = dict(os.environ)
    env["WINEPREFIX"] = str(prefix)
    env.setdefault("WINEDEBUG", "-all")
    env["DISPLAY"] = os.environ.get("DISPLAY", ":0")
    # Force Mesa software GL (llvmpipe) via glvnd.
    env["LIBGL_ALWAYS_SOFTWARE"] = "1"
    env["GALLIUM_DRIVER"] = "llvmpipe"
    env["MESA_LOADER_DRIVER_OVERRIDE"] = "llvmpipe"
    env["__GLX_VENDOR_LIBRARY_NAME"] = "mesa"
    env.setdefault("EGL_LOG_LEVEL", "fatal")
    dri = _swrast_dri_dir()
    if dri:
        env["LIBGL_DRIVERS_PATH"] = dri
    else:
        log("WARNING: no 32-bit swrast_dri.so found - software GL may be unavailable; "
            "set $GRUNTZ_DRI_DIR to a dir containing a 32-bit swrast_dri.so.")
    return env


def _reg_add(env, key, value, data, vtype="REG_SZ"):
    run(["wine", "reg", "add", key, "/v", value, "/t", vtype, "/d", data, "/f"],
        env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def provision_wineprefix(cd_dir):
    """Dedicated PLAY prefix: single virtual desktop, D:=cdrom -> cd_dir, SoundFonts off.

    Separate from the BUILD prefix (build/wineprefix) so cc-compile and play never
    fight over one wineserver."""
    env = _wine_env(WINEPREFIX_GAME)
    fresh = not (WINEPREFIX_GAME / "drive_c").is_dir()
    if fresh:
        WINEPREFIX_GAME.mkdir(parents=True, exist_ok=True)
        log(f"initialising play wine prefix at {WINEPREFIX_GAME} ...")
        run(["wineboot", "--init"], env=env,
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        run(["wineserver", "--wait"], env=env, check=False)
    else:
        log(f"play wine prefix present ({WINEPREFIX_GAME}).")

    # ONE wine-generated virtual desktop. The game is launched DIRECTLY (`wine
    # <exe>`, see play()), so it renders into this single desktop window. wine can't
    # change the real display mode under Wayland, so this registry desktop catches
    # the game's mode change. Sized DESKTOP_SIZE (= the game's max in-game res) so a
    # 640x480 -> 1024x768 change stays inside it.
    _reg_add(env, r"HKCU\Software\Wine\Explorer", "Desktop", "Default")
    _reg_add(env, r"HKCU\Software\Wine\Explorer\Desktops", "Default", DESKTOP_SIZE)

    # D: as a cdrom drive pointing at the cd tree -> the CD check passes.
    cd_win = subprocess.check_output(
        ["winepath", "-w", str(cd_dir)], env=env, text=True).strip()
    _reg_add(env, r"HKLM\Software\Wine\Drives", "d:", "cdrom")
    dosdev = WINEPREFIX_GAME / "dosdevices"
    dosdev.mkdir(parents=True, exist_ok=True)
    for nm in ("d:", "d::"):
        link = dosdev / nm
        if link.exists() or link.is_symlink():
            link.unlink()
    (dosdev / "d:").symlink_to(cd_dir.resolve())                 # drive root
    if Path("/dev/sr0").exists():
        (dosdev / "d::").symlink_to("/dev/sr0")                  # device node (best-effort)

    # SoundFonts off: the game still LoadLibrary's SFMAN32 (so the gadget runs) but
    # never resolves the (NULL) SFManager export from it.
    for sub in (r"HKLM\Software\Monolith Productions\Gruntz\1.0",
                r"HKLM\Software\Monolith Productions\Gruntz Demo\1.0",
                r"HKCU\Software\Monolith Productions\Gruntz\1.0",
                r"HKCU\Software\Monolith Productions\Gruntz Demo\1.0"):
        _reg_add(env, sub, "Disable SoundFonts", "1")
    run(["wineserver", "--wait"], env=env, check=False)
    log(f"play wine prefix configured (single virtual desktop {DESKTOP_SIZE}, "
        f"D:=cdrom, SoundFonts disabled).")
    return env


# --- 7. trace script -------------------------------------------------------
def gen_trace_script(hookset, gdir, game_exe):
    """Emit gruntz_trace.js into the game dir via gen_frida_script (edges -> cwd).

    Pass the REAL game EXE so gen_frida_script filters thunks/padding against the
    binary actually traced. _free is patched to `ret` (gen_frida_script default) so
    every ecx is a globally-unique object - the game's footprint is small, so the
    leak is harmless and clustering is cleanest."""
    out_js = gdir / "gruntz_trace.js"
    run([sys.executable, "-m", "gruntz.analysis.gen_frida_script",
         str(hookset), str(out_js), "gruntz_edges.csv", str(gdir / game_exe)],
        env=_pkg_env())
    log(f"trace script -> {out_js}")
    return out_js


# --- 8. play ---------------------------------------------------------------
def play(env, gdir, game_exe, timeout=None):
    """Launch the game DIRECTLY (`wine <exe>`) under the play prefix, Frida live.

    The game renders into the SINGLE wine virtual desktop set by
    provision_wineprefix. (Launching via `explorer /desktop=` would stack a second
    desktop window on top of the registry one.)"""
    log(f"launching WINDOWED + 32-bit software GL: (cd {gdir}) wine {game_exe}")
    if not env.get("DISPLAY"):
        log("note: no $DISPLAY - the game needs an X/Wayland display to render.")
    try:
        run(["wine", game_exe], env=env, cwd=str(gdir), timeout=timeout, check=False)
    except subprocess.TimeoutExpired:
        log(f"reached the {timeout}s play timeout - killing wine session.")
        run(["wineserver", "-k"], env=env, check=False)


# --- main ------------------------------------------------------------------
def main():
    ap = argparse.ArgumentParser(
        prog="gruntz.analysis.provision_trace",
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--demo", action="store_true",
                    help="fallback: use the on-disk Gruntz DEMO (no fetch) instead of retail.")
    ap.add_argument("--play", action="store_true",
                    help="after provisioning, launch the game (Frida traces live).")
    ap.add_argument("--play-timeout", type=int, default=None,
                    help="kill the game after N seconds (for non-interactive smoke).")
    ap.add_argument("--skip-cc", action="store_true",
                    help="skip the Ghidra cc dump (reuse build/trace/cc_all.csv).")
    args = ap.parse_args()
    mode = "demo" if args.demo else "retail"

    if not os.environ.get("MSVC_DIR") and not shutil.which("wine"):
        die("wine not found - run inside `nix develop .#build`.")

    log(f"provisioning the {mode} trace runtime under {BUILD} ...")
    gadget = fetch_gadget()
    if mode == "retail":
        asset_dir = Path(os.environ.get(RETAIL_ASSET_DIR_ENV, RETAIL_ASSET_DIR))
        fetch_retail_assets(asset_dir)
    else:
        asset_dir = None
    cc_all = (TRACE / "cc_all.csv") if args.skip_cc else dump_cc()
    if not cc_all.exists():
        die(f"no {cc_all} - drop --skip-cc to generate it.")
    hookset = build_hookset(cc_all)
    gdir, game_exe = provision_game(mode, gadget, asset_dir)
    cd_dir = provision_cd(mode, game_exe)
    env = provision_wineprefix(cd_dir)
    gen_trace_script(hookset, gdir, game_exe)

    dri = _swrast_dri_dir() or "<dir with a 32-bit swrast_dri.so>"
    log("=" * 64)
    log("provisioned. To play WINDOWED + 32-bit software GL (Frida traces live):")
    log(f"  export WINEPREFIX={WINEPREFIX_GAME} DISPLAY=:0 WINEDEBUG=-all")
    log(f"  export LIBGL_ALWAYS_SOFTWARE=1 GALLIUM_DRIVER=llvmpipe \\")
    log(f"         MESA_LOADER_DRIVER_OVERRIDE=llvmpipe __GLX_VENDOR_LIBRARY_NAME=mesa \\")
    log(f"         LIBGL_DRIVERS_PATH={dri}")
    log(f"  cd {gdir} && wine {game_exe}")
    log("  (or just: python -m gruntz.analysis.provision_trace --play)")
    log("Then: this_cluster -> tie_classes -> gen_class_stubs -> gruntz build")
    log("=" * 64)

    if args.play:
        play(env, gdir, game_exe, timeout=args.play_timeout)


if __name__ == "__main__":
    main()
