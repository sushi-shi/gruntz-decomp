#include <rva.h>
#include <Gruntz/Grunt.h>
#include <Image/CImage.h>
#include <string.h> // inlined memset / strcpy in CTimer::Serialize (rep stos / rep movs)
#include <Gruntz/GameRegistry.h> // g_gameReg singleton (0x24556c) canonical view
#include <Gruntz/Play.h>         // canonical CPlay (m_curState game-state; level-timer expiry)
#include <Gruntz/TriggerMgr.h>   // canonical CTriggerMgr (g_gameReg->m_cmdGrid; ClearRowAndRefresh)
#include <Gruntz/SerialArchive.h> // the shared CSerialArchive stream (Read @+0x2c / Write @+0x30)
#include <Gruntz/ResMgr.h>        // CResMgr (m_8 key table, m_10 image registry) + CKeyTable
#include <Gruntz/Sprite.h>        // CSprite (frame-data value) + CSpriteHashTable
#include <Gruntz/Timer.h>         // CTimer + CImage (canonical; def was local here)
class CDDrawWorkerRegistry {
public:
    i32 HasKeyEqual_155550(const char* k);
    i32 RemoveKeysEqual_155360(const char* a, const char* b);
    void Method_155630(i32 h, char* t, i32* o);
}; // 0x155550/0x155360/0x155630
// SpriteLoaders.cpp - two sibling HUD/UI sprite loaders that pull a named sprite
// out of the engine's string-keyed sprite-set hash table and cache individual
// animation frames off it (C:\Proj\Gruntz). Both share the same idiom:
//   1. look the sprite up by class-name string through the matched sprite-set
//      hash table (CSpriteHashTable::Lookup, external);
//   2. for each wanted frame number N, extract the frame pointer from the
//      sprite's frame table ONLY when N lies inside the sprite's valid frame
//      range [m_firstFrame(+0x64) .. m_lastFrame(+0x68)], else cache 0.
//
//   LoadTimerSprite      - the in-game TIMER
//       sprite ("GAME_TIMER"); looked up through the game-manager singleton
//       g_gameReg (->+0x30 ->+0x10). Caches frames 10/11 and bails to
//       0 if any required frame is missing.
//   LoadLoadingBarSprite - the loading-screen
//       progress-bar sprite ("GAME_LOADINGBAR"); looked up through this->m_resMgr
//       (->+0x10). Caches frames 1/2/3 and a loaded flag.
//
// Only offsets / code bytes are load-bearing; names are placeholders.
//
// BYTE-EXACT bodies modulo one MSVC5 scheduling coin-flip: the target SINKS the
// lookup out-param's zero-init store (`mov [&spr],0`) to just after the two arg
// pushes; our cl HOISTS it before the lea - the same 2-3 instructions, permuted
// (byte-content identical). See config/units.toml. Kept wip, not strict-exact.

// ---------------------------------------------------------------------------
// The engine string-keyed sprite-set hash table. Lookup() hashes the class-name
// key, finds the entry, and writes the found sprite pointer through *ppOut
// (returning a found-flag). Modeled minimally so the `ecx=<map>; call <helper>`
// shape reloc-masks against the matched lookup helper.
// ---------------------------------------------------------------------------
// CSprite (frame-data value) + CSpriteHashTable come from <Gruntz/Sprite.h>;
// CResMgr (image registry at m_10, key table at m_8) + CKeyTable from ResMgr.h.
// The registry's embedded name->sprite hash table is <registry>->m_10map.

// The state hung off g_gameReg->m_curState is the canonical CPlay (<Gruntz/Play.h>);
// the former CLevelState view is gone (wave 3). On level-timer expiry the handler
// resets the play state's AMBIENT-cue timer group (m_cueTimerLo/Hi @+0x3f8/+0x3fc,
// m_cueInterval @+0x400, m_cueIntervalHi @+0x404) and raises the win/lose banner
// (m_winLoseBanner @+0x4f4). It is the PLAY state, so m_curState (CState*) downcasts
// to CPlay* (CPlay : CState single-inheritance, offset-0).

// The command-grid reached off g_gameReg->m_cmdGrid (+0x68) is the canonical
// CTriggerMgr (<Gruntz/TriggerMgr.h>); the expiry path calls its ClearRowAndRefresh
// (0x7a510). The former CLevelNotify::Notify view is gone (wave 3).

