#include <rva.h>

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/Play.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/Timer.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/Warlord.h>
#include <Image/CImage.h>
#include <Io/FileMem.h>
#include <Rez/FrameClock.h>
#include <Utils/MapTyped.h>
#include <Utils/MillisPer.h>

#include <string.h>

static inline CDDrawWorker* LookupWorker(CMapStringToOb& map, LPCTSTR name) {
    CObject* found = NULL;
    map.Lookup(name, found);
    return static_cast<CDDrawWorker*>(found);
}

#define RESET_TIMER_SPRITES                                                                        \
    m_sprite = NULL;                                                                               \
    m_frameMinTens = NULL;                                                                         \
    m_frameMinOnes = NULL;                                                                         \
    m_frameColon = NULL;                                                                           \
    m_frameSecTens = NULL;                                                                         \
    m_frameSecOnes = NULL;                                                                         \
    m_active = 0

RVA(0x0009bab0, 0x35)
CTimer::CTimer() {
    // Halves, not the i64: cl5 batches a RUN of 64-bit stores as all-lo-then-all-hi,
    // so four `m_v = 0` in a row emit lo x4 / hi x4. Retail pairs them two at a time.
    m_baseTime.m_lo = 0;
    m_accum.m_lo = 0;
    m_baseTime.m_hi = 0;
    m_accum.m_hi = 0;
    m_startStamp.m_lo = 0;
    m_unusedStamp.m_lo = 0;
    m_startStamp.m_hi = 0;
    m_unusedStamp.m_hi = 0;
    RESET_TIMER_SPRITES;
    m_running = 0;
}

RVA(0x0009bb00, 0x119)
i32 CTimer::LoadTimerSprite(i32 originX, i32 originY) {
    CDDrawWorker* spr =
        LookupWorker(g_gameReg->m_world->m_imageRegistry->m_workersByName, "GAME_TIMER");
    m_sprite = spr;
    if (!spr) {
        return 0;
    }

    m_frameMinTens = spr->GetAt(10);
    if (!m_frameMinTens) {
        return 0;
    }
    m_frameMinOnes = spr->GetAt(10);
    if (!m_frameMinOnes) {
        return 0;
    }
    m_frameColon = spr->GetAt(11);
    if (!m_frameColon) {
        return 0;
    }
    m_frameSecTens = spr->GetAt(10);
    if (!m_frameSecTens) {
        return 0;
    }
    m_frameSecOnes = spr->GetAt(10);
    if (!m_frameSecOnes) {
        return 0;
    }

    m_baseX = originX;
    m_baseY = originY;
    m_active = 1;
    m_running = 0;
    return 1;
}

RVA(0x0009bc70, 0x18)
void CTimer::Reset() {
    RESET_TIMER_SPRITES;
}

// @early-stop
RVA(0x0009bca0, 0x25d)
i32 CTimer::Tick(i32 dt) {
    if (!m_running) {
        return 1;
    }

    i64 rem = m_accum.m_v - static_cast<u32>(g_frameTime) + m_baseTime.m_v;
    i32 v = (rem < 0) ? 0 : static_cast<i32>(rem);
    m_currentMs = v;

    if (v == 0) {

        // Halves, not the i64: cl5 batches a RUN of 64-bit stores as all-lo-then-
        // all-hi.  Retail pairs them, as the ctor above does.
        m_unusedStamp.m_lo = 0;
        m_unusedStamp.m_hi = 0;
        m_accum.m_lo = 0;
        m_accum.m_hi = 0;
        m_running = 0;
        m_currentMs = 0;
        CPlay* ls = static_cast<CPlay*>(g_gameReg->m_curState);
        ls->m_winLoseBanner = 1;
        ls->m_cueTiming.m_interval.m_lo = 0x1f4;
        ls->m_cueTiming.m_interval.m_hi = 0;
        ls->m_cueTiming.m_start.m_lo = g_frameTime;
        ls->m_cueTiming.m_start.m_hi = 0;
        g_gameReg->m_cmdGrid->StartPlayerDefeatSequence(g_curPlayer);
        GruntzPlayer* slot = &g_gameReg->m_options[g_curPlayer];
        if (slot != NULL) {
            slot->m_clearedRound = 1;
        }
        i32 key = g_gameReg->m_options[0].m_warlordObjectId;
        if (key != 0) {
            CGameObject* obj = NULL;
            CGameObject* hit = NULL;
            if (MapLookupById(
                    g_gameReg->m_world->m_childGroup->m_registeredGameObjectsById,
                    key,
                    obj
                )) {
                hit = obj;
            }
            if (hit != NULL && hit->m_animWorker->m_logic != NULL) {
                static_cast<CWarlord*>(hit->m_animWorker->m_logic)->ResolveDeathAnimation();
            }
        }
        return 1;
    }

    if (static_cast<u32>(v) < 0xea60) {
        i32 key = g_gameReg->m_options[0].m_warlordObjectId;
        if (key != 0) {
            CGameObject* obj = NULL;
            CGameObject* hit = NULL;
            if (MapLookupById(
                    g_gameReg->m_world->m_childGroup->m_registeredGameObjectsById,
                    key,
                    obj
                )) {
                hit = obj;
            }
            if (hit != NULL && hit->m_animWorker->m_logic != NULL) {
                static_cast<CWarlord*>(hit->m_animWorker->m_logic)->NotifyFortUnderAttack();
            }
        }
    }

    u32 t = static_cast<u32>(m_currentMs);
    i32 d10min = t / (MILLIS_PER_MINUTE * 10);
    i32 d1min = t / MILLIS_PER_MINUTE % 10;
    if (d1min == 0 && d10min != 0) {
        d1min = 10;
    }
    u32 r = t % MILLIS_PER_MINUTE;
    i32 d10sec = r / 10000;
    if (d10sec == 0 && (d10min != 0 || d1min != 0)) {
        d10sec = 10;
    }
    i32 d1sec = r / MILLIS_PER_SECOND % 10;
    if (d1sec == 0 && (d10min != 0 || d1min != 0 || d10sec != 0)) {
        d1sec = 10;
    }

    CDDrawWorker* spr = m_sprite;
    m_frameMinTens = spr->GetAt(d10min);
    m_frameMinOnes = spr->GetAt(d1min);
    m_frameSecTens = spr->GetAt(d10sec);
    m_frameSecOnes = spr->GetAt(d1sec);
    return 1;
}

