#include <rva.h>

#include <AddrWord.h>
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

#include <string.h>

RVA(0x0009bab0, 0x35)
CTimer* CTimer::Init() {
    m_baseTime.m_v = 0;
    m_accum.m_v = 0;
    m_startStamp.m_v = 0;
    m_unusedStamp.m_v = 0;
    m_sprite = NULL;
    m_frameMinTens = NULL;
    m_frameMinOnes = NULL;
    m_frameColon = NULL;
    m_frameSecTens = NULL;
    m_frameSecOnes = NULL;
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

    m_baseX = a;
    m_baseY = b;
    m_active = 1;
    m_running = 0;
    return 1;
}

RVA(0x0009bc70, 0x18)
void CTimer::Reset() {
    m_sprite = NULL;
    m_frameMinTens = NULL;
    m_frameMinOnes = NULL;
    m_frameColon = NULL;
    m_frameSecTens = NULL;
    m_frameSecOnes = NULL;
    m_active = 0;
}

// @early-stop
RVA(0x0009bca0, 0x25d)
i32 CTimer::Tick(i32 dt) {
    if (!m_running) {
        return 1;
    }

    i64 rem = m_accum.m_v - static_cast<u32>(g_frameTime) + m_baseTime.m_v;
    i32 v = (rem > 0) ? static_cast<i32>(rem) : 0;
    m_currentMs = v;

    if (v == 0) {

        m_unusedStamp.m_v = 0;
        m_accum.m_v = 0;
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
        if (slot != NULL) {
            slot->m_clearedRound = 1;
        }
        i32 key = g_gameReg->m_options[0].m_warlordObjectId;
        if (key != 0) {
            i32 found = 0;

            CGameObject* obj = 0;
            found = MapLookupById(g_gameReg->m_world->m_childGroup->m_map48, key, obj);

            AddrWord<CGameObject> raw;
            raw.m_word = key;
            CGameObject* hit = found ? obj : raw.m_addr;
            if (hit != NULL && hit->m_animWorker->m_logic != NULL) {
                static_cast<CWarlord*>(hit->m_animWorker->m_logic)->ResolveDeathAnimation();
            }
        }
        return 1;
    }

    if (static_cast<u32>(v) < 0xea60) {
        i32 key = g_gameReg->m_options[0].m_warlordObjectId;
        if (key != 0) {
            i32 found = 0;

            CGameObject* obj = 0;
            found = MapLookupById(g_gameReg->m_world->m_childGroup->m_map48, key, obj);

            AddrWord<CGameObject> raw;
            raw.m_word = key;
            CGameObject* hit = found ? obj : raw.m_addr;
            if (hit != NULL && hit->m_animWorker->m_logic != NULL) {
                static_cast<CWarlord*>(hit->m_animWorker->m_logic)->NotifyFortUnderAttack();
            }
        }
    }

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
    if (cur % 60000 / 1000 + mins > 0x3b) {
        carry = 1;
    }

    onClock = cur / 60000;
    if (onClock + secs > 0x63) {
        secs = 0x63 - onClock - carry;
    }
    u32 total = (mins + secs * 60) * 1000;
    m_accum.m_v += total;
}

// @early-stop
RVA(0x0009c1c0, 0xdb)
i32 CTimer::HandleEvent(CFileMemBase* ar, SerialMode kind, LogicTypeId typeId, i32 pObj) {
    if (ar == NULL) {
        return 0;
    }
    switch (kind) {
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

    switch (kind) {
        case SERIAL_SAVE:
            ar->Write(&m_baseTime, sizeof(m_baseTime));
            ar->Write(&m_accum, sizeof(m_accum));
            break;
        case SERIAL_LOAD:
            ar->Read(&m_baseTime, sizeof(m_baseTime));
            ar->Read(&m_accum, sizeof(m_accum));
            break;
    }

    switch (kind) {
        case SERIAL_SAVE:
            ar->Write(&m_startStamp, sizeof(m_startStamp));
            ar->Write(&m_unusedStamp, sizeof(m_unusedStamp));
            return 1;
        case SERIAL_LOAD:
            ar->Read(&m_startStamp, sizeof(m_startStamp));
            ar->Read(&m_unusedStamp, sizeof(m_unusedStamp));
            break;
    }
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
