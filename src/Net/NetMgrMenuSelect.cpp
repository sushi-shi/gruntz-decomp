// NetMgrMenuSelect.cpp - CNetMgr::LoadMenuSelectSprite (0xba620), the menu-select
// event handler (C:\Proj\NetMgr).
//
// Kept in its own TU (NOT folded into NetMgr.cpp): this handler treats
// GetPlayerData/AddSessionNode (0x178eb0/0x178b30) as methods on the +0x524
// session/player sub-object (ecx = [this+0x524]; the retail bytes at 0xba66b push
// FOUR args to 0x178b30), whereas NetMgr.cpp's byte-exact CNetMgr models 0x178b30 as
// a direct 2-arg CNetMgr method - a genuine RE ambiguity at the same RVA. So the
// +0x524 sub-object keeps its own PlayerMgr type here; the shared CNetMgr is used for
// everything else (m_peer @+0x524 is cast to PlayerMgr for the 4-arg calls).
#include <Net/NetMgr.h>      // the single shared CNetMgr
#include <Gruntz/SoundCue.h> // the shared positional-sound cue subsystem
#include <rva.h>

// The menu-select event the handler is handed (edi): +0x4 the "armed" gate (==1),
// +0x8 the player/slot id, +0x20/+0x24 the session-add params.
struct MenuSelectEvent {
    char m_pad0[0x4];
    i32 m_armed; // +0x4  armed gate (==1)
    i32 m_id;    // +0x8  player/slot id
    char m_pad0c[0x20 - 0xc];
    i32 m_20; // +0x20
    i32 m_24; // +0x24
};
SIZE_UNKNOWN(MenuSelectEvent); // event view (only touched offsets pinned); size TBD

// The session/player manager at CNetMgr+0x524.
struct PlayerNode;
struct PlayerMgr {
    PlayerNode* GetPlayerData(i32 id);                         // 0x178eb0 (__thiscall)
    PlayerNode* AddSessionNode(i32 id, i32 a, i32 b, void* c); // 0x178b30 (__thiscall)
};
SIZE_UNKNOWN(PlayerMgr); // method-only +0x524 sub-object view; retail size TBD

// The "ready options" count is CNetGameMgr::CountActiveChannels @0x492e30 (via the
// 0x38cd ILT thunk; __thiscall ret 4) - the SAME method the channel cluster gates
// on; the former OptionsHost placeholder folded into the canonical m_4 view.

// The positional-sound cue idiom (shared with CPathHazard's strike cue, see
// <Gruntz/SoundCue.h>): m_c is the sound sub-mgr; m_28 its host; look up
// "GAME_MENUS_SELECT" -> an emitter; gate on g_sndEnabled + the kill-cue clock
// cooldown, then play through the cue manager.

DATA(0x0021ab20)
extern i32 g_sndEnabled; // 0x61ab20
DATA(0x0021ab24)
extern i32 g_sndCueTag; // 0x61ab24
DATA(0x002bf3c0)
extern "C" u32 g_killCueClock; // 0x6bf3c0

// --- CNetMgr::LoadMenuSelectSprite (0xba620, __thiscall) -----------------------
// On an armed (ev->m_armed==1) menu-select event: resolve/create the player session
// node, then (unless a paused/over gate m_530/m_connected is set -> a 0x3fd stat)
// either bail with a 0x3fe stat when the ready-options count is full (>=4), or
// announce the version and play the cooldown-gated "GAME_MENUS_SELECT" cue.
//
// Uses the shared CNetMgr (NetMgr.h): the +0x524 player sub-object (its own 1-arg
// GetPlayerData / 4-arg AddSessionNode) is that TU's PlayerMgr, reached by casting
// m_peer each use (reproducing the retail double-load of [this+0x524]); the +0x4
// options host and the +0xc sound sub-mgr are cast from their shared-class slots.
// @confidence: med
// @source: decomp-xref
RVA(0x000ba620, 0x14a)
i32 CNetMgr::LoadMenuSelectSprite(void* evp) {
    MenuSelectEvent* ev = (MenuSelectEvent*)evp;
    if (ev == 0) {
        return 0;
    }
    if (ev->m_armed != 1) {
        return 0;
    }
    PlayerNode* node = ((PlayerMgr*)m_peer)->GetPlayerData(ev->m_id);
    if (node == 0) {
        node = ((PlayerMgr*)m_peer)->AddSessionNode(ev->m_id, ev->m_20, ev->m_24, node);
        if (node == 0) {
            return 0;
        }
    }
    if (m_530 == 0 && m_connected == 0) {
        if (m_useChannelLatency != 0) {
            if (m_4->CountActiveChannels(1) >= 4) {
                SendStat3(ev->m_id, 0x3fe, 1);
                return 0;
            }
            if (m_useChannelLatency != 0) {
                AnnounceVersion((i32)node);
            }
        }
        CSndHost* host = ((CSndSubMgr*)m_c)->m_28;
        if (host->m_30 == 0) {
            CSndEmitter* out = 0;
            host->m_10.Lookup("GAME_MENUS_SELECT", &out);
            CSndEmitter* e = out;
            if (e != 0) {
                i32 enabled = g_sndEnabled;
                i32 tag = g_sndCueTag;
                if (enabled != 0) {
                    u32 now = g_killCueClock;
                    if ((u32)(now - e->m_14) >= e->m_18) {
                        e->m_14 = now;
                        e->m_10->ConfigureItem(tag, 0, 0, 0);
                    }
                }
            }
        }
        return 1;
    }
    SendStat3(ev->m_id, 0x3fd, 1);
    return 1;
}
