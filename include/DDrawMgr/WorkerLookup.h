#ifndef DDRAWMGR_WORKERLOOKUP_H
#define DDRAWMGR_WORKERLOOKUP_H

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorker.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>

inline CDDrawWorker* LookupWorker(CMapStringToOb& map, LPCTSTR name) {
    CObject* found = NULL;
    map.Lookup(name, found);
    return static_cast<CDDrawWorker*>(found);
}

inline CDDrawWorker* LookupWorker(CDDrawSurfaceMgr* host, LPCTSTR name) {
    CObject* found = NULL;
    host->m_imageRegistry->m_workersByName.Lookup(name, found);
    return static_cast<CDDrawWorker*>(found);
}

#endif // DDRAWMGR_WORKERLOOKUP_H
