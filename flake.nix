{
  description = "Gruntz (1999, Monolith / WAP32 engine) decompilation - Linux matching build environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/64c08a7ca051951c8eae34e3e3cb1e202fe36786";

    rust-overlay = {
      url = "github:oxalica/rust-overlay/6cddd512fa2bf7231f098d3a2f92f6e4cff71e0a";
      inputs.nixpkgs.follows = "nixpkgs";
    };

    vostok-delinker-src = {
      # PR srp-survarium/vostok-delinker#11 (fix/absolute-data-relocs): emit DIR32
      # for absolute data/code refs, keep REL32 for branches. Pinned to the branch
      # commit until it lands on master.
      url = "github:srp-survarium/vostok-delinker/8a42a0ba6f6b90651d62d1911eb97b80a5faa149";
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

      # gruntz-exe - the decomp target.
      # EN retail v1.0, 2,511,872 bytes
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

      # gruntz-runtime - assembles the runtime DLLs into one out dir. mss32 +
      # smackw32 are always copied; sfman32 is skipped cleanly while it is a
      # fakeHash placeholder.
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
      # msvc/bin includes MSDIS100.DLL (the VC5 disassembler link.exe imports at
      # load) - bundled by scripts/create-toolchain-release.py from the same VS97
      # Disc 3 ISO, alongside MSPDB50.DLL. Tarballs built before that fix lack it;
      # scripts/gruntz/build/msdis_stub.py supplies an export-only stub fallback at
      # link time so `gruntz link` works either way (link output is identical).
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
      # The CLI is `python -m gruntz` (gruntz.__main__ -> gruntz.cli.main); scripts/
      # is THE package root, put on PYTHONPATH so the package + every `python -m
      # gruntz.<x>` child import resolves.
      gruntz-cli = pkgs.writeShellScriptBin "gruntz" ''
        # Resolve the repo/worktree root robustly: GRUNTZ_DIR (set by the shell
        # hook), else walk up from $PWD to the dir holding scripts/gruntz/cli.py
        # (so `gruntz` works from build/, src/, any subdir), else git toplevel.
        d="''${GRUNTZ_DIR:-}"
        if [ -z "$d" ]; then
          p="$PWD"
          while [ "$p" != "/" ]; do
            if [ -f "$p/scripts/gruntz/cli.py" ]; then d="$p"; break; fi
            p="$(dirname "$p")"
          done
        fi
        [ -n "$d" ] || d="$(git rev-parse --show-toplevel 2>/dev/null || true)"
        if [ ! -f "$d/scripts/gruntz/cli.py" ]; then
          echo "gruntz: repo root not found (run inside the gruntz checkout, or set GRUNTZ_DIR)" >&2
          exit 2
        fi
        export GRUNTZ_DIR="$d"
        export PYTHONPATH="$d/scripts''${PYTHONPATH:+:$PYTHONPATH}"
        exec python3 -m gruntz "$@"
      '';

      # Wrap nvim to auto-load the in-repo editor/nvim plugin (:Gruntz), leaving the
      # user's own config intact. A wrapper SCRIPT on PATH (not a shell function)
      # survives `nix develop --command fish`, like gruntz-cli; the real nvim is
      # resolved before we shadow it, GRUNTZ_NVIM_WRAPPED guards nested shells, and
      # the banner announces the change.
      nvimShimHook = ''
        if [ -z "''${GRUNTZ_NVIM_WRAPPED:-}" ] && command -v nvim >/dev/null 2>&1 && [ -d "$GRUNTZ_DIR/editor/nvim" ]; then
          _gnv_real="$(command -v nvim)"
          _gnv_bin="$GRUNTZ_DIR/build/nvim-shim"
          mkdir -p "$_gnv_bin"
          printf '#!/bin/sh\nexec "%s" --cmd "set rtp^=%s/editor/nvim" "$@"\n' "$_gnv_real" "$GRUNTZ_DIR" > "$_gnv_bin/nvim"
          chmod +x "$_gnv_bin/nvim"
          export PATH="$_gnv_bin:$PATH"
          export GRUNTZ_NVIM_WRAPPED=1
          echo "[gruntz] nvim       : WRAPPED -> nvim now auto-loads editor/nvim (:Gruntz, vt/vb/vd/vs/vx/vi/V). Plain nvim is unchanged outside this shell." >&2
        fi
      '';

      # Tools common to both shells (analysis + diffing).
      commonTools = [
        gruntz-cli
        rust
        objdiff
        objdiff-cli
        vostok-delinker
      ] ++ (with pkgs; [
        (python3.withPackages (ps: [ ps.pyghidra ]))   # pyghidra + jpype1: headless Ghidra scripting
        ghidra
        ninja

        llvm            # llvm-pdbutil
        # clang-unwrapped provides the clang DRIVER (ghidra_metadata_generate/gen_labels, via
        # $GRUNTZ_CLANG) AND clangd / clang-format / clang-tidy. It MUST be the
        # UNWRAPPED build: the nix cc-wrapper injects host (x86_64-linux) gcc/glibc
        # include paths that shadow our /imsvc MSVC headers under
        # --target=i686-pc-windows-msvc. Verified two ways:
        #   - wrapped clang  -> ghidra_metadata_generate emits 0 structs;
        #   - wrapped clangd -> <string.h> resolves to glibc -> gnu/stubs-32.h
        #     (32-bit multilib stub) "file not found".
        # So we deliberately do NOT pull in `clang-tools` (whose clangd is the
        # wrapped one with exactly that bug); clang-unwrapped supplies clangd +
        # clang-format + clang-tidy directly, and ghidra_metadata_generate reaches its driver
        # via $GRUNTZ_CLANG.
        llvmPackages.clang-unwrapped

        ripgrep
        file
        xxd
        jq
        gdb             # dynamic this/ecx tracing of the game under wine (winedbg --gdb)
      ]);

      # ONE shell for everything. `nix develop` (default) == `nix develop .#build`:
      # analysis + target-side delink + objdiff AND the MSVC 5.0/wine recompile
      # side. `.#build` is kept as an alias so every existing script/brief spelling
      # (`nix develop .#build --command ...`) keeps working. Carries all analysis
      # tools + wine + jdk21 and exports the full MSVC/wine/Ghidra env.
      gruntzShell = pkgs.mkShell {
        name = "gruntz-decomp";
        packages = commonTools ++ [ pkgs.wineWow64Packages.staging pkgs.jdk21 ];
        shellHook = ''
          # Repo/worktree root, not $PWD — `nix develop` may be entered from a
          # subdir (it walks up to find flake.nix; so must we). git toplevel is
          # worktree-aware, which is exactly right for the worker pool.
          export GRUNTZ_DIR="$(git rev-parse --show-toplevel 2>/dev/null || echo "$PWD")"
          export GRUNTZ_EXE="${gruntz-exe}"
          export GRUNTZ_CLANG="${pkgs.llvmPackages.clang-unwrapped}/bin/clang"
          # scripts/ is THE package root: on PYTHONPATH so `python -m gruntz` and
          # every `python -m gruntz.<x>` (cli/match/analysis tools) import it.
          export PYTHONPATH="$GRUNTZ_DIR/scripts''${PYTHONPATH:+:$PYTHONPATH}"

          # Enable the repo-tracked pre-commit auto-format hook (idempotent).
          if [ "$(git -C "$GRUNTZ_DIR" config --local core.hooksPath 2>/dev/null)" != ".githooks" ]; then
            git -C "$GRUNTZ_DIR" config --local core.hooksPath .githooks 2>/dev/null \
              && echo "[gruntz] hooks      : pre-commit auto-format on (core.hooksPath=.githooks)" >&2
          fi

          export WINEPREFIX="$GRUNTZ_DIR/build/wineprefix"   # generated state lives under build/
          export WINEDEBUG="fixme-all,err-kerberos"
          export WINEDLLOVERRIDES="mscoree,mshtml="
          # The per-prefix wineserver is now kept alive across builds (warm `wine
          # cl` = fast rebuilds); it is a daemon shared by every `nix develop`
          # invocation on this prefix, so `nix develop --command` (e.g. the nvim
          # build loop) reconnects to it. Reap it when YOU leave an INTERACTIVE
          # shell; `gruntz clean` reaps it before removing the prefix.
          case "$-" in *i*) trap 'wineserver -k >/dev/null 2>&1 || true' EXIT ;; esac
          export GRUNTZ_TOOLCHAIN="${gruntz-toolchain}"
          export MSVC_DIR="${gruntz-toolchain}/msvc"
          export DXSDK_DIR="${gruntz-toolchain}/dx"
          export NINJA_DIR="${gruntz-toolchain}/ninja"
          # PyGhidra (replaces Jython): pyghidra.start() bootstraps the Ghidra JVM
          # via jpype so the headless apply/export scripts run as CPython3.
          export GHIDRA_INSTALL_DIR="${pkgs.ghidra}/lib/ghidra"
          export JAVA_HOME="${pkgs.jdk21}/lib/openjdk"

          # Proprietary runtime DLLs (mss32 + smackw32, and sfman32 once pinned).
          # These run ALONGSIDE the recompiled EXE under wine - none are needed
          # to build/link it. See docs/runtime-dlls.md.
          export GRUNTZ_RUNTIME="${gruntz-runtime}"

          # Banner -> stderr so stdout stays clean for `nix develop --command`
          # piping (e.g. gruntz status ... --json | jq).
          echo "[gruntz] target EXE : $GRUNTZ_EXE" >&2
          echo "[gruntz] MSVC 5.0   : $MSVC_DIR/bin/cl.exe   (run under wine)" >&2
          echo "[gruntz] tools      : vostok-delinker, objdiff(-cli), ghidra, llvm-pdbutil (analysis + delink + objdiff)" >&2
          echo "[gruntz] clang      : $GRUNTZ_CLANG (unwrapped; ghidra_metadata_generate/gen_labels)" >&2
          echo "[gruntz] runtime    : $GRUNTZ_RUNTIME (MSS32/SMACKW32 DLLs)" >&2
          echo "[gruntz] cli        : 'gruntz <cmd>' (init/build/clangd/format/status/labels/structs/ghidra-refresh/todo)" >&2
          echo "[gruntz] shell      : ONE shell - 'nix develop' (== '.#build'); everything (analysis + build/init) is here" >&2
          ${nvimShimHook}
          # `gruntz init` is idempotent - run it on startup (set GRUNTZ_SKIP_INIT=1
          # to skip, e.g. when you only need clang/ghidra_metadata_generate and not the Ghidra DB).
          # First run builds the local env incl. the Ghidra DB (minutes); afterwards
          # the heavy Ghidra step self-skips (exports present), so it is a fast no-op.
          if [ -n "$GRUNTZ_SKIP_INIT" ]; then
            echo "[gruntz] init       : skipped (GRUNTZ_SKIP_INIT set)" >&2
          else
            if [ ! -f "$GRUNTZ_DIR/build/ghidra-enrich/exports/functions.csv" ]; then
              echo "[gruntz] init       : first-time setup - building the Ghidra DB (~minutes) ..." >&2
            fi
            python3 -m gruntz init \
              || echo "[gruntz] init       : failed - fix + re-run 'gruntz init'" >&2
          fi
        '';
      };

    in {
      packages.${system} = {
        inherit vostok-delinker objdiff objdiff-cli gruntz-exe gruntz-toolchain
          gruntz-mss32 gruntz-smackw32 gruntz-runtime;
        default = vostok-delinker;
      };

      devShells.${system} = {
        # ONE shell for everything (analysis + MSVC 5.0/wine recompile). `.#build`
        # is an ALIAS of the default so every `nix develop .#build ...` spelling in
        # scripts/briefs keeps working. staging wine is used for mspdbsrv (VC5 may
        # not need it but it is a harmless superset).
        default = gruntzShell;
        build = gruntzShell;
      };
    };
}
