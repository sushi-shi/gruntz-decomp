#include <rva.h>

#include <Gruntz/Demo.h>

#include <Bute/ButeMgr.h>
#include <Bute/SymTab.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DinMgr2/DirectInputMgr2.h>
#include <Gruntz/AnimWorker.h>
#include <Gruntz/DemoHelpers.h>
#include <Gruntz/ExitTrigger.h>
#include <Gruntz/FixedPtrArray32.h>
#include <Gruntz/FortressFlag.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntCreationPoint.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntStartingPoint.h>
#include <Gruntz/GruntzMgr.h>
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
#include <Rez/RezTypeTag.h>
#include <Wwd/AnimWorkerAct.h>

#include <fstream.h>
#include <stdlib.h>
#include <string.h>

DATA(0x0020d008)
const i32 g_directionClockwiseTable[27] = {
    0, 1, 1, 0, 2, 2, 1, 2, 3, 0, 0, 8, 1, 1, 0, 2, 2, 4, 1, 0, 7, 2, 0, 6, 2, 1, 5,
};
DATA(0x0020d078)
const i32 g_directionCounterclockwiseTable[27] = {
    1, 0, 7, 0, 0, 8, 0, 1, 1, 2, 0, 6, 1, 1, 0, 0, 2, 2, 2, 1, 5, 2, 2, 4, 1, 2, 3,
};

DATA(0x0022c450)
i32 g_buteEditLen;
DATA(0x0022c458)
char g_buteEditBuf[0x10000];

