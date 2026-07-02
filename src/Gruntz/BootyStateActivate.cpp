// BootyStateActivate.cpp - the booty/bonus game-state per-frame activation ticks
// (C:\Proj\Gruntz), re-homed from src/Stub/Backlog.cpp (where a single placeholder
// `BootyState` class conflated two sibling state classes).
//
// Attribution is vtable-proven (see the retail state vtables):
//   0x18d30 = CBootyState      slot 9 (+0x24)  vtable 0x5e9cec
//   0x1f6f0 = CMultiBootyState slot 8 (+0x20)  vtable 0x5e9bdc  (the OnActivate2)
// The two classes are siblings over CState; the shared asset-host / namespace /
// sound-chain sub-objects are modeled once below. Only offsets / code bytes are
// load-bearing; every helper is a reloc-masked external.
#include <Win32.h> // ShowCursor (reloc-masked)

#include <rva.h>

// ---------------------------------------------------------------------------
// Shared booty-state sub-objects.
// ---------------------------------------------------------------------------
// A named asset-namespace registry slot. Lookup() finds a child set by name and
// returns a handle (reloc-masked __thiscall).
SIZE_UNKNOWN(BootyNamespace);
struct BootyNamespace {
    i32 Lookup(char* szName); // FUN_0013bae0 __thiscall
};
// The registrar object reached via asset-root->m_10. Its vtable slot +0x4c registers
// a looked-up image set under a prefix (returns -1 on failure). The slot is a
// __thiscall virtual modeled as a pointer-to-member loaded from the vtable so MSVC
// emits `mov edx,[ecx]; call [edx+0x4c]`. The class must be COMPLETE before the T::*
// typedef so the PMF stays 4 bytes - see docs/patterns/pmf-complete-class-4byte.md.
struct BootyRegistrarVtbl; // opaque; the PMF lives at offset 0x4c inside it
SIZE_UNKNOWN(BootyRegistrar);
struct BootyRegistrar {
    BootyRegistrarVtbl* m_vtbl; // +0x00
    i32 CallRegister(i32 handle, char* prefix, char* sep);
};
typedef i32 (BootyRegistrar::*BootyRegFn)(i32 handle, char* prefix, char* sep);
SIZE_UNKNOWN(BootyRegistrarVtbl);
struct BootyRegistrarVtbl {
    char m_pad00[0x4c];
    BootyRegFn Register; // +0x4c
};
inline i32 BootyRegistrar::CallRegister(i32 handle, char* prefix, char* sep) {
    return (this->*(m_vtbl->Register))(handle, prefix, sep);
}
SIZE_UNKNOWN(CGruntDataLoader);
struct CGruntDataLoader { // asset-root->m_4 sub-object
    void Load();          // FUN @ 0x158ee0 __thiscall (reloc-masked)
};
SIZE_UNKNOWN(BootyAssetRoot);
struct BootyAssetRoot { // state->m_c
    char m_pad00[0x4];
    CGruntDataLoader* m_4; // +0x04
    char m_pad08[0x10 - 0x8];
    BootyRegistrar* m_10; // +0x10  vtable-bearing registrar (slot +0x4c)
};
// The shared-game-registry sound chain re-triggered by CBootyState::vfunc_9's ambient
// BOOTY_LOOP poll.
SIZE_UNKNOWN(BootySndPlayer);
struct BootySndPlayer {
    void Play(i32 token, i32, i32, i32); // FUN_001360d0 __thiscall
};
SIZE_UNKNOWN(BootySndEntry);
struct BootySndEntry {
    char m_pad00[0x10];
    BootySndPlayer* m_10; // +0x10
    u32 m_14;             // +0x14  last-played stamp
    u32 m_18;             // +0x18  interval
};
SIZE_UNKNOWN(BootySndTable);
struct BootySndTable {
    void Find(char* szName, BootySndEntry** out); // FUN_001b8438 __thiscall, out-param
};
SIZE_UNKNOWN(BootySndSet);
struct BootySndSet {
    char m_pad00[0x10];
    BootySndTable m_10; // +0x10  (&m_10 == set+0x10)
    char m_pad11[0x30 - 0x11];
    i32 m_30; // +0x30  active guard
};
SIZE_UNKNOWN(BootySndMgr);
struct BootySndMgr {
    char m_pad00[0x28];
    BootySndSet* m_28; // +0x28
};
// A typed view of the *0x64556c game-registry singleton for the sound chain.
struct BzGameReg {
    char m_pad00[0x30];
    BootySndMgr* m_30; // +0x30
    char m_pad34[0x11c - 0x34];
    i32 m_11c; // +0x11c  sound token
};
DATA(0x0024556c)
extern BzGameReg* g_bzReg;
DATA(0x0021ab20)
extern i32 g_61ab20; // BOOTY_LOOP enable gate
DATA(0x002bf3c0)
extern "C" u32 g_6bf3c0; // wrap-safe draw clock