// The per-player timer slot is CFocusSlot, the g_gameReg->m_focusSlots[] element
// (<Gruntz/GameRegistry.h>); the expiry path sets its m_24 = 1, and slot 0's m_0c
// holds the level/entity key.

// The loading bar reaches the resource object through this->m_resMgr (its own CResMgr).
// The canonical CGameRegistry view of the singleton (*0x24556c). The resource mgr
// (+0x30, typed CSpriteFactoryHolder) is reached without a cast; its +0x08 factory
// exposes both CreateSprite (grunt cluster) and the key-lookup facet (cast to
// CKeyTable here). The current-state (+0x2c) is typed CState*; the notify target
// (+0x68) is a genuinely reused slot cast locally; the per-player timer-slot array
// at +0x150 (stride 0x238) and the m_15c sub-object are reached via raw offsets.
DATA(0x0024556c)
extern "C" CGameRegistry* g_gameReg;

// ---------------------------------------------------------------------------
// CLoadingBar::LoadLoadingBarSprite
// ---------------------------------------------------------------------------
class CLoadingBar {
public:
    i32 LoadLoadingBarSprite();

    char m_pad00[0xc];
    CResMgr* m_resMgr; // +0xc
    char m_pad10[0x4bc - 0x10];
    i32 m_loaded;  // +0x4bc loaded flag (set to 1)
    i32* m_frame2; // +0x4c0 frame 2
    i32* m_frame3; // +0x4c4 frame 3
    i32* m_frame1; // +0x4c8 frame 1
};

RVA(0x000d7440, 0xad)
i32 CLoadingBar::LoadLoadingBarSprite() {
    CSprite* spr = 0;
    ((CMapStringToOb*)&m_resMgr->m_10->m_10map)->Lookup("GAME_LOADINGBAR", (CObject*&)spr);
    if (!spr) {
        return 0;
    }

    m_frame1 = (spr->m_firstFrame <= 1 && spr->m_lastFrame >= 1) ? spr->m_frames.m_pData[1] : 0;
    m_frame2 = (spr->m_firstFrame <= 2 && spr->m_lastFrame >= 2) ? spr->m_frames.m_pData[2] : 0;
    m_frame3 = (spr->m_firstFrame <= 3 && spr->m_lastFrame >= 3) ? spr->m_frames.m_pData[3] : 0;
    m_loaded = 1;
    return 1;
}

// ---------------------------------------------------------------------------
// CTimer - the on-screen game timer (C:\Proj\Gruntz). Holds the looked-up
// digit/colon sprite frames (m_frameMinTens..m_frameColon), the 64-bit base
// (m_baseTimeLo/m_baseTimeHi) and accumulated (m_accumLo/m_accumHi) clock times, a
// running flag (m_running) and the decoded current value (m_currentMs). Methods
// below are in retail-RVA order.
// ---------------------------------------------------------------------------

// CTimer + CImage are the canonical <Gruntz/Timer.h> shapes now (shared
// with CPlay::m_frameMarker / PlaySync's HandleEvent serialize entry). The
// archive/order object passed to HandleEvent + Serialize is the shared WAP32
// CSerialArchive (<Gruntz/SerialArchive.h>).

// ---------------------------------------------------------------------------
// CTimer::Init (0x9bab0) - zero the live state. eax=this idiom from cl.
// ---------------------------------------------------------------------------
RVA(0x0009bab0, 0x35)
CTimer* CTimer::Init() {
    m_baseTimeLo = 0;
    m_accumLo = 0;
    m_baseTimeHi = 0;
    m_accumHi = 0;
    m_38 = 0;
    m_40 = 0;
    m_3c = 0;
    m_44 = 0;
    m_sprite = 0;
    m_frameMinTens = 0;
    m_frameMinOnes = 0;
    m_frameColon = 0;
    m_frameSecTens = 0;
    m_frameSecOnes = 0;
    m_active = 0;
    m_running = 0;
    return this;
}

