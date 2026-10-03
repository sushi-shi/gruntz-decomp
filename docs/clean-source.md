# Standalone source export

`gruntz clean` generates one C++ project with its own build support. The exporter
stays in the reconstruction repository; its output contains source, headers,
resources, vendor notices, licensing and a small MSVC 5.0 SP3 build.

```sh
nix develop -c gruntz clean --out build/clean-source --verify
```

By default all inputs, including templates, come from committed `HEAD`.
`--ref REVISION` selects another committed revision containing the exporter.
`--working-tree` previews tracked working files, including staged new files;
untracked files are excluded. It cannot be combined with `--ref`.

The file contents are deterministic for a given input snapshot. The export
prints a content digest and records the originating commit in
`.gruntz-source-export`. Only directories bearing this generator's marker can
be replaced. Output inside the current checkout must be below `build/`.
Symlink paths and outputs containing Git repositories are refused. To start
independent development, copy the output or initialize a new repository there;
future exports must then use a different destination.

The lexer removes project comments and `RVA`, `DATA`, compiler-generated and
dynamic-initializer labels. `DATA_COMPGEN` becomes its value expression;
`OVERRIDE` disappears for VC5. `rva.h` includes become `Ints.h` includes so
integer types remain available. Strings and license notices survive; unknown
annotation forms fail generation. Vendor files retain their original contents.
The source manifest supplies compilation units and compiler flags. Matching
configuration, annotations, score tables, analysis tools and test suites are
excluded. The exported flake has only the pinned Nixpkgs input and build tools.

The shared Nix toolchain definition and low-level Wine compiler/resource/linker
bridges are reused. Exported builds use their own Wine prefix and ordinary
object/resource linking, without retail layout controls. No retail executable
is read during generation or building.

Miles and Smacker import libraries are built from the small checked-in C export
bodies under `scripts/gruntz/clean/project/imports/`. VC5's `/DLL /NOENTRY
/IMPLIB` produces correctly decorated loader imports. Only the resulting `.lib`
files enter the game link; temporary DLLs and objects are discarded, including
on failure. No hint padding or matching tables are needed. The real DLLs and game
data are still required at runtime and are not included in the export.

`--verify` enters the exported project's own Nix shell and compiles and links
`build/GRUNTZ.EXE`. It does not launch the game or claim gameplay verification.
From the output directory, the same build is:

```sh
nix develop path:. -c python3 build.py --jobs 4
```

Source changes rebuild their objects; header, build-support or toolchain-path
changes invalidate all objects. Resources and import libraries are rebuilt on
each invocation. The current compiler, MFC and DirectX APIs remain part of the
source base; compiler modernization and platform porting are subsequent work.
