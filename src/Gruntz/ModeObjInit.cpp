#include <Mfc.h>
#include <new>

#include <Ints.h>
#include <rva.h>
#include <string.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Play.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/ChatBoxOwner.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/TileTriggerContainer.h>
#include <Gruntz/TileTriggerSwitchLogic.h>

#include <Gruntz/Timer.h>

namespace modeinit {}

RVA(0x000c86d0, 0x11)
CSbiHlRow::CSbiHlRow() {

    m_lastLo = 0;
    m_intervalLo = 0;
    m_lastHi = 0;
    m_intervalHi = 0;
}

// @early-stop
RVA(0x000c7ec0, 0x5f5)
i32 CPlay::LoadGameAssetNamespaces(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId) {
    using namespace modeinit;
    {
        if (mgr == 0) {
            return 0;
        }
        GruntzPlayer* sub = mgr->m_options;
        if (sub == 0) {
            return 0;
        }
        sub->m_liveGate = 1;
        sub->m_humanControlled = 1;
        m_region0Gate = 0;
        m_region1Gate = 0;
        m_region2Gate = 0;
        m_region3Gate = 0;
        m_viewMode = 0;
        m_hudSuppressed = 1;
        m_cameraBookmarkIndex = -1;
        m_snapshotActive = 0;
        m_scrollEdgeActive = 0;
        m_scrollEdgeLock = 0;
        m_frameMarker = 0;

        if (!CState::LoadGameAssetNamespaces(mgr, areaArg, prevStateId)) {
            return 0;
        }

        CChatBoxOwner* ctl = static_cast<CChatBoxOwner*>(::operator new(0x1c));
        if (ctl) {

            ctl->m_world = 0;
            ctl->m_fontConfig = 0;
            ctl->m_attached = 0;
            ctl->m_inputActive = 0;
            ctl->m_originX = 0;
            ctl->m_originY = 0;
            ctl->m_mode = 1;
        } else {
            ctl = 0;
        }
        m_hitTest = ctl;
        if (m_hitTest->Attach(m_world, m_mgr->m_chatLog) == 0) {
            if (m_hitTest) {
                m_hitTest->Deactivate();
                ::operator delete(m_hitTest);
            }
            m_hitTest = 0;
            return 0;
        }
        m_hitTest->m_inputActive = 0;
        m_hitTest->Configure(1);

        m_guts = new CStatusBarMgr;
        if (m_guts->LoadBattlezItemConfig(m_world) == 0) {
            CStatusBarMgr* w2 = m_guts;
            if (w2 == 0) {
                return 0;
            }

            delete w2;
            m_guts = 0;
            return 0;
        }

        CTileTriggerContainer* r78 = static_cast<CTileTriggerContainer*>(::operator new(0x78));
        if (r78) {

            new (&r78->m_base) CPtrList(0xa);
            new (&r78->m_list1) CPtrList(0xa);
            new (&r78->m_list2) CPtrList(0xa);
            new (&r78->m_list3) CPtrList(0xa);
            r78->m_built = 0;
        } else {
            r78 = 0;
        }
        m_beginMarker = r78;
        if (m_beginMarker->GetFlag74() == 0) {

            delete m_beginMarker;
            m_beginMarker = 0;
            return 0;
        }

        CTimer* r50 = static_cast<CTimer*>(::operator new(0x50));
        if (r50) {
            r50->Init();
        } else {
            r50 = 0;
        }
        m_frameMarker = r50;
        if (r50 == 0) {
            return 0;
        }

        if (ShowCursor(0) >= 0) {
            while (ShowCursor(0) >= 0) {
            }
        }
        m_initialFramePending = 1;
        m_notifyLatch = 0;
        m_completedFinalLevel = 0;
        memset(&m_saveSlot, 0, sizeof(m_saveSlot));
        mgr->ResetClockGlobals();
        m_savedClock = 0;
        m_rngSeed = timeGetTime();
        m_lightFx = 0;
        if (m_mgr->m_loadingSaveGame == 0) {
            m_mgr->m_saveInfoRec = 0;
        }
        if (!LoadImageBanks()) {
            return 0;
        }
        PostLoadImageBanks();
        if (!LoadByMode(areaArg, 1)) {
            return 0;
        }
        if (!LoadCursorSprites(0, 0)) {
            return 0;
        }
        CWwdGameObjectA* peer = m_scrollSink;
        if (peer) {
            peer->m_stateFlags |= 1;
        }
        return 1;
    }
}
