{
  description = "Gruntz (1999, Monolith / WAP32 engine) decompilation - Linux matching build environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/64c08a7ca051951c8eae34e3e3cb1e202fe36786";

    rust-overlay = {
      url = "github:oxalica/rust-overlay/6cddd512fa2bf7231f098d3a2f92f6e4cff71e0a";
      inputs.nixpkgs.follows = "nixpkgs";
    };

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

      # Nightly: the delinker uses `#![feature(os_string_truncate)]`.
      rust = pkgs.rust-bin.nightly.latest.default.override {
        extensions = [ "rust-src" "rustfmt" "clippy" ];
      };
      nightly-rustPlatform = pkgs.makeRustPlatform { cargo = rust; rustc = rust; };

      # vostok-delinker - splits the EXE into per-symbol COFF "target" objects for objdiff.
      vostok-delinker = nightly-rustPlatform.buildRustPackage {
        pname = "vostok-delinker";
        version = "0.1.0";
        src = vostok-delinker-src;
        cargoHash = "sha256-ry3TH1fz7Aj/JdbmlgQFFn29m8E7EQHyGaVXnZTEcXo=";
      };

      # objdiff - upstream prebuilt Linux binaries (not in nixpkgs), so foreign ELF
      # patched by autoPatchelfHook. Supports x86 + COFF, our MSVC target.
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

      # gruntz-exe - the decomp target. The IA /download/<id>/<iso>/<inner-path>
      # URL (inner path %2F-encoded) 302s to a data node; fetchurl follows it.
      # EN retail v1.0, 2,511,872 bytes, MD5 81c7f648db99501bed6e1ee71e66e4a0.
      gruntz-exe = pkgs.fetchurl {
        name = "GRUNTZ.EXE";
        url = "https://archive.org/download/gruntz-pc/Gruntz.iso/GAME%2FGRUNTZ.EXE";
        sha256 = "sha256-cHPCU2EGrkzKMuPoLbIQAfMZZ4shTE6uLGicVJAoCLM=";
      };

      # Runtime DLLs - proprietary libs the rebuilt EXE LOADS to run.

      # Miles Sound System v4.0g runtime. 269,312 bytes.
      gruntz-mss32 = pkgs.fetchurl {
        name = "MSS32.DLL";
        url = "https://archive.org/download/gruntz-pc/Gruntz.iso/GAME%2FMSS32.DLL";
        sha256 = "sha256-rM/BX6WSTF3cwhAl81r5COTn7XV2tmSrVNcTfkUyPnU=";
      };

      # Smacker video runtime (intro/cutscene codec). 96,256 bytes.
      gruntz-smackw32 = pkgs.fetchurl {
        name = "SMACKW32.DLL";
        url = "https://archive.org/download/gruntz-pc/Gruntz.iso/GAME%2FSMACKW32.DLL";
        sha256 = "sha256-+bL9tevI5lnHrBMsIT/P0usFmhGVoSkSG7aMohaZ5eE=";
      };

      # SFMAN32.DLL - the Miles/AIL "SoundFont Manager" (LoadLibraryA'd to play
      # Gruntz.SF2). NOT Creative's similarly-named "SoundFont Master Manager" (the
      # only freely-downloadable sfman32.dll - wrong vendor/API, do not use it).
      # OPTIONAL: LoadLibrary'd, so its absence only disables SoundFont music; the
      # EXE still links and runs. It does NOT ship on the Gruntz CD (the in-ISO
      # path returns a 0-byte body) and no clean Miles/AIL copy was located; the
      # right source is a contemporaneous MSS 4.x SDK/redist.
      #
      # Kept as a src attrset so the build can detect "still a placeholder" and
      # skip realising a fetch that would fail. While sha256 == fakeHash,
      # gruntz-sfman32 is never realised. To enable: set url to the real MSS-4.x
      # SDK/redist and sha256 to what `nix-prefetch-url --name SFMAN32.DLL` prints.
      gruntz-sfman32-src = {
        url = "https://archive.org/download/PLACEHOLDER-mss4-sdk/SFMAN32.DLL";
        sha256 = pkgs.lib.fakeHash; # TODO: pin once a genuine Miles sfman32 is found
      };
      gruntz-sfman32-available = gruntz-sfman32-src.sha256 != pkgs.lib.fakeHash;
      gruntz-sfman32 = pkgs.fetchurl {
        name = "SFMAN32.DLL";
        inherit (gruntz-sfman32-src) url sha256;
      };

      # GRUNTZ.REZ - packed asset archive. Needed to RUN the game, not to build the
      # EXE, and large (~74 MiB), so left a fakeHash placeholder (never realised) to
      # avoid a big fetch. Pin with `nix-prefetch-url --name GRUNTZ.REZ <url>`.
      gruntz-rez = pkgs.fetchurl {
        name = "GRUNTZ.REZ";
        url = "https://archive.org/download/gruntz-pc/Gruntz.iso/DATA%2FGRUNTZ.REZ";
        sha256 = pkgs.lib.fakeHash; # TODO: ~74 MiB; pin when you want the assets
      };

      # gruntz-runtime - assembles the runtime DLLs into one out dir. mss32 +
      # smackw32 are always copied; sfman32 is skipped cleanly while it is a
      # fakeHash placeholder. GRUNTZ.REZ is not bundled (use `gruntz-rez` directly).
      gruntz-runtime = pkgs.runCommand "gruntz-runtime" {
        # Env var names use underscores (a hyphen is not a valid shell name).
        gruntz_mss32 = gruntz-mss32;
        gruntz_smackw32 = gruntz-smackw32;
        # Keep sfman32 out of the build graph until pinned, so fakeHash never realises.
        gruntz_sfman32 = if gruntz-sfman32-available then gruntz-sfman32 else "";
      } ''
        mkdir -p "$out"
        cp "$gruntz_mss32"    "$out/MSS32.DLL"
        cp "$gruntz_smackw32" "$out/SMACKW32.DLL"
        if [ -n "$gruntz_sfman32" ]; then
          cp "$gruntz_sfman32" "$out/SFMAN32.DLL"
        fi
      '';

      # gruntz-toolchain - MSVC 5.0 SP3, Gruntz's compiler (link 5.10.7303, cvtres 5.00.1668).
      #
      # Expected unpacked layout:
      # * msvc/{bin,include,lib (LIBCMT+NAFXCW static MFC)}
      # * dx/{Include,Lib} (DirectX 6 SDK)
      # * ninja/ninja.exe.
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

      # `gruntz` as a real PATH executable so the CLI works in ANY shell (bash, fish,
      # zsh) - a shellHook function would not survive `nix develop --command fish`.
      gruntz-cli = pkgs.writeShellScriptBin "gruntz" ''
        exec python3 "''${GRUNTZ_DIR:-$PWD}/scripts/gruntz.py" "$@"
      '';

      # Tools common to both shells (analysis + diffing; no MSVC).
      commonTools = [
        gruntz-cli
        rust
        objdiff
        objdiff-cli
        vostok-delinker
      ] ++ (with pkgs; [
        ghidra
        ninja
        python3
        ripgrep
        file
        xxd
        jq

        llvm            # llvm-pdbutil
        clang-tools     # clangd
        llvmPackages.clang-unwrapped # clang DRIVER for gen_structs/gen_labels.
                             # MUST be UNWRAPPED: the nix cc-wrapper injects host
                             # (x86_64-linux) flags that break --target=i686-pc-
                             # windows-msvc (verified: wrapped clang yields 0 structs).
                             # Reached via $GRUNTZ_CLANG so it never clashes with
                             # the wrapped clang clangd may pull in.
      ]);

    in {
      packages.${system} = {
        inherit vostok-delinker objdiff objdiff-cli gruntz-exe gruntz-toolchain
          gruntz-mss32 gruntz-smackw32 gruntz-rez gruntz-runtime;
        default = vostok-delinker;
      };

      # Both shells must be siblings under one `devShells.${system}` attrset: a
      # dynamic key in two separate attrpaths trips "dynamic attribute already
      # defined".
      devShells.${system} = {
        # Default shell - no MSVC: analysis, target-side delink, objdiff.
        default = pkgs.mkShell {
          name = "gruntz-decomp";
          packages = commonTools;
          shellHook = ''
            export GRUNTZ_DIR="$PWD"
            export GRUNTZ_EXE="${gruntz-exe}"
            export GRUNTZ_CLANG="${pkgs.llvmPackages.clang-unwrapped}/bin/clang"

            echo "[gruntz] target EXE : $GRUNTZ_EXE"
            echo "[gruntz] tools      : vostok-delinker, objdiff(-cli), ghidra, llvm-pdbutil"
            echo "[gruntz] clang      : $GRUNTZ_CLANG (unwrapped; gen_structs/gen_labels)"
            echo "[gruntz] cli        : 'gruntz <cmd>' (status/labels/structs/ghidra-refresh/todo)"
            echo "[gruntz] base/MSVC  : 'nix develop .#build' for 'gruntz build'/'init' (VC5 + wine)"
          '';
        };

        # Base/recompile shell - MSVC 5.0 under wine-staging (staging is used for
        # mspdbsrv; VC5 may not need it but it is a harmless superset).
        build = pkgs.mkShell {
          name = "gruntz-build";
          packages = commonTools ++ [ pkgs.wineWow64Packages.staging ];
          shellHook = ''
            export GRUNTZ_DIR="$PWD"
            export GRUNTZ_EXE="${gruntz-exe}"
            export GRUNTZ_CLANG="${pkgs.llvmPackages.clang-unwrapped}/bin/clang"

            export WINEPREFIX="$GRUNTZ_DIR/build/wineprefix"   # generated state lives under build/
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
            echo "[gruntz] cli        : 'gruntz <cmd>' (init/build/clangd/status/labels/structs/ghidra-refresh/todo)"
            # `gruntz init` is idempotent - run it on startup. The first run builds the
            # local env incl. the Ghidra DB (minutes); afterwards the heavy Ghidra step
            # self-skips (exports present), so it is a fast no-op.
            if [ ! -f "$GRUNTZ_DIR/build/ghidra-enrich/exports/functions.csv" ]; then
              echo "[gruntz] init       : first-time setup - building the Ghidra DB (~minutes) ..."
            fi
            python3 "$GRUNTZ_DIR/scripts/gruntz.py" init \
              || echo "[gruntz] init       : failed - fix + re-run 'gruntz init'"
          '';
        };
      };
    };
}