RVA(0x0009bfa0, 0xb4)
i32 CTimer::Draw(CDDrawSurfacePair* target, i32 force) {
    if (!m_running) {
        return 1;
    }
    if (force == 0 && static_cast<u32>(m_currentMs) < 0x2710
        && static_cast<u32>(g_timer500) >= 0xfa) {
        return 1;
    }
    if (m_frameMinTens) {
        m_frameMinTens->RenderFrame(target, m_baseX - 0x22, m_baseY, 0);
    }
    if (m_frameMinOnes) {
        m_frameMinOnes->RenderFrame(target, m_baseX - 0x10, m_baseY, 0);
    }
    if (m_frameColon) {
        m_frameColon->RenderFrame(target, m_baseX, m_baseY, 0);
    }
    if (m_frameSecTens) {
        m_frameSecTens->RenderFrame(target, m_baseX + 0x10, m_baseY, 0);
    }
    if (m_frameSecOnes) {
        m_frameSecOnes->RenderFrame(target, m_baseX + 0x22, m_baseY, 0);
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
    u32 onClock;

    u32 carry = 0;
    if (cur % MILLIS_PER_MINUTE / MILLIS_PER_SECOND + mins > 0x3b) {
        carry = 1;
    }

    onClock = cur / MILLIS_PER_MINUTE;
    if (onClock + secs > 0x63) {
        secs = 0x63 - onClock - carry;
    }
    u32 total = (mins + secs * 60) * 1000;
    m_accum.m_v += total;
}

// @early-stop
RVA(0x0009c1c0, 0xdb)
i32 CTimer::HandleEvent(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload) {
    if (ar == NULL) {
        return 0;
    }
    switch (mode) {
        case SERIAL_SAVE: {
            i32 r = Serialize(ar);
            if (!r) {
                return r;
            }
            break;
        }
        case SERIAL_LOAD: {
            i32 r = Deserialize(ar);
            if (!r) {
                return r;
            }
            break;
        }
    }

    SerBandPair(ar, mode, &m_baseTime);

    SerBandPair(ar, mode, &m_startStamp);
    return 1;
}

RVA(0x0009c2e0, 0x2b6)
i32 CTimer::Serialize(CFileMemBase* ar) {
    if (ar == NULL) {
        return 0;
    }
    CDDrawSurfaceMgr* mgr = g_gameReg->m_world;
    if (mgr == NULL) {
        return 0;
    }

    ar->Write(&m_baseX, sizeof(m_baseX));
    ar->Write(&m_baseY, sizeof(m_baseY));

    char tmp[SERIAL_NAME_LEN];

    g_serialCounter++;
    memset(tmp, 0, sizeof(tmp));
    if (m_sprite) {
        strcpy(tmp, m_sprite->m_name);
    }
    ar->Write(tmp, SERIAL_NAME_LEN);

    ar->Write(&m_active, sizeof(m_active));

    g_serialCounter++;
    memset(tmp, 0, sizeof(tmp));
    {
        i32 zero = 0;
        if (m_frameMinTens) {
            mgr->m_imageRegistry->AnyValueMatches(m_frameMinTens, tmp, &zero);
        }
        ar->Write(tmp, SERIAL_NAME_LEN);
        ar->Write(&zero, sizeof(zero));
    }

    g_serialCounter++;
    memset(tmp, 0, sizeof(tmp));
    {
        i32 zero = 0;
        if (m_frameMinOnes) {
            mgr->m_imageRegistry->AnyValueMatches(m_frameMinOnes, tmp, &zero);
        }
        ar->Write(tmp, SERIAL_NAME_LEN);
        ar->Write(&zero, sizeof(zero));
    }

    g_serialCounter++;
    memset(tmp, 0, sizeof(tmp));
    {
        i32 zero = 0;
        if (m_frameSecTens) {
            mgr->m_imageRegistry->AnyValueMatches(m_frameSecTens, tmp, &zero);
        }
        ar->Write(tmp, SERIAL_NAME_LEN);
        ar->Write(&zero, sizeof(zero));
    }

    g_serialCounter++;
    memset(tmp, 0, sizeof(tmp));
    {
        i32 zero = 0;
        if (m_frameSecOnes) {
            mgr->m_imageRegistry->AnyValueMatches(m_frameSecOnes, tmp, &zero);
        }
        ar->Write(tmp, SERIAL_NAME_LEN);
        ar->Write(&zero, sizeof(zero));
    }

    g_serialCounter++;
    memset(tmp, 0, sizeof(tmp));
    {
        i32 zero = 0;
        if (m_frameColon) {
            mgr->m_imageRegistry->AnyValueMatches(m_frameColon, tmp, &zero);
        }
        ar->Write(tmp, SERIAL_NAME_LEN);
        ar->Write(&zero, sizeof(zero));
    }

    ar->Write(&m_running, sizeof(m_running));
    ar->Write(&m_currentMs, sizeof(m_currentMs));
    return 1;
}
