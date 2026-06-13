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
 * which proves the directory MUST be sorted (the loader binary-searches it). Exact
 * byte offsets/widths and header layout are NOT recovered — all @todo.
 *
 * @unconfirmed CONTAINER OFFSETS: Gruntz uses Monolith's *RezMgr* archive, which
 * is a DIFFERENT on-disk container from the CLAW REZ that OpenClaw's
 * RezArchiveFile.cpp parses (Claw's: 127-byte header + version + recursive
 * dir/file records with isDirectoryFlag/offset/size/dateAndTime/fileId/4-char-
 * reversed-ext). OpenClaw therefore pins only the SEMANTIC field set
 * (Type/Name/Size/ID/offset/date) and the BEHAVIOUR (sorted dir, binary search,
 * recursive tree, alloc dir buffer then virtual-read it) — NOT the RezMgr byte
 * offsets. Do NOT take RezMgr container offsets from OpenClaw: reverse them from
 * the Gruntz RezMgr code (CRezDir::Load @0x13a0f0, CRezDir::OpenSub @0x13b0c0,
 * CRezDir::FindEntry @0x13c080, the CRezItm/CRezDir ctors @0x13c540/@0x13c940)
 * before writing byte-layout source. The WWD/PID formats (formats/wwd.h,
 * formats/wwd_object.h) ARE pinned; only the REZ container is @unconfirmed.
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
