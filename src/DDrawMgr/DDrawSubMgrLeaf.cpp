#include <rva.h>
// DDrawSubMgrLeaf.cpp - the tomalla-named class CDDrawSubMgrLeaf, a CObject-
// derived string-keyed catalog in the CDirectDrawMgr surface/page-manager (DDraw
// surface-manager) family (sibling of CDDrawWorkerRegistry). Its primary vftable is at
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
// IsReady is the standard CDDrawSubMgr-derived readiness predicate (ready
// when +0x0c is present and +0x04 is not the -1 sentinel). ClearContext
// clears a separate map then zeroes the handle. Cleanup is the
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
    virtual void FUN_005bef01();      // [0] 0x1bef01 (shared thunk, declared-only)
    virtual i32 ScalarDtor(i32 flag); // +0x04  scalar-deleting destructor
};

// CDDrawSubMgrLeafScan: the real sibling class (its full body + own vtable 0x5efca0
// live in CDDrawSubMgrLeafScan.cpp). Three of its methods landed in THIS TU: the
// readiness predicate (vtable slot [5], 0x157530), the map-clear (0x157bc0), and the
// ??_G scalar-deleting dtor (0x157550, which forwards to the real member-teardown
// ~CDDrawSubMgrLeafScan at 0x157570 in the sibling TU). It owns a name-keyed map at
// +0x10 plus two flag fields at +0x2c/+0x30. Polymorphic (vptr @+0x00); the virtual
// dtor's key function is out-of-line (own TU), so no ??_7 is emitted here.
class CDDrawSubMgrLeafScan {
public:
    virtual ~CDDrawSubMgrLeafScan(); // 0x157570 (member-teardown, own TU)
    i32 IsReady();                   // 0x157530  (vtable slot [5], readiness predicate)
    void ClearMap();                 // 0x157bc0
    void* ScalarDtor(i32 flag);      // 0x157550  (??_G scalar-deleting dtor)

    // vptr @+0x00 (implicit)
    char m_pad04[0x10 - 0x04]; // +0x04 .. +0x0f
    CMapStringToOb m_10;       // +0x10  name-keyed value map (0x10..0x2b)
    i32 m_2c;                  // +0x2c
    i32 m_30;                  // +0x30
};

// CDDrawSubMgrGrandBase - the CObject-like family grand-base (vptr + the three header
// fields +0x04..+0x0c). Modeled as a REAL polymorphic base (its 5-slot vtable is
// the shared g_wapObjectDtorVtbl @0x5e8cb4 = sub_1bef01 / scalar-dtor / sub_0028ec /
// sub_00106e / sub_004034) so cl emits the implicit grand-base vptr re-stamp (masks
// 0x5e8cb4) at the leaf dtor's tail - no manual `*(void**)this = &g_wapObjectDtorVtbl`.
// Slot 1 is a REGULAR virtual (not a C++ dtor) so the leaf can override it with its
// explicit ??_G scalar-deleting destructor ScalarDtor_1577c0 WITHOUT cl auto-generating
// a clashing ??_G. The field resets live in the non-virtual ~ (its body); the base
// transition stamp is implicit (the leaf dtor teardown ORDER: ~CMapStringToOb member
// BEFORE the field stores reproduces retail).
// NAME-AUDIT (vtable_hierarchy --name-audit): maps to RTTI CObject @0x1e8cb4, but
// KEPT as a real intermediate - it carries the m_04/m_08/m_0c header past the bare
// vptr, so it is NOT a bare-Wap::CObject fold (Wap32/Object.h). Do not rename to
// CObject (would ODR-clash + collapse the /GX dtor teardown level).
class CDDrawSubMgrGrandBase {
public:
    virtual void FUN_005bef01();        // [0] 0x1bef01 (shared thunk, declared-only)
    virtual void* ScalarDtor(i32 flag); // [1] scalar-deleting dtor (regular virtual)
    virtual void FUN_004028ec();        // [2] 0x0028ec (shared thunk, declared-only)
    virtual void FUN_0040106e();        // [3] 0x00106e (shared thunk, declared-only)
    virtual void FUN_00404034();        // [4] 0x004034 (shared thunk, declared-only)
    ~CDDrawSubMgrGrandBase();

    i32 m_04; // +0x04  -1 when inactive
    i32 m_08; // +0x08
    i32 m_0c; // +0x0c  parent/root handle
    CDDrawSubMgrGrandBase() {}
};

inline CDDrawSubMgrGrandBase::~CDDrawSubMgrGrandBase() {
    m_04 = -1;
    m_08 = 0;
    m_0c = 0;
}

