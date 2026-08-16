#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/AnimWorkerObj.h>
#include <DDrawMgr/AniRecord.h>
#include <DDrawMgr/DDrawPtrCollections.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorker.h>
#include <DDrawMgr/DDrawWorkerCache.h>
#include <DDrawMgr/DDrawWorkerCtx.h>
#include <DDrawMgr/DDrawWorkerList.h>
#include <DDrawMgr/DDrawWorkerMapSmall.h>
#include <DDrawMgr/DDrawWorkerNode.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/DirectDrawMgr.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/MapStringToOb.h>
#include <Gruntz/ParseSource.h>
#include <Gruntz/ResolveNode.h>
#include <Gruntz/String.h>
#include <Image/ImageSet.h>
#include <Io/FileMem.h>
#include <Pix16.h>

#include <ddraw.h>
#include <stdio.h>
#include <string.h>

// @identity-TODO BltSelf@CDDrawSurfacePair - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (43 fns) came from the static library. It belongs to another compiland.
RVA(0x0003a1d0, 0x1d)
void CDDrawSurfacePair::BltSelf(CDDrawSurfacePair* src) {
    m_surface->BltFast(0, 0, src->m_surface, &src->m_srcRect, 0x10);
}

// @identity-TODO AtChecked@CAniElement - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (43 fns) came from the static library. It belongs to another compiland.
