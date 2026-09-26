#ifndef DDRAWMGR_WORKERLOOKUP_H
#define DDRAWMGR_WORKERLOOKUP_H

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorker.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Utils/MapTyped.h>

inline CDDrawWorker* CDDrawSurfaceMgr::FindWorker(LPCTSTR name) {
    return MapFind<CDDrawWorker>(m_imageRegistry->m_workersByName, name);
}

inline CImage* CDDrawSurfaceMgr::FindFrame(LPCTSTR name, i32 index) {
    CDDrawWorker* worker = FindWorker(name);
    if (worker == NULL) {
        return NULL;
    }
    return worker->GetAt(index);
}

#endif // DDRAWMGR_WORKERLOOKUP_H
