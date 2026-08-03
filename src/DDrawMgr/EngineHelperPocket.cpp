#include <rva.h>

#include <Mfc.h>

#include <AddrWord.h>
#include <Bute/SymTab.h>
#include <DDrawMgr/AnimWorkerObj.h>
#include <DDrawMgr/AniRecord.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawPtrCollections.h>
#include <DDrawMgr/DDrawSubMgrLeaf.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorker.h>
#include <DDrawMgr/DDrawWorkerCache.h>
#include <DDrawMgr/DDrawWorkerCtx.h>
#include <DDrawMgr/DDrawWorkerHost.h>
#include <DDrawMgr/DDrawWorkerList.h>
#include <DDrawMgr/DDrawWorkerMapSmall.h>
#include <DDrawMgr/DDrawWorkerNode.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/DirectDrawMgr.h>
#include <Enums.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/ImageSets.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MapStringToOb.h>
#include <Gruntz/ParseSource.h>
#include <Gruntz/ResolveNode.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/String.h>
#include <Gruntz/UserLogic.h>
#include <Image/ImageSet.h>
#include <Io/FileMem.h>
#include <Io/FileStream.h>
#include <Pix16.h>
#include <Rez/RezTypeTag.h>
#include <Utils/MapTyped.h>
#include <Wap32/Object.h>
#include <Wwd/WwdFile.h>

#include <ddraw.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

RVA(0x0006b270, 0x1b)
CObject* CAniElement::AtChecked(i32 i) const {
    if (i >= 0 && i < m_records.GetSize()) {
        return m_records.GetAt(i);
    }
    return 0;
}

RVA(0x0006b2a0, 0x23)
CObject* CDDrawSubMgrLeaf::LookupValue(const char* key) {
    void* val = 0;
    m_animations.Lookup(key, val);
    return static_cast<CObject*>(val);
}

RVA(0x0006b330, 0x2a)
i32 CGameLevel::PointInBounds(const LevelCoordRect* r, i32 x, i32 y) {
    if (x < r->right && x >= r->left && y < r->bottom && y >= r->top) {
        return 1;
    }
    return 0;
}

// @identity-TODO LookupTile@CGameLevel - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (73 fns) came from the static library. It belongs to another compiland.
