# Dynamic `this`/ecx tracing → class-membership recovery

Run the retail game under wine, hook every engine `__thiscall` entry, and record the
`this` pointer (ecx) on each call. Functions called on the **same object** are
methods of the **same class** — so this turns a play session into a worklist that
assigns unknown `FUN_` bodies to known classes, feeding the matching campaign.

Key trick: `_free` is patched to `ret` (leak everything). With no deallocation an
address is never reused, so **every distinct ecx is a globally-unique object** for
the whole run and clustering reduces to "group functions by shared ecx".

## Why Frida (not gdb)

gdb works (`winedbg --gdb`) and reads ecx fine, but its all-stop debugging **freezes
the windowed DirectDraw present** (game runs but screen stays black), and attaching
to the live wow64 process crashes it. **Frida-gadget** hooks inline, in-process — the
game runs near-native and **keeps rendering**, so you play normally while it captures.
The gdb driver is kept at `scripts/gruntz/analysis/gdb_trace_this.py` as a fallback.

## Pieces

- **Game runtime** `build/game/retail/` (gitignored): EN `GRUNTZ.EXE` (RVA-aligned with
  all tooling) + EN `GRUNTZ.REZ`/`VRZ`/fonts + `MSS32`/`SMACKW32` DLLs. REZ/VRZ/fonts
  are fetched from the same archive.org ISO the flake pulls `GRUNTZ.EXE` from
  (`gruntz-pc/Gruntz.iso`, paths `DATA%2FGRUNTZ.REZ`, `GAME%2F...`).
- **Frida injection** = the gadget DLL placed as **`SFMAN32.DLL`**. The game
  `LoadLibrary`s SFMAN32 (the optional SoundFont DLL), so the gadget's `DllMain` starts
  Frida in-process with zero patching. `SFMAN32.config` (interaction `script`) auto-runs
  `gruntz_trace.js`. Companion precaution: set registry `Disable SoundFonts=1` (see
  gotchas) so the game never resolves the (missing) `SFManager` export from the gadget.
- **The hook set** `build/trace/thiscall_discovery.csv`: Ghidra calling-convention dump
  (`ghidra/scripts/dump_cc.py` → `build/trace/cc_all.csv`) filtered to `cc ∈
  {__thiscall, unknown}` and **minus MFC/CRT/zlib** (`config/library_labels.csv`) =
  ~7.3k engine functions (named thiscall anchors + unknown `FUN_` discovery targets).
  `gen_frida_script` then drops Frida-unsafe targets — `jmp`/padding starts and the MFC
  window/dialog band `0x1b9000–0x1c2000` (see gotchas) — leaving ~3.1k real engine bodies.
- **Trace script** `gruntz_trace.js`, emitted by `gruntz.analysis.gen_frida_script`:
  patches `_free`→`ret`; attaches a retiring hook at each `base(0x400000)+rva`; onEnter
  reads ecx, dedups on `(rva,ecx)`, streams new `(rva,ecx,seq)` edges to
  `gruntz_edges.csv`. A hook **retires** after K=24 distinct objects or MAX=64 hits, so
  hot functions drop out fast. Attach is **chunked across event-loop ticks** so the
  game is playable from t=0 while coverage ramps (committing ~7k hooks at once stalls
  the game). Tune via `GRUNTZ_TRACE_K` / `GRUNTZ_TRACE_MAX`.
- **Clustering** `gruntz.analysis.this_cluster`: label-propagation over the
  functions↔objects graph — known methods classify objects, objects classify unknown
  functions. Emits the `FUN_ → candidate class` worklist (support + purity).

## rendering (measured — THE blue-screen / two-window fixes)

Two things are load-bearing to get the game to a playable menu in **one** window.
`provision_trace` bakes both in; this is the why.

- **One wine virtual desktop, launched DIRECTLY.** wine cannot change the real display
  mode under Wayland, so fullscreen-exclusive fails (`NtUserChangeDisplaySettings -2`).
  Use ONE wine **registry** virtual desktop (`HKCU\Software\Wine\Explorer`
  `Desktop=Default`, `Desktops\Default=1024x768`) and launch the EXE **directly**
  (`wine GRUNTZ.EXE`). Do **NOT** also use `explorer /desktop=` — that stacks a SECOND
  desktop window ("Wine Desktop" + "gruntz - Wine Desktop", the *two-windows* symptom)
  and the game renders into one while you watch the other → looks like a blue screen.
