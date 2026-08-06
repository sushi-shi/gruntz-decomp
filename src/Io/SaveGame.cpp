#include <rva.h>

#include <Io/SaveGame.h>

#include <MfcWin.h>

#include <DDrawMgr/ColorDepth.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <Enums.h>
#include <Gruntz/ChainForward.h>
#include <Gruntz/CheatMgr.h>
#include <Gruntz/FontConfig.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Play.h>
#include <Gruntz/QuestLevel.h>
#include <Gruntz/SaveSlotCtrlId.h>
#include <Image/Image.h>
#include <Image/ImagePool.h>
#include <Image/RezDecodeKind.h>
#include <Io/GameSave.h>
#include <Utils/RegistryHelper.h>
#include <Wap32/ScreenGeometry.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* g_areaNames[8];
DATA(0x00213a9c)
i32 g_savedMenuCmd = -1;
DATA(0x0024c814)
CImagePool* g_previewMgr;
DATA(0x0024c864)
SaveSlot* g_slotState;
DATA(0x0024c868)
void* g_previewImage;
DATA(0x0024c86c)
CSaveGame* g_saveDlgSink = 0;

static const i32 SAVE_FILE_HEADER_BYTES = 0xa1c;
static const i32 SAVE_PREVIEW_BYTES = 0x3843a;
static const i32 SAVE_PREVIEW_BITMAP_OFFSET = 0xe;
static const u32 SAVE_PROGRESS_MAGIC = 0x42a;

RVA(0x00085b50, 0x56)
CSaveGame::~CSaveGame() {
    Reset();
}

RVA(0x000e35f0, 0x77)
i32 CALLBACK SaveGameDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_COMMAND:
            if (wParam == IDCANCEL) {
                EndDialog(hDlg, 0);
                return 1;
            }
            if (DrawSaveGameMenu(hDlg, wParam, g_saveDlgSink) != 0) {
                return 1;
            }

        default:
            return 0;
        case WM_INITDIALOG: {
            CSaveGame* v = g_gameReg->m_saveSink;
            g_savedMenuCmd = -1;
            g_saveDlgSink = v;
            FillSaveDialog(hDlg, v);
            return 1;
        }
    }
}

// @early-stop
RVA(0x000e3690, 0x2ec)
i32 CALLBACK LevelPreviewDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            HWND item = GetDlgItem(hDlg, CTRL_SAVESLOT_PREVIEW_IMAGE);
            if (g_previewMgr == NULL || g_previewImage == NULL || item == NULL) {
                return 1;
            }
            RECT wr;
            GetWindowRect(item, &wr);
            POINT pt;
            pt.x = wr.left;
            pt.y = wr.top;
            ScreenToClient(hDlg, &pt);
            i32 w = wr.right - wr.left - 1;
            i32 h = wr.bottom - wr.top - 1;
            i32 dx = pt.x;
            i32 dy = pt.y;
            if (w >= SCREEN_HALF_W_PX) {
                dx += (w - SCREEN_HALF_W_PX) / 2;
                w = SCREEN_HALF_W_PX;
            }
            if (h >= SCREEN_HALF_H_PX) {
                dy += (h - SCREEN_HALF_H_PX) / 2;
                h = SCREEN_HALF_H_PX;
            }
            PAINTSTRUCT ps;
            BeginPaint(hDlg, &ps);
            SetStretchBltMode(ps.hdc, COLORONCOLOR);
            CRezImage* img = static_cast<CRezImage*>(g_previewImage);
            if (img->m_bitCount == BPP_PALETTED_8) {
                StretchDIBits(
                    ps.hdc,
                    dx,
                    dy,
                    w,
                    h,
                    0,
                    0,
                    img->m_width,
                    img->m_height,
                    img->m_pixels,
                    &img->m_bmi,
                    DIB_PAL_COLORS,
                    SRCCOPY
                );
            } else {
                StretchDIBits(
                    ps.hdc,
                    dx,
                    dy,
                    w,
                    h,
                    0,
                    0,
                    img->m_width,
                    img->m_height,
                    img->m_pixels,
                    &img->m_bmi,
                    DIB_RGB_COLORS,
                    SRCCOPY
                );
            }
            EndPaint(hDlg, &ps);
            return 1;
        }
        case WM_INITDIALOG: {
            if (g_slotState == NULL) {
                EndDialog(hDlg, 0);
                return 1;
            }
            g_previewMgr = new CImagePool;

            if (g_previewMgr->SetHandles(g_gameReg->m_owner->m_hInstance, hDlg, 0) == 0) {
                break;
            }
            BuildLevelTitleString(hDlg, g_gameReg->m_saveSink, g_slotState);
            return 1;
        }
        case WM_COMMAND: {
            if (wParam != IDCANCEL && wParam != IDOK) {
                return 0;
            }
            if (g_previewMgr != NULL) {
                if (g_previewImage != NULL) {
                    g_previewMgr->Free(static_cast<CRezImage*>(g_previewImage));
                }
                delete g_previewMgr;
                g_previewMgr = NULL;
            }
            EndDialog(hDlg, 0);
            return 1;
        }
    }
    return 0;
}

