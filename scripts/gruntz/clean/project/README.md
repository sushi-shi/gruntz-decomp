# Gruntz source

The clean C++ base for building and extending Gruntz.

```text
       main
         |
         v
      source (you are here)
         |
         v
     your port
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

## What's included

The build needs no original game executable. `imports/` contains temporary DLL
export bodies used to generate Miles and Smacker import libraries. Their DLLs
are deleted immediately; only the import libraries enter the game link.
Game data stays outside the source tree; Nix fetches the real runtime DLLs
from pinned sources when you launch.

Generation preserves the existing compiler, platform APIs and source behavior.
This is a base for further development, not a modern compiler or platform port.
Export verification compiles and links; it does not launch or validate gameplay.

Project code is covered by `LICENSE`. Vendored SDK headers and zlib retain their
own notices. Icons, cursors and other game resources retain their original ownership.
