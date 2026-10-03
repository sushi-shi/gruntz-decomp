{ pkgs }:
{
  mss32 = pkgs.fetchurl {
    name = "MSS32.DLL";
    url = "https://archive.org/download/gruntz-pc/Gruntz.iso/GAME%2FMSS32.DLL";
    sha256 = "sha256-rM/BX6WSTF3cwhAl81r5COTn7XV2tmSrVNcTfkUyPnU=";
  };
  smackw32 = pkgs.fetchurl {
    name = "SMACKW32.DLL";
    url = "https://archive.org/download/gruntz-pc/Gruntz.iso/GAME%2FSMACKW32.DLL";
    sha256 = "sha256-+bL9tevI5lnHrBMsIT/P0usFmhGVoSkSG7aMohaZ5eE=";
  };
}
