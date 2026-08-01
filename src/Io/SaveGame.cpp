#include <Io/SaveGame.h>
#ifdef __clang__
#undef _AFX_ENABLE_INLINES
#endif
#include <afxwin.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/FontConfig.h>
#include <Image/ImagePool.h>
#include <Image/Image.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Play.h>
#include <Gruntz/CheatMgr.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <Gruntz/ChainForward.h>
#include <Io/GameSave.h>
#include <Utils/RegistryHelper.h>
#include <stdio.h>
#include <rva.h>

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

RVA(0x000e35f0, 0x77)
i32 CALLBACK winapi_0e35f0_EndDialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case 0x111:
            if (wParam == 2) {
                EndDialog(hDlg, 0);
                return 1;
            }
            if (DrawSaveGameMenu(hDlg, wParam, g_saveDlgSink) != 0) {
                return 1;
            }

        default:
            return 0;
        case 0x110: {
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
            HWND item = GetDlgItem(hDlg, 0x51d);
            if (g_previewMgr == 0 || g_previewImage == 0 || item == 0) {
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
            if (w >= 0x140) {
                dx += (w - 0x140) / 2;
                w = 0x140;
            }
            if (h >= 0xf0) {
                dy += (h - 0xf0) / 2;
                h = 0xf0;
            }
            PAINTSTRUCT ps;
            BeginPaint(hDlg, &ps);
            SetStretchBltMode(ps.hdc, 3);
            CRezImage* img = static_cast<CRezImage*>(g_previewImage);
            if (img->m_bitCount == 8) {
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
                    // API-forced BITMAPINFO header/palette view.

                    reinterpret_cast<BITMAPINFO*>(&img->m_bih),
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
                    // API-forced BITMAPINFO header/palette view.

                    reinterpret_cast<BITMAPINFO*>(&img->m_bih),
                    DIB_RGB_COLORS,
                    SRCCOPY
                );
            }
            EndPaint(hDlg, &ps);
            return 1;
        }
        case WM_INITDIALOG: {
            if (g_slotState == 0) {
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
            if (wParam != 2 && wParam != 1) {
                return 0;
            }
            if (g_previewMgr != 0) {
                if (g_previewImage != 0) {
                    g_previewMgr->Free(static_cast<CRezImage*>(g_previewImage));
                }
                delete g_previewMgr;
                g_previewMgr = 0;
            }
            EndDialog(hDlg, 0);
            return 1;
        }
    }
    return 0;
}

RVA(0x000e3a40, 0xb0)
i32 CALLBACK winapi_0e3a40_EndDialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case 0x110:
            if (g_slotState == 0) {
                // API-forced INT_PTR boundary.

                EndDialog(hDlg, reinterpret_cast<INT_PTR>(g_slotState));
                return 1;
            }
            winapi_0e4850_SetDlgItemTextA(hDlg, g_gameReg->m_saveSink, g_slotState);
            return 1;
        case 0x111:
            if (wParam == 2) {
                EndDialog(hDlg, 0);
                return 1;
            }
            if (wParam == 1) {
                (static_cast<CSaveGame*>(g_gameReg->m_saveSink))->CloseTempFile(g_slotState);
                (static_cast<CSaveGame*>(g_gameReg->m_saveSink))->Save(0, 0x81a6);
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
        case 0x110:
            if (g_slotState == 0) {
                // API-forced INT_PTR boundary.

                EndDialog(hDlg, reinterpret_cast<INT_PTR>(g_slotState));
                return 1;
            }
            winapi_0e4850_SetDlgItemTextA(hDlg, g_gameReg->m_saveSink, g_slotState);
            return 1;
        case 0x111:
            if (wParam == 2) {
                EndDialog(hDlg, 0);
                return 1;
            }
            if (wParam == 1) {
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
        case 0x110:
            return 1;
        case 0x111:
            if (wParam == 2) {
                EndDialog(hDlg, 0);
                return 1;
            }
            if (wParam == 1) {
                EndDialog(hDlg, 1);
                return 1;
            }
            break;
    }
    return 0;
}

