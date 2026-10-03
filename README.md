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
  remains alive and unchanged. Read-only interfaces accept `const char*`;
  writable Windows text output uses separate character buffers. Optional null
  names become empty strings at the boundary.
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
