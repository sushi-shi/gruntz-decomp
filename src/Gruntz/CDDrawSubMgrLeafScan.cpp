#include <rva.h>
// CDDrawSubMgrLeafScan.cpp - a sibling sub-manager of the tomalla-named
// CDDrawSubMgrLeaf family (a CDirectDrawMgr surface/page sub-manager in the
// "Harry Potter" group; see src/Stub/types/ddrawmgr_surface_family.h). This is
// the keyed-asset CACHE variant: it owns a CMapStringToOb at +0x10 keyed by
// const char* strings, a busy/loading guard at +0x30, plus the shared base
// fields (status word at +0x04, parent/root handle at +0x0c).
//
// The cluster splits into two co-operating groups, both keyed on the same +0x10
// map:
//   - map-scan helpers (Lookup / HasKeyEqual / RemoveKeysEqual / FindKeyOfValue)
//     -- byte-for-byte twins of the same helpers on CDDrawWorkerRegistry.
//   - the directory-scan / factory group (the recursive %s%s%s path walker, the
//     0x1c-byte element factory, and the throttled per-asset refresh) -- builds
//     cache entries by enumerating files via the engine's FindFirst/Next API.
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + emitted code
// bytes are load-bearing (campaign doctrine). All the engine callees (the MFC
// CMapStringToOb/CString thunks, sprintf/strncmp, the file-iteration API, the
// per-element ConfigureItem) are reloc-masked external calls.
// ---------------------------------------------------------------------------

// <Mfc.h> brings real MFC CObject / CMapStringToOb / CString / POSITION; afx-first
// so it precedes any <windows.h>/DirectX header.
#include <Mfc.h>
#include <string.h> // strncmp / sprintf inline CRT, strcpy in the path copy

// Real DSound types so MatchSub_1584f0's GetFormat / SetPrimaryFormat calls
// mangle to the retail names (the relocs pair instead of staying fuzzy).
#include <Dsndmgr/DirectSoundMgr.h>
#include <Dsndmgr/SoundDevice.h>

// The map value: only the scalar-deleting destructor slot (+0x04) is load-
// bearing for the RemoveKeysEqual/FindKey teardown dispatch. Declared only -
// never defined, so no ??_7 emitted here.
class LeafScanValue {
public:
    virtual void Slot00();            // +0x00
    virtual i32 ScalarDtor(i32 flag); // +0x04 scalar-deleting destructor
    char m_pad08[0x10 - 0x08];        // +0x08..0x0f
    i32 m_10field;                    // +0x10  compared in FindKeyOfValue
};

// ----- DSNDMGR sub-objects used by MatchSub_1584f0 -----
// arg1->m_10 is the real DirectSoundMgr buffer wrapper (its GetFormat @0x135ac0
// pairs by name). The held SoundDevice at this+0x2c exposes SetPrimaryFormat
// (0x1371a0, pairs by name) and a still-unnamed start-primary thunk at 0x137200.
class LeafScanSoundArg {
public:
    char m_pad00[0x10];   // +0x00..0x0f
    DirectSoundMgr* m_10; // +0x10
};
// 0x137200: a __thiscall SoundDevice method (reads +0x78/+0x84, calls
// CreatePrimaryBuffer); still unnamed in the export set, so this call stays
// reloc-masked. Modeled as a method on a layout-compatible view so the call
// lowers to mov ecx,m_2c; call.
class SoundDeviceStartView {
public:
    i32 StartPrimary_137200(); // 0x137200
};

// The two vtables in the dtor chain: this sibling's own (0x5efca0) and the
// grand-base dtor vtable (0x5e8cb4). Modeled with the transitional manual stamp
// because this sibling's vtable contents are not modeled here.
DATA(0x005efca0)
extern void* g_leafScanVtbl;
DATA(0x005e8cb4)
extern void* g_remusBaseDtorVtbl;

// VM18 (0x157ae0) on the existing CDDrawSubMgrLeaf TU: clears the +0x10 map and
// zeroes +0x2c. Reloc-masked external __thiscall call from the dtor.
class LeafScanVM18Sink {
public:
    void VM18(); // 0x157ae0
};

// ---------------------------------------------------------------------------
// The shared base: vptr + status word at +0x04 + handle at +0x0c. Its (inlined)
// destructor resets those fields and stamps the grand-base dtor vtable -- this is
// the tail the derived dtor chains into AFTER the map member is destroyed.
// ---------------------------------------------------------------------------
class LeafScanBase {
public:
    ~LeafScanBase();

    void* m_vptr;              // +0x00
    i32 m_04;                  // +0x04  -1 when inactive
    char m_pad08[0x0c - 0x08]; // +0x08..0x0b
    i32 m_0c;                  // +0x0c  parent/root handle
};

inline LeafScanBase::~LeafScanBase() {
    m_04 = -1;
    *(i32*)&m_pad08[0] = 0; // +0x08 = 0
    m_0c = 0;
    m_vptr = &g_remusBaseDtorVtbl;
}

// ---------------------------------------------------------------------------
// The cache sub-manager. Map at +0x10 (CMapStringToOb, 0x1c bytes -> ends at
// +0x2c). m_2c is a held sub-object (a second keyed store / scanner), m_30 the
// busy guard, m_34 a redraw arg. The map's m_nCount (class+0x1c, GetCount
// inlined) drives the GetNextAssoc start-position trick.
// ---------------------------------------------------------------------------
class CDDrawSubMgrLeafScan : public LeafScanBase {
public:
    CObject* Lookup_05b7e0(const char* key);
    i32 RemoveKeysEqual_157c70(const char* base, const char* str);
    i32 HasKeyEqual_1583c0(const char* str);
    CString FindKeyOfValue_158570(LeafScanValue* target);
    i32 MatchSub_1584f0(LeafScanSoundArg* arg1, i32 arg2);

