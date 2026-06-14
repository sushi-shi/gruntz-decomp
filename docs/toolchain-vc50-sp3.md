# Gruntz byte-match toolchain: Visual C++ 5.0 + Visual Studio 97 SP3

The matching decompilation of `GRUNTZ.EXE` must be compiled with the **exact**
toolchain that built the retail binary. Compiler-fingerprint analysis
(`docs/compiler-detection.md`) pinned that to **Microsoft Visual C++ 5.0 (Visual
Studio 97), Service Pack 3** with HIGH confidence. This document records the
located installation media (with archive.org hashes so they can be pinned and
verified *without* a download), why SP3 specifically, the extraction recipe, and
the step-by-step TODO to actually build and publish the toolchain tarball.

Mechanism: the flake
`fetchurl`s a **prebuilt** toolchain tarball from a GitHub release, and a
separate `scripts/create-toolchain-release.{nix,py}` builds that tarball from the
original media. KEY DIFFERENCE: VC++ 5.0 media are **InstallShield/compressed
CABs, not MSI**, so extraction is `7z`, not `msiexec /a`.

Date: 2026-06-13. Status: media LOCATED + scaffolding wired; tarball NOT yet
built (no large downloads performed - metadata/hashes only).

---

## 1. Located media (archive.org)

All hashes below come from `https://archive.org/metadata/<identifier>` and let
us pin/verify the file before trusting Nix's own sha256. Sizes are exact bytes.

| identifier | title | size | md5 / sha1 | what it contains |
|---|---|---|---|---|
| `microsoft-visual-studio-97-professional-edition-disc-3` | Microsoft Visual Studio 97 Professional Edition - Disc 3 | 655,605,760 (655 MB) | md5 `bf71b8fc2d23c5de13734b189dc341cb`<br>sha1 `a3ade9b0681ffab34f058a7991f6b7aa25d342e1` | **VC++ 5.0 base** (cl/c1/c2/link/cvtres/mspdb, include, lib incl. LIBCMT.LIB + NAFXCW.LIB, mfc). **PRIMARY base.** |
| `Microsoft_Visual_Studio_97_Enterprise_Edition_Disc_3_93121_Microsoft_1996` | Microsoft Visual Studio 97 Enterprise Edition (Disc 3) | 665,247,744 (665 MB) | md5 `d6c3e2b836f73df832fcbfa07523e0f6`<br>sha1 `7527e3bc59bd5ecf347a668660e1bf7ba373a55d` | VC++ 5.0 base (Enterprise). **Backup base** - equivalent compiler/libs for matching. |
| `vs97sp3` | Visual C++ 5.0 / Visual Studio 97 Service Pack 3 | 95,851,751 (91.4 MB) | md5 `e25a5de59a663cd0b5bd3d1089f8adc8`<br>sha1 `10a818f34b7223d4cbd2c99efb0e52c9efcd3bfc` | **SP3 patch** (standalone ZIP - whole replacement binaries). **PRIMARY SP3.** |
| `Microsoft_Visual_Studio_97_Service_Pack_3_X03-50158_1997` | Microsoft Visual Studio 97 (Service Pack 3)(X03-50158)(1997) | 296,183,808 (296 MB) | md5 `afdd3e3a9089a7a9f6396f9f628cf4de`<br>sha1 `89c160ab2b593016e676cb457cc57fabdd36c233` | SP3 CD ISO. **Backup SP3** (if the ZIP disappears). |

### Which disc has the compiler

In Visual Studio 97 the multi-CD retail set splits the products by disc:
**Disc 1 = Visual Basic / Visual FoxPro, Disc 2 = Visual J++ / Visual InterDev,
Disc 3 = Visual C++ 5.0** (confirmed via BetaWiki's per-disc inventory and
corroborated by the Japanese-beta item being named `VS97ENT3.iso` = Enterprise
disc 3, and by Disc 3 being the largest disc in both editions - the compiler
plus its large include/lib/MFC trees). So **Disc 3** is the one we extract for
cl.exe/link.exe/cvtres/lib. The other discs are not needed for matching.