// ---------------------------------------------------------------------------
// CTimer::LoadTimerSprite (0x9bb00)
// ---------------------------------------------------------------------------
RVA(0x0009bb00, 0x119)
i32 CTimer::LoadTimerSprite(i32 a, i32 b) {
    CSprite* spr = 0;
    ((CMapStringToOb*)&g_gameReg->m_world->m_10->m_10map)->Lookup("GAME_TIMER", (CObject*&)spr);
    m_sprite = spr;
    if (!spr) {
        return 0;
    }

    m_frameMinTens =
        (spr->m_firstFrame <= 10 && spr->m_lastFrame >= 10) ? spr->m_frames.m_pData[10] : 0;
    if (!m_frameMinTens) {
        return 0;
    }
    m_frameMinOnes =
        (spr->m_firstFrame <= 10 && spr->m_lastFrame >= 10) ? spr->m_frames.m_pData[10] : 0;
    if (!m_frameMinOnes) {
        return 0;
    }
    m_frameColon =
        (spr->m_firstFrame <= 11 && spr->m_lastFrame >= 11) ? spr->m_frames.m_pData[11] : 0;
    if (!m_frameColon) {
        return 0;
    }
    m_frameSecTens =
        (spr->m_firstFrame <= 10 && spr->m_lastFrame >= 10) ? spr->m_frames.m_pData[10] : 0;
    if (!m_frameSecTens) {
        return 0;
    }
    m_frameSecOnes =
        (spr->m_firstFrame <= 10 && spr->m_lastFrame >= 10) ? spr->m_frames.m_pData[10] : 0;
    if (!m_frameSecOnes) {
        return 0;
    }

    m_baseX = a; /* the two args captured at the tail */
    m_baseY = b;
    m_active = 1;
    m_running = 0;
    return 1;
}

// ---------------------------------------------------------------------------
// CTimer::Reset (0x9bc70) - zero the sprite-frame cache + active flag.
// ---------------------------------------------------------------------------
RVA(0x0009bc70, 0x18)
void CTimer::Reset() {
    m_sprite = 0;
    m_frameMinTens = 0;
    m_frameMinOnes = 0;
    m_frameColon = 0;
    m_frameSecTens = 0;
    m_frameSecOnes = 0;
    m_active = 0;
}

// The clock + frame-gate globals the Tick/Draw paths read (external delinked
// DATA, reloc-masked). g_645588 is the running game clock; g_644c54 a level
// base index; g_6455a0 a draw-throttle frame counter; g_645588 the start clock.
extern "C" {
    extern u32 g_645588;
}
extern i32 g_644c54;
DATA(0x006455a0)
extern u32 g_6455a0;

// The grunt the expiry / under-attack notify fires target (external, reloc-masked).
// ResolveDeathAnimation = "time up"; NotifyFortUnderAttack = "<60s remaining".
// The object FindByKey returns (or the raw m_15c when not found): its m_7c->m_18
// holds the grunt the notify fires on, gated non-null.
struct CTimerNotifyObj {
    char m_pad00[0x7c];
    struct Inner {
        char m_pad00[0x18];
        CGrunt* m_18; // +0x18
    }* m_7c;          // +0x7c
};

