# Gruntz source

This is the reconstructed Gruntz C++ source with a standalone Windows build.
Build on x86_64 Linux using Nix and the pinned MSVC 5.0 SP3 / DirectX 6 toolchain:

```sh
nix develop path:. -c python3 build.py
```

The executable is `build/GRUNTZ.EXE`. `--jobs N` controls compiler concurrency.
Objects are cached against source, header, build-script and toolchain inputs.

The build needs no original game executable. `imports/` contains temporary DLL
export bodies used to generate Miles and Smacker import libraries. Their DLLs
are deleted immediately; only the import libraries enter the game link.
The game still requires its original data and real `mss32.dll` and
`smackw32.dll` at runtime. Those files are not included here.

Generation preserves the existing compiler, platform APIs and source behavior.
This is a base for further development, not a modern compiler or platform port.
Export verification compiles and links; it does not launch or validate gameplay.

Project code is covered by `LICENSE`. Vendored SDK headers and zlib retain their
own notices. Icons, cursors and other game resources retain their original ownership.
