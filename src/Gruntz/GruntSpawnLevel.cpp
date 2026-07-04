// GruntSpawnLevel.cpp - CGruntSpawnLevel, a CString-owning collection object
// embedded in CGruntzMgr (Ghidra names the ctor @0xda790 "CGruntSpawnLevel"). Its
// own TU, sibling of GruntSpawnConfig.cpp (NOT folded into GruntzMgr.cpp - a
// separate lane edits that). The ctor (0xda790)
// builds a CString + a +0x38 sub-object then seeds the GruntzPlayer-shaped field
// block (m_0=-1 / m_18=-2 / m_14=1 / m_228=0xf hash size); the dtor (0x83260)
// reverses it. Names are placeholders; offsets + code bytes are load-bearing.
#include <Mfc.h> // CString (MFC TU - precedes <windows.h>)

#include <rva.h>

// The global empty C string the CString member is seeded from (0x6293f4).
extern "C" char g_emptyString[];

// The +0x38 sub-object: ctor 0x24dc0, dtor 0x24f80 (external/no-body, reloc-masked).
class Mgr38 {
public:
    Mgr38();  // 0x24dc0
    ~Mgr38(); // 0x24f80
};

class CGruntSpawnLevel {
public:
    CGruntSpawnLevel();  // 0xda790
    ~CGruntSpawnLevel(); // 0x83260

    i32 m_0;     // +0x00  = -1
    CString m_4; // +0x04  destructible name string
    i32 m_8;     // +0x08
    char pad_c[0x10 - 0xc];
    i32 m_10; // +0x10
    i32 m_14; // +0x14  = 1
    i32 m_18; // +0x18  = -2
    char pad_1c[0x20 - 0x1c];
    i32 m_20; // +0x20
    char pad_24[0x28 - 0x24];
    i32 m_28; // +0x28
    i32 m_2c; // +0x2c
    i32 m_30; // +0x30
    char pad_34[0x38 - 0x34];
    Mgr38 m_38; // +0x38  destructible sub-object
    char pad_3c[0x220 - 0x3c];
    i32 m_220; // +0x220
    i32 m_224; // +0x224
    i32 m_228; // +0x228  = 0xf  (hash-table size)
    i32 m_22c; // +0x22c
    i32 m_230; // +0x230
};

// ===========================================================================
// CGruntSpawnLevel::CGruntSpawnLevel  (0xda790)
// ===========================================================================
// Construct the CString m_4 + the +0x38 sub-object, then seed the field block to
// its empty state (sentinels -1/-2/1, hash size 0xf, the rest zeroed) and assign
// the empty string into m_4. The two destructible members drive the /GX frame.
// @early-stop
// /GX EH-state wall (docs/seh-eh.md): the member ctor calls (CString, +0x38 sub),
// the empty-string assign and the field seeds are recovered, but a 2-destructible
// ctor's EH state numbering + the GruntzPlayer-shaped interleave of the field
// stores is a cl scheduling/EH-machine choice (same family as the CWarlord dtor
// wall) - deferred to the final sweep once the GruntzPlayer cross-attribution of
// the shared init (0xda960) is unified.
RVA(0x000da790, 0xb0)
CGruntSpawnLevel::CGruntSpawnLevel() {
    m_22c = 0;
    m_230 = 0;
    m_0 = -1;
    m_18 = -2;
    m_20 = 0;
    m_28 = 0;
    m_14 = 1;
    m_4 = g_emptyString;
    m_8 = 0;
    m_10 = 0;
    m_220 = 0;
    m_224 = 0;
    m_228 = 0xf;
    m_2c = 0;
    m_30 = 0;
    m_22c = 0;
    m_230 = 0;
}

// ===========================================================================
// CGruntSpawnLevel::~CGruntSpawnLevel  (0x83260)
// ===========================================================================
// Reverse the ctor: clear the field block (the shared Reset at 0xda960), destruct
// the +0x38 sub-object, then the CString m_4. The /GX frame numbers the three
// teardowns 2/0/-1.
// @early-stop
// blocked by a cross-lane attribution: retail's first teardown call (the field
// Reset at 0xda960) is delinked under ??0GruntzPlayer (a DIFFERENT lane owns that
// RVA), so it cannot be referenced here without a duplicate-RVA conflict. Modeled
// as the 2-destructible-member teardown (sub + CString) only; the leading Reset
// call + the /GX state numbering are the residual, deferred to the final sweep
// once the GruntzPlayer/CGruntSpawnLevel identity is unified.
RVA(0x00083260, 0x57)
CGruntSpawnLevel::~CGruntSpawnLevel() {}
SIZE_UNKNOWN(CGruntSpawnLevel);
SIZE_UNKNOWN(Mgr38);
