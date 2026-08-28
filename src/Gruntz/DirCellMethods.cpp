#include <rva.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DinMgr2/DirectInputMgr2.h>
#include <Gruntz/Demo.h>
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

DATA(0x0020d008)
CTriRecord g_directionClockwiseTable[9] = {
    {0, 1, DIR_NORTH},
    {0, 2, DIR_NORTHEAST},
    {1, 2, DIR_EAST},
    {0, 0, DIR_NORTHWEST},
    {1, 1, DIR_CENTER},
    {2, 2, DIR_SOUTHEAST},
    {1, 0, DIR_WEST},
    {2, 0, DIR_SOUTHWEST},
    {2, 1, DIR_SOUTH},
};
DATA(0x0020d078)
CTriRecord g_directionCounterclockwiseTable[9] = {
    {1, 0, DIR_WEST},
    {0, 0, DIR_NORTHWEST},
    {0, 1, DIR_NORTH},
    {2, 0, DIR_SOUTHWEST},
    {1, 1, DIR_CENTER},
    {0, 2, DIR_NORTHEAST},
    {2, 1, DIR_SOUTH},
    {2, 2, DIR_SOUTHEAST},
    {1, 2, DIR_EAST},
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0003c820, 0x18)
bool DifferentCellTag(const GruntDirectionCell* a, const GruntDirectionCell* b) {
    return a->direction != b->direction;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0003c850, 0x38)
void GruntDirectionCell::RotateClockwise(i32 steps) {
    if (steps > 0) {
        do {
            CTriRecord next = g_directionClockwiseTable[row * 3 + column];
            row = next.row;
            column = next.column;
            direction = next.direction;
        } while (--steps);
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0003c8a0, 0x38)
void GruntDirectionCell::RotateCounterclockwise(i32 steps) {
    if (steps > 0) {
        do {
            CTriRecord next = g_directionCounterclockwiseTable[row * 3 + column];
            row = next.row;
            column = next.column;
            direction = next.direction;
        } while (--steps);
    }
}

RVA(0x0003c8f0, 0x76)
i32 CTriRecord::Serialize(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    switch (mode) {
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0003c990, 0x1bc)
BOOL CALLBACK ButeAttributezDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
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
            return true;
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
                    return true;
                }
                case IDCANCEL:
                    EndDialog(hDlg, 0);
                    return true;
            }
            break;
    }
    return false;
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

    Reset();
    m_streamBase = streamBase;
    m_filename = filename;

    m_tags.Reset();
    m_modifiedTags.Reset();
    m_addedTags.Reset();

    bool result = true;
    if (!ParseGroup()) {
        m_parseFailed = 1;
        result = false;
    }

    (static_cast<ifstream*>(m_stream))->close();
    delete static_cast<ifstream*>(m_stream);
    return result;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0003cdd0, 0x19f)
BOOL CALLBACK EditDwRectsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
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
            return true;
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
                    return true;
                }
                case IDCANCEL:
                    EndDialog(hDlg, 0);
                    return true;
            }
            break;
    }
    return false;
}

RVA(0x0003d2b0, 0xf1)
i32 DispatchGruntStartingPointLogic(CGameObject* owner) {
    CLogicRecord* record = owner->m_logicRecord;
    switch (record->LogicEvent()) {
        case ACT_UNINITIALISED: {
            record->SetLogicEvent(ACT_LIVE);
            CUserLogic* sub = new CGruntStartingPoint(owner);
            sub->Activate();
            record->m_userLogic = sub;
            break;
        }
        case ACT_OBJECT_REMOVED:
            record->m_userLogic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            record->m_userLogic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            record->m_userLogic->PrepareSave();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            record->m_userLogic->AfterLoadReferences();
            break;
        case ACT_AFTER_LOAD:
            record->m_userLogic->AfterLoad();
            break;
        case ACT_AFTER_SAVE:
            record->m_userLogic->AfterSave();
            break;
        case ACT_LIVE:
            break;
        default:
            DispatchUnhandledLogicEvent(record->m_userLogic);
            break;
    }
    return 1;
}

