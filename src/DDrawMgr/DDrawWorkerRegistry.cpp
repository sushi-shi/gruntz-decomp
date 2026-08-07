#include <rva.h>

#include <DDrawMgr/DDrawWorkerRegistry.h>

#include <Mfc.h>

#include <Bute/SymTab.h>
#include <DDrawMgr/DDrawWorker.h>
#include <DDrawMgr/DDSurface.h>
#include <Gruntz/Loadable.h>
#include <Gruntz/MapStringToOb.h>
#include <Gruntz/StateId.h>
#include <Gruntz/String.h>
#include <Image/CImage.h>
#include <Image/ImageSet.h>

#include <ddraw.h>
#include <stdio.h>
#include <string.h>

inline void* operator new(u32, void* p) {
    return p;
}

// @identity-TODO ?1CLoadable - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (26 fns) came from the static library. It belongs to another compiland.
RVA(0x00154aa0, 0x20)
i32 CDDrawWorkerRegistry::IsReady() {
    memset(&g_bltFx, 0, sizeof(g_bltFx));
    g_bltFx.dwSize = sizeof(DDBLTFX);
    return 1;
}

RVA(0x00154ac0, 0x12)
void CDDrawWorkerRegistry::Unload() {
    MapTeardown();
    g_resourceInstallActive = 0;
    g_surfaceColorKey = 0;
}

RVA(0x00154ae0, 0xfc)
CImage* CDDrawWorkerRegistry::InsertFrameByKey(void* rec, const char* key, i32 index, i32 mode) {
    CObject* worker = 0;
    m_10map.Lookup(key, worker);
    if (worker == NULL) {

        worker = new CDDrawWorker(m_ownerCtx, m_10map.GetCount());
        if (static_cast<CDDrawWorker*>(worker)->SetKey(key) == 0) {
            if (worker != NULL) {
                delete worker;
            }
            return 0;
        }
        m_10map.SetAt(key, worker);
    }
    return static_cast<CDDrawWorker*>(worker)->InsertFrame(rec, index, mode);
}

RVA(0x00154be0, 0xfc)
CImage* CDDrawWorkerRegistry::LoadFrameByKey(char* path, const char* key, i32 index, i32 keyed) {
    CObject* worker = 0;
    m_10map.Lookup(key, worker);
    if (worker == NULL) {
        worker = new CDDrawWorker(m_ownerCtx, m_10map.GetCount());
        if (static_cast<CDDrawWorker*>(worker)->SetKey(key) == 0) {
            if (worker != NULL) {
                delete worker;
            }
            return 0;
        }
        m_10map.SetAt(key, worker);
    }
    return static_cast<CDDrawWorker*>(worker)->LoadFrame(path, index, keyed);
}

RVA(0x00154ce0, 0x101)
CImage* CDDrawWorkerRegistry::CreateDescriptorFrameByKey(
    PidHeader* desc,
    FileImageFormat mode,
    const char* key,
    i32 index,
    u32 size
) {
    CObject* worker = 0;
    m_10map.Lookup(key, worker);
    if (worker == NULL) {
        worker = new CDDrawWorker(m_ownerCtx, m_10map.GetCount());
        if (static_cast<CDDrawWorker*>(worker)->SetKey(key) == 0) {
            if (worker != NULL) {
                delete worker;
            }
            return 0;
        }
        m_10map.SetAt(key, worker);
    }
    return static_cast<CDDrawWorker*>(worker)->CreateDescriptorFrame(desc, mode, index, size);
}

RVA(0x00154df0, 0x101)
CImage* CDDrawWorkerRegistry::CreateBlankFrameByKey(
    i32 width,
    i32 height,
    const char* key,
    i32 index,
    i32 keyed
) {
    CObject* worker = 0;
    m_10map.Lookup(key, worker);
    if (worker == NULL) {
        worker = new CDDrawWorker(m_ownerCtx, m_10map.GetCount());
        if (static_cast<CDDrawWorker*>(worker)->SetKey(key) == 0) {
            if (worker != NULL) {
                delete worker;
            }
            return 0;
        }
        m_10map.SetAt(key, worker);
    }
    return static_cast<CDDrawWorker*>(worker)->CreateBlankFrame(width, height, index, keyed);
}