RVA(0x000e3a40, 0xb0)
i32 CALLBACK DeleteSaveDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG:
            if (g_slotState == NULL) {
                // API-forced INT_PTR boundary.

                EndDialog(hDlg, reinterpret_cast<INT_PTR>(g_slotState));
                return 1;
            }
            SetSaveSlotDialogName(hDlg, g_gameReg->m_saveSink, g_slotState);
            return 1;
        case WM_COMMAND:
            if (wParam == IDCANCEL) {
                EndDialog(hDlg, 0);
                return 1;
            }
            if (wParam == IDOK) {
                (static_cast<CSaveGame*>(g_gameReg->m_saveSink))->CloseTempFile(g_slotState);
                (static_cast<CSaveGame*>(g_gameReg->m_saveSink))->Save(0, SAVE_STRING_SAVING_GAME);
                EndDialog(hDlg, 1);
                return 1;
            }
            break;
    }
    return 0;
}

RVA(0x000e3b20, 0x86)
i32 CALLBACK InfoLineDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG:
            if (g_slotState == NULL) {
                // API-forced INT_PTR boundary.

                EndDialog(hDlg, reinterpret_cast<INT_PTR>(g_slotState));
                return 1;
            }
            SetSaveSlotDialogName(hDlg, g_gameReg->m_saveSink, g_slotState);
            return 1;
        case WM_COMMAND:
            if (wParam == IDCANCEL) {
                EndDialog(hDlg, 0);
                return 1;
            }
            if (wParam == IDOK) {
                EndDialog(hDlg, wParam);
                return 1;
            }
            break;
    }
    return 0;
}

RVA(0x000e3be0, 0x52)
i32 CALLBACK OkCancelDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG:
            return 1;
        case WM_COMMAND:
            if (wParam == IDCANCEL) {
                EndDialog(hDlg, 0);
                return 1;
            }
            if (wParam == IDOK) {
                EndDialog(hDlg, 1);
                return 1;
            }
            break;
    }
    return 0;
}

