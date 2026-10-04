#ifndef DDRAWMGR_WORKERLOOKUP_H
#define DDRAWMGR_WORKERLOOKUP_H

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorker.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Utils/MapTyped.h>

inline CImageSet* CGameWorld::FindImageSet(LPCTSTR name) {
    return MapFind<CImageSet>(m_imageRegistry->m_imageSetsByName, name);
}

inline CImage* CGameWorld::FindFrame(LPCTSTR name, i32 index) {
    CImageSet* imageSet = FindImageSet(name);
    if (imageSet == NULL) {
        return NULL;
    }
    return imageSet->GetAt(index);
}

#endif // DDRAWMGR_WORKERLOOKUP_H
