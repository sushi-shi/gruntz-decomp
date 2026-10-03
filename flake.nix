{
  description = "Gruntz source build with MSVC 5.0 SP3";
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/64c08a7ca051951c8eae34e3e3cb1e202fe36786";
  outputs = { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
      toolchain = import ./nix/toolchain.nix { inherit pkgs; };
      runtime = import ./nix/runtime.nix { inherit pkgs; };
      launcher = pkgs.writeShellApplication {
        name = "gruntz-play";
        runtimeInputs = [ pkgs.python3 pkgs.wineWow64Packages.staging pkgs.gamescope
                          pkgs.bash pkgs.coreutils ];
        text = ''
          export MSVC_DIR="${toolchain}/msvc"
          export DXSDK_DIR="${toolchain}/dx"
          export GRUNTZ_MSS32="${runtime.mss32}"
          export GRUNTZ_SMACKW32="${runtime.smackw32}"
          export WINEDLLOVERRIDES="mscoree,mshtml="
          export WINEDEBUG="fixme-all,err-kerberos"
          exec python3 "$PWD/play.py" "$@"
        '';
      };
    in {
      apps.${system}.default = {
        type = "app";
        program = "${launcher}/bin/gruntz-play";
      };
      devShells.${system}.default = pkgs.mkShell {
        packages = [ pkgs.python3 pkgs.wineWow64Packages.staging
                     pkgs.llvmPackages.clang pkgs.llvmPackages.lld ];
        GRUNTZ_CLANG = "${pkgs.llvmPackages.clang-unwrapped}/bin/clang";
        MSVC_DIR = "${toolchain}/msvc";
        DXSDK_DIR = "${toolchain}/dx";
        shellHook = ''
          export WINEPREFIX="$PWD/build/wineprefix"
          export WINEDEBUG="fixme-all,err-kerberos"
          export WINEDLLOVERRIDES="mscoree,mshtml="
        '';
      };
    };
}
