

#include <rva.h>
#include <Bute/SymTab.h>
#include <Image/ImageSet.h>

#include <Gruntz/StateId.h>
#include <Mfc.h>
#include <Win32.h>
#include <ddraw.h>
#include <string.h>
#include <stdio.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDrawWorker.h>
#include <Gruntz/String.h>
#include <Gruntz/MapStringToOb.h>
#include <Gruntz/Loadable.h>
#include <Image/CImage.h>

VTBL(CDDrawWorker, 0x001efbe8);
inline void* operator new(u32, void* p) {
    return p;
}

RVA_COMPGEN(0x000d5d70, 0x16, ??1CLoadable@@UAE@XZ)

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
CImage* CDDrawWorkerRegistry::DispatchKeyed38(void* rec, const char* key, i32 index, i32 mode) {
    CObject* worker = 0;
    m_10map.Lookup(key, worker);
    if (worker == 0) {

        worker = new CDDrawWorker(m_ownerCtx, m_10map.GetCount());
        if (static_cast<CDDrawWorker*>(worker)->SetKey(key) == 0) {
            if (worker != 0) {
                delete worker;
            }
            return 0;
        }
        m_10map.SetAt(key, worker);
    }
    return static_cast<CDDrawWorker*>(worker)->InsertFrame(rec, index, mode);
}

RVA(0x00154be0, 0xfc)
CImage* CDDrawWorkerRegistry::DispatchKeyed34(char* path, const char* key, i32 index, i32 keyed) {
    CObject* worker = 0;
    m_10map.Lookup(key, worker);
    if (worker == 0) {
        worker = new CDDrawWorker(m_ownerCtx, m_10map.GetCount());
        if (static_cast<CDDrawWorker*>(worker)->SetKey(key) == 0) {
            if (worker != 0) {
                delete worker;
            }
            return 0;
        }
        m_10map.SetAt(key, worker);
    }
    return static_cast<CDDrawWorker*>(worker)->CreateFrame30(path, index, keyed);
}

RVA(0x00154ce0, 0x101)
CImage* CDDrawWorkerRegistry::DispatchKeyed30(
    PidHeader* desc,
    i32 mode,
    const char* key,
    i32 index,
    u32 size
) {
    CObject* worker = 0;
    m_10map.Lookup(key, worker);
    if (worker == 0) {
        worker = new CDDrawWorker(m_ownerCtx, m_10map.GetCount());
        if (static_cast<CDDrawWorker*>(worker)->SetKey(key) == 0) {
            if (worker != 0) {
                delete worker;
            }
            return 0;
        }
        m_10map.SetAt(key, worker);
    }
    return static_cast<CDDrawWorker*>(worker)->CreateFrame28(desc, mode, index, size);
}

RVA(0x00154df0, 0x101)
CImage* CDDrawWorkerRegistry::DispatchKeyed2C(
    i32 width,
    i32 height,
    const char* key,
    i32 index,
    i32 keyed
) {
    CObject* worker = 0;
    m_10map.Lookup(key, worker);
    if (worker == 0) {
        worker = new CDDrawWorker(m_ownerCtx, m_10map.GetCount());
        if (static_cast<CDDrawWorker*>(worker)->SetKey(key) == 0) {
            if (worker != 0) {
                delete worker;
            }
            return 0;
        }
        m_10map.SetAt(key, worker);
    }
    return static_cast<CDDrawWorker*>(worker)->CreateFrame24(width, height, index, keyed);
}

RVA(0x00154f00, 0x1b)
CImage* CDDrawWorkerRegistry::Forward34(char* path, CDDrawWorker* worker, i32 index, i32 keyed) {
    return worker->CreateFrame30(path, index, keyed);
}

RVA(0x00154f20, 0x1b)
CImage* CDDrawWorkerRegistry::Forward38(void* rec, CDDrawWorker* worker, i32 index, i32 mode) {
    return worker->InsertFrame(rec, index, mode);
}

RVA(0x00154f40, 0x20)
CImage* CDDrawWorkerRegistry::Forward30(
    PidHeader* desc,
    i32 mode,
    CDDrawWorker* worker,
    i32 index,
    u32 size
) {
    return worker->CreateFrame28(desc, mode, index, size);
}

RVA(0x00154f60, 0x20)
CImage*
CDDrawWorkerRegistry::Forward2C(i32 width, i32 height, CDDrawWorker* worker, i32 index, i32 keyed) {
    return worker->CreateFrame24(width, height, index, keyed);
}