class CDDrawSubMgrLeaf : public CDDrawSubMgrGrandBase {
public:
    // The leaf vtable (??_7CDDrawSubMgrLeaf @0x5efc78) is 9 slots: 5 shared CObject
    // slots from CDDrawSubMgrGrandBase (slot 1 overridden below by ScalarDtor_1577c0),
    // then 4 leaf virtuals at slots 5..8 in declaration order (the unreconstructed
    // slots 6/8 are declared-only -> reloc-masked references).
    void* ScalarDtor(i32 flag) OVERRIDE; // [1] ??_G scalar-deleting destructor (0x1577c0)
    virtual i32 IsReady();               // [5] 0x1577a0
    virtual i32 FUN_00552640();          // [6] 0x152640 (state predicate, returns 1)
    virtual void Cleanup();              // [7] 0x152650
    virtual void FUN_00554a00();         // [8] 0x154a00 (shared, declared-only)

    // Non-vtable members.
    void ClearContext(); // 0x157ae0 (not a vtable slot)
    CObject* LookupValue_06b2a0(const char* key);
    void RemoveValue_152660(CCatalogNode* target);
    void FreeAll_152720();
    i32 RemoveKeysEqual_1527d0(const char* base, const char* str);
    i32 HasKeyPrefix_152c50(const char* str);
    CString KeyOfValue_152d30(CObject* target);
    ~CDDrawSubMgrLeaf();

    CMapStringToOb m_10; // +0x10  m_map
};