RVA(0x00154f00, 0x1b)
CImage*
CDDrawWorkerRegistry::LoadFrameForWorker(char* path, CDDrawWorker* worker, i32 index, i32 keyed) {
    return worker->LoadFrame(path, index, keyed);
}

RVA(0x00154f20, 0x1b)
CImage*
CDDrawWorkerRegistry::InsertFrameForWorker(void* rec, CDDrawWorker* worker, i32 index, i32 mode) {
    return worker->InsertFrame(rec, index, mode);
}

RVA(0x00154f40, 0x20)
CImage* CDDrawWorkerRegistry::CreateDescriptorFrameForWorker(
    PidHeader* desc,
    FileImageFormat mode,
    CDDrawWorker* worker,
    i32 index,
    u32 size
) {
    return worker->CreateDescriptorFrame(desc, mode, index, size);
}

RVA(0x00154f60, 0x20)
CImage* CDDrawWorkerRegistry::CreateBlankFrameForWorker(
    i32 width,
    i32 height,
    CDDrawWorker* worker,
    i32 index,
    i32 keyed
) {
    return worker->CreateBlankFrame(width, height, index, keyed);
}

RVA(0x00154f80, 0x1d5)
i32 CDDrawWorkerRegistry::InstallTree(void* tree, const char* sub, const char* prefix) {
    CSymTab* dir = static_cast<CSymTab*>(tree);
    char* buf = new char[0x100];
    i32 count = 0;
    if (buf == NULL) {
        return count;
    }
    buf[0] = 0;
    CSymTab* e = static_cast<CSymTab*>(dir->FirstSub());
    while (e != NULL) {
        if (sub != NULL && *sub != 0) {
            sprintf(buf, "%s%s%s", sub, prefix, e->m_name);
        } else {
            strcpy(buf, e->m_name);
        }
        count += InstallTree(e, buf, prefix);
        e = static_cast<CSymTab*>(dir->NextSub(e));
    }
    if (sub != NULL && *sub != 0) {
        CObject* w = 0;
        m_10map.Lookup(sub, w);
        if (w == NULL) {
            w = new CDDrawWorker(m_ownerCtx, m_10map.GetCount());
            if (static_cast<CDDrawWorker*>(w)->SetKey(sub) == 0) {
                if (w != NULL) {
                    delete w;
                }
                return 0;
            }
            m_10map.SetAt(sub, w);
        }
        static_cast<CDDrawWorker*>(w)->BuildFramesFromSymTab(dir);
        if (static_cast<CDDrawWorker*>(w)->m_items.GetSize() == 0) {
            RemoveByKey(sub);
        } else {
            ++count;
        }
    }
    delete[] buf;
    return count;
}

RVA(0x00155160, 0x11e)
i32 CDDrawWorkerRegistry::LoadNamespace(void* tree, const char* sub, const char* prefix) {
    CSymTab* dir = static_cast<CSymTab*>(tree);
    char* buf = new char[0x100];
    i32 count = 0;
    CSymTab* e = static_cast<CSymTab*>(dir->FirstSub());
    while (e != NULL) {
        if (sub != NULL && *sub != 0) {
            sprintf(buf, "%s%s%s", sub, prefix, e->m_name);
        } else {
            strcpy(buf, e->m_name);
        }
        i32 r = LoadNamespace(e, buf, prefix);
        if (r < 0) {
            delete[] buf;
            return -1;
        }
        count += r;
        e = static_cast<CSymTab*>(dir->NextSub(e));
    }
    if (sub != NULL && *sub != 0) {
        CObject* out = 0;
        m_10map.Lookup(sub, out);
        if (out != NULL) {

            if (static_cast<CDDrawWorker*>(out)->ValidateFramesFromSymTab(dir) == -1) {
                delete[] buf;
                return -1;
            }
            if (static_cast<CDDrawWorker*>(out)->m_items.GetSize() > 0) {
                ++count;
            }
        }
    }
    delete[] buf;
    return count;
}

RVA(0x00155280, 0x22)
void CDDrawWorkerRegistry::RemoveWorker(CDDrawWorker* worker) {
    if (worker != NULL) {
        m_10map.RemoveKey(worker->m_name);
        delete worker;
    }
}