RVA(0x0003bfa0, 0x42)
i32 CDemo::LoadGameAssetNamespaces(CGruntzMgr* ctx, i32 areaArg, i32 a2) {
    ctx->m_strWorldFile.Empty();
    if (CPlay::LoadGameAssetNamespaces(ctx, areaArg, a2) == 0) {
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
    PostMessageA(m_mgr->m_gameWnd->m_hwnd, WM_COMMAND, 0x8027, 0);
    return 1;
}

RVA(0x0003c070, 0x47)
i32 CDemoSetup::SetupDemoActors() {
    m_world->m_childGroup->CreateSprite(1, 0, 0, 0, "DemoMover", 0x40003);
    m_world->m_childGroup->CreateSprite(1, 0, 0, 0x270f, "DemoSign", 0x40003);
    return 1;
}

class CParseSource;

RVA(0x0003c0e0, 0xfb)
i32 CDemo::BuildWorldLevelPath(i32 unused) {
    m_world->m_level->ReleaseChildren();
    CString key;
    key.Format("WORLDZ\\LEVEL%i", 1);
    CParseSource* node = m_levelBank->ResolveQualified(key, REZ_TAG_WWD);
    if (node == NULL) {
        return 0;
    }
    if (m_world->m_level->LoadFromSource(node) == 0) {
        return 0;
    }
    m_world->m_level->NotifyAllPlanes();
    m_world->m_level->m_flags |= 4;
    return 1;
}

RVA(0x0003c220, 0xa4)
i32 CDemo::Render() {
    CPlay::Render();
    CFixedPtrArray32* list = g_actorList;
    i32 n = list->m_count;
    for (i32 i = 0; i < n; i++) {
        if (list->m_items[i]->m_currentKeys & 0x100) {
            PostMessageA(m_mgr->m_gameWnd->m_hwnd, WM_COMMAND, 0x8023, 0);
            break;
        }
    }
    if (g_frameDelta >= static_cast<u32>(m_demoCountdown)) {
        m_demoCountdown = 0;
    } else {
        m_demoCountdown -= g_frameDelta;
    }
    if (m_demoCountdown == 0) {
        PostMessageA(m_mgr->m_gameWnd->m_hwnd, WM_COMMAND, 0x8027, 0);
    }
    return 1;
}

// @early-stop
RVA(0x0003c300, 0x183)
i32 CreateDemoMover(CGameObject* owner) {
    AnimWorkerObj* st = owner->m_animWorker;
    switch (st->ActKey()) {
        case 1: {

            CGameLevel* gh = st->m_ownerCtx->m_level;
            i32 curX = gh->m_mainPlane->m_viewRect.left;
            i32 curY = gh->m_mainPlane->m_viewRect.top;
            if (curX < st->m_scrollTargetX) {
                curX++;
            } else if (curX > st->m_scrollTargetX) {
                curX--;
            }
            if (curY < st->m_scrollTargetY) {
                curY++;
            } else if (curY > st->m_scrollTargetY) {
                curY--;
            }

            CDDrawWorkerHost* mg = gh->m_mainPlane;
            float fx = static_cast<float>(curX);
            float fy = static_cast<float>(curY);
            if (!(mg->m_flags & 1)) {
                fx *= mg->m_scaleX;
                fy *= mg->m_scaleY;
            }
            mg->m_scaledX = fx;
            mg->m_scaledY = fy;
            mg->RecomputePlaneCoords();

            for (i32 i = 0; i < gh->m_planes.GetSize(); i++) {
                CDDrawWorkerHost* p = static_cast<CDDrawWorkerHost*>(gh->m_planes[i]);
                float px = static_cast<float>(curX);
                float py = static_cast<float>(curY);
                if (!(p->m_flags & 1)) {
                    px *= p->m_scaleX;
                    py *= p->m_scaleY;
                }
                p->m_scaledX = px;
                p->m_scaledY = py;
                p->RecomputePlaneCoords();
            }

            if (st->m_scrollTargetX == curX && st->m_scrollTargetY == curY) {
                st->m_actKey = 0;
            }
            return 1;
        }
        case 0: {

            i32 rx = st->m_ownerCtx->m_level->m_mainPlane->m_wrapW;
            st->m_scrollTargetX = (rx == -1) ? (rand() % 2 - 1) : (rand() % (rx + 1));
            i32 ry = st->m_ownerCtx->m_level->m_mainPlane->m_wrapH;
            st->m_scrollTargetY = (ry == -1) ? (rand() % 2 - 1) : (rand() % (ry + 1));
            st->SetActKey(1);
            break;
        }
    }
    return 1;
}

RVA(0x0003c500, 0x6)
i32 CreateDemoSign(CGameObject* obj) {
    return 1;
}

RVA(0x0003c7f0, 0x18)
bool SameCellTag(const GruntDirectionCell* a, const GruntDirectionCell* b) {
    return a->direction == b->direction;
}

// @early-stop
RVA(0x0003c850, 0x38)
void GruntDirectionCell::RotateClockwise(i32 steps) {
    if (steps > 0) {
        do {
            const i32* e = &g_directionClockwiseTable[(row * 3 + column) * 3];
            row = e[0];
            column = e[1];
            direction = e[2];
        } while (--steps);
    }
}

// @early-stop
RVA(0x0003c8a0, 0x38)
void GruntDirectionCell::RotateCounterclockwise(i32 steps) {
    if (steps > 0) {
        do {
            const i32* e = &g_directionCounterclockwiseTable[(row * 3 + column) * 3];
            row = e[0];
            column = e[1];
            direction = e[2];
        } while (--steps);
    }
}

RVA(0x0003c8f0, 0x76)
i32 CTriRecord::Serialize(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d) {
    switch (tag) {
        case SERIAL_SAVE:
            ar->Write(&row, sizeof(row));
            ar->Write(&column, sizeof(column));
            ar->Write(&direction, sizeof(direction));
            break;
        case SERIAL_LOAD:
            ar->Read(&row, sizeof(row));
            ar->Read(&column, sizeof(column));
            ar->Read(&direction, sizeof(direction));
            break;
    }
    return 1;
}

RVA(0x0003c990, 0x1bc)
INT_PTR CALLBACK ButeAttributezDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    static_cast<void>(lParam);

    switch (msg) {
        case WM_INITDIALOG: {
            ifstream in("attributez.txt", ios::nocreate | ios::binary);
            if (in.fail()) {
                EndDialog(hDlg, 1);
            } else {
                in.read(g_buteEditBuf, 0xffff);
                g_buteEditLen = in.gcount();
                g_buteEditBuf[g_buteEditLen] = 0;
                SetDlgItemTextA(hDlg, 0x435, g_buteEditBuf);
                in.close();
            }
            return 1;
        }
        case WM_COMMAND:
            switch (wParam) {
                case IDOK: {
                    GetDlgItemTextA(hDlg, 0x435, g_buteEditBuf, 0xffff);
                    ofstream out("Attributez.txt", ios::binary);
                    g_buteEditLen = strlen(g_buteEditBuf);
                    out.write(g_buteEditBuf, g_buteEditLen);
                    out.close();
                    g_buteMgr.Parse("Attributez.txt", 0);
                    EndDialog(hDlg, 1);
                    return 1;
                }
                case IDCANCEL:
                    EndDialog(hDlg, 0);
                    return 1;
            }
            break;
    }
    return 0;
}

