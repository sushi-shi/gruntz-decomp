#include <rva.h>
// CDDrawSubMgrLeaf.cpp - the tomalla-named class CDDrawSubMgrLeaf, a CObject-
// derived string-keyed catalog in the CDirectDrawMgr surface/page-manager ("Harry
// Potter") family (sibling of CDDrawWorkerRegistry). Its primary vftable is at
// RVA 0x1efc78; the scalar-deleting destructor (0x1577c0) calls the real
// ~CDDrawSubMgrLeaf at 0x1577e0.
//
// Layout (offsets/sizes load-bearing; field NAMES are placeholders):
//   +0x00  vptr (CObject-derived)
//   +0x04  m_04  (i32, -1 when inactive)
//   +0x08  m_08  (i32)
//   +0x0c  m_0c  (i32, parent/root handle)
//   +0x10  m_map (CMapStringToOb, 0x1c bytes; keyed by const char* name)
//
// CDDrawSubMgrLeaf owns a CMapStringToOb at +0x10 keyed by name; values are
// 0x28-byte CObject-derived node elements (vtable @ 0x5efba8) created by the
// factory (0x1528d0) from a directory tree (recursive walker at 0x152ad0). The
// teardown/scan helpers iterate the map via GetNextAssoc and destroy each value
// through its own scalar-deleting destructor (vtbl +0x4, arg 1) - the same idiom
// as the sibling CDDrawWorkerRegistry. Several methods carry a /GX EH frame for a
// local CString key, so the TU is flags="eh".
//
// VirtualMethodUnknown14 is the standard Lucius-derived readiness predicate (ready
// when +0x0c is present and +0x04 is not the -1 sentinel). VirtualMethodUnknown18
// clears a separate map then zeroes the handle. VirtualMethodUnknown1C is the
// cleanup virtual (tail-calls FreeAll).
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + emitted code
// bytes are load-bearing (campaign doctrine).
// ---------------------------------------------------------------------------

#include <Mfc.h> // real MFC CObject / CMapStringToOb / CString / POSITION

// The looked-up catalog value: only the scalar-deleting destructor slot (+0x04)
// is load-bearing. Declarations only - never defined here, so no ??_7 is emitted.
class CCatalogNode {
public:
    virtual void Slot00();            // +0x00
    virtual i32 ScalarDtor(i32 flag); // +0x04  scalar-deleting destructor
};

class CDDrawMapHolder {
public:
    void ClearUnknownMap();

    // Engine-label backlog stubs.
    void VirtualMethodUnknown14();
    ~CDDrawMapHolder();
};

// The catalog's primary vftable + the CObject grand-base dtor vtable, stamped by
// the destructors (reloc-masked DIR32 data). Declared early so the base dtor can
// reference the base vtable.
DATA(0x001efc78)
extern void* g_catalogVtbl; // 0x5efc78 - the leaf primary vftable
DATA(0x001e8cb4)
extern void* g_remusBaseDtorVtbl; // 0x5e8cb4 - the CObject base dtor vtable

// CDDrawSubMgrLucius - the family base subobject (+0x04..+0x0c). Its destructor
// resets the three header fields and restores the CObject base vtable; modeled so
// the leaf destructor's teardown ORDER (derived map member first, then this base)
// reproduces retail's `~CMapStringToOb` BEFORE the field stores.
class CDDrawSubMgrLucius {
public:
    ~CDDrawSubMgrLucius();

    void* m_vptr; // +0x00
    i32 m_04;     // +0x04  -1 when inactive
    i32 m_08;     // +0x08
    i32 m_0c;     // +0x0c  parent/root handle
};

inline CDDrawSubMgrLucius::~CDDrawSubMgrLucius() {
    m_04 = -1;
    m_08 = 0;
    m_0c = 0;
    m_vptr = &g_remusBaseDtorVtbl;
}

class CDDrawSubMgrLeaf : public CDDrawSubMgrLucius {
public:
    i32 VirtualMethodUnknown14();
    void VirtualMethodUnknown18();
    void VirtualMethodUnknown1C();

    CObject* LookupValue_06b2a0(const char* key);
    void FreeAll_152720();
    i32 RemoveKeysEqual_1527d0(const char* base, const char* str);
    i32 HasKeyPrefix_152c50(const char* str);
    CString KeyOfValue_152d30(CObject* target);
    ~CDDrawSubMgrLeaf();
    void* ScalarDtor_1577c0(i32 flag); // ??_G scalar-deleting destructor

