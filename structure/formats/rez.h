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
