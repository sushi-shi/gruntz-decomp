#!/usr/bin/env bash
set -euo pipefail

expected_archive=36cf4cc8433d41d42a4450fe781cf40f972991c4f91caadaa44de8dfd924e39c
expected_exe=ef636e84cd547efe3e835811aefa6cd20964dadb9c2b427aa13860e52b2228d4
expected_zzz=da53080e4b887a4bc375d2c9345c074948e6fc7ac3b11f3d030e279f1ee16f37
expected_info=4a3139d9a1cf8917cf81b5005aff80e423e984cd15c232760da5ee4bdaa84a83
expected_rtpatch_zip=f387d14a6cb6359b01e7333410699c98637e546e8dec12debf66f77d296e70b1
expected_rtpatch_exe=b7787b81dec4e42f8a303de45077c207df1df262d7a8d3701254d25f058b3f3a

source_archive=${1:-${GRUNTZ_UPDATE_V101:-}}
source_rtpatch=${2:-${GRUNTZ_RTPATCH_V101:-}}
repo=${GRUNTZ_DIR:-$(git rev-parse --show-toplevel)}
output=${3:-$repo/build/archive-org/gruntz-1.01}

if [[ -z "$source_archive" || ! -f "$source_archive"
        || -z "$source_rtpatch" || ! -f "$source_rtpatch" ]]; then
    echo "usage: $0 [Gruntz101.exe [Grnt_101.zip [new-output-directory]]]" >&2
    echo "or enter nix develop so both pinned update artifacts are available" >&2
    exit 2
fi
if [[ -e "$output" ]]; then
    echo "refusing to overwrite existing bundle: $output" >&2
    exit 2
fi
if [[ $(sha256sum "$source_archive" | cut -d' ' -f1) != "$expected_archive" ]]; then
    echo "source archive hash is not the pinned GooRoo Gruntz 1.01 archive" >&2
    exit 1
fi
if [[ $(sha256sum "$source_rtpatch" | cut -d' ' -f1) != "$expected_rtpatch_zip" ]]; then
    echo "RTPatch ZIP hash is not the pinned Lady of the Cake artifact" >&2
    exit 1
fi

mkdir -p "$output"
install -m 0444 "$source_archive" "$output/Gruntz101.exe"
install -m 0444 "$source_rtpatch" "$output/Grnt_101.zip"
unzip -q -j "$output/Gruntz101.exe" -d "$output"
unzip -q -j "$output/Grnt_101.zip" -d "$output"

printf '%s  %s\n' \
    "$expected_archive" Gruntz101.exe \
    "$expected_exe" Gruntz.exe \
    "$expected_zzz" GRUNTZ.ZZZ \
    "$expected_info" Info_101.txt \
    "$expected_rtpatch_zip" Grnt_101.zip \
    "$expected_rtpatch_exe" Grnt_101.exe > "$output/SHA256SUMS"
(cd "$output" && sha256sum -c SHA256SUMS)
(cd "$output" && md5sum Gruntz101.exe Gruntz.exe GRUNTZ.ZZZ Info_101.txt \
    Grnt_101.zip Grnt_101.exe > MD5SUMS)

cat > "$output/README.txt" <<'EOF'
Gruntz 1.01 update preservation bundle
=======================================

Gruntz101.exe is the byte-exact file published at:
https://gooroosgruntz.info/ZIPped/Gruntz101.exe

Despite its .exe suffix it is an ordinary ZIP archive, not an executable
patcher. It contains the complete replacement Gruntz.exe, GRUNTZ.ZZZ, and the
publisher's Info_101.txt installation note. The extracted members are included
for discovery and independent verification; Gruntz101.exe is the provenance
artifact and should be preserved unchanged.

Archive SHA-256:
36cf4cc8433d41d42a4450fe781cf40f972991c4f91caadaa44de8dfd924e39c
Archive MD5:
16c50598279023463d3eb2935c7f4ba9

The same byte size and MD5 are listed by the independent ModDB mirror:
https://www.moddb.com/games/gruntz/downloads/gruntz-v101-update

Grnt_101.zip is the byte-exact download linked from:
https://www.ladyofthecake.com/gruntz/html/patch.html

Its Grnt_101.exe member is the historical RTPatch Professional 4.11 GUI apply
artifact. It carries binary delta records for GRUNTZ.EXE and GRUNTZ.ZZZ and is
included for static research and preservation; it is not a complete executable
target and was not run to prepare this bundle.

Bundle prepared by the Gruntz matching project. See SHA256SUMS and MD5SUMS.
EOF

cat > "$output/upload-to-archive-org.sh" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
here=$(cd "$(dirname "$0")" && pwd)
identifier=${1:-gruntz-v1-01-update-preservation}

if ! command -v ia >/dev/null; then
    echo "ia CLI not found; run through:" >&2
    echo "  nix shell nixpkgs#python3Packages.internetarchive -c $0 [identifier]" >&2
    exit 127
fi
if ia metadata "$identifier" >/dev/null 2>&1; then
    echo "refusing to add files to existing Archive.org item: $identifier" >&2
    echo "pass a new identifier as the first argument" >&2
    exit 2
fi

ia upload "$identifier" \
    "$here/Gruntz101.exe" \
    "$here/Gruntz.exe" \
    "$here/GRUNTZ.ZZZ" \
    "$here/Info_101.txt" \
    "$here/Grnt_101.zip" \
    "$here/Grnt_101.exe" \
    "$here/SHA256SUMS" \
    "$here/MD5SUMS" \
    "$here/README.txt" \
    --metadata="title:Gruntz v1.01 update preservation" \
    --metadata="creator:Monolith Productions" \
    --metadata="date:2000" \
    --metadata="mediatype:software" \
    --metadata="language:eng" \
    --metadata="subject:Gruntz; Monolith Productions; Windows game patch; software preservation" \
    --metadata="description:Byte-exact preservation of two Gruntz 1.01 update distributions: GooRoo's misleadingly named Gruntz101.exe ZIP with the complete replacement files, and Lady of the Cake's Grnt_101.zip containing the historical RTPatch Professional 4.11 GUI binary-delta apply artifact. Checksums and extracted members are included."
EOF
chmod 0755 "$output/upload-to-archive-org.sh"

echo "prepared $output"
echo "review README.txt, then run upload-to-archive-org.sh yourself"
