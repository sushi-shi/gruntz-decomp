# Gruntz editor — data-model notes (decomp RE)

Notes on the standalone WAP32 world editor (`GruntzEdit.exe`, and its WAP32-engine
siblings `WapWorld.exe` / `GMEdit.exe` for Get Medieval). The string dumps came from a
one-off manual pass over the editor binaries — not reproducible from this repo, so the
tables below are the surviving record. These notes back the version-independent type
knowledge ported into the game headers under `include/`, and document what is
editor-only and therefore **deliberately NOT in the game model**.

## Why the editor is useful for the GAME decomp

The editor and the game share the WAP32 engine's on-disk data formats. The editor's
property dialogs are a free, labeled field-by-field dump of the on-disk structs the
game's loader also parses:

- **WWD object record** — the editor's Object-Properties / Object-Rectangles /
  Object-Points / Object-Flags / Object-User-Values / Object-Misc-Values /
  Object-Hit-Info dialogs expose every field of the world-object record. Modeled in
  `include/Wwd/WwdFile.h` (game-side; the game's `CObject` and all gameplay
  subclasses are built from this record).
- **REZ/VRZ archive** — the editor and game share the Monolith "RezMgr Version 1"
  archive code. Directory-entry schema `{ Type (FOURCC), Name, Size, ID }` + the
  sorted-directory invariant modeled in `include/Rez/RezMgr.h`.

### Hard evidence the WWD struct is byte-shared editor <-> game

The map-validator format string is **byte-identical** in `GRUNTZ.EXE` and in the
editor binaries:

> `Plane %s: Bad map tile value (%i) at %i,%i`
> `Plane %s: Bad map image set value (%i) at %i,%i`

Same validation code => same plane/object data model on both sides. This is the
strongest available evidence that these structs are real and
version-independent.

## Editor-ONLY MFC classes — NOT part of the game

These classes exist only in the editor (an MFC SDI/MDI doc/view app). They are
mined from the editor's class-name + RTTI dumps (that same one-off pass). They must
**NOT** be added to the game model — the game (`GRUNTZ.EXE`) is not an MFC doc/view
app and contains none of these:

| Editor class | Role (MFC doc/view editor scaffolding) |
|---|---|
| `CWorldDoc`   | MFC `CDocument` — the open .WWD world document |
| `CWorldView`  | MFC `CView` — the tile/object editing canvas |
| `CMainFrame`  | MFC `CFrameWnd` — the editor main window/frame |
| `CDibG`       | DIB (device-independent bitmap) wrapper for tile/image rendering |
| `CPalId`      | palette-id helper for the editor's palette handling |

Other editor-only helper names seen in the dumps (`CGGC`, `CHWVP`, `CJIN`,
`TileDialog`, `SingleImage`) are likewise editor scaffolding and out of scope for
the game decomp.

## Other editor strings worth noting (not modeled as game types)

- Companion on-disk files seen alongside `.WWD`: `.PID`, `.RID`, `.PCX`,
  `FIXUP.WTF` (fixup tables), `.BMP`/mask export.
- The editor reuses the same resource keys as the game (image-set/prefix loading,
  `Z Coord` ladder, logic-name table) — confirming the shared `Logic:` field on the
  WWD record maps to the game's `CUserLogic` dispatch system.
