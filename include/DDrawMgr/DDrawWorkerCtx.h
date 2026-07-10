// DDrawWorkerCtx.h - the DDraw-worker owner context reached at a worker/host +0x0c.
// Shared by CDDrawWorkerBase::Helper_166040 (src/Gruntz/HelperHost.cpp) and
// CDDrawWorkerHost::RegisterNamed (src/DDrawMgr/DDrawWorkerHost.cpp): both resolve a
// named object through ctx->m_10 (a sub-manager) whose +0x10 is a CMapStringToOb.
#ifndef GRUNTZ_DDRAWMGR_DDRAWWORKERCTX_H
#define GRUNTZ_DDRAWMGR_DDRAWWORKERCTX_H

#include <Ints.h>
#include <Mfc.h> // real MFC CMapStringToOb (Lookup 0x1b8008, reloc-masked)
#include <rva.h> // SIZE_UNKNOWN

// The sub-manager ctx->m_10 points to: its +0x10 is the named-object map.
struct CDDrawWorkerCtxMap {
    char pad_00[0x10];
    CMapStringToOb m_10; // +0x10  named-object map
};

// The worker/host owner context: a sub-manager ptr at +0x10 (whose +0x10 is the
// string->object map) and an int at +0x24 (primes Helper_164790's m_3c).
struct CDDrawWorkerCtx {
    char pad_00[0x10];
    CDDrawWorkerCtxMap* m_10; // +0x10
    char pad_14[0x24 - 0x14];
    i32 m_24; // +0x24
};

SIZE_UNKNOWN(CDDrawWorkerCtxMap);
SIZE_UNKNOWN(CDDrawWorkerCtx);

#endif // GRUNTZ_DDRAWMGR_DDRAWWORKERCTX_H
