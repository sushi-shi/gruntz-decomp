// DDrawWorkerRegistry.cpp - the 0x1549d0-0x155833 original TU (wave4-L dossier
// #15, block E; the leading 0x1549d0 CResolveNode COMDAT pocket rides this obj's
// contribution and stays in ResolveNode.cpp): the CDDrawWorkerRegistry keyed
// worker registry - the four DispatchKeyed*/Forward* pairs, the tree
// install/load-namespace walkers, the map scans/teardowns, ~CDDrawWorker, and the
// ??_GCDDrawSubMgr far-sibling scalar dtor. Held at the dossier-#9 boundary 1
// (0x155840); the registry's tiny virtuals in the G obj + its DestroyAll/
// FindKeyOfValue in the T obj live in those hosts.
//
// original TU: filename unknown (@identity-TODO - no __FILE__ anchor).
//
// Field names are placeholders; only the OFFSETS + emitted code bytes are load-
// bearing (campaign doctrine).
#include <rva.h>
#include <Bute/SymTab.h>
#include <Image/ImageSet.h>

#include <Gruntz/StateId.h> // StateId (GetStateId return type)
#include <Mfc.h>
#include <Win32.h> // windows.h base types (ddraw.h needs them first)
#include <ddraw.h> // DDBLTFX (g_bltFx - the shared BltEx fx block)
#include <string.h> // strncpy (the StringCopy leaf, reloc-masked)
#include <stdio.h>  // sprintf ("%s%s%s" path builder in InstallTree / LoadNamespace)
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDrawWorker.h> // the ONE canonical keyed worker (vtable 0x1efbe8)
#include <Gruntz/String.h>
#include <Gruntz/MapStringToOb.h>
#include <Gruntz/Loadable.h> // the ONE canonical CLoadable base
#include <Image/CImage.h> // ex Globals.h

VTBL(CDDrawWorker, 0x001efbe8); // ??_7CDDrawWorker@@6B@ (17-slot CLoadable-derived vtable)
inline void* operator new(u32, void* p) {
    return p;
}

void operator delete(void*);

// The linker-kept COMDAT pair of CLoadable's (A)-form inline dtor - ??_G
// @0x155720 (this obj's span) calling ??1 @0xd5d70 (the CImage-band COMDAT pool)
// via the ILT thunk 0x429b. This TU `new`s/destroys CDDrawWorker (: CLoadable),
// so cl auto-emits BOTH as byte-identical COMDATs (verified llvm-objdump -dr:
// ??1 = xor eax,eax; m_04=-1; m_08/m_0c=0; the ??_7CObject re-stamp; ??_G = the
// standard call-~/test-flag/operator-delete shell). They are bound below by
// RVA_COMPGEN - no source spelling is possible (the canonical dtor is inline in
// <Gruntz/Loadable.h>; one-definition rule). Was the CDDrawSubMgrFar scaffold
// class (hand-written ScalarDtor + a second fabricated view in CImage.cpp) -
// dissolved onto the real ??1CLoadable/??_GCLoadable identities.
RVA_COMPGEN(0x000d5d70, 0x16, ??1CLoadable@@UAE@XZ)

static inline i32 ReadRegistryField1c(const CDDrawWorkerRegistry* p) {
    return *reinterpret_cast<const i32*>((reinterpret_cast<const char*>(p) + 0x1c));
}

static inline CDDrawWorker* MakeWorker(const CDDrawWorkerRegistry* parent) {
    // `new CDDrawWorker`: base ctor stamps 0x5efc30, the m_items CObArray member is
    // default-constructed, the derived ctor stamps 0x5efbe8 (cl-implicit vptr stores).
    CDDrawWorker* w = new CDDrawWorker;
    if (w != 0) {
        i32 field1c = ReadRegistryField1c(parent);
        i32 surfaceMgr = parent->m_ownerCtx;
        w->m_id = field1c;
        w->m_flags = 0;
        w->m_ownerCtx = surfaceMgr;
        w->m_minIndex = 99999;
        w->m_maxIndex = 0;
    }
    return w;
}

static inline CDDrawWorker* FindOrCreateWorker(CDDrawWorkerRegistry* parent, const char* key) {
    CObject* found = 0;
    parent->m_10map.Lookup(key, found);
    if (found == 0) {
        CDDrawWorker* worker = MakeWorker(parent);
        if (worker->SetKey(key) == 0) {
            if (worker != 0) {
                delete worker;
            }
            return 0;
        }
        parent->m_10map[key] = static_cast<CObject*>(worker);
        found = static_cast<CObject*>(worker);
    }
    return static_cast<CDDrawWorker*>(found);
}

RVA(0x00154aa0, 0x20)
i32 CDDrawWorkerRegistry::IsReady() {
    memset(&g_bltFx, 0, sizeof(g_bltFx));
    g_bltFx.dwSize = sizeof(DDBLTFX); // 100
    return 1;
}

RVA(0x00154ac0, 0x12)
void CDDrawWorkerRegistry::Unload() {
    MapTeardown();
    g_resourceInstallActive = 0;
    g_surfaceColorKey = 0;
}

