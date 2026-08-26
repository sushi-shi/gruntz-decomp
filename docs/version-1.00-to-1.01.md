# Gruntz 1.00 to 1.01: executable and level delta

This is the durable semantic audit behind the `decomp-1.01` source changes. It
separates facts read from the two complete executables and the seven replacement
levels from patch notes, community claims, and source-shape inferences.

The target and address migration are documented separately in
[`target-1.01-migration.md`](target-1.01-migration.md). Preservation and upload
instructions for both surviving update packages are in
[`archive-org-gruntz-101.md`](archive-org-gruntz-101.md).

## Compared artifacts

| artifact | bytes | SHA-256 |
|---|---:|---|
| English 1.00 `GRUNTZ.EXE` | 2,511,872 | `7073c2536106ae4cca32e3e82db21001f319678b214c4eae2c689c54902808b3` |
| English 1.01 `Gruntz.exe` | 2,512,896 | `ef636e84cd547efe3e835811aefa6cd20964dadb9c2b427aa13860e52b2228d4` |
| 1.01 `GRUNTZ.ZZZ` | 95,433 | `da53080e4b887a4bc375d2c9345c074948e6fc7ac3b11f3d030e279f1ee16f37` |

The executables have the same Rich-header/toolchain signature. The PE timestamps
are 1998-12-05 for 1.00 and 1999-07-27 for 1.01. The version resource is the only
changed leaf among 75 resource leaves: file/product version `1.0.0.76` became
`1.0.1.77`; the other 74 leaves are byte-identical. The executable grew by 1,024
bytes and its code moved in contribution bands, not by a single RVA delta.

The conservative body audit covered 7,036 ordinary 1.00 function bodies:

| classification | bodies |
|---|---:|
| globally unique complete-byte match | 2,814 |
| neighbour-placed complete-byte match | 821 |
| same normalized body, rel32 displacement fields only | 3,328 |
| genuinely changed | 70 |
| repeated and still ambiguous | 3 |

Thus 6,963 bodies (98.96%) retain the same instruction body after accounting
for placement and relative targets. This is an audit classifier, not a match
waiver: each 1.01 function still has to satisfy strict bytes and referents.

## Byte-proved executable changes

The following changes are structural enough to reconstruct directly. Function
names below are reconstructed identities; retail has no original PDB.

| area | 1.01 behavior |
|---|---|
| main-menu version | `CMenuState::BuildVersionString` always formats `Gruntz v%d.%d%d`, so `1.01` is not collapsed to `1.1` or the 1.00 two-component form |
| save/load dialogs | both ten-slot fill functions send `EM_LIMITTEXT` with a 32-character limit to edit controls `0x435` through `0x43e` |
| multiplayer join | `CMulti::WaitForConnect` retains its 60-second overall deadline and rebroadcasts `STAT_REQUEST_CONFIG` every 10 seconds while the joining machine is not the host |
| resolution caption | both video-resolution callbacks replaced a 64-byte `char` buffer plus `strcat` with a local `CString` plus `operator+=`; this accounts for two new EH funclets |
| fonts | the training font is fixed at 36 by 16 and the message font at 55 by 23, for both named-face and fallback creation; eight Bute integer lookups disappeared |
| 3D text | `CFontConfig::Draw3DText` gained a final centering flag; it adds `DT_CENTER` and computes horizontal centering only when requested, while retaining vertical centering |
| debug text helpers | the old page-manager wrapper moved to `0x115d70` and passes centering enabled. `0x115c90` is a distinct new surface helper whose owner/first parameter remain unproved |
| finish sound | the warpstone-exit path now tolerates a missing `GAME_FINISHLEVEL` cue and uses a 500 ms window instead of dereferencing null |
| REZ manager ABI | `CRezArchive` grew from `0x94` to `0xd4`; it now retains the 60-byte second header banner at offset `0x94`, NUL-terminates it at `0xd0`, and trims trailing spaces from index 58 downward |
| REZ banner setter | a new zero-reference one-argument member at `0x13c4d0` performs `strncpy(field, text, 60)` and terminates byte 60; its semantic name is inferred |
| REZ path lookup | both overloads use two 1,024-byte local buffers, reject input length 1,023 or greater, and bound-copy the resource component to 1,023 bytes |
| REZ name hash | `CRezArchiveEntryHashNode::Hash` now returns zero for a null entry instead of dereferencing it |

The two known `new CRezArchive` sites now request `0xd4` automatically from the
corrected class layout. `CGrunt::StepArrivalDrop` also contains the 1.01 giant-
rock fallback in both arrival branches; that behavior was already present in
the reconstructed source, so it required no semantic edit. `StepCompassMove`
and the Wire/Use candidates remain under ordinary structural matching because
the version comparison did not prove a source-level semantic change for them.

## What `GRUNTZ.ZZZ` contains