    CMapStringToOb m_10; // +0x10  m_map
};

// ---------------------------------------------------------------------------
// Ready when the parent handle is present and the status word is not -1.
// ---------------------------------------------------------------------------
RVA(0x001577a0, 0x16)
i32 CDDrawSubMgrLeaf::VirtualMethodUnknown14() {
    if (m_0c == 0) {
        goto fail;
    }
    if (m_04 != -1) {
        return 1;
    }

fail:
    return 0;
}

// ---------------------------------------------------------------------------
// Clears the parent map then zeroes a member field.
// ---------------------------------------------------------------------------
RVA(0x00157ae0, 0x11)
void CDDrawSubMgrLeaf::VirtualMethodUnknown18() {
    ((class CDDrawMapHolder*)this)->ClearUnknownMap();
    m_0c = 0;
}

// ---------------------------------------------------------------------------
// Cleanup virtual: tail-calls FreeAll (frees every value + RemoveAll the map).
RVA(0x00152650, 0x5)
void CDDrawSubMgrLeaf::VirtualMethodUnknown1C() {
    FreeAll_152720();
}

// ---------------------------------------------------------------------------
// Look up `key` in the map; return the found value (or null), ignoring the bool.
RVA(0x0006b2a0, 0x23)
CObject* CDDrawSubMgrLeaf::LookupValue_06b2a0(const char* key) {
    CObject* val = 0;
    m_10.Lookup(key, val);
    return val;
}

// ---------------------------------------------------------------------------
// Free-all: iterate every entry in m_10 via GetNextAssoc, destroying each value
// via its scalar-deleting destructor (vtbl +0x4 arg 1), then RemoveAll the map.
// Same source as the 100%-matched CDDrawWorkerRegistry::MapTeardown_1552b0.
// @early-stop
// store-scheduling coin-flip (~94.7%) - complete & correct: byte-identical to
// retail EXCEPT the `CObject* val = 0` store position (retail schedules it after
// the CString ctor; MSVC5 here emits it before the count read) + the reloc-masked
// EH-state push. The surrounding symbol set re-rolls the allocator; the identical
// sibling source matched 100% elsewhere. docs/patterns/zero-register-pinning.md.
RVA(0x00152720, 0xa2)
void CDDrawSubMgrLeaf::FreeAll_152720() {
    CObject* val = 0;
    POSITION pos = (POSITION)(m_10.GetCount() != 0 ? -1 : 0);
    CString key;
    if (*(volatile i32*)&pos != 0) {
        do {
            m_10.GetNextAssoc(pos, key, val);
            if (val != 0) {
                ((CCatalogNode*)val)->ScalarDtor(1);
            }
        } while (pos != 0);
    }
    m_10.RemoveAll();
}

// ---------------------------------------------------------------------------
// Remove every entry whose key strncmp-equals `str` over the full length of the
// (CString-built) compare string, destroying each removed value via its scalar
// dtor; returns the count. The compare string is a CString built from `base`
// then assigned `str`.
// @early-stop
// regalloc wall (~91%) - complete & correct: logic/CFG/all calls/args/offsets
// reproduced. Residue is the val/loop-flag stack-slot swap + reloc-masked EH-state
// push, identical to the sibling CDDrawWorkerRegistry::RemoveKeysEqual_155360.
// docs/patterns/zero-register-pinning.md.
RVA(0x001527d0, 0xf8)
i32 CDDrawSubMgrLeaf::RemoveKeysEqual_1527d0(const char* base, const char* str) {
    CString match(base);
    match = str;
    i32 len = match.GetLength();
    i32 n = 0;
    CObject* val = 0;
    CString key;
    POSITION pos = (POSITION)(m_10.GetCount() != 0 ? -1 : 0);
    if (*(volatile i32*)&pos != 0) {
        do {
            m_10.GetNextAssoc(pos, key, val);
            if (strncmp(key, match, len) == 0) {
                m_10.RemoveKey(key);
                if (val != 0) {
                    ((CCatalogNode*)val)->ScalarDtor(1);
                }
                ++n;
            }
        } while (pos != 0);
    }
    return n;
}