RVA(0x00154ae0, 0xfc)
i32 CDDrawWorkerRegistry::DispatchKeyed38(void* rec, const char* key, i32 a3, i32 a4) {
    CDDrawWorker* worker = FindOrCreateWorker(this, key);
    if (worker == 0) {
        return 0;
    }
    return reinterpret_cast<i32>(worker->InsertFrame(rec, a3, a4)); // slot 14 returns CImage*; forwarded as i32
}

RVA(0x00154be0, 0xfc)
i32 CDDrawWorkerRegistry::DispatchKeyed34(i32 a1, const char* key, i32 a3, i32 a4) {
    CDDrawWorker* worker = FindOrCreateWorker(this, key);
    if (worker == 0) {
        return 0;
    }
    return reinterpret_cast<i32>(worker->CreateFrame30(a1, a3, a4)); // slot returns CImage*; forwarded as i32
}

RVA(0x00154ce0, 0x101)
i32 CDDrawWorkerRegistry::DispatchKeyed30(i32 a1, i32 a2, const char* key, i32 a4, i32 a5) {
    CDDrawWorker* worker = FindOrCreateWorker(this, key);
    if (worker == 0) {
        return 0;
    }
    return reinterpret_cast<i32>(worker->CreateFrame28(a1, a2, a4, a5)); // slot returns CImage*; forwarded as i32
}

RVA(0x00154df0, 0x101)
i32 CDDrawWorkerRegistry::DispatchKeyed2C(i32 a1, i32 a2, const char* key, i32 a4, i32 a5) {
    CDDrawWorker* worker = FindOrCreateWorker(this, key);
    if (worker == 0) {
        return 0;
    }
    return reinterpret_cast<i32>(worker->CreateFrame24(a1, a2, a4, a5)); // slot returns CImage*; forwarded as i32
}

RVA(0x00154f00, 0x1b)
i32 CDDrawWorkerRegistry::Forward34(i32 a1, CDDrawWorker* worker, i32 a3, i32 a4) {
    return reinterpret_cast<i32>(worker->CreateFrame30(a1, a3, a4)); // slot returns CImage*; forwarded as i32
}

RVA(0x00154f20, 0x1b)
i32 CDDrawWorkerRegistry::Forward38(void* rec, CDDrawWorker* worker, i32 a3, i32 a4) {
    return reinterpret_cast<i32>(worker->InsertFrame(rec, a3, a4)); // slot 14 returns CImage*; forwarded as i32
}

RVA(0x00154f40, 0x20)
i32 CDDrawWorkerRegistry::Forward30(i32 a1, i32 a2, CDDrawWorker* worker, i32 a4, i32 a5) {
    return reinterpret_cast<i32>(worker->CreateFrame28(a1, a2, a4, a5)); // slot returns CImage*; forwarded as i32
}

RVA(0x00154f60, 0x20)
i32 CDDrawWorkerRegistry::Forward2C(i32 a1, i32 a2, CDDrawWorker* worker, i32 a4, i32 a5) {
    return reinterpret_cast<i32>(worker->CreateFrame24(a1, a2, a4, a5)); // slot returns CImage*; forwarded as i32
}

// ---------------------------------------------------------------------------
// 0x154f80: walk the directory tree under `dir`; for each entry build a path string
// ("<sub><prefix><name>" when sub is set, else just the name) into a 0x100 heap
// buffer and accumulate this->+0x48(entry, buf, prefix). Then, when sub is set,
// find-or-create the keyed worker, dispatch its +0x28(dir), and either run
// this->+0x54(sub) (worker inactive) or bump the count. /GX EH frame.
// @early-stop
// worker-ctor + regalloc wall: the directory walk / sprintf-vs-strcpy / +0x48
// dispatch / find-or-create / +0x28 / status branch are reproduced; retail's
// inline worker construction seeds fields before the CByteArray ctor + stamps the
// derived vtable last, and the buffer/entry register schedule differs.
// Reloc-masked EH-state + map/thunk names. Logic/CFG/offsets complete.
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
        count += InstallTree(e, buf, prefix); // recursive slot-18 self-dispatch (+0x48)
        e = static_cast<CSymTab*>(dir->NextSub(e));
    }
    if (sub != 0 && *sub != 0) {
        CDDrawWorker* w = FindOrCreateWorker(this, sub);
        if (w == 0) {
            return 0;
        }
        w->BuildFramesFromSymTab(dir);
        if (w->m_items.GetSize() == 0) {
            RemoveByKey(sub); // slot-21 self-dispatch (+0x54)
        } else {
            ++count;
        }
    }
    operator delete(buf);
    return count;
}