RVA(0x0003d3f0, 0xf1)
i32 DispatchExitTriggerLogic(CGameObject* owner) {
    CLogicRecord* record = owner->m_logicRecord;
    switch (record->LogicEvent()) {
        case ACT_UNINITIALISED: {
            record->SetLogicEvent(ACT_LIVE);
            CUserLogic* sub = new CExitTrigger(owner);
            sub->Activate();
            record->m_userLogic = sub;
            break;
        }
        case ACT_OBJECT_REMOVED:
            record->m_userLogic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            record->m_userLogic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            record->m_userLogic->PrepareSave();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            record->m_userLogic->AfterLoadReferences();
            break;
        case ACT_AFTER_LOAD:
            record->m_userLogic->AfterLoad();
            break;
        case ACT_AFTER_SAVE:
            record->m_userLogic->AfterSave();
            break;
        case ACT_LIVE:
            break;
        default:
            DispatchUnhandledLogicEvent(record->m_userLogic);
            break;
    }
    return 1;
}

RVA(0x0003d530, 0xf1)
i32 DispatchGruntCreationPointLogic(CGameObject* owner) {
    CLogicRecord* record = owner->m_logicRecord;
    switch (record->LogicEvent()) {
        case ACT_UNINITIALISED: {
            record->SetLogicEvent(ACT_LIVE);
            CUserLogic* sub = new CGruntCreationPoint(owner);
            sub->Activate();
            record->m_userLogic = sub;
            break;
        }
        case ACT_OBJECT_REMOVED:
            record->m_userLogic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            record->m_userLogic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            record->m_userLogic->PrepareSave();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            record->m_userLogic->AfterLoadReferences();
            break;
        case ACT_AFTER_LOAD:
            record->m_userLogic->AfterLoad();
            break;
        case ACT_AFTER_SAVE:
            record->m_userLogic->AfterSave();
            break;
        case ACT_LIVE:
            break;
        default:
            DispatchUnhandledLogicEvent(record->m_userLogic);
            break;
    }
    return 1;
}

RVA(0x0003d670, 0xf1)
i32 DispatchWormholeLogic(CGameObject* owner) {
    CLogicRecord* record = owner->m_logicRecord;
    switch (record->LogicEvent()) {
        case ACT_UNINITIALISED: {
            record->SetLogicEvent(ACT_LIVE);
            CUserLogic* sub = new CWormhole(owner);
            sub->Activate();
            record->m_userLogic = sub;
            break;
        }
        case ACT_OBJECT_REMOVED:
            record->m_userLogic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            record->m_userLogic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            record->m_userLogic->PrepareSave();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            record->m_userLogic->AfterLoadReferences();
            break;
        case ACT_AFTER_LOAD:
            record->m_userLogic->AfterLoad();
            break;
        case ACT_AFTER_SAVE:
            record->m_userLogic->AfterSave();
            break;
        case ACT_LIVE:
            break;
        default:
            DispatchUnhandledLogicEvent(record->m_userLogic);
            break;
    }
    return 1;
}

RVA(0x0003d7b0, 0xf1)
i32 DispatchGruntPuddleLogic(CGameObject* owner) {
    CLogicRecord* record = owner->m_logicRecord;
    switch (record->LogicEvent()) {
        case ACT_UNINITIALISED: {
            record->SetLogicEvent(ACT_LIVE);
            CUserLogic* sub = new CGruntPuddle(owner);
            sub->Activate();
            record->m_userLogic = sub;
            break;
        }
        case ACT_OBJECT_REMOVED:
            record->m_userLogic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            record->m_userLogic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            record->m_userLogic->PrepareSave();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            record->m_userLogic->AfterLoadReferences();
            break;
        case ACT_AFTER_LOAD:
            record->m_userLogic->AfterLoad();
            break;
        case ACT_AFTER_SAVE:
            record->m_userLogic->AfterSave();
            break;
        case ACT_LIVE:
            break;
        default:
            DispatchUnhandledLogicEvent(record->m_userLogic);
            break;
    }
    return 1;
}

RVA(0x0003d8f0, 0xf1)
i32 DispatchTeleporterLogic(CGameObject* owner) {
    CLogicRecord* record = owner->m_logicRecord;
    switch (record->LogicEvent()) {
        case ACT_UNINITIALISED: {
            record->SetLogicEvent(ACT_LIVE);
            CUserLogic* sub = new CTeleporter(owner);
            sub->Activate();
            record->m_userLogic = sub;
            break;
        }
        case ACT_OBJECT_REMOVED:
            record->m_userLogic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            record->m_userLogic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            record->m_userLogic->PrepareSave();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            record->m_userLogic->AfterLoadReferences();
            break;
        case ACT_AFTER_LOAD:
            record->m_userLogic->AfterLoad();
            break;
        case ACT_AFTER_SAVE:
            record->m_userLogic->AfterSave();
            break;
        case ACT_LIVE:
            break;
        default:
            DispatchUnhandledLogicEvent(record->m_userLogic);
            break;
    }
    return 1;
}

