#include <StdAfx.h>

#include <rva.h>

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

RVA(0x00154aa0, 0x20)
i32 CImageSetRegistry::IsReady() {
    memset(&g_bltFx, 0, sizeof(g_bltFx));
    g_bltFx.dwSize = sizeof(DDBLTFX);
    return 1;
}

RVA(0x00154ac0, 0x12)
void CImageSetRegistry::Unload() {
    ClearImageSets();
    g_resourceInstallActive = false;
    g_surfaceColorKey = 0;
}

RVA(0x00154ae0, 0xfc)
CImage* CImageSetRegistry::InsertFrameByKey(CRezItm* rec, const char* key, i32 index, i32 mode) {
    CObject* worker = NULL;
    m_imageSetsByName.Lookup(key, worker);
    if (worker == NULL) {

        worker = new CImageSet(m_ownerCtx, m_imageSetsByName.GetCount());
        if (static_cast<CImageSet*>(worker)->SetKey(key) == 0) {
            if (worker != NULL) {
                delete worker;
            }
            return NULL;
        }
        m_imageSetsByName.SetAt(key, worker);
    }
    return static_cast<CImageSet*>(worker)->InsertFrame(rec, index, mode);
}

RVA(0x00154be0, 0xfc)
CImage* CImageSetRegistry::LoadFrameByKey(char* path, const char* key, i32 index, i32 keyed) {
    CObject* worker = NULL;
    m_imageSetsByName.Lookup(key, worker);
    if (worker == NULL) {
        worker = new CImageSet(m_ownerCtx, m_imageSetsByName.GetCount());
        if (static_cast<CImageSet*>(worker)->SetKey(key) == 0) {
            if (worker != NULL) {
                delete worker;
            }
            return NULL;
        }
        m_imageSetsByName.SetAt(key, worker);
    }
    return static_cast<CImageSet*>(worker)->LoadFrame(path, index, keyed);
}

RVA(0x00154ce0, 0x101)
CImage* CImageSetRegistry::CreateDescriptorFrameByKey(
    PidHeader* desc,
    FileImageFormat mode,
    const char* key,
    i32 index,
    u32 size
) {
    CObject* worker = NULL;
    m_imageSetsByName.Lookup(key, worker);
    if (worker == NULL) {
        worker = new CImageSet(m_ownerCtx, m_imageSetsByName.GetCount());
        if (static_cast<CImageSet*>(worker)->SetKey(key) == 0) {
            if (worker != NULL) {
                delete worker;
            }
            return NULL;
        }
        m_imageSetsByName.SetAt(key, worker);
    }
    return static_cast<CImageSet*>(worker)->CreateDescriptorFrame(desc, mode, index, size);
}

RVA(0x00154df0, 0x101)
CImage* CImageSetRegistry::CreateBlankFrameByKey(
    i32 width,
    i32 height,
    const char* key,
    i32 index,
    i32 keyed
) {
    CObject* worker = NULL;
    m_imageSetsByName.Lookup(key, worker);
    if (worker == NULL) {
        worker = new CImageSet(m_ownerCtx, m_imageSetsByName.GetCount());
        if (static_cast<CImageSet*>(worker)->SetKey(key) == 0) {
            if (worker != NULL) {
                delete worker;
            }
            return NULL;
        }
        m_imageSetsByName.SetAt(key, worker);
    }
    return static_cast<CImageSet*>(worker)->CreateBlankFrame(width, height, index, keyed);
}

RVA(0x00154f00, 0x1b)
CImage*
CImageSetRegistry::LoadFrameForImageSet(char* path, CImageSet* worker, i32 index, i32 keyed) {
    return worker->LoadFrame(path, index, keyed);
}

RVA(0x00154f20, 0x1b)
CImage*
CImageSetRegistry::InsertFrameForImageSet(CRezItm* rec, CImageSet* worker, i32 index, i32 mode) {
    return worker->InsertFrame(rec, index, mode);
}

RVA(0x00154f40, 0x20)
CImage* CImageSetRegistry::CreateDescriptorFrameForImageSet(
    PidHeader* desc,
    FileImageFormat mode,
    CImageSet* worker,
    i32 index,
    u32 size
) {
    return worker->CreateDescriptorFrame(desc, mode, index, size);
}

RVA(0x00154f60, 0x20)
CImage* CImageSetRegistry::CreateBlankFrameForImageSet(
    i32 width,
    i32 height,
    CImageSet* worker,
    i32 index,
    i32 keyed
) {
    return worker->CreateBlankFrame(width, height, index, keyed);
}

RVA(0x00154f80, 0x1d5)
i32 CImageSetRegistry::LoadImageSetsFromTree(CRezDir* dir, const char* sub, const char* prefix) {
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
        count += LoadImageSetsFromTree(e, buf, prefix);
        e = dir->GetNextSubDir(e);
    }
    if (sub != NULL && *sub != 0) {
        CObject* w = NULL;
        m_imageSetsByName.Lookup(sub, w);
        if (w == NULL) {
            w = new CImageSet(m_ownerCtx, m_imageSetsByName.GetCount());
            if (static_cast<CImageSet*>(w)->SetKey(sub) == 0) {
                if (w != NULL) {
                    delete w;
                }
                return 0;
            }
            m_imageSetsByName.SetAt(sub, w);
        }
        static_cast<CImageSet*>(w)->BuildFramesFromArchive(dir);
        if (static_cast<CImageSet*>(w)->m_frames.GetSize() == 0) {
            RemoveByKey(sub);
        } else {
            ++count;
        }
    }
    delete[] buf;
    return count;
}

