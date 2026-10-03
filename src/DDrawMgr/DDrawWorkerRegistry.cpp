#include <StdAfx.h>

#include <Ints.h>
#include <Utils/MapTyped.h>

#include <DDrawMgr/DDrawWorkerRegistry.h>

#include <DDrawMgr/DDrawWorker.h>
#include <DDrawMgr/DDSurface.h>
#include <Gruntz/MapStringToOb.h>
#include <Gruntz/StateId.h>
#include <Gruntz/String.h>
#include <Image/CImage.h>
#include <Image/ImageSet.h>
#include <Rez/RezArchiveDir.h>
#include <Wap32/WapObj.h>

#include <ddraw.h>
#include <new>
#include <stdio.h>
#include <string.h>

i32 CDDrawWorkerRegistry::IsReady() {
    memset(&g_bltFx, 0, sizeof(g_bltFx));
    g_bltFx.dwSize = sizeof(DDBLTFX);
    return 1;
}

void CDDrawWorkerRegistry::Unload() {
    MapTeardown();
    g_resourceInstallActive = false;
    g_surfaceColorKey = 0;
}

CImage* CDDrawWorkerRegistry::InsertFrameByKey(CRezItm* rec, const char* key, i32 index, i32 mode) {
    CDDrawWorker* worker = NULL;
    MapLookup(m_workersByName, key, worker);
    if (worker == NULL) {

        worker = new CDDrawWorker(m_ownerCtx, static_cast<i32>(m_workersByName.size()));
        if (static_cast<CDDrawWorker*>(worker)->SetKey(key) == 0) {
            if (worker != NULL) {
                delete worker;
            }
            return NULL;
        }
        m_workersByName[key] = worker;
    }
    return static_cast<CDDrawWorker*>(worker)->InsertFrame(rec, index, mode);
}

CImage* CDDrawWorkerRegistry::LoadFrameByKey(char* path, const char* key, i32 index, i32 keyed) {
    CDDrawWorker* worker = NULL;
    MapLookup(m_workersByName, key, worker);
    if (worker == NULL) {
        worker = new CDDrawWorker(m_ownerCtx, static_cast<i32>(m_workersByName.size()));
        if (static_cast<CDDrawWorker*>(worker)->SetKey(key) == 0) {
            if (worker != NULL) {
                delete worker;
            }
            return NULL;
        }
        m_workersByName[key] = worker;
    }
    return static_cast<CDDrawWorker*>(worker)->LoadFrame(path, index, keyed);
}

CImage* CDDrawWorkerRegistry::CreateDescriptorFrameByKey(
    PidHeader* desc,
    FileImageFormat mode,
    const char* key,
    i32 index,
    u32 size
) {
    CDDrawWorker* worker = NULL;
    MapLookup(m_workersByName, key, worker);
    if (worker == NULL) {
        worker = new CDDrawWorker(m_ownerCtx, static_cast<i32>(m_workersByName.size()));
        if (static_cast<CDDrawWorker*>(worker)->SetKey(key) == 0) {
            if (worker != NULL) {
                delete worker;
            }
            return NULL;
        }
        m_workersByName[key] = worker;
    }
    return static_cast<CDDrawWorker*>(worker)->CreateDescriptorFrame(desc, mode, index, size);
}

CImage* CDDrawWorkerRegistry::CreateBlankFrameByKey(
    i32 width,
    i32 height,
    const char* key,
    i32 index,
    i32 keyed
) {
    CDDrawWorker* worker = NULL;
    MapLookup(m_workersByName, key, worker);
    if (worker == NULL) {
        worker = new CDDrawWorker(m_ownerCtx, static_cast<i32>(m_workersByName.size()));
        if (static_cast<CDDrawWorker*>(worker)->SetKey(key) == 0) {
            if (worker != NULL) {
                delete worker;
            }
            return NULL;
        }
        m_workersByName[key] = worker;
    }
    return static_cast<CDDrawWorker*>(worker)->CreateBlankFrame(width, height, index, keyed);
}

CImage*
CDDrawWorkerRegistry::LoadFrameForWorker(char* path, CDDrawWorker* worker, i32 index, i32 keyed) {
    return worker->LoadFrame(path, index, keyed);
}

CImage* CDDrawWorkerRegistry::InsertFrameForWorker(
    CRezItm* rec,
    CDDrawWorker* worker,
    i32 index,
    i32 mode
) {
    return worker->InsertFrame(rec, index, mode);
}

CImage* CDDrawWorkerRegistry::CreateDescriptorFrameForWorker(
    PidHeader* desc,
    FileImageFormat mode,
    CDDrawWorker* worker,
    i32 index,
    u32 size
) {
    return worker->CreateDescriptorFrame(desc, mode, index, size);
}

CImage* CDDrawWorkerRegistry::CreateBlankFrameForWorker(
    i32 width,
    i32 height,
    CDDrawWorker* worker,
    i32 index,
    i32 keyed
) {
    return worker->CreateBlankFrame(width, height, index, keyed);
}

i32 CDDrawWorkerRegistry::InstallTree(CRezDir* dir, const char* sub, const char* prefix) {
    char* buf = new char[0x100];
    i32 count = 0;
    if (buf == NULL) {
        return count;
    }
    buf[0] = 0;
    CRezDir* e = dir->GetFirstSubDir();
    while (e != NULL) {
        if (sub != NULL && *sub != 0) {
            sprintf(buf, "%s%s%s", sub, prefix, e->GetDirName());
        } else {
            strcpy(buf, e->GetDirName());
        }
        count += InstallTree(e, buf, prefix);
        e = dir->GetNextSubDir(e);
    }
    if (sub != NULL && *sub != 0) {
        CDDrawWorker* w = NULL;
        MapLookup(m_workersByName, sub, w);
        if (w == NULL) {
            w = new CDDrawWorker(m_ownerCtx, static_cast<i32>(m_workersByName.size()));
            if (static_cast<CDDrawWorker*>(w)->SetKey(sub) == 0) {
                if (w != NULL) {
                    delete w;
                }
                return 0;
            }
            m_workersByName[sub] = w;
        }
        static_cast<CDDrawWorker*>(w)->BuildFramesFromArchive(dir);
        if (static_cast<i32>((static_cast<CDDrawWorker*>(w)->m_items).size()) == 0) {
            RemoveByKey(sub);
        } else {
            ++count;
        }
    }
    delete[] buf;
    return count;
}

