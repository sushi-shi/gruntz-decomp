#include <DDrawMgr/DDrawSubMgrPages.h> // the m_drawTarget pages (full def)
#include <Gruntz/GameRegMfcPtr.h>      // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <rva.h>
#include <Rez/FrameClock.h> // g_timer500 (draw-throttle counter)
#include <Io/FileMem.h>     // the serialize stream (CFileMemBase == the real CFileMemBase)
#include <Gruntz/Grunt.h>
#include <Image/CImage.h>
#include <string.h> // inlined memset / strcpy in CTimer::Serialize (rep stos / rep movs)
#include <Gruntz/GameRegistry.h> // g_gameReg singleton (0x24556c) canonical view
#include <Gruntz/Warlord.h>      // CWarlord (the fort-under-attack notify target)
#include <Gruntz/Play.h>         // canonical CPlay (m_curState game-state; level-timer expiry)
#include <Gruntz/TriggerMgr.h>   // canonical CTriggerMgr (g_gameReg->m_cmdGrid; ClearRowAndRefresh)
#include <Gruntz/SerialArchive.h> // the shared CFileMemBase stream (Read @+0x2c / Write @+0x30)
#include <DDrawMgr/DDrawSurfaceMgr.h> // CDDrawSurfaceMgr (m_8 key table, m_10 image registry) + CDDrawChildGroup
#include <Gruntz/Sprite.h>                // CDDrawWorker (frame-data value) + CMapStringToOb
#include <Gruntz/Timer.h>                 // CTimer + CImage (canonical; def was local here)
#include <DDrawMgr/DDrawWorkerRegistry.h> // canonical CDDrawWorkerRegistry (AnyValueMatches)

RVA(0x000d7440, 0xad)
i32 CPlay::LoadLoadingBarSprite() {
    CObject* spr_ob = 0;
    m_world->m_imageRegistry->m_10map.Lookup("GAME_LOADINGBAR", spr_ob);
    CDDrawWorker* spr = static_cast<CDDrawWorker*>(spr_ob);
    if (!spr) {
        return 0;
    }

    m_revealCapStart = (spr->m_minIndex <= 1 && spr->m_maxIndex >= 1)
                           ? static_cast<CImage*>(spr->m_items.GetAt(1))
                           : 0;
    m_revealCapMid = (spr->m_minIndex <= 2 && spr->m_maxIndex >= 2)
                         ? static_cast<CImage*>(spr->m_items.GetAt(2))
                         : 0;
    m_revealCapEnd = (spr->m_minIndex <= 3 && spr->m_maxIndex >= 3)
                         ? static_cast<CImage*>(spr->m_items.GetAt(3))
                         : 0;
    m_revealFrame = 1;
    return 1;
}

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