// ---------------------------------------------------------------------------
// CTimer::Tick (0x9bca0) - recompute the remaining time from the running clock,
// clamp at 0, and on expiry stamp the level "time up" command + fire the death
// notify; under 60s fire the "fort under attack" notify. Then decode the clamped
// value into the four MM:SS digit frames (with leading-zero blanking -> frame 10)
// via the constant-divisor magic-division chain.
// ---------------------------------------------------------------------------
// @early-stop
// big function (605 B): the magic-division decode chain + the g_gameReg notify
// dispatch through the FindByKey helper; reconstruction is logically complete
// but the leading-zero-guard branch scheduling + 64-bit clamp regalloc plateau.
// Deferred to the final sweep / a leaf-first redo.
RVA(0x0009bca0, 0x25d)
i32 CTimer::Tick(i32 dt) {
    if (!m_running) {
        return 1;
    }
    // remaining = (m_accumLo:m_accumHi) - g_645588 + (m_baseTimeLo:m_baseTimeHi), clamped at 0.
    i64 rem = *(i64*)&m_accumLo - (u32)g_645588 + *(i64*)&m_baseTimeLo;
    i32 v = (rem > 0) ? (i32)rem : 0;
    m_currentMs = v;

    if (v == 0) {
        // expired: clear, stamp the level "time up" command + clock snapshot.
        m_40 = 0;
        m_44 = 0;
        m_accumLo = 0;
        m_accumHi = 0;
        m_running = 0;
        m_currentMs = 0;
        CPlay* ls = (CPlay*)g_gameReg->m_curState;
        ls->m_winLoseBanner = 1;
        ls->m_cueInterval = 0x1f4;
        ls->m_cueIntervalHi = 0;
        ls->m_cueTimerLo = g_645588;
        ls->m_cueTimerHi = 0;
        g_gameReg->m_cmdGrid->ClearRowAndRefresh(g_644c54);
        CFocusSlot* slot = &g_gameReg->m_focusSlots[g_644c54];
        if (slot != 0) {
            slot->m_24 = 1;
        }
        i32* key = (i32*)g_gameReg->m_focusSlots[0].m_0c;
        if (key != 0) {
            i32 found = 0;
            CTimerNotifyObj* obj = (CTimerNotifyObj*)((CKeyTable*)g_gameReg->m_world->m_8)
                                       ->FindByKey((i32)key, &found);
            CTimerNotifyObj* hit = found ? obj : (CTimerNotifyObj*)key;
            if (hit != 0 && hit->m_7c->m_18 != 0) {
                hit->m_7c->m_18->ResolveDeathAnimation();
            }
        }
        return 1;
    }

    if ((u32)v < 0xea60) {
        i32* key = (i32*)g_gameReg->m_focusSlots[0].m_0c;
        if (key != 0) {
            i32 found = 0;
            CTimerNotifyObj* obj = (CTimerNotifyObj*)((CKeyTable*)g_gameReg->m_world->m_8)
                                       ->FindByKey((i32)key, &found);
            CTimerNotifyObj* hit = found ? obj : (CTimerNotifyObj*)key;
            if (hit != 0 && hit->m_7c->m_18 != 0) {
                hit->m_7c->m_18->NotifyFortUnderAttack();
            }
        }
    }

    // decode v (ms) into the MM:SS digits; a 0 digit with no significant digit to
    // its left becomes 10 (the blank frame).
    u32 t = (u32)v;
    i32 d10min = t / 600000;
    i32 d1min = t / 60000 % 10;
    if (d1min == 0 && d10min != 0) {
        d1min = 10;
    }
    u32 r = t % 60000;
    i32 d10sec = r / 10000;
    if (d10sec == 0 && (d10min != 0 || d1min != 0)) {
        d10sec = 10;
    }
    i32 d1sec = r / 1000 % 10;
    if (d1sec == 0 && d10min == 0 && d1min == 0 && d10sec == 0) {
        d1sec = 10;
    }

    CSprite* spr = m_sprite;
    m_frameMinTens = (spr->m_firstFrame <= d10min && d10min <= spr->m_lastFrame)
                         ? spr->m_frames.m_pData[d10min]
                         : 0;
    m_frameMinOnes = (spr->m_firstFrame <= d1min && d1min <= spr->m_lastFrame)
                         ? spr->m_frames.m_pData[d1min]
                         : 0;
    m_frameSecTens = (spr->m_firstFrame <= d10sec && d10sec <= spr->m_lastFrame)
                         ? spr->m_frames.m_pData[d10sec]
                         : 0;
    m_frameSecOnes = (spr->m_firstFrame <= d1sec && d1sec <= spr->m_lastFrame)
                         ? spr->m_frames.m_pData[d1sec]
                         : 0;
    return 1;
}