### Candidates examined and rejected

- `vc-5.0-ent-japanese-beta` (`VS97ENT3.iso`, 651 MB) - **Japanese + Beta**;
  wrong locale and pre-release, will not byte-match. (Confirms disc-3 = VC++.)
- `wincetoolkits` (`WINCE_TOOLKITS.iso`, 516 MB) - Windows CE toolkits for VC5,
  not the x86 desktop compiler; also its ISO carries **no md5/sha1** in the
  metadata API (derive pending), so it can't be pinned anyway.
- `vs-97-sample-projects`, `msdn97` / `microsoft-visual-studio-97-msdn-library`
  - samples / MSDN docs, no compiler.
- Pro/Ent Discs 1, 2, 4 - VB/FoxPro/J++/InterDev/SQL Server, no VC++ compiler.

### Backup source (WinWorld)

`https://winworldpc.com/product/microsoft-visual-stu/97-5x` hosts:
- Microsoft Visual Studio 97 **Professional** v5.00.7008 (716.51 MB)
- Microsoft Visual Studio 97 **Enterprise** (860.49 MB)
- Microsoft Visual Studio 97 Enterprise [Japanese] (883.13 MB)

WinWorld does **not** host a standalone VS97 SP3 - use the archive.org `vs97sp3`
ZIP (or the X03-50158 SP3 ISO) for the patch. Keep WinWorld as a fallback for
the base discs only.

---

## 2. Why Service Pack 3 specifically (the byte-match driver)

From `docs/compiler-detection.md` (HIGH confidence), GRUNTZ.EXE's Rich header
carries, decisively:

- **cvtres build 1668 -> `VS97 (5.0) SP3 cvtres 5.00.1668`** - a UNIQUE,
  exact SP3 match (the single most diagnostic record). cvtres was not updated
  after the SP, so an SP3-or-later VC5 toolchain produces exactly this stamp.
- the very *existence* of a Rich header dates the build to **>= VS97 SP3** (the
  Rich/@comp.id mechanism was introduced in that service pack).
- PE optional-header **linker 5.10** + Rich Linker512/build-8034 are consistent
  with the VC5 SP3 linker family (SP3 link self-reports **5.10.7303**).
- VC++ 4.x is pre-Rich and VC++ 6.0 uses different prodIDs/builds - both
  excluded.

**Target tool versions to verify after build:** `cl 11.00.x`, `link 5.10.7303`,
`cvtres 5.00.1668`. (cl itself emits no @comp.id, so it can't be fingerprinted
from the EXE - it comes from the matching SP3 toolchain and is validated by code
byte-diff.) See `docs/compiler-detection.md` for full provenance.

### Does SP3 ship whole files or patches?

