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
Saving encodes first, exclusively creates and writes/closes an owned temporary
sibling, then replaces the destination; a write, close or replacement failure
retains the previous file. The storage assumes one writer per destination. Native Windows replacement uses its filesystem primitive; Linux
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
Inactive applications wait for input unless a shutdown deadline is pending. Frame
pacing no longer spins or waits inside the game manager. Blocking transitions and movie/input waits remain separate
lifecycle work; their conversion is required before all callbacks can return
promptly.

`ShutdownRequest` is owned by the application and advances before gameplay gates.
A quit request starts its delay on the next host callback; repeated requests do
not restart it. Unsigned elapsed-time subtraction handles clock wrap and late
callbacks. Delays are capped at `0x7fffffff` ms so they cannot become the host's
indefinite-wait sentinel. The menu cue duration plus 500 ms remains the requested
shutdown delay, with zero delay if no cue is available.

While quitting, frame updates, gameplay commands, state input and repaint-driven
rendering stop. Window events remain responsive. The Windows host consumes one
close request after the timer expires and dispatches `WM_CLOSE`, retaining normal
audio/window cleanup. Other hosts should call `Step` through the deadline even
while inactive and consume `TakeCloseRequest` at their own close boundary.

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
overrides as `check-io.py`. Shutdown cases cover zero delay, repeated requests,
signed/full clock wrap, long gaps, suspended frame scheduling and exactly-once
close delivery. No game is launched.

`nix develop --command python3 check-shutdown-windows.py` additionally runs a
standalone console test of the production `CGameApp::Step` callback, with and
without a game manager. It checks that shutdown advances while inactive or
stopped and that gameplay timestamps stop advancing. It creates no window and
never calls the game's initialization or main loop. Do not run it concurrently
with other Wine/MSVC builds.

### Transactional saved games

`io::FileTransaction` owns an exclusively created sibling file until publication.
Writes and close must succeed before `commit()` replaces the destination. Aborted
transactions remove their staging files. Destination paths are resolved at
creation, so changing the current directory cannot redirect a commit. Settings,
standalone snapshots and screenshot replacement use this same primitive.

A game save contains a snapshot plus its appended preview. Both are written to
one unique file and closed before progress metadata is published. The progress
transaction calls `commitReferencing(snapshot)` to keep that complete snapshot
only when its reference has been committed. A failed snapshot, preview or progress
write leaves the old progress record and old snapshot available. Deletion removes
the progress reference first; unsuccessful file cleanup can leave an unreferenced
file, but cannot leave progress pointing at a deleted snapshot.

The existing fixed-size progress record stores a bounded filename. `SnapshotPath`
resolves that name under the configured save directory using an owned string.
Loading accepts the original `SlotN.sav` paths and the port's unique
`SlotN.sav.stage-N` names, stripping legacy installation prefixes. Malformed or
unterminated names are rejected. The wire record size is unchanged, but new saves
must be loaded by the port: retail clients that always open `SlotN.sav` do not
follow unique snapshot references. Multiple writers to one save directory are not
supported. This ordering does not claim power-loss durability or browser IDBFS
synchronization; the browser host must persist storage explicitly. Abrupt process
termination may leave unreferenced staging files.

`check-io.py --target all` exercises replacement/abort, exclusive staging,
publication ordering, failed writes, failed replacement, stable paths, and old/new
bounded filenames on Linux and wasm32. Native tests also force a real stdio
write/close failure with `RLIMIT_FSIZE` and verify that the old referenced save
remains intact. The Windows game integration is compiled and reviewed; it is not
launched by this suite.


### Bounded REZ archives

`rez::decode` reads version-1 archive metadata through `io::RandomInput`, implemented
by owned files and memory inputs. It decodes little-endian fields explicitly,
checks complete headers and records, bounds every string/key array/member span,
and rejects overlapping directory blocks (including cycles). All decoded names
and records are owned. The caller's prior result is replaced only after the entire
directory graph passes validation. Payloads stay in the source until requested.

Default limits are 16 MiB per directory block, 64 MiB total directory bytes,
1,000,000 records and depth 256. Callers of the codec can supply other limits.
The current file backend supports offsets through 2 GiB minus one byte. Empty
members and directories are allowed; nonempty spans must start after the header.

The game validates primary and additional archives before installing their
records. Malformed replacements preserve the currently open archive. Bulk loading
owns each member separately; it does not infer a contiguous byte range from the
sum of member sizes. Member reads and seeks check bounds before pointer arithmetic.
Replacement recycles entries with their hash-node identity intact, and individual
unload invalidates the directory's loaded status.