i32 CDDrawWorkerRegistry::LoadNamespace(CRezDir* dir, const char* sub, const char* prefix) {
    char* buf = new char[0x100];
    i32 count = 0;
    CRezDir* e = dir->GetFirstSubDir();
    while (e != NULL) {
        if (sub != NULL && *sub != 0) {
            sprintf(buf, "%s%s%s", sub, prefix, e->GetDirName());
        } else {
            strcpy(buf, e->GetDirName());
        }
        i32 r = LoadNamespace(e, buf, prefix);
        if (r < 0) {
            delete[] buf;
            return -1;
        }
        count += r;
        e = dir->GetNextSubDir(e);
    }
    if (sub != NULL && *sub != 0) {
        CDDrawWorker* out = NULL;
        MapLookup(m_workersByName, sub, out);
        if (out != NULL) {

            if (static_cast<CDDrawWorker*>(out)->ValidateFramesFromArchive(dir) == -1) {
                delete[] buf;
                return -1;
            }
            if (static_cast<i32>((static_cast<CDDrawWorker*>(out)->m_items).size()) > 0) {
                ++count;
            }
        }
    }
    delete[] buf;
    return count;
}

void CDDrawWorkerRegistry::RemoveWorker(CDDrawWorker* worker) {
    if (worker != NULL) {
        m_workersByName.erase(worker->m_name);
        delete worker;
    }
}

void CDDrawWorkerRegistry::MapTeardown() {
    CObject* val = NULL;
    std::map<std::string, CDDrawWorker*>::iterator pos = m_workersByName.begin();
    std::string key;
    if (pos != m_workersByName.end()) {
        do {
            (key = pos->first, val = pos->second, ++pos);
            if (val != NULL) {
                delete (static_cast<CDDrawWorker*>(val));
            }
        } while (pos != m_workersByName.end());
    }
    m_workersByName.clear();
}

i32 CDDrawWorkerRegistry::RemoveWithPrefix(const char* prefix, const char* separator) {
    std::string match(prefix);
    match += separator;
    i32 len = static_cast<i32>((match).size());
    std::string key;
    CObject* val = NULL;
    std::map<std::string, CDDrawWorker*>::iterator pos = m_workersByName.begin();
    i32 n = 0;
    while (pos != m_workersByName.end()) {
        (key = pos->first, val = pos->second, ++pos);
        if (strncmp((key).c_str(), (match).c_str(), len) == 0) {
            m_workersByName.erase((key).c_str());
            if (val != NULL) {
                delete (static_cast<CDDrawWorker*>(val));
            }
            ++n;
        }
    }
    return n;
}

i32 CDDrawWorkerRegistry::SumSizesEqual(const char* str, i32 raw) {
    std::map<std::string, CDDrawWorker*>::iterator pos = m_workersByName.begin();
    i32 total = 0;
    CObject* val = NULL;
    std::string key;
    while (pos != m_workersByName.end()) {
        val = NULL;
        (key = pos->first, val = pos->second, ++pos);
        if (val != NULL) {
            if (str == NULL || *str == 0) {
                total += (static_cast<CDDrawWorker*>(val))->GetMemoryUsage(raw);
            } else if (strncmp((key).c_str(), str, strlen(str)) == 0) {
                total += (static_cast<CDDrawWorker*>(val))->GetMemoryUsage(raw);
            }
        }
    }
    return total;
}

i32 CDDrawWorkerRegistry::HasWithPrefix(const char* prefix) {
    i32 len = strlen(prefix);
    std::string key;
    CObject* val = NULL;
    std::map<std::string, CDDrawWorker*>::iterator pos = m_workersByName.begin();
    while (pos != m_workersByName.end()) {
        (key = pos->first, val = pos->second, ++pos);
        if (strncmp((key).c_str(), prefix, len) == 0) {
            return 1;
        }
    }
    return 0;
}

i32 CDDrawWorkerRegistry::AnyValueMatches(CImage* frame, char* outName, i32* outIndex) {
    if (frame == NULL) {
        return 0;
    }
    std::string key;
    CObject* val = NULL;
    std::map<std::string, CDDrawWorker*>::iterator pos = m_workersByName.begin();
    while (pos != m_workersByName.end()) {
        (key = pos->first, val = pos->second, ++pos);
        if (val != NULL && (static_cast<CDDrawWorker*>(val))->FindFrame(frame, outName, outIndex)) {
            return 1;
        }
    }
    return 0;
}

i32 CWapObj::IsLoaded() {
    if (m_ownerCtx != NULL && m_id != -1) {
        return 1;
    }
    return 0;
}

void CWapObj::Unload() {}

i32 CDDrawWorker::IsLoaded() {
    if (m_ownerCtx != NULL && m_id != -1) {
        return 1;
    }
    return 0;
}

LoadableClassId CDDrawWorker::GetClassId() {
    return CLASSID_WORKER;
}

CDDrawWorker::~CDDrawWorker() {

    Unload();
}

i32 CDDrawWorker::SetKey(const char* src) {
    strncpy(m_name, src, 0x3f);
    m_name[0x3f] = 0;
    return 1;
}