`GRUNTZ.ZZZ` is not a replacement `Gruntz.REZ`. It is a seven-member overlay
containing exactly the levels the official readme declares save-incompatible.
The 1.00 CD has no `GRUNTZ.ZZZ`; 1.01 loads this small overlay alongside the
base archive.

Across the seven WWDs, the decoded delta is 33 tile changes, 31 object
additions, 3 object removals, and 27 object edits. Every listed WWD checksum
changed. Retail uses that checksum as the saved-level identity, which explains
the per-level incompatibility without a save-file format change.

| level | 1.00 created | 1.01 created | checksum 1.00 | checksum 1.01 |
|---|---|---|---|---|
| W4L3 | 1998-11-10 | 1999-05-25 | `fe50e406` | `fe49de27` |
| W5L3 | 1998-11-10 | 1999-06-21 | `fe22bc0a` | `fe1de040` |
| W6L4 | 1998-11-09 | 1999-07-08 | `fc5a92ea` | `fc449ab8` |
| W7L2 | 1998-11-11 | 1999-05-25 | `fc43a8bd` | `fc329745` |
| W7L3 | 1998-11-09 | 1999-05-21 | `feb27e9b` | `fea42093` |
| W8L1 | 1998-11-10 | 1999-05-21 | `f690c88f` | `f6906d72` |
| W8L3 | 1998-11-12 | 2000-03-17 | `f3cf44d3` | `f3c46253` |

### W4L3

- Four tiles at `(9,22)` through `(9,25)` change from `1` to `0xfc`.
- Four `TileTrigger` objects are added around x=304 and y=719..813.
- Object 24 at `(114,818)` changes `LEVEL_GUMDROPS` logic from `EyeCandy`
  to `DoNothing`.

### W5L3

- Tile `(23,11)` changes from `0xe1` to `0xe3`.
- A vertical `ToobSpikez` object is added at `(786,368)`.

### W6L4

- Eleven tiles change in the x=12..16, y=34..36 area.
- A `Teleporter` is added at `(465,1105)`.
- Seven `CoveredPowerup` objects are added and one at `(939,46)` is removed.
- The advertised missing coin is the new `CoveredPowerup` at `(1869,1361)`
  with powerup ID 80 (`POWERUPZ_COIN`).
- Teleporter object 21616 at `(468,883)` changes `Speed X` from 15 to 19.

### W7L2

- Tiles `(41,14)`, `(42,14)`, and `(41,15)` change to `0xf9`.
- One `TileTriggerSwitch`, one `CoveredPowerup`, and three `TileTrigger`
  objects are added.
- The `RollingBall` at `(1264,49)` changes `Smarts` from 0 to 1.

### W7L3

- Nine `Brickz` objects are added around x=47..143 and y=684..816.
- No tiles change.

### W8L1

- The `RollingBall` at `(1552,1232)` changes `Smarts` from 0 to 1.
- No object count or tile changes occur.

### W8L3

- Fourteen tiles change around x=36..40 and y=36..38, including cleared
  `0xffffffff` cells.
- A coin icon at `(1232,1200)`, an `EyeCandy` sign at `(1680,2492)`, a
  spaceship at `(1382,2531)`, and a crate at `(1743,2538)` are added.
- A `Teleporter` at `(2097,2483)` and a `TileSecretTrigger` at `(1299,1168)`
  are removed.
- Twenty-one `TileSecretTrigger` objects in the x=1390..1460,
  y=1269..1460 block change damage from 8,000 to 12,000.
- Two more at `(1555,2035)` and `(1583,2130)` change damage from 2,000 to
  4,000.

## Patch notes and community claims

Monolith's included `Info_101.txt` names the missing W6L4 coin, two Easy Mode
problems, miscellaneous level bugs, and the same seven save-incompatible
levels. The decoded overlay proves the concrete level edits above.

The current GooRoo page additionally says the patch fixes a puddle problem.
That claim is not present in Monolith's included readme. The dedicated old/new
`gruntpuddle.c.obj` targets are byte-identical, and none of the seven WWD
deltas adds or edits a `Puddle` object. Treat the puddle claim as unproved: it
could describe an indirect effect of another changed function or a later site
annotation, but it is not evidence for changing `CGruntPuddle` source.

## Distribution provenance

The historical Lady of the Cake `Grnt_101.exe` is an RTPatch Professional 4.11
Apply program containing a binary delta. It is useful static evidence about
distribution, but cannot be a decompilation target because it is not the linked
post-update image. The developer-side RTPatch Build step would have compared a
complete old installation with a complete revised installation to produce that
payload.

GooRoo's misleadingly named `Gruntz101.exe` is instead an ordinary ZIP carrying
the complete 1.01 executable, the seven-level overlay, and the readme. That
complete executable is a valid target. The preservation script prepares both
forms without launching the GUI patcher; see
[`archive-org-gruntz-101.md`](archive-org-gruntz-101.md).