// ---------------------------------------------------------------------------
// CTimer::Draw (0x9bfa0) - blit the (up to) five cached digit/colon frames at
// the base position, fanned out around the centre by fixed pixel offsets.
// ---------------------------------------------------------------------------
RVA(0x0009bfa0, 0xb4)
i32 CTimer::Draw(i32 pSurf, i32 force) {
    if (!m_running) {
        return 1;
    }
    if (force == 0 && (u32)m_currentMs < 0x2710 && (u32)g_6455a0 >= 0xfa) {
        return 1;
    }
    if (m_frameMinTens) {
        ((CImage*)m_frameMinTens)
            ->RenderFrame((void*)(pSurf), (void*)(m_baseX - 0x22), (void*)(m_baseY), (void*)(0));
    }
    if (m_frameMinOnes) {
        ((CImage*)m_frameMinOnes)
            ->RenderFrame((void*)(pSurf), (void*)(m_baseX - 0x10), (void*)(m_baseY), (void*)(0));
    }
    if (m_frameColon) {
        ((CImage*)m_frameColon)
            ->RenderFrame((void*)(pSurf), (void*)(m_baseX), (void*)(m_baseY), (void*)(0));
    }
    if (m_frameSecTens) {
        ((CImage*)m_frameSecTens)
            ->RenderFrame((void*)(pSurf), (void*)(m_baseX + 0x10), (void*)(m_baseY), (void*)(0));
    }
    if (m_frameSecOnes) {
        ((CImage*)m_frameSecOnes)
            ->RenderFrame((void*)(pSurf), (void*)(m_baseX + 0x22), (void*)(m_baseY), (void*)(0));
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CTimer::SetTime (0x9c090) - set the decoded current value (+0x4c) from a
// (a<=0x63, b<=0x3b) pair: m_currentMs = (a*60 + b) * 1000 (ms). The two args are
// clamped, scaled to the minute, and the *1000 falls out as the lea chain
// (a*15, b+a*60, *5, *5, <<3).
// ---------------------------------------------------------------------------
RVA(0x0009c090, 0x37)
void CTimer::SetTime(i32 a, i32 b) {
    u32 av = (u32)a;
    if (av > 0x63) {
        av = 0x63;
    }
    u32 bv = (u32)b;
    if (bv > 0x3b) {
        bv = 0x3b;
    }
    m_currentMs = (i32)((av * 60 + bv) * 1000);
}

// ---------------------------------------------------------------------------
// CTimer::AddTime (0x9c0e0) - add (seconds, minutes) to the accumulated 64-bit
// clock, each clamped (sec<=0x63, min<=0x3b) with a carry-adjust on overflow.
// ---------------------------------------------------------------------------
RVA(0x0009c0e0, 0xa3)
void CTimer::AddTime(i32 seconds, i32 minutes) {
    if (!m_running) {
        return;
    }
    u32 mins = (u32)minutes;
    if (mins > 0x3b) {
        mins = 0x3b;
    }
    u32 secs = (u32)seconds;
    if (secs > 0x63) {
        secs = 0x63;
    }
    u32 cur = (u32)m_currentMs;
    // carry = 1 unless (the minute already on the clock + new minutes) fits in 0x3b.
    u32 carry = 1;
    if (cur % 60000 / 1000 + mins <= 0x3b) {
        carry = 0;
    }
    // clamp seconds against the second already on the clock (cur / 60000).
    if (cur / 60000 + secs > 0x63) {
        secs = 0x63 - cur / 60000 - carry;
    }
    u32 total = (mins + secs * 60) * 1000;
    *(u64*)&m_accumLo += total;
}

// ---------------------------------------------------------------------------
// CTimer::HandleEvent (0x9c1c0) - load (kind==4) / save (kind==7) the timer
// through the archive: dispatch the whole-object (de)serializer (kind 4 ->
// Deserialize 0x9c650, kind 7 -> Serialize 0x9c2e0) then stream the two 64-bit
// clock pairs (m_baseTimeLo/m_accumLo and m_38/m_40) field by field via the archive's
// Read(+0x2c)/Write(+0x30) virtuals.
// ---------------------------------------------------------------------------
// @early-stop
// regalloc wall (~88%): retail pins `this`->ebp and `kind`->ebx; our cl swaps
// them (this->ebx, kind->ebp), a register rename that cascades through every
// per-block `cmp kind` + `lea edi,[this+N]`. Logic (call mapping, slot mapping,
// pointer-advance, ret 0x10) exact; see docs/patterns/zero-register-pinning.md.
RVA(0x0009c1c0, 0xdb)
i32 CTimer::HandleEvent(CSerialArchive* ar, i32 kind, i32 a3, i32 a4) {
    if (ar == 0) {
        return 0;
    }
    if (kind == 4) {
        i32 r = Deserialize(ar);
        if (!r) {
            return r;
        }
    } else if (kind == 7) {
        i32 r = Serialize(ar);
        if (!r) {
            return r;
        }
    }

    i32* p = &m_baseTimeLo;
    if (kind == 4) {
        ar->Read(p, 8);
        p += 2;
        ar->Read(p, 8);
    } else if (kind == 7) {
        ar->Write(p, 8);
        p += 2;
        ar->Write(p, 8);
    }

    p = &m_38;
    if (kind == 4) {
        ar->Read(p, 8);
        p += 2;
        ar->Read(p, 8);
        return 1;
    } else if (kind == 7) {
        ar->Write(p, 8);
        p += 2;
        ar->Write(p, 8);
    }
    return 1;
}

// Per-serialize round counter the CString archive helpers bump (g_serialCounter,
// = ?g_serialCounter@@3HA @0x229ad0). The frame-name reverse-lookup helper (0x155630)
// lives on the sprite registry (g_gameReg->m_world->m_10); modeled with NO body -> reloc-masks.
DATA(0x00229ad0)
extern i32 g_serialCounter;
// The frame-name reverse-lookup is CImageRegistry::ReadField (0x155630, mgr->m_10);
// the former CStrReader view is gone (wave 3).

// ---------------------------------------------------------------------------
// CTimer::Serialize (0x9c2e0) - write the timer through the save archive: the
// position/flags, the sprite-set name, then each digit/colon frame by its
// registry name + a found flag, then the two clock pairs. Mirrors the shared
// CActionOptionsMenuBar::Serialize idiom (vtable Transfer + inlined memset/strcpy
// + per-frame name reverse-lookup).
// ---------------------------------------------------------------------------
// @early-stop
// stack-packing wall (~96%, same family as CActionOptionsMenuBar::Serialize):
// retail reuses the dead mgr spill slot for the per-block `zero` int; our cl
// gives `zero` its own slot, shifting every frame-size immediate by 4. Body
// (vtable Transfer dispatch + inlined memset/strcpy + name lookup) exact.
RVA(0x0009c2e0, 0x2b6)
i32 CTimer::Serialize(CSerialArchive* ar) {
    if (ar == 0) {
        return 0;
    }
    CSpriteFactoryHolder* mgr = g_gameReg->m_world;
    if (mgr == 0) {
        return 0;
    }

    ar->Write(&m_baseX, 4);
    ar->Write(&m_baseY, 4);

    char tmp[0x80];

    g_serialCounter++;
    memset(tmp, 0, sizeof(tmp));
    if (m_sprite) {
        strcpy(tmp, m_sprite->m_name);
    }
    ar->Write(tmp, 0x80);

    ar->Write(&m_active, 4);

    g_serialCounter++;
    memset(tmp, 0, sizeof(tmp));
    {
        i32 zero = 0;
        if (m_frameMinTens) {
            ((CDDrawWorkerRegistry*)mgr->m_10)->Method_155630((i32)m_frameMinTens, tmp, &zero);
        }
        ar->Write(tmp, 0x80);
        ar->Write(&zero, 4);
    }

    g_serialCounter++;
    memset(tmp, 0, sizeof(tmp));
    {
        i32 zero = 0;
        if (m_frameMinOnes) {
            ((CDDrawWorkerRegistry*)mgr->m_10)->Method_155630((i32)m_frameMinOnes, tmp, &zero);
        }
        ar->Write(tmp, 0x80);
        ar->Write(&zero, 4);
    }

    g_serialCounter++;
    memset(tmp, 0, sizeof(tmp));
    {
        i32 zero = 0;
        if (m_frameSecTens) {
            ((CDDrawWorkerRegistry*)mgr->m_10)->Method_155630((i32)m_frameSecTens, tmp, &zero);
        }
        ar->Write(tmp, 0x80);
        ar->Write(&zero, 4);
    }

    g_serialCounter++;
    memset(tmp, 0, sizeof(tmp));
    {
        i32 zero = 0;
        if (m_frameSecOnes) {
            ((CDDrawWorkerRegistry*)mgr->m_10)->Method_155630((i32)m_frameSecOnes, tmp, &zero);
        }
        ar->Write(tmp, 0x80);
        ar->Write(&zero, 4);
    }

    g_serialCounter++;
    memset(tmp, 0, sizeof(tmp));
    {
        i32 zero = 0;
        if (m_frameColon) {
            ((CDDrawWorkerRegistry*)mgr->m_10)->Method_155630((i32)m_frameColon, tmp, &zero);
        }
        ar->Write(tmp, 0x80);
        ar->Write(&zero, 4);
    }

    ar->Write(&m_running, 4);
    ar->Write(&m_currentMs, 4);
    return 1;
}
SIZE_UNKNOWN(CKeyTable);
SIZE_UNKNOWN(CLoadingBar);
SIZE_UNKNOWN(CTimerNotifyObj);