RVA(0x0009bb00, 0x119)
i32 CTimer::LoadTimerSprite(i32 a, i32 b) {
    CObject* spr_ob = 0;
    g_gameReg->m_world->m_imageRegistry->m_10map.Lookup("GAME_TIMER", spr_ob);
    CDDrawWorker* spr = static_cast<CDDrawWorker*>(spr_ob);
    m_sprite = spr;
    if (!spr) {
        return 0;
    }

    m_frameMinTens = (spr->m_minIndex <= 10 && spr->m_maxIndex >= 10)
                         ? static_cast<CImage*>(spr->m_items.GetAt(10))
                         : 0;
    if (!m_frameMinTens) {
        return 0;
    }
    m_frameMinOnes = (spr->m_minIndex <= 10 && spr->m_maxIndex >= 10)
                         ? static_cast<CImage*>(spr->m_items.GetAt(10))
                         : 0;
    if (!m_frameMinOnes) {
        return 0;
    }
    m_frameColon = (spr->m_minIndex <= 11 && spr->m_maxIndex >= 11)
                       ? static_cast<CImage*>(spr->m_items.GetAt(11))
                       : 0;
    if (!m_frameColon) {
        return 0;
    }
    m_frameSecTens = (spr->m_minIndex <= 10 && spr->m_maxIndex >= 10)
                         ? static_cast<CImage*>(spr->m_items.GetAt(10))
                         : 0;
    if (!m_frameSecTens) {
        return 0;
    }
    m_frameSecOnes = (spr->m_minIndex <= 10 && spr->m_maxIndex >= 10)
                         ? static_cast<CImage*>(spr->m_items.GetAt(10))
                         : 0;
    if (!m_frameSecOnes) {
        return 0;
    }

    m_baseX = a; /* the two args captured at the tail */
    m_baseY = b;
    m_active = 1;
    m_running = 0;
    return 1;
}

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
    // remaining = (m_accumLo:m_accumHi) - g_frameTime + (m_baseTimeLo:m_baseTimeHi), clamped at 0.
    i64 rem = *reinterpret_cast<i64*>(&m_accumLo) - static_cast<u32>(g_frameTime)
              + *reinterpret_cast<i64*>(&m_baseTimeLo);
    i32 v = (rem > 0) ? static_cast<i32>(rem) : 0;
    m_currentMs = v;

    if (v == 0) {
        // expired: clear, stamp the level "time up" command + clock snapshot.
        m_40 = 0;
        m_44 = 0;
        m_accumLo = 0;
        m_accumHi = 0;
        m_running = 0;
        m_currentMs = 0;
        CPlay* ls = static_cast<CPlay*>(g_gameReg->m_curState);
        ls->m_winLoseBanner = 1;
        ls->m_cueInterval = 0x1f4;
        ls->m_cueIntervalHi = 0;
        ls->m_cueTimerLo = g_frameTime;
        ls->m_cueTimerHi = 0;
        g_gameReg->m_cmdGrid->ClearRowAndRefresh(g_curPlayer);
        GruntzPlayer* slot = &g_gameReg->m_options[g_curPlayer];
        if (slot != 0) {
            slot->m_clearedRound = 1;
        }
        i32* key = reinterpret_cast<i32*>(g_gameReg->m_options[0].m_00c);
        if (key != 0) {
            i32 found = 0;
            // the +0x48 serialize map, probed directly (ex the CKeyTable::FindByKey shim -
            // FindByKey WAS CMapPtrToPtr::Lookup @0x1b8760 on the embedded m_map48)
            void* fv = 0;
            found = g_gameReg->m_world->m_childGroup->m_map48.Lookup(static_cast<void*>(key), fv);
            CGameObject* obj = static_cast<CGameObject*>(fv);
            CGameObject* hit = found ? obj : reinterpret_cast<CGameObject*>(key);
            if (hit != 0 && hit->m_7c->m_logic != 0) {
                static_cast<CWarlord*>(hit->m_7c->m_logic)->ResolveDeathAnimation();
            }
        }
        return 1;
    }

    if (static_cast<u32>(v) < 0xea60) {
        i32* key = reinterpret_cast<i32*>(g_gameReg->m_options[0].m_00c);
        if (key != 0) {
            i32 found = 0;
            // the +0x48 serialize map, probed directly (ex the CKeyTable::FindByKey shim -
            // FindByKey WAS CMapPtrToPtr::Lookup @0x1b8760 on the embedded m_map48)
            void* fv = 0;
            found = g_gameReg->m_world->m_childGroup->m_map48.Lookup(static_cast<void*>(key), fv);
            CGameObject* obj = static_cast<CGameObject*>(fv);
            CGameObject* hit = found ? obj : reinterpret_cast<CGameObject*>(key);
            if (hit != 0 && hit->m_7c->m_logic != 0) {
                static_cast<CWarlord*>(hit->m_7c->m_logic)->NotifyFortUnderAttack();
            }
        }
    }

    // decode v (ms) into the MM:SS digits; a 0 digit with no significant digit to
    // its left becomes 10 (the blank frame).
    u32 t = static_cast<u32>(v);
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

    CDDrawWorker* spr = m_sprite;
    m_frameMinTens = (spr->m_minIndex <= d10min && d10min <= spr->m_maxIndex)
                         ? static_cast<CImage*>(spr->m_items.GetAt(d10min))
                         : 0;
    m_frameMinOnes = (spr->m_minIndex <= d1min && d1min <= spr->m_maxIndex)
                         ? static_cast<CImage*>(spr->m_items.GetAt(d1min))
                         : 0;
    m_frameSecTens = (spr->m_minIndex <= d10sec && d10sec <= spr->m_maxIndex)
                         ? static_cast<CImage*>(spr->m_items.GetAt(d10sec))
                         : 0;
    m_frameSecOnes = (spr->m_minIndex <= d1sec && d1sec <= spr->m_maxIndex)
                         ? static_cast<CImage*>(spr->m_items.GetAt(d1sec))
                         : 0;
    return 1;
}

RVA(0x0009bfa0, 0xb4)
i32 CTimer::Draw(i32 pSurf, i32 force) {
    if (!m_running) {
        return 1;
    }
    if (force == 0 && static_cast<u32>(m_currentMs) < 0x2710
        && static_cast<u32>(g_timer500) >= 0xfa) {
        return 1;
    }
    if (m_frameMinTens) {
        m_frameMinTens->RenderFrame(
            reinterpret_cast<void*>((pSurf)),
            reinterpret_cast<void*>((m_baseX - 0x22)),
            reinterpret_cast<void*>((m_baseY)),
            static_cast<void*>((0))
        );
    }
    if (m_frameMinOnes) {
        m_frameMinOnes->RenderFrame(
            reinterpret_cast<void*>((pSurf)),
            reinterpret_cast<void*>((m_baseX - 0x10)),
            reinterpret_cast<void*>((m_baseY)),
            static_cast<void*>((0))
        );
    }
    if (m_frameColon) {
        m_frameColon->RenderFrame(
            reinterpret_cast<void*>((pSurf)),
            reinterpret_cast<void*>((m_baseX)),
            reinterpret_cast<void*>((m_baseY)),
            static_cast<void*>((0))
        );
    }
    if (m_frameSecTens) {
        m_frameSecTens->RenderFrame(
            reinterpret_cast<void*>((pSurf)),
            reinterpret_cast<void*>((m_baseX + 0x10)),
            reinterpret_cast<void*>((m_baseY)),
            static_cast<void*>((0))
        );
    }
    if (m_frameSecOnes) {
        m_frameSecOnes->RenderFrame(
            reinterpret_cast<void*>((pSurf)),
            reinterpret_cast<void*>((m_baseX + 0x22)),
            reinterpret_cast<void*>((m_baseY)),
            static_cast<void*>((0))
        );
    }
    return 1;
}

