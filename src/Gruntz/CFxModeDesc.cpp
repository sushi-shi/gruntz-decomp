// CFxModeDesc.cpp - the mode/effect descriptor record family (trace placeholder
// ClassUnknown_47) plus the bute section-key builder grouped with it.
//
//   - MakeButeSectionKey (0xf9280, __cdecl): appends "[" + a1 + ":" + a2 + "]"
//     onto dst, building a "[section:key]" lookup string; bails (returns 0) on a
//     null key. The five concatenations lower to inline strcat (repne scasb +
//     rep movs at /O2 /Oi).
//   - CFxModeDesc::CFxModeDesc (0x17e7b0): base init, zero the type tag.
//   - CFxModeT3::CFxModeT3 (0x17e880): base + (type=3, defaults).
//
// Names are placeholders; offsets + code bytes are load-bearing.
#include <Gruntz/CFxModeDesc.h>

#include <rva.h>
#include <string.h>

// ===========================================================================
// 0xf9280 - MakeButeSectionKey(dst, section, key): build "[section:key]" by
// appending onto dst. Returns 0 when key is null, else 1.
// ===========================================================================
RVA(0x000f9280, 0xe4)
i32 MakeButeSectionKey(char* dst, const char* section, const char* key) {
    if (!key)
        return 0;
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