RVA(0x000e3c60, 0x1a3)
void FillSaveDialog(HWND hWnd, CSaveGame* sg) {
    if (hWnd == NULL || sg == NULL) {
        return;
    }
    LabelSaveSlot(
        hWnd,
        sg->GetSlot(0),
        CTRL_SAVEDLG_SLOT0,
        CTRL_SAVESLOT_LOAD0,
        CTRL_SAVESLOT_INFO0,
        CTRL_SAVESLOT_DELETE0
    );
    LabelSaveSlot(
        hWnd,
        sg->GetSlot(1),
        CTRL_SAVEDLG_SLOT1,
        CTRL_SAVESLOT_LOAD1,
        CTRL_SAVESLOT_INFO1,
        CTRL_SAVESLOT_DELETE1
    );
    LabelSaveSlot(
        hWnd,
        sg->GetSlot(2),
        CTRL_SAVEDLG_SLOT2,
        CTRL_SAVESLOT_LOAD2,
        CTRL_SAVESLOT_INFO2,
        CTRL_SAVESLOT_DELETE2
    );
    LabelSaveSlot(
        hWnd,
        sg->GetSlot(3),
        CTRL_SAVEDLG_SLOT3,
        CTRL_SAVESLOT_LOAD3,
        CTRL_SAVESLOT_INFO3,
        CTRL_SAVESLOT_DELETE3
    );
    LabelSaveSlot(
        hWnd,
        sg->GetSlot(4),
        CTRL_SAVEDLG_SLOT4,
        CTRL_SAVESLOT_LOAD4,
        CTRL_SAVESLOT_INFO4,
        CTRL_SAVESLOT_DELETE4
    );
    LabelSaveSlot(
        hWnd,
        sg->GetSlot(5),
        CTRL_SAVEDLG_SLOT5,
        CTRL_SAVESLOT_LOAD5,
        CTRL_SAVESLOT_INFO5,
        CTRL_SAVESLOT_DELETE5
    );
    LabelSaveSlot(
        hWnd,
        sg->GetSlot(6),
        CTRL_SAVEDLG_SLOT6,
        CTRL_SAVESLOT_LOAD6,
        CTRL_SAVESLOT_INFO6,
        CTRL_SAVESLOT_DELETE6
    );
    LabelSaveSlot(
        hWnd,
        sg->GetSlot(7),
        CTRL_SAVEDLG_SLOT7,
        CTRL_SAVESLOT_LOAD7,
        CTRL_SAVESLOT_INFO7,
        CTRL_SAVESLOT_DELETE7
    );
    LabelSaveSlot(
        hWnd,
        sg->GetSlot(8),
        CTRL_SAVEDLG_SLOT8,
        CTRL_SAVESLOT_LOAD8,
        CTRL_SAVESLOT_INFO8,
        CTRL_SAVESLOT_DELETE8
    );
    LabelSaveSlot(
        hWnd,
        sg->GetSlot(9),
        CTRL_SAVEDLG_SLOT9,
        CTRL_SAVESLOT_LOAD9,
        CTRL_SAVESLOT_INFO9,
        CTRL_SAVESLOT_DELETE9
    );
}