RVA(0x0003da30, 0xf1)
i32 DispatchSecretTeleporterTriggerLogic(CGameObject* owner) {
    CLogicRecord* record = owner->m_logicRecord;
    switch (record->LogicEvent()) {
        case ACT_UNINITIALISED: {
            record->SetLogicEvent(ACT_LIVE);
            CUserLogic* sub = new CSecretTeleporterTrigger(owner);
            sub->Activate();
            record->m_userLogic = sub;
            break;
        }
        case ACT_OBJECT_REMOVED:
            record->m_userLogic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            record->m_userLogic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            record->m_userLogic->PrepareSave();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            record->m_userLogic->AfterLoadReferences();
            break;
        case ACT_AFTER_LOAD:
            record->m_userLogic->AfterLoad();
            break;
        case ACT_AFTER_SAVE:
            record->m_userLogic->AfterSave();
            break;
        case ACT_LIVE:
            break;
        default:
            DispatchUnhandledLogicEvent(record->m_userLogic);
            break;
    }
    return 1;
}

RVA(0x0003db70, 0xf4)
i32 DispatchWarlordLogic(CGameObject* owner) {
    CLogicRecord* record = owner->m_logicRecord;
    switch (record->LogicEvent()) {
        case ACT_UNINITIALISED: {
            record->SetLogicEvent(ACT_LIVE);
            CUserLogic* sub = new CWarlord(owner);
            sub->Activate();
            record->m_userLogic = sub;
            break;
        }
        case ACT_OBJECT_REMOVED:
            record->m_userLogic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            record->m_userLogic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            record->m_userLogic->PrepareSave();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            record->m_userLogic->AfterLoadReferences();
            break;
        case ACT_AFTER_LOAD:
            record->m_userLogic->AfterLoad();
            break;
        case ACT_AFTER_SAVE:
            record->m_userLogic->AfterSave();
            break;
        case ACT_LIVE:
            break;
        default:
            DispatchUnhandledLogicEvent(record->m_userLogic);
            break;
    }
    return 1;
}

RVA(0x0003dcb0, 0xf1)
i32 DispatchFortressFlagLogic(CGameObject* owner) {
    CLogicRecord* record = owner->m_logicRecord;
    switch (record->LogicEvent()) {
        case ACT_UNINITIALISED: {
            record->SetLogicEvent(ACT_LIVE);
            CUserLogic* sub = new CFortressFlag(owner);
            sub->Activate();
            record->m_userLogic = sub;
            break;
        }
        case ACT_OBJECT_REMOVED:
            record->m_userLogic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            record->m_userLogic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            record->m_userLogic->PrepareSave();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            record->m_userLogic->AfterLoadReferences();
            break;
        case ACT_AFTER_LOAD:
            record->m_userLogic->AfterLoad();
            break;
        case ACT_AFTER_SAVE:
            record->m_userLogic->AfterSave();
            break;
        case ACT_LIVE:
            break;
        default:
            DispatchUnhandledLogicEvent(record->m_userLogic);
            break;
    }
    return 1;
}

RVA(0x0003ddf0, 0xf1)
i32 DispatchSecretLevelTriggerLogic(CGameObject* owner) {
    CLogicRecord* record = owner->m_logicRecord;
    switch (record->LogicEvent()) {
        case ACT_UNINITIALISED: {
            record->SetLogicEvent(ACT_LIVE);
            CUserLogic* sub = new CSecretLevelTrigger(owner);
            sub->Activate();
            record->m_userLogic = sub;
            break;
        }
        case ACT_OBJECT_REMOVED:
            record->m_userLogic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            record->m_userLogic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            record->m_userLogic->PrepareSave();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            record->m_userLogic->AfterLoadReferences();
            break;
        case ACT_AFTER_LOAD:
            record->m_userLogic->AfterLoad();
            break;
        case ACT_AFTER_SAVE:
            record->m_userLogic->AfterSave();
            break;
        case ACT_LIVE:
            break;
        default:
            DispatchUnhandledLogicEvent(record->m_userLogic);
            break;
    }
    return 1;
}