// ---------------------------------------------------------------------------
// CBootyState (vtable 0x5e9cec).
SIZE_UNKNOWN(CBootyState);
class CBootyState {
public:
    i32 vfunc_9(i32);                                                 // slot 9 (+0x24) @0x18d30
    i32 RegisterMultiNamespaces(char* mode, i32, i32, i32, i32, i32); // reloc-masked
    void StartTimer(i32, i32, i32, i32);                              // reloc-masked

    char m_pad00[0xc];
    BootyAssetRoot* m_c; // +0x0c
};

// CBootyState::vfunc_9 (0x18d30) - the booty "bg" activation tick: install the "bg"
// multi-namespace, kick the grunt-data load + state timer, and (when the BOOTY_LOOP
// ambient sound entry exists and is enabled by g_61ab20) re-trigger it on a
// rate-limited timer keyed off the g_6bf3c0 frame counter vs the entry's last-played
// stamp + interval.
// @early-stop
// regalloc wall (~95%): retail holds `set` (reg->m_30->m_28) in eax and the play
// entry `res` live in eax with no reload; the /O2 recompile pins `set` in ecx and
// spills/reloads `res` at the Play call. Logic + all externs/strings named.
RVA(0x00018d30, 0xcd)
i32 CBootyState::vfunc_9(i32) {
    while (ShowCursor(FALSE) >= 0)
        ;
    if (!RegisterMultiNamespaces("bg", 0, 0, 0, 0, 1)) {
        return 0;
    }
    m_c->m_4->Load();
    StartTimer(0x50, 0x3e8, 0, 1);

    BzGameReg* reg = g_bzReg;
    BootySndSet* set = reg->m_30->m_28;
    i32 token = reg->m_11c;
    if (set->m_30 == 0) {
        BootySndEntry* res = 0;
        set->m_10.Find("BOOTY_LOOP", &res);
        if (res != 0 && g_61ab20 != 0) {
            u32 now = g_6bf3c0;
            if (now - res->m_14 >= res->m_18) {
                res->m_14 = now;
                res->m_10->Play(token, 0, 0, 1);
            }
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CMultiBootyState (vtable 0x5e9bdc).
SIZE_UNKNOWN(CMultiBootyState);
class CMultiBootyState {
public:
    i32 OnActivate2();                                                // slot 8 (+0x20) @0x1f6f0
    i32 BaseOnActivate();                                             // base vfunc8 (reloc-masked)
    i32 RegisterMultiNamespaces(char* mode, i32, i32, i32, i32, i32); // reloc-masked
    void OnActivated();                                               // reloc-masked
    void StartTimer(i32, i32, i32, i32);                              // reloc-masked

    char m_pad00[0xc];
    BootyAssetRoot* m_c; // +0x0c
    char m_pad10[0x28 - 0x10];
    BootyNamespace* m_28; // +0x28  "LEVEL" image namespace
    BootyNamespace* m_2c; // +0x2c  "BOOTY" image namespace
    BootyNamespace* m_30; // +0x30  "GRUNTZ" image namespace
};

// CMultiBootyState::OnActivate2 (0x1f6f0) - on state activation: chain the base
// activate, hide the cursor, register the BOOTY/GRUNTZ/LEVEL image namespaces under
// the asset host, install the "multi" namespaces, then kick the grunt-data load +
// the state timer.
RVA(0x0001f6f0, 0x10b)
i32 CMultiBootyState::OnActivate2() {
    if (!BaseOnActivate()) {
        return 0;
    }

    while (ShowCursor(FALSE) >= 0)
        ;

    i32 h = m_2c->Lookup("IMAGEZ");
    if (!h) {
        return 0;
    }
    BootyRegistrar* reg = m_c->m_10;
    if (reg->CallRegister(h, "BOOTY", "_") == -1) {
        return 0;
    }

    h = m_30->Lookup("IMAGEZ");
    if (!h) {
        return 0;
    }
    reg = m_c->m_10;
    if (reg->CallRegister(h, "GRUNTZ", "_") == -1) {
        return 0;
    }

    h = m_28->Lookup("IMAGEZ");
    if (!h) {
        return 0;
    }
    reg = m_c->m_10;
    if (reg->CallRegister(h, "LEVEL", "_") == -1) {
        return 0;
    }

    if (!RegisterMultiNamespaces("multi", 0, 0, 0, 0, 1)) {
        return 0;
    }

    OnActivated();
    m_c->m_4->Load();
    StartTimer(0x50, 0x3e8, 0, 1);
    return 1;
}
