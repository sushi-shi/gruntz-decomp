# Pinned VC5 toolchain

For normal work, run `nix develop` from the intended checkout. The flake fetches
the pinned compiler package; there is no need to rebuild or republish it.

To reproduce the package from its original media, inspect
[create-toolchain-release.nix](../scripts/create-toolchain-release.nix) and
[create-toolchain-release.py](../scripts/create-toolchain-release.py), then run:

```sh
nix-shell scripts/create-toolchain-release.nix
```

That entry point fetches the pinned VC5, VS97 SP3, DirectX 6 SDK and Ninja media
and runs the packager. The scripts own URLs, hashes, extraction rules, and
validation; avoid a second hand-maintained inventory here.

The package layout is:

```text
msvc/
  bin/       compiler/linker, resource tools, required shared-IDE DLLs
  include/   C/C++ and MFC headers
  lib/       CRT, MFC and import libraries
dx/
  Include/   DirectX SDK headers
  Lib/       DirectX SDK libraries
ninja/
  ninja.exe
```

VC5 media predates MSI. The packager extracts the archives and overlays the
service-pack files. Shared IDE dependencies such as MSPDB50.DLL and MSDIS100.DLL
must accompany the command-line tools; copying only VC/bin is insufficient.
See [compiler identification](compiler-detection.md) for the version checks.

Output is `build/gruntz-toolchain-vc50.tar.xz`. Packaging does not authorize
publishing or overwriting a release. Review content and hashes before any
separately authorized release or flake update; check the applicable reuse terms.
