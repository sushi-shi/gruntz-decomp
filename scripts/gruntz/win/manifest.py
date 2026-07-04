"""manifest.py - the hash-pinned Windows artifact table (the flake's fetchurl set).

Every entry mirrors a flake.nix package, in its WINDOWS form. SHA256 are hex
(the flake stores Nix SRI base64; these were converted + cross-checked, and the
GitHub release/Adoptium checksums agree). Versions track the pinned nixpkgs
(64c08a7c): ghidra 12.0.4, llvm 21.1.8, ninja 1.13.2, ripgrep 15.1.0, jq 1.8.1,
JDK 21 (Temurin GA). objdiff 3.7.1 + the VC5 toolchain match flake.nix exactly.

Pinning philosophy is the flake's: a download only counts if its SHA256 matches.
The one artifact that is NOT a pinned binary is vostok-delinker - the flake builds
it from a forked source commit, so we pin the SOURCE tarball and `cargo build` it
(see provision.py). sfman32 stays a placeholder, exactly as in flake.nix.

Layout under build/win-toolchain/ (git-ignored, like everything in build/):
    toolchain/  msvc/ + dx/ + ninja/   (MSVC 5.0 SP3, DX6 SDK, ninja.exe)
    llvm/       bin/ (clang, clangd, clang-format/-tidy, llvm-pdbutil/-nm/-undname)
    ghidra/     a Ghidra 12.0.4 install (support/, Ghidra/, ghidraRun.bat)
    jdk/        a Temurin JDK 21 (bin/, lib/) - Ghidra's runtime
    bin/        loose tools on PATH: objdiff(-cli), ninja, rg, jq, vostok-delinker
    assets/     GRUNTZ.EXE + the runtime DLLs (MSS32/SMACKW32)
    cache/      downloaded archives, kept for idempotent re-runs
"""

from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class Artifact:
    filename: str            # local name in cache/
    url: str
    sha256: str              # hex
    kind: str                # "raw" | "zip" | "tarxz" | "targz"
    dest: str                # subdir under win-toolchain/ to install into
    strip: int = 0           # leading path components to drop (tar) / top dir (zip)
    rename: str | None = None        # for kind=="raw": final filename
    bin_picks: tuple = ()            # files to also copy into bin/ after extract
    optional: bool = False           # skipped while placeholder (sfman32)


# The GUI objdiff binary is large and only needed for interactive diffing; the
# CLI (objdiff-cli) is what the build graph actually invokes. `--no-gui` skips it.
GUI_KEYS = ("objdiff",)