    ~CDDrawSubMgrLeafScan();

    CMapStringToOb m_10; // +0x10  keyed asset cache (ends +0x2c)
    SoundDevice* m_2c;   // +0x2c  held DSound device
    i32 m_30;            // +0x30  busy/loading guard
    i32 m_34;            // +0x34  redraw arg
};

// ---------------------------------------------------------------------------
// 0x5b7e0: Lookup `key` in the map and return the found CObject* (null if not).
RVA(0x0005b7e0, 0x23)
CObject* CDDrawSubMgrLeafScan::Lookup_05b7e0(const char* key) {
    CObject* val = 0;
    m_10.Lookup(key, val);
    return val;
}

// ---------------------------------------------------------------------------
// 0x157570: the (non-deleting) destructor. Stamps this class's vtable, runs the
// VM18 cleanup (clears the map + zeroes +0x2c), the +0x10 map's own destructor,
// resets the base fields, then chains to the grand-base dtor vtable. /GX EH frame
// (VM18 / map dtor may throw).
// @early-stop
// 95% — reloc-masked plateau: every code byte matches retail (confirmed by the
// instruction-by-instruction objdiff); the only residual rows are differently-
// named symbol operands (the EH unwind label, the VM18 / ~CMapStringToOb thunk
// addresses, and the two vtable DATA symbols). objdiff-reloc-scoring.
RVA(0x00157570, 0x68)
CDDrawSubMgrLeafScan::~CDDrawSubMgrLeafScan() {
    m_vptr = &g_leafScanVtbl;
    ((LeafScanVM18Sink*)this)->VM18();
    // m_10 (CMapStringToOb) member dtor auto-fires here, then the LeafScanBase
    // destructor resets +0x04/+0x08/+0x0c and stamps the grand-base vtable.
}

// ---------------------------------------------------------------------------
// 0x157c70: remove every map entry whose key strncmp-equals `str` (over its full
// length), destroying each removed value via its scalar dtor; returns the count.
// The compare string is a CString built from `base` then assigned `str`. Twin of
// CDDrawWorkerRegistry::RemoveKeysEqual_155360.
// @early-stop
// 91.67% — identical to the matched twin's wall: the val/loop-flag stack-slot
// swap (0x10<->0x14 coin-flip) + the reloc-masked EH-state push. Logic/CFG/calls/
// offsets reproduced; no source lever. docs/patterns/zero-register-pinning.md.
RVA(0x00157c70, 0xf8)
i32 CDDrawSubMgrLeafScan::RemoveKeysEqual_157c70(const char* base, const char* str) {
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
                    ((LeafScanValue*)val)->ScalarDtor(1);
                }
                ++n;
            }
        } while (pos != 0);
    }
    return n;
}

// ---------------------------------------------------------------------------
// 0x1583c0: return 1 if any map key strncmp-equals `str` over strlen(str), else
// 0. Twin of CDDrawWorkerRegistry::HasKeyEqual_155550. /GX EH frame for the local
// CString key.
// @early-stop
// 61.36% — identical to the matched twin's optimizer loop-peel wall: MSVC5 peels
// the first iteration of this do/while+early-return (retail is a single loop).
// Body/calls/args match. docs/patterns/zero-register-pinning.md.
RVA(0x001583c0, 0xdc)
i32 CDDrawSubMgrLeafScan::HasKeyEqual_1583c0(const char* str) {
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
// 0x1584f0: if `arg1` and the held DSound manager (m_2c) are both present, probe
// arg1's sound source for format 0x12 (vtable +0x14), then Prepare the manager,
// then -- only when arg2 is set -- Commit it. Returns 1 on full success; the
// failing sub-result (0) otherwise. arg1==0 returns arg1 (0).
RVA(0x001584f0, 0x80)
i32 CDDrawSubMgrLeafScan::MatchSub_1584f0(LeafScanSoundArg* arg1, i32 arg2) {
    if (arg1 == 0) {
        return (i32)arg1;
    }
    if (m_2c == 0) {
        return 0;
    }
    char fmt[0x12]; // WAVEFORMATEX scratch (0x12 = 18 bytes)
    if (arg1->m_10->GetFormat(fmt, 0x12, 0) == 0) {
        return 0;
    }
    i32 prep;
    if (m_2c->SetPrimaryFormat(&prep) == 0) {
        return 0;
    }
    if (arg2 != 0) {
        if (((SoundDeviceStartView*)m_2c)->StartPrimary_137200() == 0) {
            return 0;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// 0x158570: return (by value) the key of the first map entry whose VALUE POINTER
// equals `target`; the (empty) key if `target` is null or no entry matches.
// Returns the running `key` CString in every exit (NRVO into the return slot).
// @early-stop
// 70.77% — the target==0 guard, the pointer-identity search, and both `return
// key` copy-ctor paths reproduced. Residual is the optimizer loop-peel + the
// pos/flag stack-slot/register choice (same family as the twin's NRVO wall).
// docs/patterns/zero-register-pinning.md.
RVA(0x00158570, 0xd4)
CString CDDrawSubMgrLeafScan::FindKeyOfValue_158570(LeafScanValue* target) {
    CString key;
    if (target == 0) {
        return key;
    }
    CObject* val = 0;
    POSITION pos = (POSITION)(m_10.GetCount() != 0 ? -1 : 0);
    if (*(volatile i32*)&pos != 0) {
        do {
            m_10.GetNextAssoc(pos, key, val);
            if (val == (CObject*)target) {
                return key;
            }
        } while (pos != 0);
    }
    return key;
}