Run the portable decoder checks and optionally inspect local assets with:

```sh
nix develop .#portable --command python3 check-rez.py --target all [archive.rez ...]
```

Linux uses ASan/UBSan; wasm32 runs the synthetic fixtures in Node. Local archive
arguments are read on Linux only. Tests cover truncation boundaries, missing
terminators, invalid kinds/counts/ranges, cycles, configured limits, failed reads,
transactional result replacement and deterministic malformed-input mutations.

The production archive manager has a separate Windows console component test:

```sh
nix develop --command python3 check-rez-windows.py
```

It compiles the actual archive, storage and container implementations with MSVC
and runs only the test executable under Wine. It covers malformed primary/overlay
archives, repeated replacement, noncontiguous member data, loaded/unloaded reads,
invalid seeks, end of member, reset and close/reopen. Do not run it concurrently
with the Windows game build; they share the worktree's toolchain prefix. Neither
suite launches the game. Directory-backed emulation/path lookup still uses the
legacy Windows implementation and is not covered by the portable codec claim.


### Bounded PCX and PID raster data

`raster::decodePcx` and `decodePid` consume a byte span and produce an owned image:
tight top-down rows of palette indices or RGB, optional owned RGB palette entries,
and the validated compressed-byte count. They decode wire fields explicitly and
leave the previous result unchanged on failure. No decoder requires DirectDraw,
a Windows bitmap, or a pointer cast onto a packed record.

PCX supports 8 bits per plane, one indexed plane or three RGB planes, encoded or
uncompressed rows, and the header's bytes-per-line padding. Indexed PCX requires
the trailing palette marker and 256-color palette. PID tag 10 (and legacy tag 0
written by older game exports) supports byte runs and skip/literal packets, optional palettes and transparent fill. Reads stop
at the pixel payload boundary. Zero runs, truncated packets, invalid dimensions,
output overruns and runs beyond the final row are rejected. The default decoded
limit is 64 MiB, including padded PCX rows; callers may supply another limit.

The DirectDraw and DIB PCX/PID paths use this decoder. Surface upload converts
RGB to BGR only at adapters that require it. `copyRows` checks the destination
span and pitch and leaves row padding untouched. DIB pitch is measured in bytes
at every depth, including 16/24/32-bit images; raw pixel input has explicit row
order. The shaded-sprite builder validates skiprun input and palette requirements
before conversion, retaining only the validated compressed span. Its portable
16-bit transcode checks complete rows and has a 128 MiB output limit.

```sh
nix develop .#portable --command python3 check-raster.py --target all [archive.rez ...]
nix develop --command python3 check-raster-windows.py
```

The portable suite runs under native ASan/UBSan and actual wasm32 in Node. It
checks truncation, palette extents, dimensions, both PID grammars, odd widths,
padded rows, channel order, run boundaries, failed-result preservation and
malformed-input mutations. Optional archives are read on Linux and every PCX/PID
member is decoded. The Windows console component test creates only offscreen
DIBs to verify the production upload, byte pitches and row order. Do not run it
concurrently with another Wine/MSVC build. No game is launched.

RID retains its legacy width-dependent row orientation pending a dedicated codec.
BMP/RID parsing, the remaining clipped/shaded raster operations and the rendering
host still need their own bounds/portability work. These component checks are not
a whole-game rendering or browser-host validation.


### Returning scene fades

`FadePlayback` owns one `FadeEffect` and advances it from admitted frame deltas.
Starting renders frame zero; the first callback establishes the timing epoch.
Each later callback renders at most one new frame, including after a long gap.
Lead time and duration use separate bounded counters; frame interpolation uses
64-bit integer multiplication. Zero-duration fades still render their final frame.
Final-only mode retains the delay but skips intermediate rendering.

Completion, cancellation, replacement and render failure release the owned effect.
A busy renderer requests another callback without completing or losing the final
frame. FrameScheduler's zero delta on resume prevents inactive time from advancing
an in-progress fade. Cancellation does not invoke the state's completion hook.

