// FxModeDesc.cpp - the mode/effect descriptor record family (trace placeholder
// tomalla-47) plus the bute section-key builder grouped with it.
//
//   - MakeButeSectionKey (0xf9280, __cdecl): appends "[" + a1 + ":" + a2 + "]"
//     onto dst, building a "[section:key]" lookup string; bails (returns 0) on a
//     null key. The five concatenations lower to inline strcat (repne scasb +
//     rep movs at /O2 /Oi).
//   - CFxModeDesc::CFxModeDesc (0x17e7b0): base init, zero the type tag.
//   - CFxModeT3::CFxModeT3 (0x17e880): base + (type=3, defaults).
//
// Names are placeholders; offsets + code bytes are load-bearing.
#include <Gruntz/FxModeT1.h> // Mfc.h (afx-first) + FxModeDesc.h + the CString-bearing CFxModeT1
#include <Gruntz/FxModeDesc.h>

#include <rva.h>
#include <string.h>

// ===========================================================================
// 0xf9280 - MakeButeSectionKey(dst, section, key): build "[section:key]" by
// appending onto dst. Returns 0 when key is null, else 1.
// ===========================================================================
RVA(0x000f9280, 0xe4)
i32 MakeButeSectionKey(char* dst, const char* section, const char* key) {
    if (!key) {
        return 0;
    }
    strcat(dst, "[");
    strcat(dst, section);
    strcat(dst, ":");
    strcat(dst, key);
    strcat(dst, "]");
    return 1;
}

// ===========================================================================
// 0x17e7b0 - CFxModeDesc(): zero the type discriminator.
// ===========================================================================
RVA(0x0017e7b0, 0x9)
CFxModeDesc::CFxModeDesc() {
    m_type = 0;
}

// ===========================================================================
// 0x17e7c0 - CFxModeT1() (re-homed from src/Stub/BoundaryUpper2Eh.cpp): the type-1
// variant ctor. Runs the CFxModeDesc base ctor + the CString member ctor, stamps the
// type-1 record (type=1, m_10=0x32, m_14=1, m_18=1), then assigns the empty string to
// the +0x24 CString. The destructible CString member forces the /GX frame. __thiscall.
// ===========================================================================
extern "C" char g_emptyString[]; // 0x6293f4
RVA(0x0017e7c0, 0x7a)
CFxModeT1::CFxModeT1() {
    m_type = 1;
    m_04 = 0;
    m_08 = 0;
    m_0c = 0;
    m_10 = 0x32;
    m_14 = 1;
    m_18 = 1;
    m_1c = 0;
    m_20 = 0;
    m_24 = g_emptyString;
    m_28 = 0;
}

// ===========================================================================
// 0x17e840 - CFxModeT2(): base ctor, then stamp the type-2 record
// (type=2, m_10=1, m_18=0x140, m_1c=0xf0; the rest zeroed).
// ===========================================================================
// @early-stop
// constant-materialization/store-scheduling wall: same stores, same offsets/values
// as retail, but retail pre-loads 0x140->eax / 0xf0->ecx / 0->edx and emits the
// stores in offset order, while cl keeps 0x140/0xf0 as immediates and groups the
// zero stores. The other five ctors (no two large distinct constants) match 100%;
// this one's register-vs-immediate choice is not source-steerable (store order +
// `int w=0x140` locals both fold back). ~72%.
RVA(0x0017e840, 0x37)
CFxModeT2::CFxModeT2() {
    m_type = 2;
    m_04 = 0;
    m_08 = 0;
    m_10 = 1;
    m_14 = 0;
    m_18 = 0x140;
    m_1c = 0xf0;
    m_20 = 0;
}

// ===========================================================================
// 0x17e880 - CFxModeT3(): run the base ctor, then stamp the type-3 record
// (type=3, m_0c=1, m_10=0xf; the rest zeroed).
// ===========================================================================
RVA(0x0017e880, 0x28)
CFxModeT3::CFxModeT3() {
    m_type = 3;
    m_04 = 0;
    m_08 = 0;
    m_0c = 1;
    m_10 = 0xf;
}

// ===========================================================================
// 0x17e8b0 - CFxModeT4(): base ctor, then stamp the type-4 record
// (type=4, m_0c=1; the rest zeroed). m_0c stored last.
// ===========================================================================
RVA(0x0017e8b0, 0x27)
CFxModeT4::CFxModeT4() {
    m_type = 4;
    m_04 = 0;
    m_08 = 0;
    m_10 = 0;
    m_14 = 0;
    m_0c = 1;
}

// ===========================================================================
// 0x17e8e0 - CFxModeT5(): base ctor, then stamp the type-5 record
// (type=5, m_10=0x19; the rest zeroed). m_10 stored last.
// ===========================================================================
RVA(0x0017e8e0, 0x27)
CFxModeT5::CFxModeT5() {
    m_type = 5;
    m_04 = 0;
    m_08 = 0;
    m_0c = 0;
    m_14 = 0;
    m_10 = 0x19;
}

// ===========================================================================
// 0x17e910 - CFxModeT6(): base ctor, then stamp the type-6 record
// (type=6; m_04..m_20 zeroed, m_0c untouched).
// ===========================================================================
RVA(0x0017e910, 0x29)
CFxModeT6::CFxModeT6() {
    m_type = 6;
    m_04 = 0;
    m_08 = 0;
    m_10 = 0;
    m_14 = 0;
    m_18 = 0;
    m_1c = 0;
    m_20 = 0;
}