RVA_COMPGEN(0x0003cbc0, 0x14, ??_Difstream@@QAEXXZ)
RVA_COMPGEN(0x0003cbf0, 0x14, ??_Dofstream@@QAEXXZ)

// @early-stop
RVA(0x0003cc20, 0x14e)
bool CButeMgr::Parse(CString filename, int streamBase) {

    ifstream* s = new ifstream(filename, ios::in | ios::nocreate);
    m_stream = s;
    if (s->fail()) {
        return false;
    }

    Init();
    m_streamBase = streamBase;
    m_str108 = filename;

    m_tree.Reset();
    m_tree48.Reset();
    m_tree74.Reset();

    bool result = true;
    if (!ParseGroup()) {
        m_parseFailed = 1;
        result = false;
    }

    (static_cast<ifstream*>(m_stream))->sync();
    delete static_cast<ifstream*>(m_stream);
    return result;
}

DATA(0x0023f790)
char g_dwRectsEditBuf[0x4000];
DATA(0x00243790)
i32 g_dwRectsEditLen;

RVA(0x0003cdd0, 0x19f)
INT_PTR CALLBACK EditDwRectsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    static_cast<void>(lParam);
    switch (msg) {
        case WM_INITDIALOG: {
            ifstream in("dwrects.txt", ios::nocreate | ios::binary);
            if (in.fail()) {
                EndDialog(hDlg, 1);
            } else {
                in.read(g_dwRectsEditBuf, 0x4000);
                g_dwRectsEditLen = in.gcount();
                g_dwRectsEditBuf[g_dwRectsEditLen] = 0;
                SetDlgItemTextA(hDlg, 0x435, g_dwRectsEditBuf);
                in.close();
            }
            return 1;
        }
        case WM_COMMAND:
            switch (wParam) {
                case IDOK: {
                    GetDlgItemTextA(hDlg, 0x435, g_dwRectsEditBuf, 0x4000);
                    ofstream out("dwrects.txt", ios::binary);
                    g_dwRectsEditLen = strlen(g_dwRectsEditBuf);
                    out.write(g_dwRectsEditBuf, g_dwRectsEditLen);
                    out.close();
                    EndDialog(hDlg, 1);
                    return 1;
                }
                case IDCANCEL:
                    EndDialog(hDlg, 0);
                    return 1;
            }
            break;
    }
    return 0;
}

RVA(0x0003d2b0, 0xf1)
i32 CreateGruntStartingPoint(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_animWorker;
    switch (static_cast<u32>(rec->ActKey())) {
        case ACT_UNINITIALISED: {
            rec->SetActKey(ACT_LIVE);
            CUserLogic* sub = new CGruntStartingPoint(owner);
            sub->Activate();
            rec->m_logic = sub;
            break;
        }
        case ACT_OBJECT_REMOVED:
            rec->m_logic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            rec->m_logic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            rec->m_logic->PrepareSave();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            rec->m_logic->AfterLoadReferences();
            break;
        case ACT_AFTER_LOAD:
            rec->m_logic->AfterLoad();
            break;
        case ACT_AFTER_SAVE:
            rec->m_logic->AfterSave();
            break;
        case ACT_LIVE:
            break;
        default:
            Worker_DefaultPump(rec->m_logic);
            break;
    }
    return 1;
}