- **32-bit software GL (Mesa llvmpipe).** wine's DirectDraw goes through OpenGL. The
  game is a **32-bit** PE, so wine runs it 32-bit and it loads **32-bit** Linux GL
  drivers. Force Mesa llvmpipe via glvnd and point `LIBGL_DRIVERS_PATH` at the **32-bit**
  dri tree — on NixOS `/run/opengl-driver-32/lib/dri`. Pointing it at the **64-bit** tree
  (`/run/opengl-driver/lib/dri`) is the wrong ABI: the 32-bit loader can't load it,
  falls back to the (broken) hardware driver, and the game shows a **blue screen**. Env:
  `LIBGL_ALWAYS_SOFTWARE=1 GALLIUM_DRIVER=llvmpipe MESA_LOADER_DRIVER_OVERRIDE=llvmpipe
  __GLX_VENDOR_LIBRARY_NAME=mesa LIBGL_DRIVERS_PATH=/run/opengl-driver-32/lib/dri`.
  (The `winevulkan` "libnvidia-glvkspirv … unable to open" line at startup is harmless
  vulkan-probe noise; this DirectDraw game never touches vulkan.)
- **Resolution**: starts at 640x480 (centered in the 1024x768 desktop). 800x600 and
  1024x768 are selectable in Options→Video and stay *inside* the 1024x768 desktop, so
  the in-game mode change holds (it doesn't touch the real display mode).

## wine / host gotchas (measured)

- **CD check**: `IsGruntzCDInAnyDrive`/`GetGruntzDriveLetter` need a `DRIVE_CDROM`-type
  drive holding `<L>:\GAME\GRUNTZ.EXE` (no volume-label check). A wine drive of type
  `cdrom` (`HKLM\Software\Wine\Drives` `"d:"="cdrom"`) pointing at a tiny `build/game/cd`
  tree (symlinks to the exe + DATA/REZ) satisfies it.
- **SFMAN32 / SFManager**: the gadget is `SFMAN32.DLL`, so `LoadLibrary` succeeds and the
  game could resolve the missing `SFManager` export (NULL). As a precaution set
  `HKLM\Software\Monolith Productions\Gruntz\1.0` `"Disable SoundFonts"="1"` so the game
  skips `SFManager` (it still `LoadLibrary`s SFMAN32, so the gadget runs).
- **Hooking MFC window/dialog funcs corrupts vtables:** hooking certain functions in the
  MFC band `0x1b9000–0x1c2000` makes Frida's inline trampoline corrupt a stack object's
  vtable → a NULL virtual call in `HandleSetFont` (WM_SETFONT) when the save/checkpoint
  dialog opens. `gen_frida_script` excludes this band (`_SKIP_RANGES`) plus `jmp`/padding
  starts; it's MFC, not engine, so no coverage is lost. Diagnose any residual fault via the
  script's `Process.setExceptionHandler` dump in `frida_crash.log` (faulting pc + stack
  return addrs as GRUNTZ RVAs).

## Run it

```sh
cd <repo>
nix develop .#build   # provides wine + python + the gruntz CLI

# (one-time, in build/wineprefix-game) registry: virtual desktop, D: cdrom,
#   "Disable SoundFonts"=1  (see gotchas) -- persisted in the prefix.

# (one-time) build the hook set + script:
python -m gruntz.analysis.gen_frida_script build/trace/thiscall_discovery.csv \
    build/game/retail/gruntz_trace.js

# play (Frida is in-process via the gadget; edges stream live):
export WINEPREFIX="$PWD/build/wineprefix-game" __GLX_VENDOR_LIBRARY_NAME=nvidia \
       EGL_LOG_LEVEL=fatal WINEDEBUG=-all
cd build/game/retail && wine GRUNTZ.EXE
# ...play a level, drive gruntz/AI/combat... then quit

# turn the trace into a class-membership worklist:
python -m gruntz.analysis.this_cluster build/game/retail/gruntz_edges.csv \
    --json build/trace/this_clusters.json

# tie functions to classes, hierarchy-aware: a method seen across sibling
# subclasses is assigned to their shared BASE (not a derived class - we can't tell
# own-vs-inherited from ecx); new groups -> ClassUnknown_n. Grounded in the
# reconstructed class hierarchy incl. PR #52's RTTI parents. -> the committed table.
python -m gruntz.analysis.tie_classes build/game/retail/gruntz_edges.csv \
    --out build/trace/this-pointer-classes.csv

# integrate the ties into the codebase: minimal class decls + empty __thiscall RVA
# stubs (own TU `engine_discovered` in config/units.toml; objdiff tracks them like
# any backlog stub). Then `gruntz build` to compile/delink/diff.
python -m gruntz.analysis.gen_class_stubs   # -> include/Stub/discovered.h, src/Stub/Discovered.cpp
```

Deferred ideas (recorded for later): hook `operator new`/`malloc` returns to add object
**size** + **offset** (multiple-inheritance subobjects, `new[]` strides); run Ghidra's
Decompiler-Parameter-ID analyzer to classify the `cc=unknown` set instead of hooking it
all and filtering by clustering.
