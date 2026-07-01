// NetMgrMenuSelect.cpp - CNetMgr::LoadMenuSelectSprite (0xba620), the menu-select
// event handler (C:\Proj\NetMgr). Re-homed from src/Stub/CNetMgr.cpp.
//
// Kept in its own TU (NOT folded into NetMgr.cpp): this handler's collaborator
// shape treats GetPlayerData/AddSessionNode (0x178eb0/0x178b30) as methods on the
// +0x524 session/player sub-object (ecx = [this+0x524]), whereas NetMgr.cpp's
// byte-exact CNetMgr models 0x178eb0 as a direct CNetMgr method (ecx = this) - a
// genuine RE ambiguity at the same RVA. Folding it in would force one calling shape
// on both and regress the other, so the self-contained model is preserved here (a
// separate CNetMgr view TU; byte-identical to the former stub).
#include <rva.h>

// The menu-select event the handler is handed (edi): +0x4 the "armed" gate (==1),
// +0x8 the player/slot id, +0x20/+0x24 the session-add params.
struct MenuSelectEvent {
    char m_pad0[0x4];
    i32 m_4; // +0x4  armed gate
    i32 m_8; // +0x8  id
    char m_pad0c[0x20 - 0xc];
    i32 m_20; // +0x20
    i32 m_24; // +0x24
};

// The session/player manager at CNetMgr+0x524.
struct PlayerNode;
struct PlayerMgr {
    PlayerNode* GetPlayerData(i32 id);                         // 0x178eb0 (__thiscall)
    PlayerNode* AddSessionNode(i32 id, i32 a, i32 b, void* c); // 0x178b30 (__thiscall)
};

// The options host at CNetMgr+0x4 (CountReadyOptionsSlots @0x92e30 via the 0x38cd
// ILT thunk; __thiscall ret 4).
struct OptionsHost {
    i32 CountReadyOptionsSlots(i32 flag);
};

// The shared positional-sound cue idiom (same shape as CPathHazard's strike cue):
// m_c is the sound sub-mgr; m_28 its host; find "GAME_MENUS_SELECT" -> an emitter;
// gate on g_sndEnabled + the kill-cue clock cooldown, then Play.
struct CSndPlayHost {
    void Play(i32 tag, i32 a, i32 b, i32 c); // 0x1360d0 (__thiscall)
};
struct CSndEmitter {
    char m_pad00[0x10];
    CSndPlayHost* m_10; // +0x10
    u32 m_14;           // +0x14 last-play clock
    u32 m_18;           // +0x18 cooldown interval
};
struct CSndFinder {
    void Find(const char* name, CSndEmitter** out); // 0x1b8438 (__thiscall)
};
struct CSndHost {
    char m_pad00[0x10];
    CSndFinder m_10;           // +0x10 embedded finder
    char m_pad11[0x30 - 0x11]; // -> +0x30
    i32 m_30;                  // +0x30 gate (must be 0 to emit)
};
struct CSndSubMgr {
    char m_pad00[0x28];
    CSndHost* m_28; // +0x28
};

DATA(0x0021ab20)
extern i32 g_sndEnabled; // 0x61ab20
DATA(0x0021ab24)
extern i32 g_sndCueTag; // 0x61ab24
DATA(0x002bf3c0)
extern "C" u32 g_killCueClock; // 0x6bf3c0

class CNetMgr {
public:
    i32 LoadMenuSelectSprite(MenuSelectEvent* ev);

    // The two stat senders reached __thiscall on `this` (via the ILT thunks
    // 0x3d0a -> SendStat3 0xb9410 and 0x3599 -> AnnounceVersion 0xbd180).
    i32 SendStat3(i32 id, i32 code, i32 flag);
    void AnnounceVersion(PlayerNode* node);

    char m_pad0[0x4];
    OptionsHost* m_4; // +0x4
    char m_pad8[0xc - 0x8];
    CSndSubMgr* m_c; // +0xc
    char m_pad10[0x524 - 0x10];
    PlayerMgr* m_524; // +0x524
    i32 m_528;        // +0x528
    char m_pad52c[0x530 - 0x52c];
    i32 m_530; // +0x530
    char m_pad534[0x580 - 0x534];
    i32 m_580; // +0x580
};

// --- CNetMgr::LoadMenuSelectSprite (0xba620, __thiscall) -----------------------
// On an armed (ev->m_4==1) menu-select event: resolve/create the player session
// node, then (unless a paused/over gate m_530/m_580 is set -> a 0x3fd stat) either
// bail with a 0x3fe stat when the ready-options count is full (>=4), or announce
// the version and play the cooldown-gated "GAME_MENUS_SELECT" cue.
// @confidence: med
// @source: decomp-xref
RVA(0x000ba620, 0x14a)
i32 CNetMgr::LoadMenuSelectSprite(MenuSelectEvent* ev) {
    if (ev == 0) {
        return 0;
    }
    if (ev->m_4 != 1) {
        return 0;
    }
    PlayerNode* node = m_524->GetPlayerData(ev->m_8);
    if (node == 0) {
        node = m_524->AddSessionNode(ev->m_8, ev->m_20, ev->m_24, node);
        if (node == 0) {
            return 0;
        }
    }
    if (m_530 == 0 && m_580 == 0) {
        if (m_528 != 0) {
            if (m_4->CountReadyOptionsSlots(1) >= 4) {
                SendStat3(ev->m_8, 0x3fe, 1);
                return 0;
            }
            if (m_528 != 0) {
                AnnounceVersion(node);
            }
        }
        CSndHost* host = m_c->m_28;
        if (host->m_30 == 0) {
            CSndEmitter* out = 0;
            host->m_10.Find("GAME_MENUS_SELECT", &out);
            CSndEmitter* e = out;
            if (e != 0) {
                i32 enabled = g_sndEnabled;
                i32 tag = g_sndCueTag;
                if (enabled != 0) {
                    u32 now = g_killCueClock;
                    if ((u32)(now - e->m_14) >= e->m_18) {
                        e->m_14 = now;
                        e->m_10->Play(tag, 0, 0, 0);
                    }
                }
            }
        }
        return 1;
    }
    SendStat3(ev->m_8, 0x3fd, 1);
    return 1;
}
