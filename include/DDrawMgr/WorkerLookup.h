#ifndef DDRAWMGR_WORKERLOOKUP_H
#define DDRAWMGR_WORKERLOOKUP_H

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorker.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Utils/MapTyped.h>

inline CDDrawWorker* CDDrawSurfaceMgr::FindWorker(const std::string& name) {
    return m_imageRegistry->FindWorker(name);
}

inline CImage* CDDrawSurfaceMgr::FindFrame(const std::string& name, i32 index) {
    CDDrawWorker* worker = FindWorker(name);
    if (worker == NULL) {
        return NULL;
    }
    return worker->GetAt(index);
}

#endif