// @early-stop
RVA(0x000e3e80, 0x86)
void LabelSaveSlot(HWND hWnd, SaveSlot* item, i32 id3, i32 id4, i32 id5, i32 id6) {
    i32 flag;
    if (TempFileExists(item)) {
        SetDlgItemTextA(hWnd, id3, item->m_name);
        flag = 1;
    } else {
        SetDlgItemTextA(hWnd, id3, "(Empty)");
        flag = 0;
    }
    EnableWindow(GetDlgItem(hWnd, id3), 1);
    EnableWindow(GetDlgItem(hWnd, id4), 1);
    EnableWindow(GetDlgItem(hWnd, id5), flag);
    EnableWindow(GetDlgItem(hWnd, id6), flag);
}
RVA(0x000e3f40, 0x478)
i32 DrawSaveGameMenu(HWND hDlg, i32 cmd, CSaveGame* obj) {
    i32 c;
    if (cmd == IDOK) {
        c = g_savedMenuCmd;
        if (c == -1) {
            return 0;
        }
    } else {
        c = cmd;
    }

    if (HIWORD(c) == EN_SETFOCUS) {
        switch (LOWORD(c)) {
            case CTRL_SAVEDLG_SLOT0:
                g_savedMenuCmd = CTRL_SAVESLOT_LOAD0;
                break;
            case CTRL_SAVEDLG_SLOT1:
                g_savedMenuCmd = CTRL_SAVESLOT_LOAD1;
                break;
            case CTRL_SAVEDLG_SLOT2:
                g_savedMenuCmd = CTRL_SAVESLOT_LOAD2;
                break;
            case CTRL_SAVEDLG_SLOT3:
                g_savedMenuCmd = CTRL_SAVESLOT_LOAD3;
                break;
            case CTRL_SAVEDLG_SLOT4:
                g_savedMenuCmd = CTRL_SAVESLOT_LOAD4;
                break;
            case CTRL_SAVEDLG_SLOT5:
                g_savedMenuCmd = CTRL_SAVESLOT_LOAD5;
                break;
            case CTRL_SAVEDLG_SLOT6:
                g_savedMenuCmd = CTRL_SAVESLOT_LOAD6;
                break;
            case CTRL_SAVEDLG_SLOT7:
                g_savedMenuCmd = CTRL_SAVESLOT_LOAD7;
                break;
            case CTRL_SAVEDLG_SLOT8:
                g_savedMenuCmd = CTRL_SAVESLOT_LOAD8;
                break;
            case CTRL_SAVEDLG_SLOT9:
                g_savedMenuCmd = CTRL_SAVESLOT_LOAD9;
                break;
        }
    }

    i32 info = -1;
    switch (c) {
        case CTRL_SAVESLOT_INFO0:
            info = 0;
            break;
        case CTRL_SAVESLOT_INFO1:
            info = 1;
            break;
        case CTRL_SAVESLOT_INFO2:
            info = 2;
            break;
        case CTRL_SAVESLOT_INFO3:
            info = 3;
            break;
        case CTRL_SAVESLOT_INFO4:
            info = 4;
            break;
        case CTRL_SAVESLOT_INFO5:
            info = 5;
            break;
        case CTRL_SAVESLOT_INFO6:
            info = 6;
            break;
        case CTRL_SAVESLOT_INFO7:
            info = 7;
            break;
        case CTRL_SAVESLOT_INFO8:
            info = 8;
            break;
        case CTRL_SAVESLOT_INFO9:
            info = 9;
            break;
    }
    if (info != -1) {
        g_slotState = obj->GetSlot(info);
        if (g_slotState == NULL) {
            return 0;
        }
        EnableWindow(hDlg, FALSE);
        g_gameReg->RunModalDialog("GAME_INFO", LevelPreviewDlgProc, 0);
        EnableWindow(hDlg, TRUE);
        return 0;
    }

    i32 del = -1;
    switch (c) {
        case CTRL_SAVESLOT_DELETE0:
            del = 0;
            break;
        case CTRL_SAVESLOT_DELETE1:
            del = 1;
            break;
        case CTRL_SAVESLOT_DELETE2:
            del = 2;
            break;
        case CTRL_SAVESLOT_DELETE3:
            del = 3;
            break;
        case CTRL_SAVESLOT_DELETE4:
            del = 4;
            break;
        case CTRL_SAVESLOT_DELETE5:
            del = 5;
            break;
        case CTRL_SAVESLOT_DELETE6:
            del = 6;
            break;
        case CTRL_SAVESLOT_DELETE7:
            del = 7;
            break;
        case CTRL_SAVESLOT_DELETE8:
            del = 8;
            break;
        case CTRL_SAVESLOT_DELETE9:
            del = 9;
            break;
    }
    if (del != -1) {
        g_slotState = obj->GetSlot(del);
        if (g_slotState == NULL) {
            return 0;
        }
        EnableWindow(hDlg, FALSE);
        i32 ok = g_gameReg->RunModalDialog("GAME_DELETE", DeleteSaveDialogProc, 0);
        EnableWindow(hDlg, TRUE);
        if (ok == 0) {
            return 0;
        }
        FillSaveDialog(hDlg, obj);
        return 0;
    }

    i32 slot = -1;
    i32 nameId;
    switch (c) {
        case CTRL_SAVESLOT_LOAD0:
            slot = 0;
            nameId = CTRL_SAVEDLG_SLOT0;
            break;
        case CTRL_SAVESLOT_LOAD1:
            slot = 1;
            nameId = CTRL_SAVEDLG_SLOT1;
            break;
        case CTRL_SAVESLOT_LOAD2:
            slot = 2;
            nameId = CTRL_SAVEDLG_SLOT2;
            break;
        case CTRL_SAVESLOT_LOAD3:
            slot = 3;
            nameId = CTRL_SAVEDLG_SLOT3;
            break;
        case CTRL_SAVESLOT_LOAD4:
            slot = 4;
            nameId = CTRL_SAVEDLG_SLOT4;
            break;
        case CTRL_SAVESLOT_LOAD5:
            slot = 5;
            nameId = CTRL_SAVEDLG_SLOT5;
            break;
        case CTRL_SAVESLOT_LOAD6:
            slot = 6;
            nameId = CTRL_SAVEDLG_SLOT6;
            break;
        case CTRL_SAVESLOT_LOAD7:
            slot = 7;
            nameId = CTRL_SAVEDLG_SLOT7;
            break;
        case CTRL_SAVESLOT_LOAD8:
            slot = 8;
            nameId = CTRL_SAVEDLG_SLOT8;
            break;
        case CTRL_SAVESLOT_LOAD9:
            slot = 9;
            nameId = CTRL_SAVEDLG_SLOT9;
            break;
    }

    if (slot != -1) {
        char name[0x20];
        GetDlgItemTextA(hDlg, nameId, name, 0x20);
        if (_strcmpi(name, "(Empty)") == 0) {
            sprintf(name, "Saved Game #%i", slot + 1);
        }
        if (TempFileExists(obj->GetSlot(slot))) {
            g_slotState = obj->GetSlot(slot);
            if (g_slotState != NULL) {
                EnableWindow(hDlg, FALSE);
                i32 ok = g_gameReg->RunModalDialog("GAME_OVERWRITE", InfoLineDialogProc, 0);
                EnableWindow(hDlg, TRUE);
                if (ok == 0) {
                    return 1;
                }
            }
        }
        obj->InitializeNamedSlotAt(slot, name, g_gameReg);
        g_gameReg->FillSaveInfo(obj->GetSlot(slot), static_cast<void*>(name));
        EndDialog(hDlg, 1);
        if (!obj->Save(obj->GetSlot(slot)->m_savePath, SAVE_STRING_SAVING_GAME)) {
            g_gameReg->EnterModalUI("ERROR - Cannot Save Game.");
        }
        return 1;
    }
    return 0;
}