**Whole files.** VS97 SP3 (both the standalone `vs97sp3.zip` and the X03-50158
CD) is a self-extracting archive of complete replacement binaries - the updated
versioned tools `cl.exe`, `c1.exe`, `c1xx.exe`, `c2.exe`, `link.exe`,
`cvtres.exe`, `mspdb*.dll`, plus refreshed libs. There is no binary-delta /
in-place patcher to run (unlike VS2008's `msiexec /p` MSP path). So applying SP3
is simply **overlaying its files over the base VC `bin/` (and any `lib/` it
updates)** - which is exactly what `step2_apply_sp3()` in the .py does.

---

## 3. Extraction recipe (7z, NOT msiexec)

VC++ 5.0 is pre-MSI: the VS97 CDs are **InstallShield with compressed CAB
cabinets**, and SP3 is a self-extracting whole-file archive. Everything unpacks
with `7z` - no Wine `msiexec /a`, no admin install, no MSI File-table key
matching.

```
# 1. Extract the VC++ 5.0 CD (VS97 Disc 3)
7z x -y vs97-pro-disc3-vc5.iso -o vc5-iso/
#    then expand any nested InstallShield/MSCAB cabinets:
7z x -y <each *.cab> -o <dest>/      # repeated until no cabs remain

# 2. Locate the VC tree (the dir whose bin/ holds CL.EXE)
#    VS97 layout: .../DevStudio/VC/{bin,include,lib}, .../DevStudio/VC/mfc/{include,lib,src}
#      bin/    -> CL.EXE C1.EXE C1XX.EXE C2.EXE LINK.EXE CVTRES.EXE MSPDB*.DLL ...
#      include/-> C/C++ headers
#      lib/    -> LIBCMT.LIB, LIBC.LIB, MSVCRT.LIB, OLDNAMES.LIB, kernel32.lib, ...
#      mfc/lib -> NAFXCW.LIB, NAFXCWD.LIB, MFC42.LIB (import), MFCS42.LIB, ...
#      mfc/include -> AFX*.H, etc.

# 3. Apply SP3 (overlay whole replacement files onto bin/ and lib/)
7z x -y vs97sp3.zip -o sp3/
cp sp3/.../{CL.EXE,C1.EXE,C1XX.EXE,C2.EXE,LINK.EXE,CVTRES.EXE,MSPDB*.DLL} vc5/bin/

# 4. Verify the SP3 markers
strings -a vc5/bin/LINK.EXE   | grep 5.10.7303   # SP3 linker
strings -a vc5/bin/CVTRES.EXE | grep 5.00.1668   # the UNIQUE SP3 fingerprint
```

If a particular VS97 layer turns out to be an InstallShield IS3 /
PackageForTheWeb archive that `7z` cannot open, fall back to `cabextract` or
`unshield` (note it here). The script `step1_vc5_base()` prints a diagnostic and
exits if `CL.EXE` is not found after extract+cab-expand.

### Final tarball layout (`gruntz-toolchain-vc50.tar.xz`)

```
msvc/bin/      cl.exe c1.exe c2.exe link.exe cvtres.exe mspdb*.dll ...   (SP3-overlaid)
msvc/include/  C/C++ + MFC 4.2 headers (mfc/include merged in)
msvc/lib/      LIBCMT.LIB  NAFXCW.LIB  + MFC42/CRT/import libs (mfc/lib merged in)
msvc/mfc-src/  MFC 4.2 sources (handy for matching inlined MFC bodies)
dx/Include/    DirectX 6 SDK headers   (PLACEHOLDER - see below)
dx/Lib/        DirectX 6 import libs   (PLACEHOLDER - see below)
ninja/ninja.exe
```

This matches the `gruntz-toolchain` derivation's expected unpack layout in
`flake.nix` (`msvc/{bin,include,lib}`, `dx/`, `ninja/`).

---

## 4. LIBCMT.LIB and NAFXCW.LIB (Function-ID signature source)

Per `docs/libraries-and-funcid.md`, GRUNTZ.EXE statically links the multithreaded
CRT and static-release MFC 4.2, so the two highest-value FID inputs are:

| file | what it is | from the VC5 install |
|---|---|---|
| `LIBCMT.LIB` | static **multithreaded** CRT (release) - also covers the bundled iostreams | `DevStudio\VC\lib\LIBCMT.LIB` |
| `NAFXCW.LIB` | static **MFC 4.2 release** (the `…42s` flavor) | `DevStudio\VC\mfc\lib\NAFXCW.LIB` |

`create-toolchain-release.py` merges `mfc/lib` into `msvc/lib`, so **both end up
co-located in `msvc/lib/`** after build, and `verify_sp3()` aborts if either is
missing. These libs MUST come from a **VC5** install (the MFC `42` token is
shared with VC4.2/VC6, but bodies differ) - and ideally the SP3 revision; if FID
hit-rate is low, retry with the other VC5 SP libs (caveat in funcid doc).

---

## 5. How the tarball is built (reproduce from media)

The toolchain tarball is **built, published, and pinned**: the flake's
`gruntz-toolchain` fetches it from the GitHub release with a real `sha256`, so
`nix develop .#build` works today. The steps below reproduce that tarball from
the original media. The base-disc and SP3 media hashes are pinned; what remains a
`pkgs.lib.fakeHash` placeholder is the DirectX 6 SDK (not yet located — see Open
items) and the `ninja-zip` hash.

1. **Download + pin the base disc.** Fetch the VC5 ISO and let Nix compute the
   real sha256 (it will error on the fakeHash and print "got: sha256-…"):
   ```
   nix-prefetch-url --type sha256 \
     'https://archive.org/download/microsoft-visual-studio-97-professional-edition-disc-3/Microsoft%20Visual%20Studio%2097%20Professional%20Edition%20-%20Disc%203.iso'
   ```
   Before trusting it, verify the download's md5 == `bf71b8fc2d23c5de13734b189dc341cb`
   (and sha1 == `a3ade9b0…`). Put the sha256 into `vc5-iso` in
   `scripts/create-toolchain-release.nix`.
2. **Download + pin SP3.** Same for `vs97sp3.zip` (verify md5
   `e25a5de59a663cd0b5bd3d1089f8adc8`), set `sp3-zip` sha256.
3. **Pin ninja.** Set the `ninja-zip` sha256 for v1.12.1 `ninja-win.zip`.
4. **(Optional) Locate + pin the DirectX 6 SDK.** Not found on archive.org
   during scaffolding (see below). When found, set `DXSDK_EXE` in the .nix
   shellHook and the `dxsdk-archive` sha256; otherwise the .py writes a `dx/`
   placeholder and continues.
5. **Run the build:**
   ```
   nix-shell scripts/create-toolchain-release.nix
   ```
   It 7z-extracts the ISO, expands cabs, locates the VC tree, overlays SP3,
   verifies `link 5.10.7303` + `cvtres 5.00.1668` + `LIBCMT.LIB`/`NAFXCW.LIB`,
   assembles `msvc/`+`dx/`+`ninja/`, and writes a reproducible
   `build/gruntz-toolchain-vc50.tar.xz`, printing its sha256.
6. **Publish the release** (the .py prints the exact command):
   ```
   gh release upload toolchain-vc50-sp3 build/gruntz-toolchain-vc50.tar.xz \
     --repo sushi-shi/gruntz-decomp --clobber
   ```
7. **Update the flake hash (only if you rebuilt the tarball).** The
   `gruntz-toolchain` fetchurl in `flake.nix` is already pinned to the published
   release (real URL + `sha256`). If you republish a new tarball, replace that
   `sha256` with the printed digest. `nix develop .#build` then provides MSVC 5.0
   SP3 under Wine.

### Reproducibility note

`step5_package()` normalises all tar
metadata (`--sort=name`, fixed `--mtime`, zeroed owner/group, `--format=gnu`), so
the same pinned media + tooling yield a byte-identical tarball on any machine.

---

## 6. Open items / flags

- **DirectX 6 SDK NOT located on archive.org** during scaffolding. Gruntz imports
  `DDRAW`/`DINPUT`/`DSOUND`/`DPLAYX` (DirectX 6 era); matching needs the DX6
  import libs + headers. Candidates to check later: an MSDN/Platform SDK disc of
  the era, or a dedicated "DirectX 6.0 SDK"/"DX6 SDK" item. Until then the `dx/`
  step is a `fakeHash`-only placeholder and the .py emits `dx/README.TODO`.
- **Exact linker build (8034 caveat).** `docs/compiler-detection.md` flags that
  the Rich linker build 8034 is tagged "Likely Libs" and not cleanly tied to a
  single named SP installer; the cvtres 1668 match already anchors SP3. After
  building, empirically link a tiny PE with this `link.exe` and compare its Rich
  `prodID 0x13` build to 8034; if it differs, hunt for the patched linker.
- **ninja sha256** in the .nix is a fakeHash placeholder (re-prefetch v1.12.1
  `ninja-win.zip` and set it).
