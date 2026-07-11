// FxModeDesc.cpp - MakeButeSectionKey (0xf9280), the "[section:key]" bute-lookup-key
// builder. waveM-mech moved the CFxModeDesc/T1-T6 mode-descriptor ctors (@0x17e7b0-
// 0x17e910) into Fader.cpp (their real obj - the 0x17e450 fader TU); MakeButeSectionKey
// stays here because its RVA (0xf9280) is ~0.44M below the fader band, i.e. a separate
// obj (@identity-TODO its true home). Names are placeholders; code bytes load-bearing.
#include <Ints.h>
#include <rva.h>
#include <string.h>

// ===========================================================================
// 0xf9280 - MakeButeSectionKey(dst, section, key): build "[section:key]" by
// appending onto dst. Returns 0 when key is null, else 1. The five concatenations
// lower to inline strcat (repne scasb + rep movs at /O2 /Oi).
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
