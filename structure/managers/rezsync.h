#ifndef MANAGERS_REZSYNC_H
#define MANAGERS_REZSYNC_H

/*
 * RezSync / CRezDir — the REZ/VRZ resource-archive loader.
 *
 * No leaked .cpp path for this one; the names are mined from strings:
 *   - "RezSync"  (manager name, STRINGS_ANALYSIS.md §17 / §10 resource sync)
 *   - "CRezDir::Load Failed! (File is not sorted!)"  (the REZ directory must be
 *     sorted — STRINGS_ANALYSIS.md §4)
 * Neither is in RTTI — no @rtti tags. NO layout recovered — all @todo.
 *
 * Archives: Gruntz.REZ (main asset archive) + GRUNTZ.VRZ (video/secondary).
 * RezSync also participates in multiplayer resource synchronization (§10).
 * Resource keys are %s-formatted namespaces into the REZ: GRUNTZ_<type>,
 * IMAGEZ_%s, ANIZ_%s, SOUNDZ_%s, VOICES_%s, etc.
 */

#undef UNICODE
#undef _UNICODE
#include <afxwin.h>

/*
 * CRezDir — a directory entry / node within a REZ archive.
 * Name source: "CRezDir::Load Failed!" string.
 */
class CRezDir
{
public:
    //@size: unknown @todo
    //@todo: directory tree of resources; requires sorted entries (binary search).
    //       Load(): "CRezDir::Load Failed! (File is not sorted!)".
};

/*
 * RezSync — the REZ archive loader / resource sync manager.
 * Name source: "RezSync" string.
 */
class RezSync
{
public:
    //@size: unknown @todo
    //@todo: opens Gruntz.REZ / GRUNTZ.VRZ, resolves resource keys, and syncs
    //       resources across networked players. Layout unknown.
};

#endif /* MANAGERS_REZSYNC_H */
