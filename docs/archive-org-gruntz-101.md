# Preserving the Gruntz 1.01 update

The GooRoo download is currently the primary community location found for the
full-file update archive. Its included readme is Monolith's 1.01 patch note. A
byte-identical ModDB mirror exists, but no
dedicated Internet Archive item containing this exact 1.01 archive was found
in the audited Gruntz item manifests. The existing `gruntz-pc` item is the 1.00
CD image.

Prepare a complete upload directory without opening or running either patcher:

```sh
nix develop
scripts/prepare-gruntz-101-archive.sh
```

The command creates `build/archive-org/gruntz-1.01/` containing:

- the byte-exact GooRoo `Gruntz101.exe` full-file ZIP archive;
- its three extracted files (`Gruntz.exe`, `GRUNTZ.ZZZ`, `Info_101.txt`);
- the byte-exact Lady of the Cake `Grnt_101.zip` and its extracted RTPatch 4.11
  `Grnt_101.exe` apply artifact;
- `SHA256SUMS` and `MD5SUMS`;
- a provenance README;
- `upload-to-archive-org.sh`, with item metadata and an overridable identifier.

Inspect the generated README and confirm that uploading this commercial-game
update is permitted for the account/collection you will use. Then configure
the Internet Archive CLI and run the generated uploader yourself:

```sh
nix shell nixpkgs#python3Packages.internetarchive -c ia configure
nix shell nixpkgs#python3Packages.internetarchive -c \
  build/archive-org/gruntz-1.01/upload-to-archive-org.sh
```

The uploader intentionally does not store credentials and is not invoked by
the preparation command. Pass another unused identifier as its first argument
if `gruntz-v1-01-update-preservation` is already occupied.

The executable and seven-level content comparison is recorded in
[`version-1.00-to-1.01.md`](version-1.00-to-1.01.md); the full-file ZIP is the
matching target, while the RTPatch executable is preservation evidence only.

Known public sources:

- GooRoo update page: <https://gooroosgruntz.info/patch.html>
- GooRoo full-file archive: <https://gooroosgruntz.info/ZIPped/Gruntz101.exe>
- byte-identical ModDB mirror: <https://www.moddb.com/games/gruntz/downloads/gruntz-v101-update>
- historical RTPatch GUI delta: <https://www.ladyofthecake.com/gruntz/html/patch.html>
- 1.00 CD image item: <https://archive.org/details/gruntz-pc>