// @early-stop

RVA(0x000e44e0, 0x2b2)
void BuildLevelTitleString(HWND hDlg, CSaveGame* gate, SaveSlot* lev) {
    char title[0x80];
    char readBuf[SAVE_PREVIEW_BYTES];

    if (!hDlg) {
        return;
    }
    if (!gate) {
        return;
    }
    if (!lev) {
        return;
    }

    if (lev->m_isCustom == 0 && lev->m_isBattlez == 0) {

        i32 n = lev->m_levelId;
        wsprintfA(
            title,
            "Questz: Stage %d of %s",
            (n > IDX(QUESTLEVEL_LAST) && n < IDX(QUESTLEVEL_TRAINING_END))
                ? n - IDX(QUESTLEVEL_LAST)
                : (n - 1) % 4 + 1,
            (n > IDX(QUESTLEVEL_LAST) && n < IDX(QUESTLEVEL_TRAINING_END))
                ? static_cast<const char*>(CString("Training"))
                : g_areaNames[(n - 1) / 4]
        );
    } else if (lev->m_isBattlez != 0 && lev->m_isCustom == 0) {

        wsprintfA(title, "Battlez: %s", lev->m_levelName);
    } else {

        char* bs = strrchr(lev->m_levelName, '\\');
        if (bs != NULL) {
            if (lev->m_isBattlez) {
                wsprintfA(title, "Custom Battlez Level: ");
            } else {
                wsprintfA(title, "Custom Questz Level: ");
            }
            strcat(title, bs + 1);
            char* dot = strchr(title, '.');
            if (dot != NULL) {
                *dot = 0;
            }
        } else {
            if (lev->m_isBattlez) {
                wsprintfA(title, "Custom Battlez Level");
            } else {
                wsprintfA(title, "Custom Questz Level");
            }
        }
    }

    CFile f;
    if (f.Open(lev->m_savePath, CFile::typeBinary | CFile::modeRead, 0) == 0) {
        g_previewImage = NULL;
    } else {
        f.Seek(-SAVE_PREVIEW_BYTES, CFile::end);
        if (f.Read(readBuf, sizeof(readBuf)) != sizeof(readBuf)) {
            g_previewImage = NULL;
            f.Close();
        } else {
            f.Close();
            g_previewImage =
                g_previewMgr->AddSurfaceOp(&readBuf[SAVE_PREVIEW_BITMAP_OFFSET], DECODE_BMP, 0);
            SetDlgItemTextA(hDlg, CTRL_SAVESLOT_PREVIEW_TITLE, title);
        }
    }
}

