#include <StdAfx.h>

#include <Ints.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DinMgr2/DirectInputMgr2.h>
#include <Gruntz/Demo.h>
#include <Gruntz/DemoHelpers.h>
#include <Gruntz/DemoMoverState.h>
#include <Gruntz/ExitTrigger.h>
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
#include <Gruntz/InputDeviceGroup.h>
#include <Gruntz/LogicEventDispatch.h>
#include <Gruntz/LogicRecordHandler.h>
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

i32 g_buteEditLen;

char g_buteEditBuf[0x10000];

char g_dwRectsEditBuf[0x4000];

i32 g_dwRectsEditLen;

bool SameCellTag(const GruntDirectionCell* a, const GruntDirectionCell* b) {
    return a->m_direction == b->m_direction;
}

bool DifferentCellTag(const GruntDirectionCell* a, const GruntDirectionCell* b) {
    return a->m_direction != b->m_direction;
}

void GruntDirectionCell::RotateClockwise(i32 steps) {
    if (steps > 0) {
        do {
            CTriRecord next = g_directionClockwiseTable[m_row * 3 + m_column];
            m_row = next.m_row;
            m_column = next.m_column;
            m_direction = next.m_direction;
        } while (--steps);
    }
}

void GruntDirectionCell::RotateCounterclockwise(i32 steps) {
    if (steps > 0) {
        do {
            CTriRecord next = g_directionCounterclockwiseTable[m_row * 3 + m_column];
            m_row = next.m_row;
            m_column = next.m_column;
            m_direction = next.m_direction;
        } while (--steps);
    }
}

i32 CTriRecord::Serialize(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    switch (mode) {
        case SERIAL_SAVE:
            ar->Write(&m_row, sizeof(m_row));
            ar->Write(&m_column, sizeof(m_column));
            ar->Write(&m_direction, sizeof(m_direction));
            break;
        case SERIAL_LOAD:
            ar->Read(&m_row, sizeof(m_row));
            ar->Read(&m_column, sizeof(m_column));
            ar->Read(&m_direction, sizeof(m_direction));
            break;
    }
    return 1;
}

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

bool CButeMgr::Parse(CString filename, int streamBase) {

    ifstream* s = new ifstream(filename, ios::in | ios::nocreate);
    m_pData = s;
    if (s->fail()) {
        return false;
    }

    Reset();
    m_decryptCode = streamBase;
    m_sAttributeFilename = filename;

    m_tagTab.clear();
    m_auxTagTab.clear();
    m_newTagTab.clear();

    bool result = true;
    if (!TagList()) {
        m_bErrorFlag = 1;
        result = false;
    }

    (static_cast<ifstream*>(m_pData))->close();
    delete static_cast<ifstream*>(m_pData);
    return result;
}

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

i32 DispatchGruntStartingPointLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CGruntStartingPoint)
}

i32 DispatchExitTriggerLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CExitTrigger)
}

i32 DispatchGruntCreationPointLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CGruntCreationPoint)
}

i32 DispatchWormholeLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CWormhole)
}

i32 DispatchGruntPuddleLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CGruntPuddle)
}

i32 DispatchTeleporterLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CTeleporter)
}

i32 DispatchSecretTeleporterTriggerLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CSecretTeleporterTrigger)
}

i32 DispatchWarlordLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CWarlord)
}

i32 DispatchFortressFlagLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CFortressFlag)
}

i32 DispatchSecretLevelTriggerLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CSecretLevelTrigger)
}