RVA(0x00154f80, 0x1d5)
i32 CDDrawWorkerRegistry::InstallTree(void* tree, const char* sub, const char* prefix) {
    CSymTab* dir = static_cast<CSymTab*>(tree);
    char* buf = static_cast<char*>(operator new(0x100));
    i32 count = 0;
    if (buf == 0) {
        return count;
    }
    buf[0] = 0;
    CSymTab* e = static_cast<CSymTab*>(dir->FirstSub());
    while (e != 0) {
        if (sub != 0 && *sub != 0) {
            sprintf(buf, "%s%s%s", sub, prefix, e->m_name);
        } else {
            strcpy(buf, e->m_name);
        }
        count += InstallTree(e, buf, prefix);
        e = static_cast<CSymTab*>(dir->NextSub(e));
    }
    if (sub != 0 && *sub != 0) {
        CObject* w = 0;
        m_10map.Lookup(sub, w);
        if (w == 0) {
            w = new CDDrawWorker(m_ownerCtx, m_10map.GetCount());
            if (static_cast<CDDrawWorker*>(w)->SetKey(sub) == 0) {
                if (w != 0) {
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
    operator delete(buf);
    return count;
}

RVA(0x00155160, 0x11e)
i32 CDDrawWorkerRegistry::LoadNamespace(void* tree, const char* sub, const char* prefix) {
    CSymTab* dir = static_cast<CSymTab*>(tree);
    char* buf = static_cast<char*>(operator new(0x100));
    i32 count = 0;
    CSymTab* e = static_cast<CSymTab*>(dir->FirstSub());
    while (e != 0) {
        if (sub != 0 && *sub != 0) {
            sprintf(buf, "%s%s%s", sub, prefix, e->m_name);
        } else {
            strcpy(buf, e->m_name);
        }
        i32 r = LoadNamespace(e, buf, prefix);
        if (r < 0) {
            operator delete(buf);
            return -1;
        }
        count += r;
        e = static_cast<CSymTab*>(dir->NextSub(e));
    }
    if (sub != 0 && *sub != 0) {
        CObject* out = 0;
        m_10map.Lookup(sub, out);
        if (out != 0) {

            if (static_cast<CDDrawWorker*>(out)->ValidateFramesFromSymTab(dir) == -1) {
                operator delete(buf);
                return -1;
            }
            if (static_cast<CDDrawWorker*>(out)->m_items.GetSize() > 0) {
                ++count;
            }
        }
    }
    operator delete(buf);
    return count;
}

RVA(0x00155280, 0x22)
void CDDrawWorkerRegistry::RemoveWorker(CDDrawWorker* worker) {
    if (worker != 0) {
        m_10map.RemoveKey(worker->m_name);
        delete worker;
    }
}

RVA(0x001552b0, 0xa2)
void CDDrawWorkerRegistry::MapTeardown() {
    CObject* val = 0;
    POSITION pos = m_10map.GetStartPosition();
    CString key;
    if (pos != 0) {
        do {
            m_10map.GetNextAssoc(pos, key, val);
            if (val != 0) {
                delete (static_cast<CDDrawWorker*>(val));
            }
        } while (pos != 0);
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
    while (pos != 0) {
        m_10map.GetNextAssoc(pos, key, val);
        if (strncmp(key, match, len) == 0) {
            m_10map.RemoveKey(key);
            if (val != 0) {
                delete (static_cast<CDDrawWorker*>(val));
            }
            ++n;
        }
    }
    return n;
}

// @early-stop
RVA(0x00155460, 0xe2)
i32 CDDrawWorkerRegistry::SumSizesEqual(const char* str, i32 raw) {
    CString key;
    CObject* val = 0;
    POSITION pos = m_10map.GetStartPosition();
    i32 total = 0;
    while (pos != 0) {
        m_10map.GetNextAssoc(pos, key, val);
        if (val != 0) {
            if (str == 0 || *str == 0 || strncmp(key, str, strlen(str)) == 0) {
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
    while (pos != 0) {
        m_10map.GetNextAssoc(pos, key, val);
        if (strncmp(key, str, len) == 0) {
            return 1;
        }
    }
    return 0;
}

RVA(0x00155630, 0xc5)
i32 CDDrawWorkerRegistry::AnyValueMatches(CImage* frame, char* outName, i32* outIndex) {
    if (frame == 0) {
        return 0;
    }
    CString key;
    CObject* val = 0;
    POSITION pos = m_10map.GetStartPosition();
    while (pos != 0) {
        m_10map.GetNextAssoc(pos, key, val);
        if (val != 0 && (static_cast<CDDrawWorker*>(val))->FindFrame(frame, outName, outIndex)) {
            return 1;
        }
    }
    return 0;
}

RVA(0x00155700, 0x16)
i32 CLoadable::IsLoaded() {
    if (m_ownerCtx != 0 && m_id != -1) {
        return 1;
    }
    return 0;
}

RVA_COMPGEN(0x00155720, 0x1e, ??_GCLoadable@@UAEPAXI@Z)

RVA(0x00155740, 0x1)
void CLoadable::Unload() {}

RVA(0x00155750, 0x16)
i32 CDDrawWorker::IsLoaded() {
    if (m_ownerCtx != 0 && m_id != -1) {
        return 1;
    }
    return 0;
}

RVA(0x00155770, 0x6)
i32 CDDrawWorker::GetClassId() {
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
