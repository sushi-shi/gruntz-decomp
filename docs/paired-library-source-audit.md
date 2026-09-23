# Paired library source-evidence audit

Executed 2026-09-23 on main `da8fc1ac5`, separately from PR #79. This is an
artifact/search report, not a replacement wall queue or adoption ledger.

## Result

- Inventoried all **31 AVP2 Debug/Release library pairs**: 228 paired members
  plus two Release-only members. Walked 136,898 Debug symbol records, including
  35,222 frame-relative parameter/receiver/local records, without a parse error.
- Found **206 distinct exact-name Gruntz baseline overlaps present in both
  configurations**. All already have historical MAX 100. These are candidates,
  not 206 newly proven identities or matches.
- The signature-relaxed, below-historical-100 search finds only
  `CButeMgr::Save` (three library configurations). Its source decision is
  `bute-save-debug-long-local` in `config/lithtech_lineage.tsv`; existing revision
  decisions remain `bute-save-adaptation` and `nolf-bute-boundary-revisions`.
- Tested the one supported local-type correction. The **entire normalized
  ButeMgr object is byte-identical before/after**, including its referents:
  SHA256 `5dae99adbcb9b5f3713df5cc796413097ce392f45a42235a4a2f5b305b4a05c5`.
  No new exact function and no matching regression is claimed.
- Also inspected the two library pairs in `tools/ltdmtest` at the pinned
  LithTech revision. Their 78 Rez and 35 Lith exact-name overlaps are already
  historical 100; neither adds a below-100 same-owner candidate.
- The checked-out `FEAR/` subtree contains no `.lib`, `.obj`, or `.pdb` files.
  This is a statement about that checkout, not every F.E.A.R. distribution.

No additional math class or helper import is justified by this pass. The
existing source-adoption decisions remain in the canonical lineage ledger.
Unrelated-name structural analogues are not ruled out by a symbol-name census;
this pass does not claim to have exhausted all future binary/source searches.

## Provenance and reproduction