RVA(0x0003d3f0, 0xf1)
i32 CreateExitTrigger(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_animWorker;
    switch (static_cast<u32>(rec->ActKey())) {
        case ACT_UNINITIALISED: {
            rec->SetActKey(ACT_LIVE);
            CUserLogic* sub = new CExitTrigger(owner);
            sub->Activate();
            rec->m_logic = sub;
            break;
        }
        case ACT_OBJECT_REMOVED:
            rec->m_logic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            rec->m_logic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            rec->m_logic->PrepareSave();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            rec->m_logic->AfterLoadReferences();
            break;
        case ACT_AFTER_LOAD:
            rec->m_logic->AfterLoad();
            break;
        case ACT_AFTER_SAVE:
            rec->m_logic->AfterSave();
            break;
        case ACT_LIVE:
            break;
        default:
            Worker_DefaultPump(rec->m_logic);
            break;
    }
    return 1;
}

RVA(0x0003d530, 0xf1)
i32 CreateGruntCreationPoint(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_animWorker;
    switch (static_cast<u32>(rec->ActKey())) {
        case ACT_UNINITIALISED: {
            rec->SetActKey(ACT_LIVE);
            CUserLogic* sub = new CGruntCreationPoint(owner);
            sub->Activate();
            rec->m_logic = sub;
            break;
        }
        case ACT_OBJECT_REMOVED:
            rec->m_logic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            rec->m_logic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            rec->m_logic->PrepareSave();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            rec->m_logic->AfterLoadReferences();
            break;
        case ACT_AFTER_LOAD:
            rec->m_logic->AfterLoad();
            break;
        case ACT_AFTER_SAVE:
            rec->m_logic->AfterSave();
            break;
        case ACT_LIVE:
            break;
        default:
            Worker_DefaultPump(rec->m_logic);
            break;
    }
    return 1;
}

RVA(0x0003d670, 0xf1)
i32 CreateWormhole(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_animWorker;
    switch (static_cast<u32>(rec->ActKey())) {
        case ACT_UNINITIALISED: {
            rec->SetActKey(ACT_LIVE);
            CUserLogic* sub = new CWormhole(owner);
            sub->Activate();
            rec->m_logic = sub;
            break;
        }
        case ACT_OBJECT_REMOVED:
            rec->m_logic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            rec->m_logic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            rec->m_logic->PrepareSave();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            rec->m_logic->AfterLoadReferences();
            break;
        case ACT_AFTER_LOAD:
            rec->m_logic->AfterLoad();
            break;
        case ACT_AFTER_SAVE:
            rec->m_logic->AfterSave();
            break;
        case ACT_LIVE:
            break;
        default:
            Worker_DefaultPump(rec->m_logic);
            break;
    }
    return 1;
}

RVA(0x0003d7b0, 0xf1)
i32 CreateGruntPuddle(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_animWorker;
    switch (static_cast<u32>(rec->ActKey())) {
        case ACT_UNINITIALISED: {
            rec->SetActKey(ACT_LIVE);
            CUserLogic* sub = new CGruntPuddle(owner);
            sub->Activate();
            rec->m_logic = sub;
            break;
        }
        case ACT_OBJECT_REMOVED:
            rec->m_logic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            rec->m_logic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            rec->m_logic->PrepareSave();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            rec->m_logic->AfterLoadReferences();
            break;
        case ACT_AFTER_LOAD:
            rec->m_logic->AfterLoad();
            break;
        case ACT_AFTER_SAVE:
            rec->m_logic->AfterSave();
            break;
        case ACT_LIVE:
            break;
        default:
            Worker_DefaultPump(rec->m_logic);
            break;
    }
    return 1;
}

RVA(0x0003d8f0, 0xf1)
i32 CreateTeleporter(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_animWorker;
    switch (static_cast<u32>(rec->ActKey())) {
        case ACT_UNINITIALISED: {
            rec->SetActKey(ACT_LIVE);
            CUserLogic* sub = new CTeleporter(owner);
            sub->Activate();
            rec->m_logic = sub;
            break;
        }
        case ACT_OBJECT_REMOVED:
            rec->m_logic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            rec->m_logic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            rec->m_logic->PrepareSave();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            rec->m_logic->AfterLoadReferences();
            break;
        case ACT_AFTER_LOAD:
            rec->m_logic->AfterLoad();
            break;
        case ACT_AFTER_SAVE:
            rec->m_logic->AfterSave();
            break;
        case ACT_LIVE:
            break;
        default:
            Worker_DefaultPump(rec->m_logic);
            break;
    }
    return 1;
}