RVA(0x000e4850, 0x29)
void SetSaveSlotDialogName(HWND hWnd, void* gate, SaveSlot* item) {
    if (hWnd && gate && item) {
        SetDlgItemTextA(hWnd, CTRL_SAVESLOT_NAME, item->m_name);
    }
}

RVA(0x000e4b60, 0x158)
i32 CSaveGame::SaveGameFile(const char* dir) {
    if (dir == NULL) {
        return 0;
    }
    m_str0 = dir;
    m_name = m_str0 + "Gruntz.sav";
    memset(m_header, 0, SAVE_FILE_HEADER_BYTES);
    Init();
    Load();
    for (i32 i = 0; i < SAVE_SLOT_COUNT; i++) {
        SaveSlot* slot = GetSlot(i);
        if (slot != NULL) {
            char numbuf[16];
            _itoa(i + 1, numbuf, 10);
            wsprintfA(slot->m_savePath, m_str0 + "Slot" + numbuf + ".sav");
        }
    }
    return 1;
}

RVA(0x000e4d20, 0x12)
void CSaveGame::Reset() {
    Init();
    m_name.Empty();
}

RVA(0x000e4d50, 0x2f)
void CSaveGame::Init() {
    m_maxLevel = QUESTLEVEL_TRAINING_FIRST;
    for (i32 i = 0; i < SAVE_SLOT_COUNT; i++) {
        SaveSlot* p = GetSlot(i);
        if (p != NULL) {
            memset(p, 0, sizeof(SaveSlot));
        }
    }
}

RVA(0x000e4d90, 0xcc)
i32 CSaveGame::Load() {
    CFile file;
    if (!file.Open(m_name, CFile::modeRead, 0)) {
        return 0;
    }
    file.Read(m_header, SAVE_FILE_HEADER_BYTES);
    file.Read(m_slots, sizeof(m_slots));
    file.Close();
    if (!Verify()) {
        Init();
    }
    return 1;
}

RVA(0x000e4ea0, 0x18c)
i32 CSaveGame::Save(char* path, i32 msgId) {
    CWaitCursor wait;
    CFile file;
    if (!file.Open(m_name, CFile::modeCreate, 0)) {
        return 0;
    }
    file.Close();
    if (!file.Open(m_name, CFile::modeWrite, 0)) {
        return 0;
    }
    ComputeAll();
    file.Write(m_header, SAVE_FILE_HEADER_BYTES);
    file.Write(m_slots, sizeof(m_slots));
    file.Close();
    Verify();
    if (path != NULL) {
        CPlay* state = static_cast<CPlay*>(g_gameReg->m_curState);
        g_gameReg->m_world->m_drawTarget->TransEnter();
        state->LoadSBITextEdges(msgId);
        if (!SaveGame(g_gameReg, path)) {
            return 0;
        }
        if (!ChainForward(
                g_gameReg->m_settings,
                g_gameReg,
                SCREEN_HALF_W_PX,
                SCREEN_HALF_H_PX,
                path,
                1
            )) {
            return 0;
        }
    }
    return 1;
}