RVA(0x000e3c60, 0x1a3)
void FillSaveDialog(HWND hWnd, CSaveGame* sg) {
    if (hWnd == 0 || sg == 0) {
        return;
    }
    LabelSaveSlot(hWnd, sg->GetSlot(0), 0x435, 0x490, 0x49a, 0x4a4);
    LabelSaveSlot(hWnd, sg->GetSlot(1), 0x436, 0x491, 0x49b, 0x4a5);
    LabelSaveSlot(hWnd, sg->GetSlot(2), 0x437, 0x492, 0x49c, 0x4a6);
    LabelSaveSlot(hWnd, sg->GetSlot(3), 0x438, 0x493, 0x49d, 0x4a7);
    LabelSaveSlot(hWnd, sg->GetSlot(4), 0x439, 0x494, 0x49e, 0x4a8);
    LabelSaveSlot(hWnd, sg->GetSlot(5), 0x43a, 0x495, 0x49f, 0x4a9);
    LabelSaveSlot(hWnd, sg->GetSlot(6), 0x43b, 0x496, 0x4a0, 0x4aa);
    LabelSaveSlot(hWnd, sg->GetSlot(7), 0x43c, 0x497, 0x4a1, 0x4ab);
    LabelSaveSlot(hWnd, sg->GetSlot(8), 0x43d, 0x498, 0x4a2, 0x4ac);
    LabelSaveSlot(hWnd, sg->GetSlot(9), 0x43e, 0x499, 0x4a3, 0x4ad);
}

