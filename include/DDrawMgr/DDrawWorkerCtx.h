// DDrawWorkerCtx.h - the DDraw-worker owner context reached through the +0x0c
// CLoadable owner handle. Shared by CDDrawWorkerB::Helper_166040 + CResolveNode::
// SetPosition (src/DDrawMgr/DDrawSurfacePair.cpp) and CDDrawWorkerHost::RegisterNamed
// (src/DDrawMgr/DDrawWorkerHost.cpp): the named-object resolvers go through
// ctx->m_10 (a sub-manager) whose +0x10 is a CMapStringToOb.
#ifndef GRUNTZ_DDRAWMGR_DDRAWWORKERCTX_H
#define GRUNTZ_DDRAWMGR_DDRAWWORKERCTX_H

#include <Ints.h>
#include <Mfc.h> // real MFC CMapStringToOb (Lookup 0x1b8008, reloc-masked)
#include <rva.h> // SIZE_UNKNOWN

// (CDDrawWorkerCtx/CDDrawWorkerCtxMap are GONE - the "+0x0c owner context" IS the
// CDDrawSurfaceMgr (its +0x10 m_imageRegistry is the ctx "sub-manager", whose +0x10
// CMapStringToOb is the named-object map; its +0x24 m_level primes SetPosition's
// m_3c). Reach it via CLoadable::OwnerMgr().)
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>

#endif // GRUNTZ_DDRAWMGR_DDRAWWORKERCTX_H
