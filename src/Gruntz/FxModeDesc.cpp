// FxModeDesc.cpp - SINGLE-FN REMNANT (waveM-strays). The file's original CFxModeDesc/T1-T6
// mode-descriptor ctors (@0x17e7b0-0x17e910) were moved by waveM-mech into Fader.cpp (their
// real obj - the 0x17e450 fader TU). All that is left here is one orphan utility leaf:
//
//   0xf9280  MakeButeSectionKey(dst, section, key) - the "[section:key]" bute-lookup-key
//            builder (5 inline strcat's).
//
// waveM-strays verification: it does NOT belong to the 0xfa1f0 Attract TU (wave3-H). 0xf9280
// sits ~0.06M BELOW the attract contiguous block (0xfa1f0-0xfb1c0) inside a COMDAT-pool-exile
// band of unrelated single-fn leaves (bracketvalue 0xf9160, netsession AppendInt 0xf93b0,
// splashstate, gamemode, apphelpers, gameassetnamespaces, gamemodebase) - a different obj, so
// it is LEFT here.
//
// @identity-TODO: xref recovers its real domain as NetMgr/NetSession (called by
// CNetMgr::JoinAndRegisterChannel + the adjacent netsession leaf AppendInt@0xf93b0, both
// building channel-registration keys). Its true home is that NetMgr utility obj (likely
// alongside AppendInt); a clean re-home is deferred (the whole 0xf9160-0xfa150 pool is orphan
// leaves needing per-fn obj recovery). Names are placeholders; code bytes load-bearing.
#include <Ints.h>
#include <rva.h>
#include <string.h>

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
