# Entry point for building the gruntz-toolchain release tarball (MSVC 5.0 SP3).
#
# Fetches the original VC++ 5.0 media + VS97 Service Pack 3 into the Nix store,
# sets up Wine + 7z, and runs create-toolchain-release.py automatically.
#
# Usage (from the gruntz/ repo root):
#   nix-shell scripts/create-toolchain-release.nix
#
# ---------------------------------------------------------------------------
# DIFFERENCE FROM vostok's create-toolchain-release.nix
# ---------------------------------------------------------------------------
# vostok packages VS2008, whose media are MSI (admin-installed with `msiexec
# /a PATCH=`). VC++ 5.0 media are NOT MSI - the VS97 CDs use InstallShield with
# compressed CAB cabinets, and SP3 ships as a self-extracting archive of whole
# replacement binaries. So this flow needs only `p7zip` (7z) for extraction;
# `msitools`/`xvfb-run`/Wine-msiexec are NOT required for the extract+overlay.
# (Wine is kept available in case a step ever needs to run a 16/32-bit unpacker,
# but the default path is pure 7z and does not touch Wine.)
#
# ---------------------------------------------------------------------------
# LOCATED MEDIA (archive.org) - see docs/toolchain-vc50-sp3.md
# ---------------------------------------------------------------------------
# Every sha256 below is now pinned to the real fetched media. The archive.org
# md5/sha1 + size + URL are recorded in the comment above each fetch so the item
# can be re-verified WITHOUT a download (compare the archive.org md5/sha1 to the
# fetched file). The VS97 Disc 3 + SP3 ZIP were downloaded to build/toolchain-media/
# and added to the Nix store with `nix-store --add-fixed sha256 <file>`, so their
# pinned fetchurls resolve from the store without a network fetch; the DX6 SDK +
# ninja were pinned via `nix-prefetch-url`.

{ pkgs ? import <nixpkgs> {} }:

let
  # --- BASE COMPILER: VS97 Professional, Disc 3 = Visual C++ 5.0 -------------
  # archive.org id : microsoft-visual-studio-97-professional-edition-disc-3
  # title          : Microsoft Visual Studio 97 Professional Edition - Disc 3
  # size           : 655,605,760 bytes (655 MB)
  # md5            : bf71b8fc2d23c5de13734b189dc341cb
  # sha1           : a3ade9b0681ffab34f058a7991f6b7aa25d342e1
  # contains       : VC++ 5.0 - DEVSTUDIO/VC/BIN/{CL,C2,LINK,CVTRES}.EXE,
  #                  SHAREDIDE/BIN/MSPDB50.DLL, VC/LIB/LIBCMT.LIB,
  #                  VC/MFC/LIB/NAFXCW.LIB, VC/{INCLUDE,LIB}, VC/MFC/{include,lib,src}
  # (Alternative base discs - VS97 Enterprise Disc 3, WinWorld - are recorded in
  #  docs/toolchain-vc50-sp3.md; NOT wired here. The .py never used a base backup.)
  vc5-iso = pkgs.fetchurl {
    name = "vs97-pro-disc3-vc5.iso";
    url = "https://archive.org/download/microsoft-visual-studio-97-professional-edition-disc-3/Microsoft%20Visual%20Studio%2097%20Professional%20Edition%20-%20Disc%203.iso";
    # pinned from the locally-downloaded media (build/toolchain-media/); resolves
    # from the Nix store via `nix-store --add-fixed sha256` without a network fetch.
    sha256 = "sha256-mRAfsB9WS6nMUMvL+GLDNM0b4WoujCd8uB+aUAy2QT8="; # archive.org md5 bf71b8fc..., sha1 a3ade9b0...
  };

  # --- SERVICE PACK 3 (the byte-match driver: cvtres 5.00.1668 / link 5.10.7303)
  # Preferred for convenience: the standalone SP3 ZIP - small, no ISO mount,
  # ships whole replacement binaries (cl/c1/c1xx/c2/link/cvtres/mspdb*) you
  # overlay on the base VC bin/.
  # archive.org id : vs97sp3      size 95,851,751 B
  # md5 e25a5de59a663cd0b5bd3d1089f8adc8 / sha1 10a818f34b7223d4cbd2c99efb0e52c9efcd3bfc
  # AUTHENTICITY NOTE: this is a repackaged ZIP (~238 downloads) vs the original
  # SP3 CD image `...X03-50158...` (~3,448 downloads). No MS hashes exist for
  # VS97-era media, so authenticity rests on cross-source/redump - if you prefer
  # the more-canonical original media, re-add an `sp3-iso` fetchurl + export
  # VS97_SP3_ISO and unset VS97_SP3_ZIP (the .py falls back to VS97_SP3_ISO).
  sp3-zip = pkgs.fetchurl {
    name = "vs97sp3.zip";
    url = "https://archive.org/download/vs97sp3/vs97sp3.zip";
    # pinned from the locally-downloaded media (build/toolchain-media/); resolves
    # from the Nix store via `nix-store --add-fixed sha256` without a network fetch.
    sha256 = "sha256-vUfPYZKXs9Sye1pf5g2cDfCbFRng5+LI6ooJz5jPiCw="; # archive.org md5 e25a5de5..., sha1 10a818f3...
  };

  # --- SP3 (original CD image) - REMOVED to avoid a needless 296 MB download.
  # The .py only reads VS97_SP3_ISO as a fallback when VS97_SP3_ZIP is unset, and
  # we always set VS97_SP3_ZIP (the small repackaged SP3 ZIP, which ships the same
  # whole replacement binaries). The original CD image
  # (Microsoft_Visual_Studio_97_Service_Pack_3_X03-50158_1997, 296,183,808 B,
  # md5 afdd3e3a9089a7a9f6396f9f628cf4de) is documented in
  # docs/toolchain-vc50-sp3.md if a more-canonical source is ever wanted.

  # --- DirectX 6 SDK (Gruntz imports DDRAW/DINPUT/DSOUND/DPLAYX) --------------
  # archive.org id : directx6sdk   "Microsoft DirectX 6.0 SDK" (~6,694 downloads)
  # file           : DIRECTX6_SDK.EXE  (self-extracting SDK installer)
  # size           : 218,620,051 bytes (209 MB)
  # md5            : b0b541fcdf244ee22153d3adc27b7eb1
  # sha1           : 3bdcf0925c09afc648ad2196104214402df408d7
  # Provides the ddraw/dinput/dsound/dplay import libs + headers we link against.
  dxsdk-archive = pkgs.fetchurl {
    name = "directx6-sdk.exe";
    url = "https://archive.org/download/directx6sdk/DIRECTX6_SDK.EXE";
    sha256 = "sha256-Oqd0mnJQGaT6tjoWJyT5rbzXrBIXGARnzvJc06fbTt0="; # archive.org md5 b0b541fc..., sha1 3bdcf09...
  };

  # --- ninja.exe (build driver, run under Wine; same as vostok) --------------
  ninja-zip = pkgs.fetchurl {
    name = "ninja-win.zip";
    url = "https://github.com/ninja-build/ninja/releases/download/v1.12.1/ninja-win.zip";
    sha256 = "sha256-9VD+xwW21v9Y8ts8N0wid6N2kWeNarpGOty7EpEIRno="; # ninja v1.12.1 ninja-win.zip
  };

in pkgs.mkShell {
  packages = [
    pkgs.p7zip                        # 7z: extract the VC5 ISO + InstallShield/CAB + the SP3 zip/cabs
    pkgs.unrar                        # the DirectX 6 SDK .EXE is a RAR self-extractor (7z cannot open it)
    pkgs.binutils                     # `strings` for the SP3 version-marker verify gate
    pkgs.gnutar                       # reproducible tar -cJf
    pkgs.xz                           # .tar.xz compression
    pkgs.python3                      # runs create-toolchain-release.py
    pkgs.wineWow64Packages.staging    # only if a step needs a Win unpacker; default path is 7z/unrar
  ];

  shellHook = ''
    export VC5_ISO="${vc5-iso}"
    export VS97_SP3_ZIP="${sp3-zip}"
    export DXSDK_EXE="${dxsdk-archive}"   # DirectX 6.0 SDK (archive.org directx6sdk)
    export NINJA_WIN_ZIP="${ninja-zip}"
    export GRUNTZ_DIR="$PWD"
    exec python3 ${./create-toolchain-release.py}
  '';
}
