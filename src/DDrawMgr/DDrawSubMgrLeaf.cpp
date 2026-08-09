#include <rva.h>

#include <DDrawMgr/DDrawSubMgrLeaf.h>

#include <Mfc.h>

#include <Bute/SymTab.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/ParseSource.h>
#include <Rez/RezTypeTag.h>
#include <Utils/MapTyped.h>

#include <stdio.h>
#include <string.h>

DATA(0x0021ab18)
// Retail has this at 0x21ab18, in `.data` (characteristics 0xc0000040,
// WRITE set) - so it is not const; spelled const it went to our `.rdata`
// and the storage mismatch withheld it from the manifest entirely.
char g_fmtPathJoin[] = "%s%s%s";

// @identity-TODO LookupValue@CDDrawSubMgrLeaf - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (12 fns) came from the static library. It belongs to another compiland.
RVA(0x00152640, 0x6)
i32 CDDrawSubMgrLeaf::IsReady() {
    return 1;
}
RVA(0x00152650, 0x5)
void CDDrawSubMgrLeaf::Unload() {
    FreeAll();
}

// @early-stop
// Same swapped CString/POSITION stack slots as CDDrawSubMgrLeafScan::RemoveByValue;
// declaration order is not the lever there either.
RVA(0x00152660, 0xb2)
void CDDrawSubMgrLeaf::RemoveValue(CAniElement* target) {
    if (target == NULL) {
        return;
    }
    POSITION pos = m_animations.GetStartPosition();
    CString key;
    CAniElement* val = 0;
    while (pos != NULL) {
        MapGetNext(m_animations, pos, key, val);
        if (target == val) {
            m_animations.RemoveKey(key);
            delete target;
            break;
        }
    }
}

RVA(0x00152720, 0xa2)
void CDDrawSubMgrLeaf::FreeAll() {
    POSITION pos = m_animations.GetStartPosition();
    CString key;
    void* val = 0;
    if (pos != NULL) {
        do {
            m_animations.GetNextAssoc(pos, key, val);
            if (val != NULL) {
                delete (static_cast<CAniElement*>(val));
            }
        } while (pos != NULL);
    }
    m_animations.RemoveAll();
}

RVA(0x001527d0, 0xf8)
i32 CDDrawSubMgrLeaf::RemoveKeysEqual(const char* base, const char* str) {
    CString match(base);
    match += str;
    i32 len = match.GetLength();
    CString key;
    void* val = 0;
    POSITION pos = m_animations.GetStartPosition();
    i32 n = 0;
    while (pos != NULL) {
        m_animations.GetNextAssoc(pos, key, val);
        if (strncmp(key, match, len) == 0) {
            m_animations.RemoveKey(key);
            if (val != NULL) {
                delete (static_cast<CAniElement*>(val));
            }
            ++n;
        }
    }
    return n;
}

RVA(0x001528d0, 0xdd)
CAniElement* CDDrawSubMgrLeaf::CreateAniEntry(const char* key, void* entry) {
    CAniElement* el = new CAniElement;
    if (el == NULL) {
        return 0;
    }
    if (el->Configure(OwnerMgr()->m_soundRegistry, entry, 0) == 0) {

        delete el;
        return 0;
    }
    m_animations[key] = el;
    return el;
}

RVA(0x001529b0, 0xdd)
CAniElement* CDDrawSubMgrLeaf::CreateAniEntry2(const char* key, void* entry) {
    CAniElement* el = new CAniElement;
    if (el == NULL) {
        return 0;
    }
    if (el->LoadFile(OwnerMgr()->m_soundRegistry, entry, 0) == 0) {

        delete el;
        return 0;
    }
    m_animations[key] = el;
    return el;
}

RVA(0x00152ad0, 0x17f)
i32 CDDrawSubMgrLeaf::ScanTree(CSymTab* tree, const char* prefix, const char* suffix) {
    i32 count = 0;
    char* buf = new char[0x100];
    if (buf == NULL) {
        return 0;
    }
    buf[0] = 0;
    CSymTab* node = static_cast<CSymTab*>(tree->FirstSub());
    while (node != NULL) {
        if (prefix != NULL && *prefix != 0) {
            sprintf(buf, g_fmtPathJoin, prefix, suffix, node->m_name);
        } else {
            strcpy(buf, node->m_name);
        }
        count += ScanTree(node, buf, suffix);
        node = static_cast<CSymTab*>(tree->NextSub(node));
    }
    void* grp = tree->FirstSym();
    if (grp != NULL) {
        do {

            CParseSource* fn = static_cast<CParseSource*>(tree->NextSym2(grp));
            while (fn != NULL) {
                if (fn->GetEntryTag() == REZ_TAG_ANI) {
                    if (prefix != NULL && *prefix != 0) {
                        sprintf(buf, g_fmtPathJoin, prefix, suffix, fn->m_name);
                    } else {
                        strcpy(buf, fn->m_name);
                    }
                    if (CreateAniEntry(buf, fn) != NULL) {
                        ++count;
                    }
                }
                fn = static_cast<CParseSource*>(tree->NextSym3(fn));
            }
            grp = tree->NextSym(grp);
        } while (grp != NULL);
    }
    delete[] buf;
    return count;
}

RVA(0x00152c50, 0xdc)
i32 CDDrawSubMgrLeaf::HasKeyPrefix(const char* str) {
    i32 len = strlen(str);
    CString key;
    void* val = 0;
    POSITION pos = m_animations.GetStartPosition();
    while (pos != NULL) {
        m_animations.GetNextAssoc(pos, key, val);
        if (strncmp(key, str, len) == 0) {
            return 1;
        }
    }
    return 0;
}

RVA(0x00152d30, 0xd4)
CString CDDrawSubMgrLeaf::KeyOfValue(CObject* target) {
    CString key;
    if (target == NULL) {
        return key;
    }
    void* val = 0;
    POSITION pos = m_animations.GetStartPosition();
    while (pos != NULL) {
        m_animations.GetNextAssoc(pos, key, val);
        if (val == target) {
            return key;
        }
    }
    key.Empty();
    return key;
}

RVA_COMPGEN(0x00152e10, 0x1e, ??_GCAniElement@@UAEPAXI@Z)
RVA(0x00152e30, 0x53)
CAniElement::~CAniElement() {
    DeleteAll();
}