RVA(0x0003da30, 0xf1)
i32 CreateSecretTeleporterTrigger(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_animWorker;
    switch (static_cast<u32>(rec->ActKey())) {
        case ACT_UNINITIALISED: {
            rec->SetActKey(ACT_LIVE);
            CUserLogic* sub = new CSecretTeleporterTrigger(owner);
            sub->Activate();
            rec->m_logic = sub;
            break;
        }
        case ACT_OBJECT_REMOVED:
            rec->m_logic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            rec->m_logic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            rec->m_logic->PrepareSave();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            rec->m_logic->AfterLoadReferences();
            break;
        case ACT_AFTER_LOAD:
            rec->m_logic->AfterLoad();
            break;
        case ACT_AFTER_SAVE:
            rec->m_logic->AfterSave();
            break;
        case ACT_LIVE:
            break;
        default:
            Worker_DefaultPump(rec->m_logic);
            break;
    }
    return 1;
}

RVA(0x0003db70, 0xf4)
i32 CreateWarlord(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_animWorker;
    switch (static_cast<u32>(rec->ActKey())) {
        case ACT_UNINITIALISED: {
            rec->SetActKey(ACT_LIVE);
            CUserLogic* sub = new CWarlord(owner);
            sub->Activate();
            rec->m_logic = sub;
            break;
        }
        case ACT_OBJECT_REMOVED:
            rec->m_logic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            rec->m_logic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            rec->m_logic->PrepareSave();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            rec->m_logic->AfterLoadReferences();
            break;
        case ACT_AFTER_LOAD:
            rec->m_logic->AfterLoad();
            break;
        case ACT_AFTER_SAVE:
            rec->m_logic->AfterSave();
            break;
        case ACT_LIVE:
            break;
        default:
            Worker_DefaultPump(rec->m_logic);
            break;
    }
    return 1;
}

RVA(0x0003dcb0, 0xf1)
i32 CreateFortressFlag(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_animWorker;
    switch (static_cast<u32>(rec->ActKey())) {
        case ACT_UNINITIALISED: {
            rec->SetActKey(ACT_LIVE);
            CUserLogic* sub = new CFortressFlag(owner);
            sub->Activate();
            rec->m_logic = sub;
            break;
        }
        case ACT_OBJECT_REMOVED:
            rec->m_logic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            rec->m_logic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            rec->m_logic->PrepareSave();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            rec->m_logic->AfterLoadReferences();
            break;
        case ACT_AFTER_LOAD:
            rec->m_logic->AfterLoad();
            break;
        case ACT_AFTER_SAVE:
            rec->m_logic->AfterSave();
            break;
        case ACT_LIVE:
            break;
        default:
            Worker_DefaultPump(rec->m_logic);
            break;
    }
    return 1;
}

RVA(0x0003ddf0, 0xf1)
i32 CreateSecretLevelTrigger(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_animWorker;
    switch (static_cast<u32>(rec->ActKey())) {
        case ACT_UNINITIALISED: {
            rec->SetActKey(ACT_LIVE);
            CUserLogic* sub = new CSecretLevelTrigger(owner);
            sub->Activate();
            rec->m_logic = sub;
            break;
        }
        case ACT_OBJECT_REMOVED:
            rec->m_logic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            rec->m_logic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            rec->m_logic->PrepareSave();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            rec->m_logic->AfterLoadReferences();
            break;
        case ACT_AFTER_LOAD:
            rec->m_logic->AfterLoad();
            break;
        case ACT_AFTER_SAVE:
            rec->m_logic->AfterSave();
            break;
        case ACT_LIVE:
            break;
        default:
            Worker_DefaultPump(rec->m_logic);
            break;
    }
    return 1;
}
