// BootyStateActivate.cpp - the CBootyState / CMultiBootyState per-frame activation
// slot bodies (C:\Proj\Gruntz), FOLDED onto the canonical GameMode.h siblings.
//
// Attribution is vtable-proven (see the retail state vtables):
//   0x18d30 = CBootyState      slot 9 (+0x24)  vtbl 0x5e9cec  (Vslot09)
//   0x1f6f0 = CMultiBootyState slot 8 (+0x20)  vtbl 0x5e9bdc  (InputVirtual / OnActivate2)
// The two are CONFIRMED-distinct siblings over CState (<Gruntz/GameMode.h>). The old
// per-TU CBootyState/CMultiBootyState views (with BootyAssetRoot @+0x0c, BootyRegistrar,
// BootyNamespace @+0x2c/+0x28/+0x30) are folded away here onto the CState + CSpriteFactoryHolder facets:
//   - m_c (+0x0c)  == CSpriteFactoryHolder (the shared render/resource context - see <Gruntz/View.h>).
//     The +0x04 loader (was CGruntDataLoader::Load) is CSpriteFactoryHolder's m_renderState->Flush
//     (0x158ee0); the +0x10 registrar (was BootyRegistrar::CallRegister) is CSpriteFactoryHolder's
//     m_imageRegistry->LoadNamespace (vtable slot +0x4c).
//   - the +0x2c/+0x28/+0x30 asset sources (were BootyNamespace) are CState::m_2c /
//     m_levelBank / m_gruntzBank (CResSource; LookupSet == the old Lookup, 0x13bae0).
// Only the g_mgrSettings world sound set below stays a local facet view - a separate
// object web from the +0x0c context (deferred canonical world-sound model). Only offsets
// / code bytes are load-bearing; every helper is a reloc-masked external.
#include <Gruntz/SoundCueMgr.h>
#include <Bute/SymTab.h>
#include <Bute/SymParser.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <Mfc.h> // ShowCursor (reloc-masked); GameMode.h needs the afx umbrella

#include <rva.h>
#include <Gruntz/BankMgr.h> // CResSource::LookupSet (CState::m_2c/m_levelBank/m_gruntzBank)
#include <Gruntz/GameMode.h> // canonical CBootyState/CMultiBootyState : CState + CSpriteFactoryHolder facet

// ---------------------------------------------------------------------------
// The game-registry (*0x24556c == g_mgrSettings, the CGameRegistry singleton) WORLD
// sound set, re-triggered by CBootyState::Vslot09's ambient BOOTY_LOOP poll. This is a
// SEPARATE object web from the +0x0c CSpriteFactoryHolder context (the world holder's sound SET, reached
// as reg->m_world(+0x30)->m_28); the vfunc_9 global-load path reads it off the singleton,
// not off `this`, so it stays a local facet view here pending a canonical world-sound
// model. Reloc-masked __thiscall entries.
SIZE_UNKNOWN(BootySndEntry);
struct BootySndEntry {
    char m_pad00[0x10];
    CSoundCueMgr* m_player; // +0x10  the player
    u32 m_lastPlayed;       // +0x14  last-played frame stamp
    u32 m_interval;         // +0x18  min replay interval
};
SIZE_UNKNOWN(BootySndTable);
SIZE_UNKNOWN(BootySndSet);
struct BootySndSet {
    char m_pad00[0x10];
    char m_table; // +0x10 (CMapStringToPtr body starts here; cast at Find)
    char m_pad11[0x30 - 0x11];
    i32 m_activeGuard; // +0x30  active guard (nonzero -> skip the ambient poll)
};
SIZE_UNKNOWN(BootySndWorld);
struct BootySndWorld { // g_mgrSettings->m_world (+0x30) sound facet
    char m_pad00[0x28];
    BootySndSet* m_soundSet; // +0x28
};
SIZE_UNKNOWN(BzGameReg);
struct BzGameReg { // == *0x24556c (g_mgrSettings), viewed for the world sound set
    char m_pad00[0x30];
    BootySndWorld* m_world; // +0x30
    char m_pad34[0x11c - 0x34];
    i32 m_soundToken; // +0x11c  ambient sound token
};
DATA(0x0024556c)
extern BzGameReg* g_mgrSettings;
DATA(0x0021ab20)
extern i32 g_sndEnabled; // BOOTY_LOOP enable gate
DATA(0x002bf3c0)
extern "C" u32 g_killCueClock; // wrap-safe draw clock