// ---------------------------------------------------------------------------
// Ready when the parent handle is present and the status word is not -1.
// ---------------------------------------------------------------------------
RVA(0x001577a0, 0x16)
i32 CDDrawSubMgrLeaf::IsReady() {
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
void CDDrawSubMgrLeaf::ClearContext() {
    ((CDDrawSubMgrLeafScan*)this)->ClearMap();
    m_0c = 0;
}

// ---------------------------------------------------------------------------
// Cleanup virtual: tail-calls FreeAll (frees every value + RemoveAll the map).
RVA(0x00152650, 0x5)
void CDDrawSubMgrLeaf::Cleanup() {
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
// 0x152660: remove ONE value by pointer identity. When `target` is non-null,
// walk the map via GetNextAssoc; on the first entry whose value pointer equals
// `target`, RemoveKey it, destroy `target` through its scalar-deleting dtor
// (vtbl +0x4 arg 1), and stop. /GX EH frame for the local CString key. 1 arg.
// @early-stop
// ~99.85% - map-scan idiom (top-tested while + real GetStartPosition kills the
// peel, pos declared before key computes it before the ctor like retail; docs/
// patterns/mfc-map-walk-while-not-guard-dowhile.md). Every instruction byte
// matches; the sole residue is a key<->pos stack-slot swap (retail key=[esp+0x20]/
// pos=[esp+0x8], ours the reverse) - the stack-slot-coalesce coin-flip, only the
// [esp+N] displacement bytes differ. docs/patterns/stack-slot-coalesce-frame-4b.md.
RVA(0x00152660, 0xb2)
void CDDrawSubMgrLeaf::RemoveValue_152660(CCatalogNode* target) {
    if (target == 0) {
        return;
    }
    POSITION pos = m_10.GetStartPosition();
    CString key;
    CObject* val = 0;
    while (pos != 0) {
        m_10.GetNextAssoc(pos, key, val);
        if ((CObject*)target == val) {
            m_10.RemoveKey(key);
            target->ScalarDtor(1);
            break;
        }
    }
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
    CString key;
    CObject* val = 0;
    POSITION pos = m_10.GetStartPosition();
    i32 n = 0;
    while (pos != 0) {
        m_10.GetNextAssoc(pos, key, val);
        if (strncmp(key, match, len) == 0) {
            m_10.RemoveKey(key);
            if (val != 0) {
                ((CCatalogNode*)val)->ScalarDtor(1);
            }
            ++n;
        }
    }
    return n;
}

// ---------------------------------------------------------------------------
// Return 1 if any key strncmp-equals `str` over strlen(str), else 0.
RVA(0x00152c50, 0xdc)
i32 CDDrawSubMgrLeaf::HasKeyPrefix_152c50(const char* str) {
    i32 len = strlen(str);
    CString key;
    CObject* val = 0;
    POSITION pos = m_10.GetStartPosition();
    while (pos != 0) {
        m_10.GetNextAssoc(pos, key, val);
        if (strncmp(key, str, len) == 0) {
            return 1;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Reverse lookup: return (by value) the key of the entry whose value pointer ==
// `target`; an empty key for target==0 or no match. The map-scan idiom (top-tested
// while + real GetStartPosition, key.Empty() before the final return key) closes it.
RVA(0x00152d30, 0xd4)
CString CDDrawSubMgrLeaf::KeyOfValue_152d30(CObject* target) {
    CString key;
    if (target == 0) {
        return key;
    }
    CObject* val = 0;
    POSITION pos = m_10.GetStartPosition();
    while (pos != 0) {
        m_10.GetNextAssoc(pos, key, val);
        if (val == target) {
            return key;
        }
    }
    key.Empty();
    return key;
}

// ---------------------------------------------------------------------------
// Destructor (real ??1 body; the scalar-deleting ScalarDtor at 0x1577c0 calls it):
// now a real polymorphic teardown. cl stamps ??_7CDDrawSubMgrLeaf (masks g_catalogVtbl
// @0x5efc78) at entry, runs the cleanup virtual (FreeAll/VM1C), then the embedded map
// dtor and the CDDrawSubMgrGrandBase grand-base dtor (field resets + implicit ??_7-base
// re-stamp masking 0x5e8cb4). No manual `m_vptr = &g_*Vtbl`. /GX EH frame.
// @early-stop
// vptr-position wall + reloc-masked EH-state push (~95%): the instruction stream is
// byte-identical to retail EXCEPT the grand-base re-stamp position (cl emits it before
// the m_04/m_08/m_0c resets; retail sinks it after - the implicit base transition
// forces stamp-first; same wall as CDDrawWorker/CDDrawWorkerMapSmall) + the entry
// `push <ehfuncinfo>` reloc operand. docs/patterns/eh-state-numbering-base.md.
RVA(0x001577e0, 0x68)
CDDrawSubMgrLeaf::~CDDrawSubMgrLeaf() {
    Cleanup();
    // implicit: ~m_10 (CMapStringToOb), then ~CDDrawSubMgrGrandBase (resets the three
    // header fields + restamps the base vtable) - reproduces retail's teardown order.
}

// operator delete (called by the scalar-deleting dtor under the delete flag).
void operator delete(void*);

// ---------------------------------------------------------------------------
// Scalar-deleting destructor (the vtable slot+4 override): run the real ~, then
// operator delete this if the low flag bit is set. Out-of-line dtor -> the
// thunk emits `call ??1`. SYMBOL() pins the ??_G mangling (reloc-masked); it
// overrides CDDrawSubMgrGrandBase's slot-1 regular virtual so the leaf vtable carries
// it at slot 1 WITHOUT cl auto-generating a clashing ??_G.
SYMBOL(??_GCDDrawSubMgrLeaf @@UAEPAXI@Z)
RVA(0x001577c0, 0x1e)
void* CDDrawSubMgrLeaf::ScalarDtor(i32 flag) {
    this->~CDDrawSubMgrLeaf();
    if (flag & 1) {
        operator delete(this);
    }
    return this;
}

// Engine-label backlog stubs (moved from src/Stub/CDDrawMapHolder.cpp).

// Leaf vtable slot [6] (0x152640): constant state predicate returning 1.
RVA(0x00152640, 0x6)
i32 CDDrawSubMgrLeaf::FUN_00552640() {
    return 1;
}

// ---------------------------------------------------------------------------
// Readiness predicate: ready when either flag field (+0x2c / +0x30) is set.
RVA(0x00157530, 0x17)
i32 CDDrawSubMgrLeafScan::IsReady() {
    if (m_2c != 0) {
        return 1;
    }
    if (m_30 != 0) {
        return 1;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Scalar-deleting destructor (??_G of CDDrawSubMgrLeafScan at 0x157550): run the real
// member-teardown ~CDDrawSubMgrLeafScan (0x157570) then operator delete under the flag.
SYMBOL(??_GCDDrawSubMgrLeafScan @@UAEPAXI@Z)
RVA(0x00157550, 0x1e)
void* CDDrawSubMgrLeafScan::ScalarDtor(i32 flag) {
    this->CDDrawSubMgrLeafScan::~CDDrawSubMgrLeafScan();
    if (flag & 1) {
        operator delete(this);
    }
    return this;
}

// ---------------------------------------------------------------------------
// 0x157bc0: iterate every entry of the name-keyed map via GetNextAssoc, destroying
// each value through its scalar-deleting destructor (vtbl +0x4 arg 1), then RemoveAll.
// /GX EH frame for the local CString key. Same shape as FreeAll_152720.
// @early-stop
// store-scheduling coin-flip (~94%): byte-identical to retail except the `val = 0`
// store position + the reloc-masked EH-state push (same family wall as FreeAll_152720).
// docs/patterns/zero-register-pinning.md.
RVA(0x00157bc0, 0xa2)
void CDDrawSubMgrLeafScan::ClearMap() {
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

SIZE_UNKNOWN(CCatalogNode);
SIZE_UNKNOWN(CDDrawSubMgrLeafScan);
SIZE_UNKNOWN(CDDrawSubMgrGrandBase);
SIZE_UNKNOWN(CDDrawSubMgrLeaf);
VTBL(CDDrawSubMgrLeaf, 0x001efc78); // ??_7CDDrawSubMgrLeaf (was g_catalogVtbl)