RVA(0x0009c090, 0x37)
void CTimer::SetTime(i32 a, i32 b) {
    u32 av = static_cast<u32>(a);
    if (av > 0x63) {
        av = 0x63;
    }
    u32 bv = static_cast<u32>(b);
    if (bv > 0x3b) {
        bv = 0x3b;
    }
    m_currentMs = static_cast<i32>(((av * 60 + bv) * 1000));
}

RVA(0x0009c0e0, 0xa3)
void CTimer::AddTime(i32 seconds, i32 minutes) {
    if (!m_running) {
        return;
    }
    u32 mins = static_cast<u32>(minutes);
    if (mins > 0x3b) {
        mins = 0x3b;
    }
    u32 secs = static_cast<u32>(seconds);
    if (secs > 0x63) {
        secs = 0x63;
    }
    u32 cur = static_cast<u32>(m_currentMs);
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
    *reinterpret_cast<u64*>(&m_accumLo) += total;
}

// ---------------------------------------------------------------------------
// CTimer::HandleEvent (0x9c1c0) - save (kind==4) / load (kind==7) the timer
// through the archive: dispatch the whole-object serializer (kind 4 ->
// Serialize 0x9c2e0, kind 7 -> Deserialize 0x9c650) then stream the two 64-bit
// clock pairs (m_baseTimeLo/m_accumLo and m_38/m_40) field by field via the archive's
// Read(+0x2c)/Write(+0x30) virtuals.
// ---------------------------------------------------------------------------
// @early-stop
// regalloc wall (~88%): retail pins `this`->ebp and `kind`->ebx; our cl swaps
// them (this->ebx, kind->ebp), a register rename that cascades through every
// per-block `cmp kind` + `lea edi,[this+N]`. Logic (call mapping, slot mapping,
// pointer-advance, ret 0x10) exact; see docs/patterns/zero-register-pinning.md.
RVA(0x0009c1c0, 0xdb)
i32 CTimer::HandleEvent(CFileMemBase* ar, i32 kind, i32 a3, i32 a4) {
    if (ar == 0) {
        return 0;
    }
    if (kind == 4) {
        i32 r = Serialize(ar);
        if (!r) {
            return r;
        }
    } else if (kind == 7) {
        i32 r = Deserialize(ar);
        if (!r) {
            return r;
        }
    }

    i32* p = &m_baseTimeLo;
    if (kind == 4) {
        ar->Write(p, 8);
        p += 2;
        ar->Write(p, 8);
    } else if (kind == 7) {
        ar->Read(p, 8);
        p += 2;
        ar->Read(p, 8);
    }

    p = &m_38;
    if (kind == 4) {
        ar->Write(p, 8);
        p += 2;
        ar->Write(p, 8);
        return 1;
    } else if (kind == 7) {
        ar->Read(p, 8);
        p += 2;
        ar->Read(p, 8);
    }
    return 1;
}

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
i32 CTimer::Serialize(CFileMemBase* ar) {
    if (ar == 0) {
        return 0;
    }
    CDDrawSurfaceMgr* mgr = g_gameReg->m_world;
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
            mgr->m_imageRegistry->AnyValueMatches(m_frameMinTens, tmp, &zero);
        }
        ar->Write(tmp, 0x80);
        ar->Write(&zero, 4);
    }

    g_serialCounter++;
    memset(tmp, 0, sizeof(tmp));
    {
        i32 zero = 0;
        if (m_frameMinOnes) {
            mgr->m_imageRegistry->AnyValueMatches(m_frameMinOnes, tmp, &zero);
        }
        ar->Write(tmp, 0x80);
        ar->Write(&zero, 4);
    }

    g_serialCounter++;
    memset(tmp, 0, sizeof(tmp));
    {
        i32 zero = 0;
        if (m_frameSecTens) {
            mgr->m_imageRegistry->AnyValueMatches(m_frameSecTens, tmp, &zero);
        }
        ar->Write(tmp, 0x80);
        ar->Write(&zero, 4);
    }

    g_serialCounter++;
    memset(tmp, 0, sizeof(tmp));
    {
        i32 zero = 0;
        if (m_frameSecOnes) {
            mgr->m_imageRegistry->AnyValueMatches(m_frameSecOnes, tmp, &zero);
        }
        ar->Write(tmp, 0x80);
        ar->Write(&zero, 4);
    }

    g_serialCounter++;
    memset(tmp, 0, sizeof(tmp));
    {
        i32 zero = 0;
        if (m_frameColon) {
            mgr->m_imageRegistry->AnyValueMatches(m_frameColon, tmp, &zero);
        }
        ar->Write(tmp, 0x80);
        ar->Write(&zero, 4);
    }

    ar->Write(&m_running, 4);
    ar->Write(&m_currentMs, 4);
    return 1;
}