// ---------------------------------------------------------------------------
// CBootyState::Vslot09 (slot 9, 0x18d30) - the booty "bg" activation tick: hide the
// cursor, fade in the "bg" title, kick the render worker apply (m_c->m_drawTarget->Flush)
// + the booty page timer, and (when the BOOTY_LOOP ambient sound entry exists and is
// enabled by g_sndEnabled) re-trigger it on a rate-limited timer keyed off the
// g_killCueClock frame counter vs the entry's last-played stamp + interval.
// @early-stop
// regalloc wall (~95%): retail holds `set` (reg->m_world->m_28) in eax and the play entry
// `res` live in eax with no reload; the /O2 recompile pins `set` in ecx and spills/reloads
// `res` at the Play call. Logic + all externs/strings named/folded.
RVA(0x00018d30, 0xcd)
i32 CBootyState::Vslot09(i32) {
    while (ShowCursor(FALSE) >= 0)
        ;
    if (!FadeInTitle("bg", 0, 0, 0, 0, 1)) {
        return 0;
    }
    ((CDDrawSubMgrPages*)m_c->m_drawTarget)->Method_158ee0();
    BuildPage(0x50, 0x3e8, 0, 1);

    BzGameReg* reg = g_mgrSettings;
    BootySndSet* set = reg->m_world->m_soundSet;
    i32 token = reg->m_soundToken;
    if (set->m_activeGuard == 0) {
        BootySndEntry* res = 0;
        ((CMapStringToPtr*)&set->m_table)->Lookup("BOOTY_LOOP", (void*&)res);
        if (res != 0 && g_sndEnabled != 0) {
            u32 now = g_killCueClock;
            if (now - res->m_lastPlayed >= res->m_interval) {
                res->m_lastPlayed = now;
                res->m_player->ConfigureItem(token, 0, 0, 1);
            }
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CMultiBootyState::InputVirtual (slot 8, 0x1f6f0) - on state activation: chain the base
// image-load gate, hide the cursor, resolve+register the BOOTY/GRUNTZ/LEVEL "IMAGEZ" sets
// through the CSpriteFactoryHolder image registry (LoadNamespace +0x4c), fade in the "multi" title, run
// the post-activate hook, then kick the render worker apply + the page timer.
RVA(0x0001f6f0, 0x10b)
i32 CMultiBootyState::InputVirtual() {
    if (!BaseOnActivate()) {
        return 0;
    }

    while (ShowCursor(FALSE) >= 0)
        ;

    void* tree = SymTab2c()->ResolvePath("IMAGEZ");
    if (!tree) {
        return 0;
    }
    CImageRegistry* reg = m_c->m_10;
    if (reg->LoadNamespace(tree, "BOOTY", "_") == -1) {
        return 0;
    }

    tree = ((CSymTab*)m_gruntzBank)->ResolvePath("IMAGEZ");
    if (!tree) {
        return 0;
    }
    reg = m_c->m_10;
    if (reg->LoadNamespace(tree, "GRUNTZ", "_") == -1) {
        return 0;
    }

    tree = ((CSymTab*)m_levelBank)->ResolvePath("IMAGEZ");
    if (!tree) {
        return 0;
    }
    reg = m_c->m_10;
    if (reg->LoadNamespace(tree, "LEVEL", "_") == -1) {
        return 0;
    }

    if (!FadeInTitle("multi", 0, 0, 0, 0, 1)) {
        return 0;
    }

    OnActivated();
    ((CDDrawSubMgrPages*)m_c->m_drawTarget)->Method_158ee0();
    BuildPage(0x50, 0x3e8, 0, 1);
    return 1;
}