AVP2 is the licensed, read-only game-source v1.0.9.6 release. The user confirmed
permission to consult it. The archive is `si_avp2_sourcecode_v1.rar`, SHA256
`62b58050bb0e7542b69efd61d602cd2db7fa87e5602a559f4ab726408775ad9d`, from the
[public release archive](https://archive.thedatadungeon.com/aliens_versus_predator_2_2001/distributions/2002-07-15_v1.0.9.6_game_source_code/).
Libraries live under `si_avp2_sourcecode_v1/proj/LT2/lithshared/libs/`.
No reference binaries or source files are redistributed in this change.

Run from the repository's pinned environment, substituting the local extraction
path. Output is JSON; redirect it to an ignored/local artifact if desired.

```sh
nix develop
gruntz lineage objects --debug /path/to/libs/debug --release /path/to/libs/release
gruntz lineage objects --debug /path/to/libs/debug/ButeMgr.lib --release /path/to/libs/release/ButeMgr.lib --member ButeMgr.obj
gruntz lineage objects --debug /path/to/libs/debug/CryptMgr.lib --release /path/to/libs/release/CryptMgr.lib --member CryptMgr.obj
PYTHONPATH=scripts python3 -m unittest gruntz.lineage.test_objects gruntz.lineage.test_lineage
```

The census records library/member SHA256 hashes, unpaired files/members,
defined code symbols, compiler records where decoded, type-server references,
and exact versus changed-signature baseline candidates. `--member` adds every
symbol/type record, preserving unknown payloads rather than silently discarding
them. Windows path spelling and case are normalized only for member pairing;
ambiguous basenames are rejected. Mangled names are not rewritten for ABI
matching. An absent Release definition is not counted as a paired definition.

The secondary source is LithTech `845119c`, with these Git blob identities:

| Artifact | Git blob |
|---|---|
| `tools/ltdmtest/RezMgrd.lib` | `8db90c770ed3c6c7262404650b9419b1eb6f19bd` |
| `tools/ltdmtest/RezMgr.lib` | `0a22906f156b9eaafc88422384f4a6b0cddfef77` |
| `tools/ltdmtest/lithd.lib` | `ed5562b1687a0ef5f2540f440c98643801559041` |
| `tools/ltdmtest/lith.lib` | `1bea4b19b0a86c4fa61c12469455f4a5bdb3c345` |

## Census coverage

Counts are per configuration group, not unique cross-library owners.

| Library group | Pairs | Paired members | Exact-name overlap per pair |
|---|---:|---:|---:|
| ButeMgr / MfcDll / NoMFC | 3 | 12 | 20 / 17 / 20 |
| CryptMgr / MfcDll | 2 | 4 | 6 / 6 |
| dibmgr / dll | 2 | 6 | 19 / 19 |
| RezMgr / Rezmgrfull | 2 | 6 | 78 / 1 |
| RegMgr | 1 | 1 | 9 |
| lith | 1 | 10 | 35 |
| MFCStub | 1 | 3 | 1 |
| Zlib | 1 | 14 | 38 |
| StdLith and five configurations | 6 | 82 | 0 |
| GameSpyClientMgr / Demo | 2 | 20 | 0 |
| GameSpyMgr / Demo | 2 | 4 | 0 |
| LithFontMgr_LT2 | 1 | 5 | 0 |
| lithtrackmgr / LithTrackMgrClient | 2 | 32 | 0 |
| ltguimgr_LT2 | 1 | 16 | 0 |
| controlfilemgr / GenRegMgr / Redmgr | 3 | 3 | 0 |
| WONAPI | 1 | 10 | 0 |

WONAPI's additional Release-only `gqueryreporting.obj` and `nonport.obj` were
also scanned; neither has an exact-name Gruntz baseline overlap.

Each AVP2 ButeMgr variant contains only `Stdafx.obj`, `ButeMgr.obj`,
`AVector.obj`, and `ARange.obj`. It does not contain the earlier NOLF
`ptins.obj`/`ptadd.obj` pair that supplied the previous Patricia-tree closures.

## What the records actually prove

The standard LLVM 21.1.8 CodeView reader rejects the first CryptMgr member's
legacy type stream. The new reader instead walks C11 length-prefixed records
using the public [Microsoft record definitions](https://github.com/microsoft/microsoft-pdb/blob/master/include/cvinfo.h).
It validates lengths and scope balance and reports unsupported payloads.
It is deliberately not a PDB/type-layout resolver or an instruction matcher.

Three controls matter:

1. CryptMgr's primary `.debug$S` contribution has signature 2; its subsequent
   COMDAT contributions do **not** repeat that signature. They retain the
   `ios::eof`, `istream::gcount`, and `ostream::put` helper bodies. Dropping
   those sections would manufacture an absence of inline-boundary evidence.
2. CryptMgr has embedded types. Array type `0x116f` is an `LF_ARRAY_ST` with
   element `0x70` (char), index `0x11`, and eight-byte extent; its Encrypt and
   Decrypt local records reference it. This independently corroborates the
   already restored buffer family, not a new union or math-type opportunity.
3. **117 of 228 Debug members reference external type-server PDBs.** ButeMgr's
   complex type IDs refer to `vc60.pdb`; its single `LF_TYPESERVER_ST` record is
   not a local type definition at index 0x1000. Primitive IDs remain directly
   interpretable. The reader reports missing type authority instead of
   inventing class layouts from those IDs. Sixteen paired Release members also
   contain type-server references; most have no debug records at all.

Scope membership and emitted record order are reported separately. Debug frame
positions and record order alone do not prove optimized registers, source
declaration order, or VC5 inline decisions. Inspect the complete surviving
declaration and paired Release instructions, then test against Gruntz retail.

## Verification

The parser tests cover sibling scopes sharing a frame slot, signed locations,
bare continuation sections, raw unknown records, external PDB references,
truncated records, unbalanced scopes, ambiguous archive members, changed
signatures, and a full archive-to-COFF-to-CLI integration path with a malformed
input negative control. The licensed corpus itself is a separate integration
check, not required by the unit tests.

The source A/B and full `gruntz build` passed: **3,844 / 4,426 exact, 95.66%
full-engine fuzzy**, unchanged from the main-based baseline, with no fresh MAX
failure. PR #79 remains separate and unmerged.

`gruntz lineage verify --complete` is **not green on the parent main**: its
existing ledger produces 30 findings (reason-code/decision mismatches, duplicate
source claims, comma-separated RVAs, and landing-commit fields). This pass does
not relax that verifier or rewrite unrelated decisions. The newly added local-
type row is validated separately, including its actual landing commit. These
ledger-contract issues are distinct from the passing build/MAX gates above.