RVA(0x000e50a0, 0x3e)
i32 CSaveGame::ComputeAll() {
    i32 sum = 0;
    for (i32 i = 0; i < SAVE_SLOT_COUNT; i++) {
        // Byte-forced checksum view.

        sum += Encode(reinterpret_cast<u8*>(GetSlot(i)));
    }
    m_header[0] = 0;
    m_header[1] = 1;
    m_header[2] = sum;
    m_header[3] = 0;
    return 1;
}

RVA(0x000e50f0, 0x2f)
i32 CSaveGame::Verify() {
    i32 sum = 0;
    for (i32 i = 0; i < SAVE_SLOT_COUNT; i++) {
        // Byte-forced checksum view.
        sum += Decode(reinterpret_cast<u8*>(GetSlot(i)));
    }
    return m_header[2] == sum;
}

RVA(0x000e5130, 0x78)
i32 CSaveGame::InitializeNamedSlot(SaveSlot* dst, const char* name, void* mgr) {
    if (dst == NULL) {
        return 0;
    }
    if (mgr == NULL) {
        return 0;
    }
    dst->m_type = SAVESLOT_PRESENT;
    CGruntzMgr* reg = static_cast<CGruntzMgr*>(mgr);
    dst->m_levelId = (static_cast<CPlay*>(reg->m_curState))->m_levelIndex;
    dst->m_count = 0;
    dst->m_active = 1;
    if (reg->m_cheatMgr->m_cheatsUsed != 0) {
        dst->m_type = SAVESLOT_PRESENT | SAVESLOT_CHEATS_USED;
    }
    strncpy(dst->m_name, name, sizeof(dst->m_name));
    dst->m_checksum = Register(dst);
    return 1;
}

RVA(0x000e51d0, 0x49)
i32 CSaveGame::CopySlot(SaveSlot* dst, const SaveSlot* src) {
    if (dst == NULL) {
        return 0;
    }
    if (src == NULL) {
        return 0;
    }
    dst->m_type = src->m_type;
    dst->m_levelId = src->m_levelId;
    dst->m_count = src->m_count;
    dst->m_active = src->m_active;
    dst->m_checksum = src->m_checksum;
    dst->m_checksum = Register(dst);
    return 1;
}

RVA(0x000e5240, 0x54)
i32 CSaveGame::InitializeLevelSlot(SaveSlot* dst, i32 levelId, void* mgr) {
    if (dst == NULL) {
        return 0;
    }
    if (mgr == NULL) {
        return 0;
    }
    dst->m_type = SAVESLOT_PRESENT;
    dst->m_levelId = levelId;
    dst->m_count = 0;
    if ((static_cast<CGruntzMgr*>(mgr))->m_cheatMgr->m_cheatsUsed != 0) {
        dst->m_type = SAVESLOT_PRESENT | SAVESLOT_CHEATS_USED;
    }
    dst->m_checksum = Register(dst);
    return 1;
}

RVA(0x000e52c0, 0x99)
i32 CSaveGame::VerifySlot(SaveSlot* slot) {
    if (slot == NULL) {
        return 0;
    }
    i32 fc = slot->m_pathHi;
    i32 f8 = slot->m_pathLo;
    const char* name = (fc == 0 && f8 == 0) ? g_emptyString : slot->m_levelName;
    i32 r = g_gameReg->BuildLevelRezPath(fc == 0, fc, f8, slot->m_levelId, CString(name));
    if (r == 0) {
        g_gameReg->EnterModalUI(
            "The level that this game was saved on does not exist!\n\nThis "
            "saved game cannot be loaded and should be deleted."
        );
        return 0;
    }
    if (slot->m_checksum != r) {
        g_gameReg->EnterModalUI(
            "The level that this game was saved on has changed!\n\nThis "
            "saved game cannot be loaded and should be deleted."
        );
        return 0;
    }
    return 1;
}