RVA(0x001552b0, 0xa2)
void CDDrawWorkerRegistry::MapTeardown() {
    CObject* val = 0;
    POSITION pos = m_10map.GetStartPosition();
    CString key;
    if (pos != NULL) {
        do {
            m_10map.GetNextAssoc(pos, key, val);
            if (val != NULL) {
                delete (static_cast<CDDrawWorker*>(val));
            }
        } while (pos != NULL);
    }
    m_10map.RemoveAll();
}

RVA(0x00155360, 0xf8)
i32 CDDrawWorkerRegistry::RemoveKeysEqual(const char* base, const char* str) {
    CString match(base);
    match = str;
    i32 len = match.GetLength();
    CString key;
    CObject* val = 0;
    POSITION pos = m_10map.GetStartPosition();
    i32 n = 0;
    while (pos != NULL) {
        m_10map.GetNextAssoc(pos, key, val);
        if (strncmp(key, match, len) == 0) {
            m_10map.RemoveKey(key);
            if (val != NULL) {
                delete (static_cast<CDDrawWorker*>(val));
            }
            ++n;
        }
    }
    return n;
}

RVA(0x00155460, 0xe2)
i32 CDDrawWorkerRegistry::SumSizesEqual(const char* str, i32 raw) {
    POSITION pos = m_10map.GetStartPosition();
    i32 total = 0;
    CObject* val = 0;
    CString key;
    while (pos != NULL) {
        val = NULL;
        m_10map.GetNextAssoc(pos, key, val);
        if (val != NULL) {
            if (str == NULL || *str == 0) {
                total += (static_cast<CDDrawWorker*>(val))->GetMemoryUsage(raw);
            } else if (strncmp(key, str, strlen(str)) == 0) {
                total += (static_cast<CDDrawWorker*>(val))->GetMemoryUsage(raw);
            }
        }
    }
    return total;
}

RVA(0x00155550, 0xdc)
i32 CDDrawWorkerRegistry::HasKeyEqual(const char* str) {
    i32 len = strlen(str);
    CString key;
    CObject* val = 0;
    POSITION pos = m_10map.GetStartPosition();
    while (pos != NULL) {
        m_10map.GetNextAssoc(pos, key, val);
        if (strncmp(key, str, len) == 0) {
            return 1;
        }
    }
    return 0;
}

RVA(0x00155630, 0xc5)
i32 CDDrawWorkerRegistry::AnyValueMatches(CImage* frame, char* outName, i32* outIndex) {
    if (frame == NULL) {
        return 0;
    }
    CString key;
    CObject* val = 0;
    POSITION pos = m_10map.GetStartPosition();
    while (pos != NULL) {
        m_10map.GetNextAssoc(pos, key, val);
        if (val != NULL && (static_cast<CDDrawWorker*>(val))->FindFrame(frame, outName, outIndex)) {
            return 1;
        }
    }
    return 0;
}

RVA(0x00155700, 0x16)
i32 CLoadable::IsLoaded() {
    if (m_ownerCtx != NULL && m_id != -1) {
        return 1;
    }
    return 0;
}

RVA_COMPGEN(0x00155720, 0x1e, ??_GCLoadable@@UAEPAXI@Z)

RVA(0x00155740, 0x1)
void CLoadable::Unload() {}

RVA(0x00155750, 0x16)
i32 CDDrawWorker::IsLoaded() {
    if (m_ownerCtx != NULL && m_id != -1) {
        return 1;
    }
    return 0;
}

RVA(0x00155770, 0x6)
LoadableClassId CDDrawWorker::GetClassId() {
    return CLASSID_WORKER;
}

RVA_COMPGEN(0x00155780, 0x1e, ??_GCDDrawWorker@@UAEPAXI@Z)
RVA(0x001557a0, 0x68)
CDDrawWorker::~CDDrawWorker() {

    Unload();
}

RVA(0x00155810, 0x23)
i32 CDDrawWorker::SetKey(const char* src) {
    strncpy(m_name, src, 0x3f);
    m_name[0x3f] = 0;
    return 1;
}
