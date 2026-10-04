# Gruntz source

The clean C++ base for building and extending Gruntz.

```text
       main
         |
         v
      source
         |
         v
     port-mfc-stdlib (this branch)
```

[`main`](https://github.com/sushi-shi/gruntz-decomp/tree/main) owns reconstruction
and the exporter. [`source`](https://github.com/sushi-shi/gruntz-decomp/tree/source)
is a generated, single-commit snapshot. Create your own branch from `source`
for ongoing development; regeneration replaces the snapshot.

## Play

On x86_64 Linux, run this from the source directory:

```sh
nix run path:. -- --data "/path/to/your/Gruntz"
```

Nix supplies the compiler, Wine, gamescope, and the real Miles/Smacker DLLs.
The command builds the game and starts it. Point `--data` at a game folder
containing `Gruntz.REZ`, `GRUNTZ.VRZ`, and the four `.FNT` files.
The folder is remembered, so subsequent launches are simply:

```sh
nix run path:.
```

Saves and the game Wine prefix live under `build/game/`. Keep that directory to
keep your progress. The original game-data directory is not modified.

## Build only

To compile without launching:

```sh
nix develop path:. -c python3 build.py
```

The executable is `build/GRUNTZ.EXE`. `--jobs N` controls compiler concurrency.
Objects are cached against source, header, build-script and toolchain inputs.

## Port development

This branch replaces game-owned MFC strings and collections with `std::string`,
`std::vector`, `std::list`, and typed `std::map` instances. It still uses the
Windows application, graphics, audio, and networking backends. A complete Linux
or WebAssembly game build requires the later platform work.

The Clang Windows target compiles all units to COFF objects without launching:

```sh
nix develop path:. -c python3 build-clang.py
```

Use `--syntax-only` for a faster check, `--jobs N` to set concurrency, and optional
unit names to select sources. Diagnostics and the compilation database live in
`build/clang/`. This target checks compilation; the complete executable is still
linked by `build.py` with the historical Windows toolchain. Clang uses a generated
SDK header copy with declaration-syntax corrections for the old headers.

The portable helpers have native Linux tests:

```sh
nix develop path:. -c python3 check-stdlib.py
```

These exercise formatting, byte-oriented case handling, substring bounds,
sparse vector growth (including aliased input), and list extraction under
AddressSanitizer and UndefinedBehaviorSanitizer. If the runner uses ptrace,
set `ASAN_OPTIONS=detect_leaks=0`; LeakSanitizer cannot run under ptrace.

### Text and collection contracts

- Strings own their bytes. A `c_str()` pointer is borrowed only while the owner
  remains alive and unchanged. Config lookups and text formatters return owned
  strings; migrated read-only text inputs use `const std::string&`. C APIs receive
  `const char*`, and writable Windows text output uses separate character buffers.
  Optional null names become empty strings at the boundary.
- Text remains in the existing byte encoding. Resource-key case conversion is
  explicitly ASCII; bytes above ASCII are preserved. Unicode conversion and
  platform path rules belong to the later filesystem/UI port.
- Vectors own their storage; pointer elements retain their existing explicit
  deletion or pool-return rules. Growing a vector does not relocate the pointed
  objects. Lists retain traversal order and stable element iterators. A cursor
  terminates at its own container's `end()`.
- Maps own their keys and use deterministic key order: numeric object IDs and
  lexical resource names. This replaces MFC hash-bucket enumeration, including
  sound-cue enumeration and object serialization passes. Rendering/update list
  order and equal-sort-key insertion order remain unchanged. Runtime and retail
  multiplayer/save interoperability are not established by compilation checks.
- Runtime resource keys retain their complete names. Legacy save fields keep their
  fixed width; resource-reference writers reject names that do not fit or contain
  embedded nulls.
- Animation, sound, palette, and logic registries reuse existing keys without
  replacing borrowed objects. MIDI registration rejects duplicate keys. Resource
  registry insertion is private.
- Animation act names use a node-based standard map. The previous raw-reallocated
  array cannot safely store `std::string`; the map keeps referenced names stable
  when new IDs are registered.

## What's included

The build needs no original game executable. `imports/` contains temporary DLL
export bodies used to generate Miles and Smacker import libraries. Their DLLs
are deleted immediately; only the import libraries enter the game link.
Game data stays outside the source tree; Nix fetches the real runtime DLLs
from pinned sources when you launch.

The source export supplies the original platform backends. This branch starts
the portability work above. Build verification does not launch or validate gameplay.

Project code is covered by `LICENSE`. Vendored SDK headers and zlib retain their
own notices. Icons, cursors and other game resources retain their original ownership.

### Owned file I/O

`io::File` owns a standard binary file and is noncopyable. Open modes distinguish
read-only, replacement, and update-without-truncation. Short reads, failed seeks,
and failed writes retain an error until the next open; `finish()` closes the file
and reports buffered-write/close failures. Destruction closes on early returns.
The legacy runtime accepts file positions up to 2 GiB minus one; larger inputs
are rejected explicitly. Paths are supplied by callers in the platform encoding.

Font, palette, image, level, FEC, save, and logging callers no longer use MFC file
or archive objects. The Windows Smacker backend borrows a native handle at its
existing decoder boundary. Binary codecs contain no Windows APIs; path resolution
and file replacement isolate their native filesystem operations in `Io/File.cpp`.

Run `ASAN_OPTIONS=detect_leaks=0 nix develop --command python3 check-io.py` for
native file ownership/error tests. This does not launch the game.

### Storage-independent binary I/O

`io::Input` and `io::Output` separate byte transport from binary parsing. Files own
their handles; memory inputs copy their bytes and memory outputs own their vectors.
`BinaryReader`/`BinaryWriter` encode 32-bit fields explicitly in little-endian order.
Font and shade-table parsing uses identical code for disk and memory sources, with
bounds checked before allocation and failed reads leaving decoded output unchanged.

Snapshot save/restore overloads accept byte sources/sinks through `CStreamArchive`.
The adapter borrows them for the synchronous call and does not close the owner.
Existing snapshot field layouts remain unchanged; this is not yet a redesign of
the complete save format or all malformed-world/graphics decoder validation.

### Configuration storage

Preferences use `Settings`, an owning typed map with explicit `load()` and `save()`.
The default file is `gruntz.cfg`; set `GRUNTZ_CONFIG` to select another file.
The path is resolved when loaded, so later working-directory changes cannot redirect
a save. The parent directory must exist. Missing files start with defaults; malformed
or unreadable files fail without replacing live settings or overwriting the file.

The text format starts with `GRUNTZ CONFIG 1`, followed by `i key=integer` or
`s key=text` lines. Keys match case-insensitively for ASCII. Percent escapes preserve
`%`, `=`, control characters and non-ASCII bytes; embedded nulls are rejected.
LF and CRLF files are accepted. Integers are signed 32-bit, and the file is limited
to 1 MiB. For example:

```text
GRUNTZ CONFIG 1
i music=1
i music volume=75
s player name=Player
```

Game preference edits remain in memory and are flushed after state teardown at
shutdown. The startup options dialog saves on confirmation and reports failures.
Saving encodes first, writes/closes a temporary sibling, then replaces the destination;
a write or replacement failure retains the previous file. The storage assumes one
writer per path. Native Windows replacement uses its filesystem primitive; Linux
and Emscripten use standard rename. Power-loss durability is not claimed.

Existing registry values are not imported. Optional `CdRom Drive` and
`Portal Executable` settings replace installer-registry lookups. Windows CD discovery,
process launch, and URL association handling remain platform-backend work.
Browser hosts can supply a path in a mounted persistent filesystem or persist the
encoded bytes; browser persistence synchronization is separate from `save()`.

### Linux and WebAssembly verification

Run the identical component suite on native Linux and real wasm32:

```sh
ASAN_OPTIONS=detect_leaks=0 nix develop .#portable --command python3 check-io.py --target all
```

The dedicated shell provides pinned Clang, Emscripten, Node and Python without
requiring the historical Windows toolchain. `--target native` (the default) runs
ASan/UBSan; `--target wasm` compiles and executes in Node with Emscripten MEMFS.
Compiler and runtime failures fail the command. `CXX`, `EMXX`, `NODE` and `EM_CACHE`
can override tools/cache paths. Outputs and compiler caches stay under `build/`.
The LeakSanitizer override above is needed only for ptrace-based runners; omit it
elsewhere to retain leak checking.

Both runs exercise file modes and errors, exact little-endian bytes, font and shade
file/memory parity, truncated/oversized inputs, owned memory, borrowed snapshots,
typed configuration persistence, corrupt input, failed-save preservation and stable
configuration paths across directory changes. Pointer-width output confirms native
64-bit and wasm32 execution. No game executable is launched.

This verifies the portable components. It does not establish browser persistence,
asset downloads, rendering, audio, gameplay, or whole-game Linux/WASM support.
Browser storage integration must synchronize persistence explicitly; see the
[Emscripten filesystem documentation](https://emscripten.org/docs/porting/files/file_systems_overview.html).

### Portable frame timing

`FrameTiming` owns frame timestamps, the application periodic timer, frame pacing
state, and the displayed FPS sample. The host supplies unsigned 32-bit monotonic
milliseconds; subtraction supports clock wrap when samples are less than one full
clock cycle (about 49.7 days) apart. No system clock, window, or wait is used by this
component. Each game manager owns its timing; consumers read its `Timing()` view.

`FrameScheduler::poll(now)` captures frame start and returns false until pacing
allows an update. `delayMs(now)` tells the host when to call again. Pending polls
retain their start timestamp internally; timing, timer and FPS state are published
only when a frame is admitted, so cancellation cannot consume unseen timer state. A true
result admits one game update; late callbacks do not run catch-up updates.
`deltaMs()` stays based on frame start, so waiting contributes to the next frame's
delta. Resetting the frame clock cancels a pending frame. Suspending also cancels it;
the first resumed poll resets frame time using the resume timestamp, so paused
time never enters the gameplay delta even without a separate activation handler.

The Windows host calls `CGameApp::Step(now)` once after each bounded input batch.
This callback runs at most one game update and returns a requested delay; the host
waits for that deadline or incoming input using `MsgWaitForMultipleObjects`.
Inactive applications wait for input. Frame pacing no longer spins or waits inside
the game manager. Blocking transitions and movie/input waits remain separate
lifecycle work; their conversion is required before all callbacks can return
promptly.

The periodic timer exposes zero for one frame on expiry and rearms on the next
frame without consuming that frame's delta. Changing its period takes effect at
rearm. Frame deltas remain unclamped here; the gameplay clock retains its existing
100 ms clamp. FPS keeps the existing frame-count/2 calculation at two-second
sample boundaries, including after long pauses. `resetFrameTime(now)` clears the
frame delta and pacing epoch on resume while retaining the timer, FPS sample and
rate limit; `reset(now)` starts a fresh session. Zero is a valid pacing timestamp.

Run the same deterministic timing cases on Linux and wasm32:

```sh
ASAN_OPTIONS=detect_leaks=0 nix develop .#portable --command python3 check-timing.py --target all
```

The suite checks expiry/rearm, period changes, frame pacing, FPS windows, clock
wrap, long pauses, resume and independent manager state. The native run uses fatal
ASan/UBSan checks; wasm32 executes in Node. The runner accepts the same tool/cache
overrides as `check-io.py`. No game is launched.