// ---------------------------------------------------------------------------
// Return 1 if any key strncmp-equals `str` over strlen(str), else 0.
// @early-stop
// optimizer loop-peel wall (~61%) - complete & correct. MSVC5 peels the first
// iteration of this `do/while + early return`; body/calls/args match. Same wall
// as the sibling CDDrawWorkerRegistry::HasKeyEqual_155550.
// docs/patterns/zero-register-pinning.md.
RVA(0x00152c50, 0xdc)
i32 CDDrawSubMgrLeaf::HasKeyPrefix_152c50(const char* str) {
    i32 len = strlen(str);
    CObject* val = 0;
    CString key;
    POSITION pos = (POSITION)(m_10.GetCount() != 0 ? -1 : 0);
    if (*(volatile i32*)&pos != 0) {
        do {
            m_10.GetNextAssoc(pos, key, val);
            if (strncmp(key, str, len) == 0) {
                return 1;
            }
        } while (pos != 0);
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Reverse lookup: return (by value) the key of the entry whose value pointer ==
// `target`; an empty key for target==0 or no match.
// @early-stop
// NRVO/regalloc wall (~69%) - complete & correct: the empty-key short-circuit,
// the GetNextAssoc walk, the identity compare, and the `return key` copy-ctor
// path are reproduced. Residue is the by-value CString return (no-RVO empty-temp
// materialization on the no-match path) + the surrounding-symbol regalloc roll -
// the same family as the sibling's FindKeyOfValue_165360. Not source-steerable.
RVA(0x00152d30, 0xd4)
CString CDDrawSubMgrLeaf::KeyOfValue_152d30(CObject* target) {
    CString key;
    if (target == 0) {
        return key;
    }
    CObject* val = 0;
    POSITION pos = (POSITION)(m_10.GetCount() != 0 ? -1 : 0);
    if (*(volatile i32*)&pos != 0) {
        do {
            m_10.GetNextAssoc(pos, key, val);
            if (val == target) {
                return key;
            }
        } while (pos != 0);
    }
    key.Empty();
    return key;
}

// ---------------------------------------------------------------------------
// Destructor (real ??1 body; the scalar-deleting ??_G at 0x1577c0 calls it):
// stamp own vtable, cleanup virtual (FreeAll), then the embedded map dtor and the
// CDDrawSubMgrLucius base dtor (resets the three header fields + restores the
// CObject base vtable). The base-subobject model reproduces retail's teardown
// ORDER (~CMapStringToOb before the field stores) - 65%->95%. /GX EH frame.
// @early-stop
// reloc-masked EH-state push (~95%) - the instruction stream is byte-identical to
// retail; the only residue is the entry `push <ehfuncinfo>` operand (state-index
// constant, reloc-masked). docs/patterns/eh-state-numbering-base.md.
RVA(0x001577e0, 0x68)
CDDrawSubMgrLeaf::~CDDrawSubMgrLeaf() {
    m_vptr = &g_catalogVtbl;
    VirtualMethodUnknown1C();
    // implicit: ~m_10 (CMapStringToOb), then ~CDDrawSubMgrLucius (resets the three
    // header fields + restores the base vtable) - reproduces retail's teardown order.
}

// operator delete (called by the scalar-deleting dtor under the delete flag).
void operator delete(void*);

// ---------------------------------------------------------------------------
// Scalar-deleting destructor (the vtable slot+4 thunk): run the real ~, then
// operator delete this if the low flag bit is set. Out-of-line dtor -> the
// thunk emits `call ??1`. SYMBOL() pins the ??_G mangling (reloc-masked).
// @early-stop
// EH-dtor wall - the real ??1 destructor's member-dtor ordering / EH-state
// scheduling does not byte-match (the ~CMapStringToOb member dtor runs after the
// field resets in our model but in the middle in retail). The ??_G thunk shape
// matches; deferred to the final sweep. docs/patterns/eh-dtor-needs-base-subobject.md.
SYMBOL(??_GCDDrawSubMgrLeaf @@UAEPAXI@Z)
RVA(0x001577c0, 0x1e)
void* CDDrawSubMgrLeaf::ScalarDtor_1577c0(i32 flag) {
    this->~CDDrawSubMgrLeaf();
    if (flag & 1) {
        operator delete(this);
    }
    return this;
}

// Engine-label backlog stubs (moved from src/Stub/CDDrawMapHolder.cpp).

// @confidence: high
// @source: tomalla
// @stub
RVA(0x00157530, 0x17)
void CDDrawMapHolder::VirtualMethodUnknown14() {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x00157550, 0x1e)
CDDrawMapHolder::~CDDrawMapHolder() {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x00157bc0, 0xa2)
void CDDrawMapHolder::ClearUnknownMap() {}