// ---------------------------------------------------------------------------
// 0x155160 LoadNamespace (slot 19): the read-side twin of InstallTree - walk the directory tree, build
// the same path string, and accumulate this->+0x4c(entry, buf, prefix) (a negative
// result aborts to -1). Then, when sub is set, Lookup it in the map; if present,
// dispatch its +0x3c(dir) (a -1 aborts to -1) and bump the count when the value's
// +0x18 is positive. No EH frame (plain /O2 leaf).
// @early-stop
// regalloc/loop-schedule wall: the walk / sprintf-vs-strcpy / +0x4c dispatch + <0
// abort / Lookup / +0x3c dispatch + -1 abort / +0x18 count are reproduced; the
// buffer/entry/count register schedule + reloc-masked thunk names are the residual.
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
        i32 r = LoadNamespace(e, buf, prefix); // recursive slot-19 self-dispatch (+0x4c)
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
            // Typed map-value retrieval: the stored values are CDDrawWorker.
            CDDrawWorker* w = static_cast<CDDrawWorker*>(out);
            if (w->ValidateFramesFromSymTab(dir) == -1) {
                operator delete(buf);
                return -1;
            }
            if (w->m_items.GetSize() > 0) {
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
    POSITION pos = reinterpret_cast<POSITION>((m_10map.GetCount() != 0 ? -1 : 0));
    CString key;
    if (*reinterpret_cast<volatile i32*>(&pos) != 0) {
        do {
            m_10map.GetNextAssoc(pos, key, val);
            if (val != 0) {
                delete (static_cast<CDDrawWorker*>(val)); // the map values ARE the keyed workers
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
                delete (static_cast<CDDrawWorker*>(val)); // the map values ARE the keyed workers
            }
            ++n;
        }
    }
    return n;
}

// ---------------------------------------------------------------------------
// Map scan: sum each non-null value's GetMemoryUsage(a2) over the entries whose
// key strncmp-matches `str` (a null/empty `str` matches every entry). /GX frame.
// @early-stop
// zero-register-pin wall (~69%): the GetNextAssoc scan, match-all guard,
// strlen+strncmp compare, and per-value thiscall accumulation all reproduced.
// Residue: retail pins 0 in edi and holds a2 in ebx across the body where our
// cl uses test/immediate + a per-call a2 reload - regalloc coin-flip.
RVA(0x00155460, 0xe2)
i32 CDDrawWorkerRegistry::SumSizesEqual(const char* str, i32 a2) {
    CString key;
    CObject* val = 0;
    POSITION pos = m_10map.GetStartPosition();
    i32 total = 0;
    if (pos != 0) {
        do {
            m_10map.GetNextAssoc(pos, key, val);
            if (val != 0) {
                if (str == 0 || *str == 0 || strncmp(key, str, strlen(str)) == 0) {
                    total += (static_cast<CDDrawWorker*>(val))->GetMemoryUsage(a2);
                }
            }
        } while (pos != 0);
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

// CLoadable::IsLoaded (0x155700): the slot-5 base default - loaded iff the owner
// context (m_0c) is set and m_04 != -1. Identical body to CDDrawWorker's override
// below (MSVC5 has no /OPT:ICF, so both are emitted). Un-phantoms the slot.
RVA(0x00155700, 0x16)
i32 CLoadable::IsLoaded() {
    if (m_ownerCtx != 0 && m_id != -1) {
        return 1;
    }
    return 0;
}

// (0x155720 = ??_GCLoadable - the auto-emitted COMDAT, RVA_COMPGEN-bound at the
// file head; the hand-written CDDrawSubMgrFar::ScalarDtor stand-in is dissolved.)
// (CLoadable::Unload @0x155740 is a bare `ret` = i32-slot no-op; MSVC5 rejects an
//  empty-body non-void fn (C2561) and the slot can't be void (derived overrides
//  return their eax residue), so this base default stays declared-only.)

RVA_COMPGEN(0x00155720, 0x1e, ??_GCLoadable@@UAEPAXI@Z)

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

// ===========================================================================
// 0x1557a0 - ~CDDrawWorker: stamp own vtable, run the slot-7 Unload
// (devirtualized in the dtor to a direct call - the body lives in the S1 obj,
// reloc-masked), then the array member destructs and ~CLoadable folds in.
// /GX frame from the destructible base+member.
// ===========================================================================
// 100%: re-basing onto the canonical CLoadable : CWapObj : CObject resolved the
// grand-base vptr-stamp-position wall - the real CObject grand-base sinks the
// 0x5e8cb4 re-stamp after the m_04/m_08/m_0c resets exactly as retail.
// The cl-auto scalar-deleting destructor (vtable slot 1; generated from the
// virtual dtor below - RVA_COMPGEN pairs the retail copy with the base COMDAT).
RVA_COMPGEN(0x00155780, 0x1e, ??_GCDDrawWorker@@UAEPAXI@Z)
RVA(0x001557a0, 0x68)
CDDrawWorker::~CDDrawWorker() {
    // retail: a DIRECT call - cl devirtualizes virtual calls in a dtor (proven
    // by ~CDDrawWorkerCache's plain DestroyAll() compiling to `call 0x165210`).
    Unload(); // slot-7 body @0x151eb0
    // m_items.~::CObArray() (trylevel 0) + ~CLoadable() (field resets +
    // grand-base vtable stamp) fold here.
}

RVA(0x00155810, 0x23)
i32 CDDrawWorker::SetKey(const char* src) {
    strncpy(m_name, src, 0x3f);
    m_name[0x3f] = 0;
    return 1;
}
