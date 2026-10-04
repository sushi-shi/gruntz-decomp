#ifndef DDRAWMGR_WORKERLOOKUP_H
#define DDRAWMGR_WORKERLOOKUP_H

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorker.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Utils/MapTyped.h>

inline CImageSet* CDDrawSurfaceMgr::FindWorker(LPCTSTR name) {
    return MapFind<CImageSet>(m_imageRegistry->m_imageSetsByName, name);
}

inline CImage* CDDrawSurfaceMgr::FindFrame(LPCTSTR name, i32 index) {
    CImageSet* worker = FindWorker(name);
    if (worker == NULL) {
        return NULL;
    }
    return worker->GetAt(index);
}

#endif // DDRAWMGR_WORKERLOOKUP_H