ARTIFACTS: dict[str, Artifact] = {
    # --- the VC5 toolchain: MSVC 5.0 + DX6 SDK + ninja.exe (native on Windows) --
    "toolchain": Artifact(
        "gruntz-toolchain-vc50.tar.xz",
        "https://github.com/sushi-shi/gruntz-decomp/releases/download/toolchain-vc50-sp3/gruntz-toolchain-vc50.tar.xz",
        "64f98e188f241387084ec844aea3af4591fd308c5761c543d789c3ed93d44511",
        "tarxz", "toolchain", strip=1,
    ),

    # --- the decomp target + the runtime DLLs it loads --------------------------
    "gruntz_exe": Artifact(
        "GRUNTZ.EXE",
        "https://archive.org/download/gruntz-pc/Gruntz.iso/GAME%2FGRUNTZ.EXE",
        "7073c2536106ae4cca32e3e82db21001f319678b214c4eae2c689c54902808b3",
        "raw", "assets",
    ),
    "mss32": Artifact(
        "MSS32.DLL",
        "https://archive.org/download/gruntz-pc/Gruntz.iso/GAME%2FMSS32.DLL",
        "accfc15fa5924c5ddcc21025f35af908e4e7ed7576b664ab54d7137e45323e75",
        "raw", "assets",
    ),
    "smackw32": Artifact(
        "SMACKW32.DLL",
        "https://archive.org/download/gruntz-pc/Gruntz.iso/GAME%2FSMACKW32.DLL",
        "f9b2fdb5ebc8e659c7ac132c213fcfd2eb059a1195a129121bb68ca21699e5e1",
        "raw", "assets",
    ),

    # --- objdiff 3.7.1 (Windows builds; flake fetches the Linux ones) -----------
    "objdiff_cli": Artifact(
        "objdiff-cli-windows-x86_64.exe",
        "https://github.com/encounter/objdiff/releases/download/v3.7.1/objdiff-cli-windows-x86_64.exe",
        "97f906acf83442040e3624688341f821238db367c9dba7585213d301216310d2",
        "raw", "bin", rename="objdiff-cli.exe",
    ),
    "objdiff": Artifact(
        "objdiff-windows-x86_64.exe",
        "https://github.com/encounter/objdiff/releases/download/v3.7.1/objdiff-windows-x86_64.exe",
        "951452619c7999a1814c2f2d8b36709059f4cbd3b328213fe0e2683ab5b2dc9f",
        "raw", "bin", rename="objdiff.exe",
    ),

    # --- LLVM 21.1.8 (clang driver + clangd + clang-format/-tidy + llvm-pdbutil) -
    # The official x86_64-pc-windows-msvc tarball; verified to contain clang.exe,
    # clangd.exe, clang-format.exe, clang-tidy.exe, llvm-pdbutil/-nm/-undname.exe.
    "llvm": Artifact(
        "clang+llvm-21.1.8-x86_64-pc-windows-msvc.tar.xz",
        "https://github.com/llvm/llvm-project/releases/download/llvmorg-21.1.8/clang%2Bllvm-21.1.8-x86_64-pc-windows-msvc.tar.xz",
        "749d22f565fcd5718dbed06512572d0e5353b502c03fe1f7f17ee8b8aca21a47",
        "tarxz", "llvm", strip=1,
    ),

    # --- Ghidra 12.0.4 (matches nixpkgs; the tracked library_labels.csv is        -
    # calibrated to 12.x's function carving) ------------------------------------
    "ghidra": Artifact(
        "ghidra_12.0.4_PUBLIC_20260303.zip",
        "https://github.com/NationalSecurityAgency/ghidra/releases/download/Ghidra_12.0.4_build/ghidra_12.0.4_PUBLIC_20260303.zip",
        "c3b458661d69e26e203d739c0c82d143cc8a4a29d9e571f099c2cf4bda62a120",
        "zip", "ghidra", strip=1,
    ),

    # --- JDK 21 (Ghidra/pyghidra runtime). The exact patch is irrelevant - it is  -
    # not part of the matching surface; Temurin 21 GA, Windows x64. ---------------
    "jdk": Artifact(
        "OpenJDK21U-jdk_x64_windows_hotspot_21.0.11_10.zip",
        "https://github.com/adoptium/temurin21-binaries/releases/download/jdk-21.0.11%2B10/OpenJDK21U-jdk_x64_windows_hotspot_21.0.11_10.zip",
        "d3625e7cadf23787ea540229544b6e2ab494b3b54da1801879e583e1dfee0a64",
        "zip", "jdk", strip=1,
    ),

    # --- the small CLI tools the flake pulls from nixpkgs -----------------------
    "ninja": Artifact(
        "ninja-win.zip",
        "https://github.com/ninja-build/ninja/releases/download/v1.13.2/ninja-win.zip",
        "07fc8261b42b20e71d1720b39068c2e14ffcee6396b76fb7a795fb460b78dc65",
        "zip", "bin",                         # ninja.exe sits at the zip root
    ),
    "ripgrep": Artifact(
        "ripgrep-15.1.0-x86_64-pc-windows-msvc.zip",
        "https://github.com/BurntSushi/ripgrep/releases/download/15.1.0/ripgrep-15.1.0-x86_64-pc-windows-msvc.zip",
        "124510b94b6baa3380d051fdf4650eaa80a302c876d611e9dba0b2e18d87493a",
        "zip", "ripgrep", strip=1, bin_picks=("rg.exe",),
    ),
    "jq": Artifact(
        "jq-windows-amd64.exe",
        "https://github.com/jqlang/jq/releases/download/jq-1.8.1/jq-windows-amd64.exe",
        "23cb60a1354eed6bcc8d9b9735e8c7b388cd1fdcb75726b93bc299ef22dd9334",
        "raw", "bin", rename="jq.exe",
    ),

    # --- vostok-delinker: pinned SOURCE (forked branch), built with cargo nightly -
    "vostok_src": Artifact(
        "vostok-delinker-8a42a0ba.tar.gz",
        "https://github.com/srp-survarium/vostok-delinker/archive/8a42a0ba6f6b90651d62d1911eb97b80a5faa149.tar.gz",
        "991131c0215ad067b52ee751c4bb6fd8df8cc30076811cd2e96ccab9c584ee08",
        "targz", "vostok-src", strip=1,
    ),
}

# sfman32: a fakeHash placeholder in flake.nix (no genuine Miles DLL located). It
# is LoadLibrary'd at runtime, so its absence only disables SoundFont music; we
# carry it here for parity but never fetch it - mirrors gruntz-sfman32-available.
SFMAN32_PLACEHOLDER = True


def selected(no_gui: bool = False) -> dict[str, Artifact]:
    """The artifacts to provision, honouring --no-gui."""
    return {k: a for k, a in ARTIFACTS.items() if not (no_gui and k in GUI_KEYS)}
