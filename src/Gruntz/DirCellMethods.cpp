#include <rva.h>

#include <Bute/ButeMgr.h>
#include <Bute/SymTab.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DinMgr2/DirectInputMgr2.h>
#include <Gruntz/AnimWorker.h>
#include <Gruntz/Demo.h>
#include <Gruntz/DemoHelpers.h>
#include <Gruntz/DemoMoverState.h>
#include <Gruntz/ExitTrigger.h>
#include <Gruntz/FixedPtrArray32.h>
#include <Gruntz/FortressFlag.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntCreationPoint.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntStartingPoint.h>
#include <Gruntz/GruntzCommandId.h>
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

DATA(0x0023f790)
char g_dwRectsEditBuf[0x4000];
DATA(0x00243790)
i32 g_dwRectsEditLen;

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
            direction = static_cast<GruntDirection>(e[2]);
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
            direction = static_cast<GruntDirection>(e[2]);
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
    AnimWorkerAct act = rec->WorkerAct();
    switch (act) {
        case ACT_UNINITIALISED: {
            rec->SetWorkerAct(ACT_LIVE);
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
    AnimWorkerAct act = rec->WorkerAct();
    switch (act) {
        case ACT_UNINITIALISED: {
            rec->SetWorkerAct(ACT_LIVE);
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
    AnimWorkerAct act = rec->WorkerAct();
    switch (act) {
        case ACT_UNINITIALISED: {
            rec->SetWorkerAct(ACT_LIVE);
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
    AnimWorkerAct act = rec->WorkerAct();
    switch (act) {
        case ACT_UNINITIALISED: {
            rec->SetWorkerAct(ACT_LIVE);
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
    AnimWorkerAct act = rec->WorkerAct();
    switch (act) {
        case ACT_UNINITIALISED: {
            rec->SetWorkerAct(ACT_LIVE);
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
    AnimWorkerAct act = rec->WorkerAct();
    switch (act) {
        case ACT_UNINITIALISED: {
            rec->SetWorkerAct(ACT_LIVE);
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
    AnimWorkerAct act = rec->WorkerAct();
    switch (act) {
        case ACT_UNINITIALISED: {
            rec->SetWorkerAct(ACT_LIVE);
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
    AnimWorkerAct act = rec->WorkerAct();
    switch (act) {
        case ACT_UNINITIALISED: {
            rec->SetWorkerAct(ACT_LIVE);
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
    AnimWorkerAct act = rec->WorkerAct();
    switch (act) {
        case ACT_UNINITIALISED: {
            rec->SetWorkerAct(ACT_LIVE);
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
    AnimWorkerAct act = rec->WorkerAct();
    switch (act) {
        case ACT_UNINITIALISED: {
            rec->SetWorkerAct(ACT_LIVE);
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