RVA(0x00155160, 0x11e)
i32 CImageSetRegistry::ReloadImageSetsFromTree(CRezDir* dir, const char* sub, const char* prefix) {
    char* buf = new char[0x100];
    i32 count = 0;
    CRezDir* e = dir->GetFirstSubDir();
    while (e != NULL) {
        if (sub != NULL && *sub != 0) {
            sprintf(buf, "%s%s%s", sub, prefix, e->GetDirName());
        } else {
            strcpy(buf, e->GetDirName());
        }
        i32 r = ReloadImageSetsFromTree(e, buf, prefix);
        if (r < 0) {
            delete[] buf;
            return -1;
        }
        count += r;
        e = dir->GetNextSubDir(e);
    }
    if (sub != NULL && *sub != 0) {
        CObject* out = NULL;
        m_imageSetsByName.Lookup(sub, out);
        if (out != NULL) {

            if (static_cast<CImageSet*>(out)->ReloadFramesFromArchive(dir) == -1) {
                delete[] buf;
                return -1;
            }
            if (static_cast<CImageSet*>(out)->m_frames.GetSize() > 0) {
                ++count;
            }
        }
    }
    delete[] buf;
    return count;
}

RVA(0x00155280, 0x22)
void CImageSetRegistry::RemoveImageSet(CImageSet* worker) {
    if (worker != NULL) {
        m_imageSetsByName.RemoveKey(worker->GetName());
        delete worker;
    }
}

RVA(0x001552b0, 0xa2)
void CImageSetRegistry::ClearImageSets() {
    CObject* val = NULL;
    POSITION pos = m_imageSetsByName.GetStartPosition();
    CString key;
    if (pos != NULL) {
        do {
            m_imageSetsByName.GetNextAssoc(pos, key, val);
            if (val != NULL) {
                delete (static_cast<CImageSet*>(val));
            }
        } while (pos != NULL);
    }
    m_imageSetsByName.RemoveAll();
}

RVA(0x00155360, 0xf8)
i32 CImageSetRegistry::RemoveWithPrefix(const char* prefix, const char* separator) {
    CString match(prefix);
    match += separator;
    i32 len = match.GetLength();
    CString key;
    CObject* val = NULL;
    POSITION pos = m_imageSetsByName.GetStartPosition();
    i32 n = 0;
    while (pos != NULL) {
        m_imageSetsByName.GetNextAssoc(pos, key, val);
        if (strncmp(key, match, len) == 0) {
            m_imageSetsByName.RemoveKey(key);
            if (val != NULL) {
                delete (static_cast<CImageSet*>(val));
            }
            ++n;
        }
    }
    return n;
}

RVA(0x00155460, 0xe2)
i32 CImageSetRegistry::GetMemoryUsageByPrefix(const char* str, i32 raw) {
    POSITION pos = m_imageSetsByName.GetStartPosition();
    i32 total = 0;
    CObject* val = NULL;
    CString key;
    while (pos != NULL) {
        val = NULL;
        m_imageSetsByName.GetNextAssoc(pos, key, val);
        if (val != NULL) {
            if (str == NULL || *str == 0) {
                total += (static_cast<CImageSet*>(val))->GetMemoryUsage(raw);
            } else if (strncmp(key, str, strlen(str)) == 0) {
                total += (static_cast<CImageSet*>(val))->GetMemoryUsage(raw);
            }
        }
    }
    return total;
}

RVA(0x00155550, 0xdc)
i32 CImageSetRegistry::HasWithPrefix(const char* prefix) {
    i32 len = strlen(prefix);
    CString key;
    CObject* val = NULL;
    POSITION pos = m_imageSetsByName.GetStartPosition();
    while (pos != NULL) {
        m_imageSetsByName.GetNextAssoc(pos, key, val);
        if (strncmp(key, prefix, len) == 0) {
            return 1;
        }
    }
    return 0;
}

RVA(0x00155630, 0xc5)
i32 CImageSetRegistry::FindFrameIdentity(CImage* frame, char* outName, i32* outIndex) {
    if (frame == NULL) {
        return 0;
    }
    CString key;
    CObject* val = NULL;
    POSITION pos = m_imageSetsByName.GetStartPosition();
    while (pos != NULL) {
        m_imageSetsByName.GetNextAssoc(pos, key, val);
        if (val != NULL && (static_cast<CImageSet*>(val))->FindFrame(frame, outName, outIndex)) {
            return 1;
        }
    }
    return 0;
}

RVA(0x00155700, 0x16)
i32 CWapObj::IsLoaded() {
    if (m_ownerCtx != NULL && m_id != -1) {
        return 1;
    }
    return 0;
}

RVA_COMPGEN(0x00155720, 0x1e, ??_GCWapObj@@UAEPAXI@Z)

RVA(0x00155740, 0x1)
void CWapObj::Unload() {}

RVA(0x00155750, 0x16)
i32 CImageSet::IsLoaded() {
    if (m_ownerCtx != NULL && m_id != -1) {
        return 1;
    }
    return 0;
}

RVA(0x00155770, 0x6)
LoadableClassId CImageSet::GetClassId() {
    return CLASSID_IMAGE_SET;
}

RVA_COMPGEN(0x00155780, 0x1e, ??_GCImageSet@@UAEPAXI@Z)
RVA(0x001557a0, 0x68)
CImageSet::~CImageSet() {

    Unload();
}

RVA(0x00155810, 0x23)
i32 CImageSet::SetKey(const char* src) {
    strncpy(m_name, src, 0x3f);
    m_name[0x3f] = 0;
    return 1;
}
