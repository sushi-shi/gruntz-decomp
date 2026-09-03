#include <rva.h>

#include <Gruntz/Demo.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DinMgr2/DirectInputMgr2.h>
#include <Gruntz/DemoHelpers.h>
#include <Gruntz/DemoMoverState.h>
#include <Gruntz/ExitTrigger.h>
#include <Gruntz/FixedPtrArray32.h>
#include <Gruntz/FortressFlag.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntCreationPoint.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntStartingPoint.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicRecordDispatchInline.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SecretLevelTrigger.h>
#include <Gruntz/SecretTeleporterTrigger.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialRecords.h>
#include <Gruntz/Teleporter.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/Warlord.h>
#include <Gruntz/Wormhole.h>
#include <Ints.h>
#include <Io/FileMem.h>
#include <Rez/RezArchiveDir.h>
#include <Rez/RezTypeTag.h>
#include <Wwd/LogicRecordEvent.h>

#include <fstream.h>
#include <stdlib.h>
#include <string.h>

RVA(0x0003bfa0, 0x42)
i32 CDemo::LoadGameAssetNamespaces(CGruntzMgr* ctx, i32 areaArg, i32 prevStateId) {
    ctx->m_strWorldFile.Empty();
    if (CPlay::LoadGameAssetNamespaces(ctx, areaArg, prevStateId) == 0) {
        return 0;
    }
    m_demoCountdown = 0x124f80;
    return 1;
}

RVA(0x0003c010, 0x5)
void CDemo::ReleaseResources() {
    CPlay::ReleaseResources();
}

RVA(0x0003c030, 0x22)
i32 CDemo::CompleteLevel() {
    PostMessageA(m_mgr->m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_ATTRACT), 0);
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0003c070, 0x47)
i32 CDemoSetup::SetupDemoActors() {
    m_world->m_childGroup
        ->CreateSprite(1, 0, 0, 0, "DemoMover", WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE);
    m_world->m_childGroup
        ->CreateSprite(1, 0, 0, 0x270f, "DemoSign", WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE);
    return 1;
}

class CRezItm;

RVA(0x0003c0e0, 0xfb)
i32 CDemo::BuildWorldLevelPath(i32 unused) {
    m_world->m_level->ReleaseChildren();
    CString key;
    key.Format("WORLDZ\\LEVEL%i", 1);
    CRezItm* node = m_levelResources->GetRezFromPath(key, REZ_TAG_WWD);
    if (node == NULL) {
        return 0;
    }
    if (m_world->m_level->LoadFromSource(node) == 0) {
        return 0;
    }
    m_world->m_level->NotifyAllPlanes();
    m_world->m_level->m_flags |= WWD_LEVEL_FLAG_DIRECT_MOVEMENT;
    return 1;
}

RVA(0x0003c220, 0xa4)
i32 CDemo::Render() {
    CPlay::Render();
    CFixedPtrArray32* list = g_actorList;
    i32 n = list->m_count;
    for (i32 i = 0; i < n; i++) {
        if (list->m_items[i]->m_pressedButtons & IDX(INPUT_BUTTON8)) {
            PostMessageA(m_mgr->m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_MAIN_MENU), 0);
            break;
        }
    }
    if (g_frameDelta >= static_cast<u32>(m_demoCountdown)) {
        m_demoCountdown = 0;
    } else {
        m_demoCountdown -= g_frameDelta;
    }
    if (m_demoCountdown == 0) {
        PostMessageA(m_mgr->m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_ATTRACT), 0);
    }
    return 1;
}

// @early-stop
RVA(0x0003c300, 0x183)
i32 DispatchDemoMoverLogic(CGameObject* owner) {
    CLogicRecord* st = owner->m_logicRecord;
    switch (static_cast<DemoMoverState>(st->EventCode())) {
        case DEMO_MOVER_SCROLL_TO_TARGET: {

            CGameLevel* gh = st->m_ownerCtx->m_level;
            Coord current = gh->m_mainPlane->m_scrollPixel;
            if (current.m_x < st->m_scrollTarget.m_x) {
                current.m_x++;
            } else if (current.m_x > st->m_scrollTarget.m_x) {
                current.m_x--;
            }
            if (current.m_y < st->m_scrollTarget.m_y) {
                current.m_y++;
            } else if (current.m_y > st->m_scrollTarget.m_y) {
                current.m_y--;
            }

            CDDrawWorkerHost* mg = gh->m_mainPlane;
            mg->SetScrollPosition(current.m_x, current.m_y);

            Coord snapped = gh->m_mainPlane->m_scrollPixel;
            for (i32 i = 0; i < gh->m_planes.GetSize(); i++) {
                if (i != gh->m_mainIndex) {
                    CDDrawWorkerHost* p = static_cast<CDDrawWorkerHost*>(gh->m_planes[i]);
                    p->SetScrollPosition(snapped.m_x, snapped.m_y);
                }
            }

            if (st->m_scrollTarget == current) {
                st->SetEventCode(IDX(DEMO_MOVER_CHOOSE_TARGET));
            }
            return 1;
        }
        case DEMO_MOVER_CHOOSE_TARGET: {

            CSize range = st->m_ownerCtx->m_level->m_mainPlane->m_planePixelSize;
            Coord target;
            target.m_x = (range.cx == -1) ? (rand() % 2 - 1) : (rand() % (range.cx + 1));
            target.m_y = (range.cy == -1) ? (rand() % 2 - 1) : (rand() % (range.cy + 1));
            st->m_scrollTarget = target;
            st->SetEventCode(IDX(DEMO_MOVER_SCROLL_TO_TARGET));
            break;
        }
    }
    return 1;
}

RVA(0x0003c500, 0x6)
i32 DispatchDemoSignLogic(CGameObject* obj) {
    return 1;
}
