{ pkgs }:
pkgs.runCommand "gruntz-toolchain-vc50" {
  src = pkgs.fetchurl {
    name = "gruntz-toolchain-vc50.tar.xz";
    url = "https://github.com/sushi-shi/gruntz-decomp/releases/download/toolchain-vc50-sp3-r3/gruntz-toolchain-vc50.tar.xz";
    sha256 = "sha256-sZgl957g2+6wlrAPxIa1OcaDqlcG8PXsXVOKWc5KeZ8=";
  };
  nativeBuildInputs = [ pkgs.gnutar pkgs.xz ];
} ''
  mkdir -p "$out"
  tar xf "$src" -C "$out" --strip-components=1
''