Help entry and menu/credits entry and restoration use this returning path. Menu entry
reveals the cursor and starts music only on completion; restoring an existing menu
does not restart its music. Menu/credits draw failure restores the title without
restarting the failed effect. Recovery presentation retries busy surfaces on later
callbacks for up to two seconds of admitted time, then reports failure; successful
presentation invokes the same completion hook. Credits returns immediately when
restoration begins a fade so scrolling and drawing cannot overwrite it. Preview fade methods also use
this path, with the preview timer reset by completion; the existing preview class
has no runtime factory binding in the current source. The manager advances active
fades before normal state updates and suppresses competing state input, commands (except
quit), and repaint rendering. Resource reload, mode changes, state changes and
shutdown cancel playback before changing its borrowed surfaces.

The Windows sine adapter uses nonwaiting locks for returning playback. It retries
busy surfaces, unlocks before reporting failure, and releases playback before
asking the state to restore its display. The adapter retains its existing bounded
2000-row/sample storage and rejects larger dimensions. Unmigrated synchronous
callers retain their waiting/restoring lock path. Gameplay entry/restoration and booty fades
still require caller-continuation rewrites. The returning state-departure path
below retains states through exit fades and audio ramps. This is not completion
of the scene-transition or rendering-host port.

```sh
ASAN_OPTIONS=detect_leaks=0 nix develop .#portable --command python3 check-fades.py --target all
```

The portable suite executes the same playback code under Linux ASan/UBSan and
wasm32 Node. Probe effects verify frame order, timing boundaries, cancellation and
ownership, failed initialization/rendering, busy final frames, bounded recovery
retries and timeout reset/saturation, arithmetic limits,
and scheduler wrap/suspension. These tests do not execute DirectDraw rendering,
state UI interactions or the game.

### Returning state departures

State-change requests are owned by `CGruntzMgr`. A successful request means it was
accepted; departure, installation and arrival run on later frame callbacks.
Requests made by a state never delete that state on its own call stack. The
manager retains the departing state and its resources until its fade/audio work
finishes, then replaces it, pushes it onto the state stack or reloads it in place.
Input, commands other than quit, and repaint rendering are gated during this work.
Quit cancels pending transitions and completion actions.

Menu, attract and both booty states advance departure audio ramps from callbacks.
The menu activation-cue delay uses admitted elapsed time. Gameplay/multiplayer exit
fades now return, with player-unit removal deferred until the fade completes. A
lost departure surface rebuilds the loading screen once instead of restoring the
normal gameplay scene or repeating departure side effects. The audio adapter
forces stop at the ramp deadline and reports failure if the stop fails.

Requests own save snapshot paths, error/fallback choices and follow-up commands.
Snapshot restoration and multiplayer connection run after the destination has
entered and its returning fade has completed. A failed transition can request its
configured fallback once. Reentrant cancellation during a phase cannot resurrect
the canceled request.

```sh
ASAN_OPTIONS=detect_leaks=0 nix develop .#portable --command python3 check-state-transitions.py --target all
```

The portable tests cover phase order, retained-state lifetime, per-phase failure,
cancellation and replacement during callbacks, delay saturation, clock wrap and
suspension. They do not execute the production graphics/audio adapters or game UI.
Gameplay entry/restoration fades, booty scene fades, movie playback and
multiplayer waits remain synchronous. Menu `StopMusicChain` still has separate
modal/movie callers to migrate. This is not a complete browser-host lifecycle yet.

### Staged level loading

Destination installation now starts loading; the manager polls it before entering
the state. `CPlay` owns the loading title and advances one ordered asset batch per
frame callback. The title fade and final 100 ms display delay return to the host;
the delay excludes suspended time. Individual file/codec operations within each
batch remain synchronous, as do multiplayer lobby dialogs and readiness waits.

Level completion runs before namespace completion. Namespace completion runs only
for a newly created state, preserving single-player cursor setup and multiplayer
readiness/session setup without repeating them on round reloads. Save restoration
and follow-up commands still wait until arrival completes. Quit cancels the owned
loading continuation; failed loads never enter gameplay, and a failed reload is
discarded before a configured fallback can depart it as if it were playable.

Loading-surface recovery restores installed images and presents the loading page
once, then redraws progress without repeating world/actor installation. It never
uses the partially loaded world's normal gameplay restoration path. Temporary
resource-namespace selection is restored before returning, including on failure.

`check-state-transitions.py --target all` also tests asset order, one batch per
callback, new-state versus reload completion, failure/cancellation at every load
step, reentrant replacement, and presentation delays across wrap and suspension.
The integration compiles with both Windows toolchains; graphics recovery and
network readiness are not exercised by the portable component tests.