// @early-stop
RVA(0x000e3f40, 0x478)
i32 DrawSaveGameMenu(HWND hDlg, i32 cmd, CSaveGame* obj) {
    i32 c;
    if (cmd == 1) {
        c = g_savedMenuCmd;
        if (c == -1) {
            return 0;
        }
    } else {
        c = cmd;
    }

    if (HIWORD(c) == 0x100) {
        switch (LOWORD(c)) {
            case 0x435:
                g_savedMenuCmd = 0x490;
                break;
            case 0x436:
                g_savedMenuCmd = 0x491;
                break;
            case 0x437:
                g_savedMenuCmd = 0x492;
                break;
            case 0x438:
                g_savedMenuCmd = 0x493;
                break;
            case 0x439:
                g_savedMenuCmd = 0x494;
                break;
            case 0x43a:
                g_savedMenuCmd = 0x495;
                break;
            case 0x43b:
                g_savedMenuCmd = 0x496;
                break;
            case 0x43c:
                g_savedMenuCmd = 0x497;
                break;
            case 0x43d:
                g_savedMenuCmd = 0x498;
                break;
            case 0x43e:
                g_savedMenuCmd = 0x499;
                break;
        }
    }

    i32 info = -1;
    switch (c) {
        case 0x49a:
            info = 0;
            break;
        case 0x49b:
            info = 1;
            break;
        case 0x49c:
            info = 2;
            break;
        case 0x49d:
            info = 3;
            break;
        case 0x49e:
            info = 4;
            break;
        case 0x49f:
            info = 5;
            break;
        case 0x4a0:
            info = 6;
            break;
        case 0x4a1:
            info = 7;
            break;
        case 0x4a2:
            info = 8;
            break;
        case 0x4a3:
            info = 9;
            break;
    }
    if (info != -1) {
        g_slotState = obj->GetSlot(info);
        if (g_slotState == 0) {
            return 0;
        }
        EnableWindow(hDlg, FALSE);
        g_gameReg->RunModalDialog("GAME_INFO", LevelPreviewDlgProc, 0);
        EnableWindow(hDlg, TRUE);
        return 0;
    }

    i32 del = -1;
    switch (c) {
        case 0x4a4:
            del = 0;
            break;
        case 0x4a5:
            del = 1;
            break;
        case 0x4a6:
            del = 2;
            break;
        case 0x4a7:
            del = 3;
            break;
        case 0x4a8:
            del = 4;
            break;
        case 0x4a9:
            del = 5;
            break;
        case 0x4aa:
            del = 6;
            break;
        case 0x4ab:
            del = 7;
            break;
        case 0x4ac:
            del = 8;
            break;
        case 0x4ad:
            del = 9;
            break;
    }
    if (del != -1) {
        g_slotState = obj->GetSlot(del);
        if (g_slotState == 0) {
            return 0;
        }
        EnableWindow(hDlg, FALSE);
        i32 ok = g_gameReg->RunModalDialog("GAME_DELETE", winapi_0e3a40_EndDialog, 0);
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
        case 0x490:
            slot = 0;
            nameId = 0x435;
            break;
        case 0x491:
            slot = 1;
            nameId = 0x436;
            break;
        case 0x492:
            slot = 2;
            nameId = 0x437;
            break;
        case 0x493:
            slot = 3;
            nameId = 0x438;
            break;
        case 0x494:
            slot = 4;
            nameId = 0x439;
            break;
        case 0x495:
            slot = 5;
            nameId = 0x43a;
            break;
        case 0x496:
            slot = 6;
            nameId = 0x43b;
            break;
        case 0x497:
            slot = 7;
            nameId = 0x43c;
            break;
        case 0x498:
            slot = 8;
            nameId = 0x43d;
            break;
        case 0x499:
            slot = 9;
            nameId = 0x43e;
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
            if (g_slotState != 0) {
                EnableWindow(hDlg, FALSE);
                i32 ok = g_gameReg->RunModalDialog("GAME_OVERWRITE", InfoLineDialogProc, 0);
                EnableWindow(hDlg, TRUE);
                if (ok == 0) {
                    return 1;
                }
            }
        }
        obj->FillSlotByIndex(slot, name, g_gameReg);
        g_gameReg->FillSaveInfo(obj->GetSlot(slot), static_cast<void*>(name));
        EndDialog(hDlg, 1);
        if (!obj->Save(obj->GetSlot(slot)->m_savePath, 0x81a6)) {
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
    char readBuf[0x3843a];

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
            (n > 0x24 && n < 0x29) ? n - 0x24 : (n - 1) % 4 + 1,
            (n > 0x24 && n < 0x29) ? static_cast<const char*>(CString("Training"))
                                   : g_areaNames[(n - 1) / 4]
        );
    } else if (lev->m_isBattlez != 0 && lev->m_isCustom == 0) {

        wsprintfA(title, "Battlez: %s", lev->m_levelName);
    } else {

        char* bs = strrchr(lev->m_levelName, '\\');
        if (bs != 0) {
            if (lev->m_isBattlez) {
                wsprintfA(title, "Custom Battlez Level: ");
            } else {
                wsprintfA(title, "Custom Questz Level: ");
            }
            strcat(title, bs + 1);
            char* dot = strchr(title, '.');
            if (dot != 0) {
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
    if (f.Open(lev->m_savePath, 0x8000, 0) == 0) {
        g_previewImage = 0;
    } else {
        f.Seek(-0x3843a, 2);
        if (f.Read(readBuf, 0x3843a) != 0x3843a) {
            g_previewImage = 0;
            f.Close();
        } else {
            f.Close();
            g_previewImage = g_previewMgr->AddSurfaceOp(&readBuf[0xe], 2, 0);
            SetDlgItemTextA(hDlg, 0x4b3, title);
        }
    }
}

RVA(0x000e4850, 0x29)
void winapi_0e4850_SetDlgItemTextA(HWND hWnd, void* gate, SaveSlot* item) {
    if (hWnd && gate && item) {
        SetDlgItemTextA(hWnd, 0x40d, item->m_name);
    }
}

RVA(0x00085b50, 0x56)
CSaveGame::~CSaveGame() {
    Reset();
}

RVA(0x000e4b60, 0x158)
i32 CSaveGame::SaveGameFile(const char* dir) {
    if (dir == 0) {
        return 0;
    }
    m_str0 = dir;
    m_name = m_str0 + "Gruntz.sav";
    memset(m_header, 0, 0xa1c);
    Init();
    Load();
    for (i32 i = 0; i < 10; i++) {
        SaveSlot* slot = GetSlot(i);
        if (slot != 0) {
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
    m_maxLevel = 0x25;
    for (i32 i = 0; i < 10; i++) {
        SaveSlot* p = GetSlot(i);
        if (p != 0) {
            memset(p, 0, sizeof(SaveSlot));
        }
    }
}

RVA(0x000e4d90, 0xcc)
i32 CSaveGame::Load() {
    CFile file;
    if (!file.Open(m_name, 0, 0)) {
        return 0;
    }
    file.Read(m_header, 0xa1c);
    file.Read(m_slots, 0xa00);
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
    if (!file.Open(m_name, 0x1000, 0)) {
        return 0;
    }
    file.Close();
    if (!file.Open(m_name, 1, 0)) {
        return 0;
    }
    ComputeAll();
    file.Write(m_header, 0xa1c);
    file.Write(m_slots, 0xa00);
    file.Close();
    Verify();
    if (path != 0) {
        CPlay* state = static_cast<CPlay*>(g_gameReg->m_curState);
        g_gameReg->m_world->m_drawTarget->TransEnter();
        state->LoadSBITextEdges(msgId);
        if (!SaveGame(g_gameReg, path)) {
            return 0;
        }
        if (!ChainForward(g_gameReg->m_settings, g_gameReg, 0x140, 0xf0, path, 1)) {
            return 0;
        }
    }
    return 1;
}

RVA(0x000e50a0, 0x3e)
i32 CSaveGame::ComputeAll() {
    i32 sum = 0;
    for (i32 i = 0; i < 10; i++) {
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
    for (i32 i = 0; i < 10; i++) {
        // Byte-forced checksum view.
        sum += Decode(reinterpret_cast<u8*>(GetSlot(i)));
    }
    return m_header[2] == sum;
}

RVA(0x000e5130, 0x78)
i32 CSaveGame::FillSlot(SaveSlot* dst, const char* name, void* src) {
    if (dst == 0) {
        return 0;
    }
    if (src == 0) {
        return 0;
    }
    dst->m_type = 1;
    CGruntzMgr* reg = static_cast<CGruntzMgr*>(src);
    dst->m_levelId = (static_cast<CPlay*>(reg->m_curState))->m_levelIndex;
    dst->m_count = 0;
    dst->m_active = 1;
    if (reg->m_cheatMgr->m_124 != 0) {
        dst->m_type = 3;
    }
    strncpy(dst->m_name, name, 0x20);
    dst->m_checksum = Register(dst);
    return 1;
}

RVA(0x000e51d0, 0x49)
i32 CSaveGame::CopySlot(SaveSlot* dst, const SaveSlot* src) {
    if (dst == 0) {
        return 0;
    }
    if (src == 0) {
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
i32 CSaveGame::FillSlot2(SaveSlot* dst, i32 name, void* src) {
    if (dst == 0) {
        return 0;
    }
    if (src == 0) {
        return 0;
    }
    dst->m_type = 1;
    dst->m_levelId = name;
    dst->m_count = 0;
    if ((static_cast<CGruntzMgr*>(src))->m_cheatMgr->m_124 != 0) {
        dst->m_type = 3;
    }
    dst->m_checksum = Register(dst);
    return 1;
}

RVA(0x000e52c0, 0x99)
i32 CSaveGame::VerifySlot(SaveSlot* slot) {
    if (slot == 0) {
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
    if (slot == 0) {
        return 0;
    }
    i32 fc = slot->m_pathHi;
    i32 f8 = slot->m_pathLo;
    const char* name = (fc == 0 && f8 == 0) ? g_emptyString : slot->m_levelName;

    return g_gameReg->BuildLevelRezPath(fc == 0, fc, f8, slot->m_levelId, CString(name));
}

RVA(0x000e5410, 0x3d)
i32 CSaveGame::Encode(u8* buf) {
    if (buf == 0) {
        return 0;
    }
    i32 acc = 0;
    for (u32 i = 0; i < 0x100; i++) {
        u8 t = buf[i];
        acc += static_cast<i32>((t & 0xff)) * static_cast<i32>(i);
        buf[i] = static_cast<u8>((t ^ i));
    }
    return acc;
}

RVA(0x000e5460, 0x3f)
i32 CSaveGame::Decode(u8* buf) {
    if (buf == 0) {
        return 0;
    }
    i32 acc = 0;
    for (u32 i = 0; i < 0x100; i++) {
        u8 t = static_cast<u8>((i ^ buf[i]));
        buf[i] = t;
        acc += static_cast<i32>((t & 0xff)) * static_cast<i32>(i);
    }
    return acc;
}

RVA(0x000e54b0, 0x1f)
SaveSlot* CSaveGame::GetSlot(i32 i) {
    if (i < 0 || i >= 10) {
        return 0;
    }
    return &m_slots[i];
}

RVA(0x000e54e0, 0x25)
i32 CSaveGame::FillSlotByIndex(i32 idx, const char* name, void* src) {

    return FillSlot(GetSlot(idx), name, src);
}

RVA(0x000e5520, 0x20)
i32 CSaveGame::StoreSlot(i32 idx, const SaveSlot* src) {
    return CopySlot(GetSlot(idx), src);
}

RVA(0x000e5550, 0x9a)
i32 CSaveGame::CloseTempFile(SaveSlot* p) {
    if (p == 0) {
        return 0;
    }
    CFile file;
    if (file.Open(p->m_savePath, 0, 0)) {
        file.Close();
        CFile::Remove(p->m_savePath);
    }
    p->m_type = 0;
    return 1;
}

RVA(0x000e5620, 0x27)
void CSaveGame::SetMaxLevel(i32 v) {
    if ((v < 0x21 && (static_cast<u32>(v) > m_maxLevel || m_maxLevel > 0x24))
        || (m_maxLevel > 0x24 && static_cast<u32>(v) > m_maxLevel)) {
        m_maxLevel = v;
    }
}

RVA(0x000e5660, 0x1e)
void CSaveGame::SetCurLevel(i32 v) {
    if (v >= 0x21) {
        return;
    }
    if (v <= m_curLevel) {
        return;
    }
    m_curLevel = v;
    if (v == 0x20) {
        SetMagic();
    }
}

RVA(0x000e5690, 0xf)
i32 CSaveGame::CheckMagic() {
    i32 v = m_magic;
    return v == 0x42a;
}

RVA(0x000e56b0, 0x8)
void CSaveGame::SetMagic() {
    m_magic = 0x42a;
}

RVA(0x000e5700, 0x9e)
int TempFileExists(SaveSlot* p) {
    if (p != 0 && (p->m_type & 1)) {
        CFile file;
        if (file.Open(p->m_savePath, 0, 0)) {
            file.Close();
            return 1;
        }
    }
    return 0;
}

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
