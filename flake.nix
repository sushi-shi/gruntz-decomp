{
  description = "Gruntz (1999, Monolith / WAP32 engine) decompilation - Linux matching build environment";

  # Mechanism mirrors srp-survarium/vostok's flake: pinned nixpkgs + rust-overlay,
  # the engine-agnostic delinker reused as-is, objdiff from upstream prebuilt
  # binaries, the game binary fetched from the Internet Archive, and the original
  # MSVC toolchain packaged once + fetched as a release tarball, run under Wine.
  # Difference from vostok: Gruntz was built with MSVC *5.0* (PE linker 5.10), not
  # VS2008 - so the toolchain payload is older and the VS2008-specific SP1/CRT-
  # manifest/mspdbsrv machinery does not apply.
  inputs = {
    # Pinned to vostok's locked revs so the rust-nightly that produced the
    # delinker cargoHash below is reproduced exactly.
    nixpkgs.url = "github:NixOS/nixpkgs/64c08a7ca051951c8eae34e3e3cb1e202fe36786";

    rust-overlay = {
      url = "github:oxalica/rust-overlay/6cddd512fa2bf7231f098d3a2f92f6e4cff71e0a";
      inputs.nixpkgs.follows = "nixpkgs";
    };

    # The delinker is engine-agnostic (operates on any 32-bit PE + PDB). Pinned to
    # the revision whose Cargo.lock matches `cargoHash` below.
    vostok-delinker-src = {
      url = "github:srp-survarium/vostok-delinker/5118e2a36d9ab3797094259db8311cae11e0770d";
      flake = false;
    };
  };

  outputs = { self, nixpkgs, rust-overlay, vostok-delinker-src }:
    let
      system = "x86_64-linux";

      pkgs = import nixpkgs {
        inherit system;
        overlays = [ rust-overlay.overlays.default ];
      };

      # Nightly Rust - the delinker uses `#![feature(os_string_truncate)]`.
      rust = pkgs.rust-bin.nightly.latest.default.override {
        extensions = [ "rust-src" "rustfmt" "clippy" ];
      };
      nightly-rustPlatform = pkgs.makeRustPlatform { cargo = rust; rustc = rust; };

      # -----------------------------------------------------------------------
      # vostok-delinker - splits the original EXE into per-symbol COFF .obj files
      # ("target" objects) for objdiff. Needs a PDB; since no Gruntz PDB ever
      # shipped/leaked we synthesise a fake one (Ghidra + pdbgen, see docs).
      # HARD-REQUIRES a `.reloc` section (it rebuilds symbol relocations from the
      # PE base-reloc table). GRUNTZ.EXE keeps its `.reloc`; CLAW.EXE / MEDIEVAL.EXE
      # were linked with relocs stripped, so only Gruntz is delinkable.
      # cargoHash: reused from vostok's flake (delinker rev pinned above). Update by
      # running `nix build .#vostok-delinker` after bumping the input.
      # -----------------------------------------------------------------------
      vostok-delinker = nightly-rustPlatform.buildRustPackage {
        pname = "vostok-delinker";
        version = "0.1.0";
        src = vostok-delinker-src;
        cargoHash = "sha256-ry3TH1fz7Aj/JdbmlgQFFn29m8E7EQHyGaVXnZTEcXo=";
      };

      # -----------------------------------------------------------------------
      # objdiff - upstream's prebuilt Linux binaries (not in nixpkgs). Foreign ELF;
      # autoPatchelfHook rewrites the interpreter + RPATH into the store. `objdiff`
      # is the GUI, `objdiff-cli` the differ. objdiff supports x86 + COFF - exactly
      # our MSVC target. (Derivations lifted verbatim from vostok's flake.)
      # -----------------------------------------------------------------------
      objdiffVersion = "3.7.1";
      objdiffUrl = name:
        "https://github.com/encounter/objdiff/releases/download/v${objdiffVersion}/${name}";
      objdiffGuiLibs = with pkgs; [
        libGL libxkbcommon wayland fontconfig freetype
        libx11 libxcursor libxi libxrandr libxcb
      ];

      objdiff-cli = pkgs.stdenv.mkDerivation {
        pname = "objdiff-cli";
        version = objdiffVersion;
        src = pkgs.fetchurl {
          url = objdiffUrl "objdiff-cli-linux-x86_64";
          hash = "sha256-QNhW2gHgpnbA8zr1NOVi8JjNUORey2Tzs0ZBjHsmSuY=";
        };
        dontUnpack = true;
        nativeBuildInputs = [ pkgs.autoPatchelfHook ];
        buildInputs = [ pkgs.stdenv.cc.cc.lib ];
        installPhase = "install -Dm755 $src $out/bin/objdiff-cli";
      };

      objdiff = pkgs.stdenv.mkDerivation {
        pname = "objdiff";
        version = objdiffVersion;
        src = pkgs.fetchurl {
          url = objdiffUrl "objdiff-linux-x86_64";
          hash = "sha256-LpBPYyWPzuX5jm02WUovzqJQyqz+l8SbRURHDWgFqq8=";
        };
        dontUnpack = true;
        nativeBuildInputs = [ pkgs.autoPatchelfHook pkgs.makeWrapper ];
        buildInputs = [ pkgs.stdenv.cc.cc.lib ] ++ objdiffGuiLibs;
        installPhase = ''
          install -Dm755 $src $out/bin/objdiff
          wrapProgram $out/bin/objdiff \
            --prefix LD_LIBRARY_PATH : "${pkgs.lib.makeLibraryPath objdiffGuiLibs}"
        '';
      };

      # -----------------------------------------------------------------------
      # gruntz-exe - the decomp target, pulled straight out of the Internet Archive
      # CD image. The canonical /download/<id>/<iso>/<inner-path> URL 302s to a
      # data node; fetchurl follows it. English retail v1.0, 2,511,872 bytes,
      # MD5 81c7f648db99501bed6e1ee71e66e4a0.
      # -----------------------------------------------------------------------
      gruntz-exe = pkgs.fetchurl {
        name = "GRUNTZ.EXE";
        url = "https://archive.org/download/gruntz-pc/Gruntz.iso/GAME%2FGRUNTZ.EXE";
        sha256 = "sha256-cHPCU2EGrkzKMuPoLbIQAfMZZ4shTE6uLGicVJAoCLM=";
      };

      # -----------------------------------------------------------------------
      # Runtime DLLs - the proprietary third-party libraries the rebuilt EXE
      # LOADS to run (mirrors vostok's `vostok-libs`). NONE are needed at
      # build/link time: mss32/smackw32 are load-time imports satisfied by an
      # import lib we synthesise from the DLL (or the SDK); sfman32/ctl3d32 are
      # LoadLibraryA'd at runtime, so they create no link dependency at all. The
      # byte-match-critical static code (CRT/MFC/zlib) comes from the VC5
      # toolchain + zlib, NOT from these. See docs/runtime-dlls.md.
      #
      # Pulled straight out of the retail CD image on the Internet Archive (same
      # /download/<id>/<iso>/<inner-path> 302 trick as gruntz-exe; %2F-encode
      # the inner path). Confirmed present as loose files in GAME/. Hashes +
      # sizes were obtained by actually fetching each file
      # (`nix-prefetch-url --name <n> <url>`).
      # -----------------------------------------------------------------------

      # Miles Sound System v4.0g runtime (RAD Game Tools, (C) 1991-98). Digital
      # audio + XMI/MIDI + the AIL_DLS_* software wave synthesizer. 269,312 bytes.
      gruntz-mss32 = pkgs.fetchurl {
        name = "MSS32.DLL";
        url = "https://archive.org/download/gruntz-pc/Gruntz.iso/GAME%2FMSS32.DLL";
        sha256 = "sha256-rM/BX6WSTF3cwhAl81r5COTn7XV2tmSrVNcTfkUyPnU=";
      };

      # Smacker video runtime (RAD Game Tools, (C) 1994-97). The intro/cutscene
      # codec (SmackOpen/SmackDoFrame/...). 96,256 bytes.
      gruntz-smackw32 = pkgs.fetchurl {
        name = "SMACKW32.DLL";
        url = "https://archive.org/download/gruntz-pc/Gruntz.iso/GAME%2FSMACKW32.DLL";
        sha256 = "sha256-+bL9tevI5lnHrBMsIT/P0usFmhGVoSkSG7aMohaZ5eE=";
      };

      # SFMAN32.DLL - the Miles/AIL "SoundFont Manager" (the `SFManager` export
      # the EXE LoadLibraryA's to play Gruntz.SF2 / Gruntz4.SF2). This is the
      # MSS-4.x SoundFont companion to mss32 above, NOT Creative Technology's
      # similarly-named "SoundFont Master Manager" (the only sfman32.dll that is
      # freely downloadable - wrong vendor, wrong API, do not use it).
      #
      # OPTIONAL: it is LoadLibrary'd, so its absence only disables SoundFont
      # music (the game falls back to its other MIDI path); the EXE still links
      # and runs without it. It does NOT ship anywhere on the Gruntz retail CD
      # (verified: the in-ISO GAME/SFMAN32.DLL path returns a 0-byte body, i.e.
      # absent), and no clean standalone Miles/AIL sfman32.dll was located on
      # archive.org or the web. The right source is a contemporaneous MSS 4.x SDK
      # / redistributable. Best candidate not yet fetched/pinned:
      #   archive.org item `lithengine` (LithTech Jupiter SDK ISO, Monolith) -
      #   likely bundles the MSS redist incl. sfman32, but the in-ISO path is
      #   unknown (IA does not enumerate inner files; a 636 MB fetch to confirm).
      # Until a real copy is pinned, this is a fakeHash placeholder so the rest of
      # gruntz-runtime still builds (the runCommand tolerates its absence).
      # TODO: fetch a real MSS-4.x sfman32.dll, set name/url/sha256 below.
      # The SoundFont manager's source URL + sha256, kept in one attrset so the
      # build below can detect "still a placeholder" and skip realising a fetch
      # that would fail. To enable it: set `url` to the real MSS-4.x SDK/redist
      # location and `sha256` to the value `nix-prefetch-url --name SFMAN32.DLL`
      # prints. While `sha256 == fakeHash`, gruntz-sfman32 is never realised.
      gruntz-sfman32-src = {
        # TODO: replace with the real MSS-4.x SDK/redist URL once located.
        url = "https://archive.org/download/PLACEHOLDER-mss4-sdk/SFMAN32.DLL";
        sha256 = pkgs.lib.fakeHash; # TODO: pin once a genuine Miles sfman32 is found
      };
      gruntz-sfman32-available = gruntz-sfman32-src.sha256 != pkgs.lib.fakeHash;
      gruntz-sfman32 = pkgs.fetchurl {
        name = "SFMAN32.DLL";
        inherit (gruntz-sfman32-src) url sha256;
      };

      # GRUNTZ.REZ - the packed asset archive (graphics/sound/levels). Needed to
      # actually RUN the game, but NOT to build/link the EXE, and it is large
      # (77,253,149 bytes / ~74 MiB), so it is left as a fakeHash placeholder to
      # avoid a big fetch during scaffolding. Uncomment the real sha256 (printed
      # by `nix-prefetch-url --name GRUNTZ.REZ <url>`) when you want it pinned.
      gruntz-rez = pkgs.fetchurl {
        name = "GRUNTZ.REZ";
        url = "https://archive.org/download/gruntz-pc/Gruntz.iso/DATA%2FGRUNTZ.REZ";
        sha256 = pkgs.lib.fakeHash; # TODO: ~74 MiB; pin when you want the assets
      };

      # gruntz-runtime - assembles the runtime DLLs into one out dir, the way
      # vostok-libs exposes its third-party DLLs. mss32 + smackw32 are real and
      # always copied; sfman32 is optional (skipped cleanly while it is a
      # fakeHash placeholder so this derivation still builds). GRUNTZ.REZ is NOT
      # bundled here (large, and an asset archive rather than a DLL) - reference
      # `gruntz-rez` directly if you need it.
      gruntz-runtime = pkgs.runCommand "gruntz-runtime" {
        # Env var names use underscores (a hyphen is not a valid shell name).
        gruntz_mss32 = gruntz-mss32;
        gruntz_smackw32 = gruntz-smackw32;
        # Only wire sfman32 in once it is a real (pinned) fetch; otherwise it
        # stays out of the build graph entirely so the fakeHash never realises.
        gruntz_sfman32 = if gruntz-sfman32-available then gruntz-sfman32 else "";
      } ''
        mkdir -p "$out"
        cp "$gruntz_mss32"    "$out/MSS32.DLL"
        cp "$gruntz_smackw32" "$out/SMACKW32.DLL"
        if [ -n "$gruntz_sfman32" ]; then
          cp "$gruntz_sfman32" "$out/SFMAN32.DLL"
        fi
      '';

      # -----------------------------------------------------------------------
      # gruntz-toolchain - MSVC 5.0 SP3 (Gruntz's compiler: PE optional-header
      # linker 5.10; Rich linker build 8034, cvtres 5.00.1668 = the VS97 SP3
      # signature - see docs/compiler-detection.md). Target tools: cl 11.00.x,
      # link 5.10.7303, cvtres 5.00.1668. SAME packaging mechanism as
      # vostok-toolchain, older payload.
      #
      # This is the PREBUILT tarball placeholder (fakeHash until published). The
      # tarball is produced once from the original VC++ 5.0 media + VS97 SP3 by
      #     scripts/create-toolchain-release.nix  (fetches media, runs the .py)
      #     scripts/create-toolchain-release.py   (7z-extracts the InstallShield/
      #         CAB media - NOT VS2008's MSI admin-install path - applies SP3,
      #         assembles the layout below, reproducible tar -cJf)
      # then published as a GH release; fill url + sha256 here afterwards.
      # See docs/toolchain-vc50-sp3.md for the located media + step-by-step build.
      # Expected layout once unpacked:
      #     msvc/{bin/cl.exe,c1.exe,c2.exe,link.exe,cvtres.exe,mspdb*.dll}
      #     msvc/include, msvc/lib (incl. LIBCMT.LIB + NAFXCW.LIB static MFC 4.2)
      #     dx/{Include,Lib}     - DirectX 6 SDK (ddraw/dinput/dsound/dplay headers + import libs)
      #     ninja/ninja.exe
      # Also needed for matching (bundle here or as a sibling `gruntz-libs`):
      # static MFC (Gruntz statically links MFC), Miles Sound System (mss32) and
      # Smacker (smackw32) import libs.
      #
      # ONLY the base/recompile side needs this; the target-side delink does not,
      # so the default devShell omits it and is usable before the tarball exists.
      # Build it to get nix to print the real hash:  nix build .#gruntz-toolchain
      # -----------------------------------------------------------------------
      gruntz-toolchain = pkgs.runCommand "gruntz-toolchain-vc50" {
        src = pkgs.fetchurl {
          name = "gruntz-toolchain-vc50.tar.xz";
          url = "https://github.com/sushi-shi/gruntz-decomp/releases/download/toolchain-vc50-sp3/gruntz-toolchain-vc50.tar.xz";
          sha256 = "sha256-ZPmOGI8kE4cITshErqOvRZH9MIxXYcVD14nD7ZPURRE=";
        };
        nativeBuildInputs = [ pkgs.gnutar pkgs.xz ];
      } ''
        mkdir -p "$out"
        tar xf "$src" -C "$out" --strip-components=1
      '';

      # Tools common to both shells (analysis + diffing; no MSVC).
      commonTools = [
        rust
        objdiff
        objdiff-cli
        vostok-delinker
        pkgs.ghidra        # auto-analysis + fake-PDB generation (add the pdbgen script)
        pkgs.llvm          # llvm-pdbutil: inspect / synthesise PDBs
        pkgs.ninja         # native incremental driver for the matching build (configure.py -> build.ninja)
        pkgs.python3
        pkgs.ripgrep
        pkgs.file
        pkgs.xxd
        pkgs.jq
        pkgs.clang-tools     # clangd over compile_commands.json (reader only)
        pkgs.detect-it-easy  # diec: PE compiler/linker fingerprint confirmer (Rich/@comp.id)
      ];

    in {
      packages.${system} = {
        inherit vostok-delinker objdiff objdiff-cli gruntz-exe gruntz-toolchain
          gruntz-mss32 gruntz-smackw32 gruntz-rez gruntz-runtime;
        default = vostok-delinker;
      };

      # Both shells live under a single `devShells.${system}` attrset: Nix's
      # flake evaluator rejects the dynamic key `${system}` appearing in two
      # separate attrpaths (`devShells.${system}.default` + `.build`) with
      # "dynamic attribute already defined", so they must be siblings here.
      devShells.${system} = {
        # Default shell - works TODAY. Analysis (Ghidra/llvm-pdbutil),
        # target-side delink, and objdiff. No MSVC needed.
        default = pkgs.mkShell {
          name = "gruntz-decomp";
          packages = commonTools;
          shellHook = ''
            export GRUNTZ_DIR="$PWD"
            export GRUNTZ_EXE="${gruntz-exe}"
            echo "[gruntz] target EXE : $GRUNTZ_EXE"
            echo "[gruntz] tools      : vostok-delinker, objdiff(-cli), ghidra, llvm-pdbutil"
            echo "[gruntz] base/MSVC  : 'nix develop .#build' (needs the VC5 toolchain tarball)"
          '';
        };

        # Base/recompile shell - adds MSVC 5.0 under wine-staging. wine-staging is
        # carried over from vostok (it spawns mspdbsrv for cl /Zi); VC5 may not
        # need it, but it is a harmless superset.
        build = pkgs.mkShell {
          name = "gruntz-build";
          packages = commonTools ++ [ pkgs.wineWow64Packages.staging ];
          shellHook = ''
            export GRUNTZ_DIR="$PWD"
            export GRUNTZ_EXE="${gruntz-exe}"
            export WINEPREFIX="$GRUNTZ_DIR/binaries/.wineprefix"
            export WINEDEBUG="fixme-all,err-kerberos"
            export WINEDLLOVERRIDES="mscoree,mshtml="
            export GRUNTZ_TOOLCHAIN="${gruntz-toolchain}"
            export MSVC_DIR="${gruntz-toolchain}/msvc"
            export DXSDK_DIR="${gruntz-toolchain}/dx"
            export NINJA_DIR="${gruntz-toolchain}/ninja"
            # Proprietary runtime DLLs (mss32 + smackw32, and sfman32 once pinned).
            # These run ALONGSIDE the recompiled EXE under wine - none are needed
            # to build/link it. See docs/runtime-dlls.md.
            export GRUNTZ_RUNTIME="${gruntz-runtime}"
            echo "[gruntz] MSVC 5.0   : $MSVC_DIR/bin/cl.exe   (run under wine)"
            echo "[gruntz] runtime    : $GRUNTZ_RUNTIME (MSS32/SMACKW32 DLLs)"
            echo "[gruntz] target EXE : $GRUNTZ_EXE"
          '';
        };
      };
    };
}
