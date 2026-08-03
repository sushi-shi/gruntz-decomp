#include <rva.h>

#include <Mfc.h>

#include <Bute/SymTab.h>
#include <DDrawMgr/AnimWorkerObj.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawShadeBlit.h>
#include <DDrawMgr/DDrawSubMgrLeaf.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorker.h>
#include <DDrawMgr/DDrawWorkerCache.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDSurface.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/Blk6c.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/ParseSource.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/WwdGameObject.h>
#include <Image/CImage.h>
#include <Image/ImageSet.h>
#include <Ints.h>
#include <Io/FileMem.h>
#include <Rez/FrameClock.h>
#include <Utils/MapTyped.h>
#include <Wap32/Object.h>
#include <Wwd/WwdGameObjectFamily.h>

#include <ddraw.h>
#include <stdlib.h>
#include <string.h>

// @identity-TODO ApplyGeometryDirect@CWwdGameObjectA - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (44 fns) came from the static library. It belongs to another compiland.
RVA(0x00058b60, 0x2d)
void CWwdGameObjectA::ApplyGeometryDirect(CAniElement* srcSprite, i32 applyDefault) {
    m_animCursor.Setup(srcSprite);
    if (applyDefault) {
        m_animCursor.Advance(g_engineFrameDelta);
    }
}

// @early-stop
