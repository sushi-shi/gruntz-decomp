#ifndef FORMATS_REZ_H
#define FORMATS_REZ_H

/*
 * REZ / VRZ archive directory format — "RezMgr" (Monolith resource manager).
 *
 * Gruntz assets live in Gruntz.REZ (main) + GRUNTZ.VRZ (video/secondary). The
 * archive is the Monolith "RezMgr" format; its banner string is embedded verbatim:
 *   "RezMgr Version 1 Copyright (C) 1995 MONOLITH INC."
 * (mined from the editor binaries, build/editor-strings/*.ascii.txt; the same
 * RezMgr code is shared between the game's RezSync/CRezDir loader and the editor.)
 *
 * VERSION-INDEPENDENT: the on-disk archive layout is a stable Monolith format,
 * identical between game and editor.
 *
 * PROVENANCE: the directory-entry FIELD SET { Type (FOURCC), Name, Size, ID } and
 * the sorted-directory invariant are derived from the editor's resource-management
 * strings (e.g. "Rez Name = %s ID = %i", "Type", and the load-failure assert) and
 * the game's loader assert
 *   "CRezDir::Load Failed! (File is not sorted!)"
 * which proves the directory MUST be sorted (the loader walks it in order and
 * asserts on an out-of-order entry). NOTE: the function tomalla labeled
 * "CRezDir::FindEntry binary search" (@0x13c080) is, by its bytes, a filesystem
 * STAT (it stat's the name via 0x18c780 and returns whether the entry's attribute
 * dword has the directory bit 0x4000) — NOT an in-memory binary search; the
 * sorted-dir invariant is enforced by CRezDir::Load's recursive walk instead.
 * Exact on-disk byte offsets/widths and header layout are NOT recovered — @todo.
 *
 * CONTAINER OFFSETS — CONFIRMED (was @unconfirmed). Reversed byte-exact from the
 * Gruntz RezMgr code, not from OpenClaw (Gruntz's container differs from CLAW
 * REZ). Pinned by the in-memory node ctors @0x13c540/@0x13c940 (operator-new
 * sizes 0x24 / 0x38) and the directory walk Load @0x13a0f0 / FindEntry @0x13c080.
 * See src/Rez/RezMgr.{h,cpp} for the byte-matched source; the IN-MEMORY tree-node
 * layouts (NOT the on-disk record layout, still @todo) are:
 *
 *   CRezItmBase (16 B) — shared node base; ctor @0x13c4e0:
 *     +0x00 vtable (base @0x5ef768)   +0x0c parent pointer
 *
 *   CRezItm : CRezItmBase (0x24 = 36 B) — leaf resource/file node; ctor @0x13c540:
 *     +0x00 vtable @0x5ef788 (derived)   +0x10 = 0   +0x14 = 0
 *     +0x18/+0x1c set by the load        +0x20 = -1 (resource id/handle, unset)
 *
 *   CRezDir : CRezItmBase (0x38 = 56 B) — subdirectory node; ctor @0x13c940:
 *     +0x00 vtable @0x5ef7a8 (derived)
 *     +0x10/+0x1c = 0x5ef7c8 (an embedded child-collection's two vtables),
 *     +0x14 head = 0, +0x18 tail = 0   (the list at +0x10 is appended via 0x1851e0)
 *     +0x20/+0x24/+0x28/+0x34 = 0      +0x2c = owning RezMgr back-pointer (ctor arg)
 *     +0x30 = 1 (initialized flag)
 *
 *   NOTE: the recursively-walked dir NODE (Load's `this`, CRezDirNode in the src)
 *   is a THIRD, distinct layout (it reuses +0x10 as a payload SIZE, +0x18 as the
 *   archive-source pointer, +0x48 as the loaded-payload buffer / "already-loaded"
 *   gate, +0x38 as its child collection). All three "CRezDir"-labeled functions
 *   in tomalla's notes operate on different node classes.
 *
 * On-disk record FIELD SET (Type/Name/Size/ID/offset/date) + the sorted-dir /
 * recursive-tree BEHAVIOUR remain as below; the on-disk byte offsets are @todo.
 *
 * See ../managers/rezsync.h for the RezSync / CRezDir loader classes (the in-memory
 * directory tree built from this on-disk format).
 */

#undef UNICODE
#undef _UNICODE
#include <afxwin.h>

/* FOURCC resource type tag (4 ASCII chars), e.g. the archive's per-entry "Type". */
typedef char RezFourCC[4];

/*
 * RezDirEntry — one directory entry within a REZ/VRZ archive.
 * @size: unknown @todo (offsets/widths/string encoding not confirmed)
 *
 * The directory is kept SORTED (by Name and/or Type+ID) so the loader can binary-
 * search it; an unsorted directory triggers "CRezDir::Load Failed! (File is not
 * sorted!)".
 */
struct RezDirEntry
{
    //@offset: ? @todo
    RezFourCC type;   // "Type" — FOURCC resource-type tag (4 chars)
    //@offset: ? @todo
    CString   name;   // "Rez Name" — entry name (on disk: fixed/var-len char buffer)
    //@offset: ? @todo
    int       size;   // "Size" — payload byte length
    //@offset: ? @todo
    int       id;     // "ID" — numeric resource id ("ID = %i")
    //@todo: also expected on disk: offset/position of the payload within the
    //       archive (not surfaced as an editor label, so unmodeled here).
};

/*
 * Resource keys (the %s-formatted namespaces resolved through the directory):
 *   GRUNTZ_<type>, IMAGEZ_%s, ANIZ_%s, SOUNDZ_%s, VOICES_%s, POWERUPZ_%s, …
 * (see STRINGS_ANALYSIS.md / ../managers/rezsync.h). Companion editor files seen
 * alongside .REZ: .PID, .RID, .WWD, .PCX, "FIXUP.WTF".
 */

#endif /* FORMATS_REZ_H */