RVA(0x000e5390, 0x59)
i32 CSaveGame::Register(SaveSlot* slot) {
    if (slot == NULL) {
        return 0;
    }
    i32 fc = slot->m_pathHi;
    i32 f8 = slot->m_pathLo;
    const char* name = (fc == 0 && f8 == 0) ? g_emptyString : slot->m_levelName;

    return g_gameReg->BuildLevelRezPath(fc == 0, fc, f8, slot->m_levelId, CString(name));
}

RVA(0x000e5410, 0x3d)
i32 CSaveGame::Encode(u8* buf) {
    if (buf == NULL) {
        return 0;
    }
    i32 acc = 0;
    for (u32 i = 0; i < sizeof(SaveSlot); i++) {
        u8 t = buf[i];
        acc += static_cast<i32>((t & 0xff)) * static_cast<i32>(i);
        buf[i] = static_cast<u8>((t ^ i));
    }
    return acc;
}

RVA(0x000e5460, 0x3f)
i32 CSaveGame::Decode(u8* buf) {
    if (buf == NULL) {
        return 0;
    }
    i32 acc = 0;
    for (u32 i = 0; i < sizeof(SaveSlot); i++) {
        u8 t = static_cast<u8>((i ^ buf[i]));
        buf[i] = t;
        acc += static_cast<i32>((t & 0xff)) * static_cast<i32>(i);
    }
    return acc;
}

RVA(0x000e54b0, 0x1f)
SaveSlot* CSaveGame::GetSlot(i32 i) {
    if (i < 0 || i >= SAVE_SLOT_COUNT) {
        return 0;
    }
    return &m_slots[i];
}

RVA(0x000e54e0, 0x25)
i32 CSaveGame::InitializeNamedSlotAt(i32 index, const char* name, void* mgr) {

    return InitializeNamedSlot(GetSlot(index), name, mgr);
}

RVA(0x000e5520, 0x20)
i32 CSaveGame::StoreSlot(i32 idx, const SaveSlot* src) {
    return CopySlot(GetSlot(idx), src);
}

RVA(0x000e5550, 0x9a)
i32 CSaveGame::CloseTempFile(SaveSlot* p) {
    if (p == NULL) {
        return 0;
    }
    CFile file;
    if (file.Open(p->m_savePath, CFile::modeRead, 0)) {
        file.Close();
        CFile::Remove(p->m_savePath);
    }
    p->m_type = SAVESLOT_EMPTY;
    return 1;
}

RVA(0x000e5620, 0x27)
void CSaveGame::SetMaxLevel(QuestLevel v) {
    u32 maxLevel = static_cast<u32>(IDX(m_maxLevel));
    if ((v < QUESTLEVEL_CAMPAIGN_END
         && (static_cast<u32>(IDX(v)) > maxLevel || maxLevel > IDX(QUESTLEVEL_LAST)))
        || (maxLevel > IDX(QUESTLEVEL_LAST) && static_cast<u32>(IDX(v)) > maxLevel)) {
        m_maxLevel = v;
    }
}

RVA(0x000e5660, 0x1e)
void CSaveGame::SetCurLevel(QuestLevel v) {
    if (v >= QUESTLEVEL_CAMPAIGN_END) {
        return;
    }
    if (v <= CurrentLevel()) {
        return;
    }
    m_curLevel = v;
    if (v == QUESTLEVEL_CAMPAIGN_LAST) {
        SetMagic();
    }
}

RVA(0x000e5690, 0xf)
i32 CSaveGame::CheckMagic() {
    i32 v = m_magic;
    return v == SAVE_PROGRESS_MAGIC;
}

RVA(0x000e56b0, 0x8)
void CSaveGame::SetMagic() {
    m_magic = SAVE_PROGRESS_MAGIC;
}

RVA(0x000e5700, 0x9e)
int TempFileExists(SaveSlot* p) {
    if (p != NULL && (p->m_type & SAVESLOT_PRESENT)) {
        CFile file;
        if (file.Open(p->m_savePath, CFile::modeRead, 0)) {
            file.Close();
            return 1;
        }
    }
    return 0;
}
