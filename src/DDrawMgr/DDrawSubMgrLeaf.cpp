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

VTBL(CAniElement, 0x001efba8);

DATA(0x0021ab18)
const char g_fmtPathJoin[] = "%s%s%s";

// @identity-TODO LookupValue@CDDrawSubMgrLeaf - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (12 fns) came from the static library. It belongs to another compiland.
RVA(0x0006b2a0, 0x23)
CObject* CDDrawSubMgrLeaf::LookupValue(const char* key) {
    void* val = 0;
    m_animations.Lookup(key, val);
    return static_cast<CObject*>(val);
}

RVA(0x00152640, 0x6)
i32 CDDrawSubMgrLeaf::IsReady() {
    return 1;
}
RVA(0x00152650, 0x5)
void CDDrawSubMgrLeaf::Unload() {
    FreeAll();
}

// @early-stop
RVA(0x00152660, 0xb2)
void CDDrawSubMgrLeaf::RemoveValue(CAniElement* target) {
    if (target == 0) {
        return;
    }
    POSITION pos = m_animations.GetStartPosition();
    CString key;
    CAniElement* val = 0;
    while (pos != 0) {
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
    if (pos != 0) {
        do {
            m_animations.GetNextAssoc(pos, key, val);
            if (val != 0) {
                delete (static_cast<CAniElement*>(val));
            }
        } while (pos != 0);
    }
    m_animations.RemoveAll();
}

RVA(0x001527d0, 0xf8)
i32 CDDrawSubMgrLeaf::RemoveKeysEqual(const char* base, const char* str) {
    CString match(base);
    match = str;
    i32 len = match.GetLength();
    CString key;
    void* val = 0;
    POSITION pos = m_animations.GetStartPosition();
    i32 n = 0;
    while (pos != 0) {
        m_animations.GetNextAssoc(pos, key, val);
        if (strncmp(key, match, len) == 0) {
            m_animations.RemoveKey(key);
            if (val != 0) {
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
    if (el == 0) {
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
    if (el == 0) {
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
    char* buf = static_cast<char*>(operator new(0x100));
    if (buf == 0) {
        return 0;
    }
    buf[0] = 0;
    CSymTab* node = static_cast<CSymTab*>(tree->FirstSub());
    while (node != 0) {
        if (prefix != 0 && *prefix != 0) {
            sprintf(buf, g_fmtPathJoin, prefix, suffix, node->m_name);
        } else {
            strcpy(buf, node->m_name);
        }
        count += ScanTree(node, buf, suffix);
        node = static_cast<CSymTab*>(tree->NextSub(node));
    }
    void* grp = tree->FirstSym();
    if (grp != 0) {
        do {

            CParseSource* fn = static_cast<CParseSource*>(tree->NextSym2(grp));
            while (fn != 0) {
                if (fn->GetEntryTag() == REZ_TAG_ANI) {
                    if (prefix != 0 && *prefix != 0) {
                        sprintf(buf, g_fmtPathJoin, prefix, suffix, fn->m_name);
                    } else {
                        strcpy(buf, fn->m_name);
                    }
                    if (CreateAniEntry(buf, fn) != 0) {
                        ++count;
                    }
                }
                fn = static_cast<CParseSource*>(tree->NextSym3(fn));
            }
            grp = tree->NextSym(grp);
        } while (grp != 0);
    }
    ::operator delete(buf);
    return count;
}

RVA(0x00152c50, 0xdc)
i32 CDDrawSubMgrLeaf::HasKeyPrefix(const char* str) {
    i32 len = strlen(str);
    CString key;
    void* val = 0;
    POSITION pos = m_animations.GetStartPosition();
    while (pos != 0) {
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
    if (target == 0) {
        return key;
    }
    void* val = 0;
    POSITION pos = m_animations.GetStartPosition();
    while (pos != 0) {
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
