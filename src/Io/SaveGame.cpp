// SaveGame.cpp - CSaveGame, the WAP32 save-game / saved-slot roster manager.
// See SaveGame.h for the layout and the provenance note (these were mislabeled
// "CFileIO" by the this/ecx tracer; they are the OWNER class that USES CFileIO).
//
// Each file-touching method (Load/Save) builds a stack-local CFileIO and drives
// Open/Read/Write/Close/~CFileIO on it; the CString member + the CFileIO temp's
// dtor put a C++ EH frame on those methods -> /GX (the `mfc` /O1 profile, matching
// FileStream.cpp). The leaf accessors are frameless register-frame functions.
//
// All cross-class callees (CFileIO, CString, CGameRegistry, the CRT _strncpy, the
// MFC wait-cursor helpers) are modeled as external/no-body so their reloc
// operands are masked in objdiff.
#include <Io/SaveGame.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/FontConfig.h>
// The level-preview / save-menu dialog half of this TU (ex LevelInfoDlg.cpp +
// SaveGameMenu.cpp, merged wave3-J - the 0x0e3690-0x0e579e interval's text is an
// L-S-L-S sandwich (levelinfodlg x3 | savegame x3 | ... | levelinfodlg x2 |
// savegame x23), impossible for two first-link objs; the private initialized-
// .data extents are contiguous 0x213ac8..0x213c10 across both).
#include <Gruntz/LevelInfo.h> // the canonical CLevelInfo (BuildLevelTitleString arg3)
#include <Image/ImagePool.h>  // the canonical CImagePool (g_previewMgr)
#include <Image/Image.h>      // CRezImage (g_previewImage; the DIB StretchDIBits reads)
#include <Gruntz/GruntzMgr.h> // CGruntzMgr (RunModalDialog/FillSaveInfo, DrawSaveGameMenu)
#include <Globals.h>          // g_previewMgr / g_previewImage
#include <stdio.h>            // sprintf (reloc-masked)
#include <rva.h>

#include <stdlib.h> // _itoa
#include <string.h> // memset -> inline rep stos

// The save dialog's per-slot labeller (0x0e3e80): labels one CSaveGame::GetSlot()
// record into four dialog controls. Forward-declared here (the FillSaveDialog caller
// precedes it in retail-RVA order), DEFINED at the end. The slot pointer flows in as
// the real Io/SaveGame.h SaveSlot* GetSlot() returns. All reloc-masked.
// (The GAME_INFO-dialog twin FillGameInfoDialog @0x9e0b0 + LabelGameInfoSlot @0x9e2d0
// were re-homed to src/Gruntz/LoadGameMenu.cpp - they sit inside the loadgamemenu
// .text obj block, between GruntzLoadGameDlgProc @0x9dff0 and LoadGameCommand @0x9e390,
// and are called by both; REHOME package D7.)
// The slot-occupancy probe IS TempFileExists_e5700 (0x2694 jmp-thunk -> 0xe5700,
// defined below): it reads the canonical SaveSlot directly (m_type@0 = the flag word,
// m_savePath@0x35 = the temp path) - the former SaveTempRec view is dissolved onto it.
// All reloc-masked.
int TempFileExists_e5700(SaveSlot* p); // 0x0e5700 (defined below)
void LabelSaveSlot(HWND hWnd, SaveSlot* item, i32 id3, i32 id4, i32 id5, i32 id6); // 0x0e3e80

// --- the dialog half's shared state/decls (ex LevelInfoDlg.cpp/SaveGameMenu.cpp) ---
// The 8-entry area-name pointer table (.bss, runtime-filled). DEFINED here (owner
// TU), reference extern stays in <Globals.h>. (REHOME DD-G)
char* g_areaNames[8]; // 0x6454e8
// g_gameReg comes typed from <Io/SaveGame.h> (the DATA(0x0024556c) binding lives in
// GruntzMgr.cpp); g_gameReg is the CGruntzMgr view of the SAME 0x24556c datum.
// The 0x64556c singleton IS CGruntzMgr (RTTI-confirmed, vftable 0x5e9b64) - declared at
// the REAL class so its methods emit DEFINED symbols instead of CGameRegistry phantoms
// (?RunModalDialog@CGameRegistry@@... etc. are names no obj and no .LIB can ever define).
// extern "C" keeps ONE C symbol (_g_gameReg) whatever C++ type a TU declares it at.
DATA(0x00213a9c)
i32 g_savedMenuCmd = -1;
// The level-preview image pool + previewed DIB (.bss). DEFINED here (owner TU),
// reference externs stay in <Globals.h>. (REHOME DD-G)
DATA(0x0024c814)
CImagePool* g_previewMgr; // 0x64c814
// The last-selected save record @0x24c864: read as an i32 SaveSlot* handle in the
// selection code and as a char*/SaveSlot* (its leading bytes) in the save-confirm
// info dialog - ONE datum, so the ex `g_dlgInfoText` char* view folds onto g_slotState
// (the tree winner; the C++-mangled g_dlgInfoText lost the per-rva dedup).
DATA(0x0024c864)
i32 g_slotState;
DATA(0x0024c868)
void* g_previewImage;                           // 0x64c868  (CRezImage* previewed DIB)
i32 __stdcall CloseTempFile_e5550(SaveSlot* r); // defined below (0x0e5550)
// The SetDlgItemTextA helper (0x0e4850) + the title builder (0x0e44e0), defined below.
void winapi_0e4850_SetDlgItemTextA(HWND hWnd, void* gate, char* item);
void BuildLevelTitleString(HWND hDlg, CSaveGame* gate, CLevelInfo* lev);
// FUN_00002e05 (thunk): per-slot delete driver (reloc-masked).
void DeleteSaveSlot(HWND hDlg, CSaveGame* obj);
// The three dialog sub-proc thunks passed as callbacks (reloc-masked code ptrs).
extern "C" void SaveInfoProc();
extern "C" void SaveDeleteProc();
extern "C" void SaveOverwriteProc();

// LevelPreviewDlgProc (0x0e3690) - the level-select preview dialog proc. WM_INITDIALOG
// builds the g_previewMgr image pool + the level title; WM_COMMAND (IDOK/IDCANCEL)
// frees the preview image + destroys the pool + closes; WM_PAINT centre-stretch-blits
// the previewed DIB (g_previewImage, 8bpp -> DIB_PAL_COLORS else DIB_RGB_COLORS) into
// a 320x240 box inside the preview item (0x51d). /GX EH frame (the pool ctor/dtor).
// @early-stop
// ~81%: logic byte-faithful (the msg sub-chain dispatch, the pool new/SetHandles/
// BuildLevelTitleString init, the Free+delete teardown, the rect-centre math, the
// 8bpp/rgb StretchDIBits branch). Residue is three codegen walls, not logic:
// (1) the /GX SEH prologue order (`push -1` vs `mov eax,fs:0` first + esi/edi
// shrink-wrap; docs/patterns/shrink-wrapped-callee-save-push.md, topic:wall);
// (2) epilogue tail-merge - retail funnels every return to ONE shared `mov eax,1` +
// epilogue via jmp, cl duplicates the epilogue per return (single-return `r` var
// tried: regressed to 77%); (3) the pool local coloring - cl reuses the hDlg-arg
// stack slot [esp+0x78] where retail reuses the msg slot [esp+0x7c], shifting the
// front-half displacements. None are source-steerable.
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
                    reinterpret_cast<BITMAPINFO*>(img),
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
                    reinterpret_cast<BITMAPINFO*>(img),
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
            // Retail dataflow (byte-proven at 0xe37c6..0xe3803): BOTH the pool's
            // second handle and the title call's window are edi = the dialog's own
            // hDlg (stack arg), and the gate is g_gameReg->m_saveSink pushed raw -
            // the old spelling passed g_previewMgr through two nonsense casts.
            if (g_previewMgr->SetHandles(
                    reinterpret_cast<i32>(* reinterpret_cast<HINSTANCE*>((reinterpret_cast<char*>(g_gameReg->m_owner) + 0xc))),
                    reinterpret_cast<i32>(hDlg),
                    0
                )
                == 0) {
                return 0;
            }
            BuildLevelTitleString(hDlg, g_gameReg->m_saveSink, reinterpret_cast<CLevelInfo*>(g_slotState));
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

// -------------------------------------------------------------------------
// 0x0e3a40. __stdcall dialog
// proc: OK closes; Cancel runs the save-confirm sub-object (CloseTempFile then
// CSaveGame::Save); WM_INITDIALOG fills the info line via the 0x0e4850 helper.
RVA(0x000e3a40, 0xb0)
i32 CALLBACK winapi_0e3a40_EndDialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case 0x110:
            if (g_slotState == 0) {
                EndDialog(hDlg, static_cast<INT_PTR>(g_slotState));
                return 1;
            }
            winapi_0e4850_SetDlgItemTextA(hDlg, g_gameReg->m_saveSink, reinterpret_cast<char*>(g_slotState));
            return 1;
        case 0x111:
            if (wParam == 2) {
                EndDialog(hDlg, 0);
                return 1;
            }
            if (wParam == 1) {
                CloseTempFile_e5550(reinterpret_cast<SaveSlot*>(g_slotState));
                (static_cast<CSaveGame*>(g_gameReg->m_saveSink))->Save(0, 0x81a6);
                EndDialog(hDlg, 1);
                return 1;
            }
            break;
    }
    return 0;
}

// -------------------------------------------------------------------------
// 0x0e3b20.
// __stdcall dialog proc: WM_INITDIALOG fills the info line via the 0x0e4850 helper;
// WM_COMMAND OK/Cancel just close the dialog (no save-confirm sub-object).
RVA(0x000e3b20, 0x86)
i32 CALLBACK InfoLineDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case 0x110:
            if (g_slotState == 0) {
                EndDialog(hDlg, static_cast<INT_PTR>(g_slotState));
                return 1;
            }
            winapi_0e4850_SetDlgItemTextA(hDlg, g_gameReg->m_saveSink, reinterpret_cast<char*>(g_slotState));
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

// ---------------------------------------------------------------------------
// 0x0e3be0. __stdcall dialog
// proc: WM_COMMAND OK/Cancel end the dialog; self-contained (no info line).
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

// ---------------------------------------------------------------------------
// FillSaveDialog  (0x000e3c60): walk the ten save slots of `sg`, labelling each
// into its row of four dialog controls (base IDs 0x435 / 0x490 / 0x49a / 0x4a4 +
// slot index). __cdecl(HWND, CSaveGame*); both pointers null-checked up front.
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
// jump-table scoring wall: the code bytes match the retail dispatch (the four
// dense-case jump tables, the deferred-command latch, the GAME_INFO/DELETE/
// OVERWRITE/save arms, EnableWindow/GetDlgItemText/EndDialog gates and the two
// return tails), but MSVC emits each switch's jump table as its own `$Lnnn` COMDAT
// while the delinker inlines the four `switchdataD_*` tables into the function body,
// so the four `jmp [id*4+table]` base relocs + the table data don't pair (see
// docs/patterns/switch-jumptable-separate-comdat.md + jumptable-data-overlap.md).
RVA(0x000e3f40, 0x3d6)
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

    // Latch a pending command from a control notification.
    if ((static_cast<u32>(c) >> 16) == 0x100) {
        switch (c & 0xffff) {
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

    // GAME_INFO buttons.
    i32 info;
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
        default:
            info = -1;
            break;
    }
    if (info != -1) {
        g_slotState = reinterpret_cast<i32>(obj->GetSlot(info));
        if (g_slotState == 0) {
            return 0;
        }
        EnableWindow(hDlg, FALSE);
        g_gameReg->RunModalDialog("GAME_INFO", static_cast<void*>(SaveInfoProc), 0);
        EnableWindow(hDlg, TRUE);
        return 0;
    }

    // GAME_DELETE buttons.
    i32 del;
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
        default:
            del = -1;
            break;
    }
    if (del != -1) {
        g_slotState = reinterpret_cast<i32>(obj->GetSlot(del));
        if (g_slotState == 0) {
            return 0;
        }
        EnableWindow(hDlg, FALSE);
        i32 ok = g_gameReg->RunModalDialog("GAME_DELETE", static_cast<void*>(SaveDeleteProc), 0);
        EnableWindow(hDlg, TRUE);
        if (ok == 0) {
            return 0;
        }
        DeleteSaveSlot(hDlg, obj);
        return 0;
    }

    // Save / overwrite buttons.
    i32 slot = -1;
    switch (c) {
        case 0x490:
            slot = 0;
            break;
        case 0x491:
            slot = 1;
            break;
        case 0x492:
            slot = 2;
            break;
        case 0x493:
            slot = 3;
            break;
        case 0x494:
            slot = 4;
            break;
        case 0x495:
            slot = 5;
            break;
        case 0x496:
            slot = 6;
            break;
        case 0x497:
            slot = 7;
            break;
        case 0x498:
            slot = 8;
            break;
        case 0x499:
            slot = 9;
            break;
    }
    if (slot == -1) {
        return 0;
    }

    char name[0x20];
    GetDlgItemTextA(hDlg, 0x435 + slot, name, 0x20);
    if (_strcmpi(name, "(Empty)") == 0) {
        sprintf(name, "Saved Game #%i", slot + 1);
    }
    if (TempFileExists_e5700(obj->GetSlot(slot))) {
        g_slotState = reinterpret_cast<i32>(obj->GetSlot(slot));
        if (g_slotState != 0) {
            EnableWindow(hDlg, FALSE);
            i32 ok = g_gameReg->RunModalDialog("GAME_OVERWRITE", static_cast<void*>(SaveOverwriteProc), 0);
            EnableWindow(hDlg, TRUE);
            if (ok == 0) {
                return 1;
            }
        }
    }
    obj->FillSlotByIndex(slot, reinterpret_cast<i32>(name), g_gameReg);
    g_gameReg->FillSaveInfo(reinterpret_cast<SaveInfo*>(reinterpret_cast<i32>(obj->GetSlot(slot))), static_cast<void*>(name));
    EndDialog(hDlg, 1);
    if (!obj->Save(reinterpret_cast<i32>(obj->GetSlot(slot)) + 0x35, 0x81a6)) {
        g_gameReg->EnterModalUI("ERROR - Cannot Save Game.");
    }
    return 1;
}

// The 11-entry area-name table (questz "Stage %d of <area>"). An array of char*
// indexed by (level-1)/4; modeled by-address so the load is reloc-masked.

// The preview-image loader singleton g_previewMgr is the canonical CImagePool
// (<Image/ImagePool.h>); the former CPreviewMgr view is gone (wave 3). The preview
// blit goes through CImagePool::AddSurfaceOp @0x1751f0.

// @early-stop  (94.46% - logic/frame/branches all faithful)
// Residual is three documented walls, not logic:
//   (1) prologue order + callee-save shrink-wrap: retail emits `push -1` before
//       `mov eax,fs:0` and pushes esi/edi UPFRONT; our /O2 swaps the first pair
//       and shrink-wraps esi/edi past the null guards (shrink-wrapped-callee-save
//       -push.md, topic:wall topic:regalloc - not source-steerable).
//   (2) the CString("Training") EH cleanup funclet is emitted inline at our tail
//       but lives in a separate COMDAT in retail (delinked target omits it).
//   (3) the 4 wsprintfA + 1 SetDlgItemTextA calls are `call [__imp__*]` (DIR32
//       reloc) vs retail's absolute IAT slot - reloc-typing scoring artifact,
//       code bytes identical (reloc-typing-vptr-global.md).
RVA(0x000e44e0, 0x2b2)
void BuildLevelTitleString(HWND hDlg, CSaveGame* gate, CLevelInfo* lev) {
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

    // m_isCustom/m_isBattlez are re-read at each use (not cached) to match retail's reloads.
    if (lev->m_isCustom == 0 && lev->m_isBattlez == 0) {
        // Standard questz/training level. The "Training" CString lives inside the
        // wsprintf full-expression so its construction is branch-conditional and
        // its destruction flag-guarded (the /GX temp).
        i32 n = lev->m_levelNum;
        wsprintfA(
            title,
            "Questz: Stage %d of %s",
            (n > 0x24 && n < 0x29) ? n - 0x24 : (n - 1) % 4 + 1,
            (n > 0x24 && n < 0x29) ? static_cast<const char*>(CString("Training")) : g_areaNames[(n - 1) / 4]
        );
    } else if (lev->m_isCustom == 0) {
        // Battlez level (named).
        wsprintfA(title, "Battlez: %s", lev->m_name);
    } else {
        // Custom level: format "Custom <mode> Level[: <basename>]". The mode-keyed
        // format strings are branched (if/else) so cl tail-merges to one wsprintf
        // call with two literal pushes (boolarg-branch-push-not-sete).
        char* bs = strrchr(lev->m_name, '\\');
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

    // Open the level file & extract the embedded preview image. Inverted guards
    // (Open == 0 / Read != full) put the g_previewImage=0 failure store inline
    // after each test (retail's jne-to-success / je-to-success layout).
    CFileIO f;
    if (f.Open(lev->m_path, 0x8000, 0) == 0) {
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

// -------------------------------------------------------------------------
// 0x0e4850. __cdecl helper:
// SetDlgItemTextA(hWnd, 0x40d, &item->text) when all three pointers are non-null.
RVA(0x000e4850, 0x29)
void winapi_0e4850_SetDlgItemTextA(HWND hWnd, void* gate, char* item) {
    if (hWnd && gate && item) {
        SetDlgItemTextA(hWnd, 0x40d, item + 0x14);
    }
}

// (FillGameInfoDialog @0x0009e0b0 re-homed to src/Gruntz/LoadGameMenu.cpp - see the
// labeller note at the top of this file.)

// ---------------------------------------------------------------------------
// CSaveGame::~CSaveGame  (0x00085b50)
// Reset() then the two CString members destruct in reverse declaration order
// (m_name @+4, then m_str0 @+0) under the EH unwind frame.
RVA(0x00085b50, 0x56)
CSaveGame::~CSaveGame() {
    Reset();
}

// ---------------------------------------------------------------------------
// CSaveGame::SaveGameFile
// Seed the roster from a directory: m_str0 = dir, m_name = dir +
// "Gruntz.sav", zero the 0xa1c header, Init() + Load() the roster, then for each
// of the ten slots that exists format its per-slot file path dir + "Slot" +
// (i+1) + ".sav" into the slot record at +0x35 (wsprintfA hoists the IAT pointer
// into ebx across the loop). The chained CString operator+ temps + the assigned
// CString member force the /GX EH frame.
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

// ---------------------------------------------------------------------------
// CSaveGame::Reset  (0x000e4d20)
// Init() the slots, then empty the file-name CString.
RVA(0x000e4d20, 0x12)
void CSaveGame::Reset() {
    Init();
    m_name.Empty();
}

// ---------------------------------------------------------------------------
// CSaveGame::Init  (0x000e4d50)
// Header field @+0x18 = 0x25, then zero all ten 0x100-byte slot records.
RVA(0x000e4d50, 0x2f)
void CSaveGame::Init() {
    m_maxLevel = 0x25;
    for (i32 i = 0; i < 10; i++) {
        SaveSlot* p = GetSlot(i);
        if (p != 0) {
            for (i32 j = 0; j < 0x40; j++) {
                (reinterpret_cast<i32*>(p))[j] = 0;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// CSaveGame::Load  (0x000e4d90)
// Open(m_name) read-only, read the 0xa1c header then the 0xa00 slot block, close.
RVA(0x000e4d90, 0xcc)
i32 CSaveGame::Load() {
    CFileIO file;
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

// ---------------------------------------------------------------------------
// CSaveGame::Save  (0x000e4ea0)
// @early-stop
// Wait-cursor (AfxGetThreadState()->...->BeginWaitCursor/EndWaitCursor) around a
// create+write+close sequence with two g_gameReg error/notify branches. The MFC
// wait-cursor / module-state internals and the two divergent error paths are not
// yet modeled; logic outline below, byte-match deferred to the final sweep.
RVA(0x000e4ea0, 0x18c)
i32 CSaveGame::Save(i32 a, i32 b) {
    CFileIO file;
    i32 ok = 0;
    if (file.Open(m_name, 0x1000, 0)) {
        file.Close();
        if (file.Open(m_name, 1, 0)) {
            ComputeAll();
            file.Write(m_header, 0xa1c);
            file.Write(m_slots, 0xa00);
            file.Close();
            ok = 1;
        }
    }
    static_cast<void>(a);
    static_cast<void>(b);
    static_cast<void>(ok);
    return 1;
}

// ---------------------------------------------------------------------------
// CSaveGame::ComputeAll  (0x000e50a0)
// Sum Encode() over all ten slots; store header fields (@+0x08..+0x14).
// @early-stop
// regalloc wall (~91%): retail materializes the `1` store via `mov eax,1; mov
// [edi+0xc],eax`; recompile uses the `mov $1,mem` immediate form. Logic exact.
RVA(0x000e50a0, 0x3e)
void CSaveGame::ComputeAll() {
    i32 sum = 0;
    for (i32 i = 0; i < 10; i++) {
        sum += Encode(reinterpret_cast<u8*>(GetSlot(i)));
    }
    *reinterpret_cast<i32*>(&m_header[0]) = 0;
    *reinterpret_cast<i32*>(&m_header[4]) = 1;
    *reinterpret_cast<i32*>(&m_header[8]) = sum;
    *reinterpret_cast<i32*>(&m_header[0xc]) = 0;
}

// ---------------------------------------------------------------------------
// CSaveGame::Verify  (0x000e50f0)
// Re-decode every slot, sum, compare to the stored checksum @ (this+0x18).
RVA(0x000e50f0, 0x2f)
i32 CSaveGame::Verify() {
    i32 sum = 0;
    for (i32 i = 0; i < 10; i++) {
        sum += Decode(reinterpret_cast<u8*>(GetSlot(i)));
    }
    return *reinterpret_cast<i32*>(&m_header[8]) == sum;
}

// ---------------------------------------------------------------------------
// CSaveGame::FillSlot  (0x000e5130)
// `src` is the live game-state object being captured; only two of its members are
// probed here (a level ptr @+0x2c whose +0x1c is the level id, and a world ptr
// @+0x44 whose +0x124 flags a custom world). That object's class is not modeled in
// this TU, so the two fields are read as binary-proven pointer arithmetic (the same
// forced opaque cross-class read the doctrine allows for un-recovered externs).
RVA(0x000e5130, 0x78)
i32 CSaveGame::FillSlot(SaveSlot* dst, const char* name, void* src) {
    if (dst == 0) {
        return 0;
    }
    if (src == 0) {
        return 0;
    }
    dst->m_type = 1;
    dst->m_levelId = *reinterpret_cast<i32*>((reinterpret_cast<char*>(*reinterpret_cast<void**>((reinterpret_cast<char*>(src) + 0x2c))) + 0x1c));
    dst->m_count = 0;
    dst->m_active = 1;
    if (*reinterpret_cast<i32*>((reinterpret_cast<char*>(*reinterpret_cast<void**>((reinterpret_cast<char*>(src) + 0x44))) + 0x124)) != 0) {
        dst->m_type = 3;
    }
    strncpy(dst->m_name, name, 0x20);
    dst->m_checksum = Register(dst);
    return 1;
}

// ---------------------------------------------------------------------------
// CSaveGame::CopySlot  (0x000e51d0)
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

// ---------------------------------------------------------------------------
// CSaveGame::FillSlot2  (0x000e5240)
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
    if (*reinterpret_cast<i32*>((reinterpret_cast<char*>(*reinterpret_cast<void**>((reinterpret_cast<char*>(src) + 0x44))) + 0x124)) != 0) {
        dst->m_type = 3;
    }
    dst->m_checksum = Register(dst);
    return 1;
}

// ---------------------------------------------------------------------------
// CSaveGame::VerifySlot  (0x000e52c0)
// Re-derive the slot's level rez-path id and validate it: fail (with an "does
// not exist" notice) if the registry can't resolve it, fail (with a "has
// changed" notice) if it no longer matches the slot's stored checksum (+0x10),
// else succeed. Same name-fallback (g_emptyString) and BuildLevelRezPath shape
// as Register.
// @early-stop
// EH-frame wall (same as Register, ~45%): retail builds the local CString name
// temp WITHOUT a /GX unwind frame and never destroys it (no fs:0 prologue, no
// ~CString); the faithful `CString s(name)` forces MSVC5 to emit the EH prolog +
// dtor cleanup, shifting the frame. Field reads, name fallback, BuildLevelRezPath
// args, both error branches and the checksum compare are all exact - only the
// extra frame differs. Deferred to the final sweep.
RVA(0x000e52c0, 0x99)
i32 CSaveGame::VerifySlot(SaveSlot* slot) {
    if (slot == 0) {
        return 0;
    }
    i32 fc = slot->m_pathHi;
    i32 f8 = slot->m_pathLo;
    const char* name = (fc == 0 && f8 == 0) ? g_emptyString : slot->m_levelName;
    CString s(name);
    i32 r = g_gameReg->BuildLevelRezPath(fc == 0, fc, f8, slot->m_levelId, s);
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

// ---------------------------------------------------------------------------
// CSaveGame::Register  (0x000e5390)
// Build a CString from the slot's name (or the empty string) and hand the slot's
// level id / flags to g_gameReg->BuildLevelRezPath().
// @early-stop
// EH-frame wall (~45%): retail builds the local CString temp WITHOUT a /GX unwind
// frame and never destroys it (no fs:0 prologue, no ~CString); the reconstructed
// `CString s(name)` local forces MSVC5 to emit the __EH_prolog + dtor cleanup.
// Field reads, name-fallback (g_emptyString) and the BuildLevelRezPath args are
// all exact - only the missing/extra frame differs. Deferred to the final sweep.
RVA(0x000e5390, 0x59)
i32 CSaveGame::Register(SaveSlot* slot) {
    if (slot == 0) {
        return 0;
    }
    i32 fc = slot->m_pathHi;
    i32 f8 = slot->m_pathLo;
    const char* name = (fc == 0 && f8 == 0) ? g_emptyString : slot->m_levelName;
    CString s(name);
    return g_gameReg->BuildLevelRezPath(fc == 0, fc, f8, slot->m_levelId, s);
}

// ---------------------------------------------------------------------------
// CSaveGame::Encode  (0x000e5410)
// Running XOR-fold checksum over a 0x100-byte slot (forward key). Checksums the
// PLAINTEXT byte (pre-XOR) then writes the XOR'd byte back.
// @early-stop
// regalloc wall (~89%): retail gratuitously saves edi (push edi) and holds the
// reloaded plaintext byte there (spill slot [esp+0xc]); recompile keeps it in the
// volatile edx (spill slot [esp+0x8], no edi save). Logic + spill-reload idiom
// exact, only the temp's register/slot differs.
RVA(0x000e5410, 0x3d)
i32 CSaveGame::Encode(u8* buf) {
    if (buf == 0) {
        return 0;
    }
    i32 acc = 0;
    for (u32 i = 0; i < 0x100; i++) {
        u8 t = buf[i];
        buf[i] = static_cast<u8>((t ^ i));
        acc += static_cast<i32>((t & 0xff)) * static_cast<i32>(i);
    }
    return acc;
}

// ---------------------------------------------------------------------------
// CSaveGame::Decode  (0x000e5460)
// @early-stop
// regalloc-tiebreak churn (~84%): body byte-identical to the pre-pristine 100%
// match; the pristine field renames elsewhere in the TU perturbed MSVC5's
// identifier-interning-driven register coloring, tipping this Encode/Decode
// checksum-loop family's fragile edi/edx spill choice (same wall as Encode). Not
// source-steerable; deferred to the final sweep (recover the edi/edx pin).
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

// ---------------------------------------------------------------------------
// CSaveGame::GetSlot  (0x000e54b0)
// Bounds-checked accessor into the +0xa24 record array.
RVA(0x000e54b0, 0x1f)
SaveSlot* CSaveGame::GetSlot(i32 i) {
    if (i < 0 || i >= 10) {
        return 0;
    }
    return &m_slots[i];
}

// ---------------------------------------------------------------------------
// CSaveGame::FillSlotByIndex  (0x000e54e0)
RVA(0x000e54e0, 0x25)
i32 CSaveGame::FillSlotByIndex(i32 idx, i32 name, void* src) {
    // retail forwards to FillSlot (0xe5130, the const char* name-string variant), not
    // FillSlot2 (0xe5240); `name` flows in as a char* (see DrawSaveGameMenu's caller).
    return FillSlot(GetSlot(idx), reinterpret_cast<const char*>(name), src);
}

// CSaveGame::StoreSlot  (0x000e5520) - copy `src` into the slot at index `idx`.
RVA(0x000e5520, 0x20)
i32 CSaveGame::StoreSlot(i32 idx, const SaveSlot* src) {
    return CopySlot(GetSlot(idx), src);
}

// The two temp-file helpers below probe the canonical SaveSlot directly: its m_type
// @+0x00 doubles as the "has a temp file" flag word (bit0; cleared to 0 by the
// closer) and m_savePath @+0x35 is the temp-file path. Only these two offsets are
// touched here (the former SaveTempRec view is dissolved).

// The temp-file delete at 0x1bf559 is the static MFC CFile::Remove (NAFXCW library;
// library_labels ?Remove@CFile@@SGXPBD@Z) - call it by its real name (reloc-masked).

// ---------------------------------------------------------------------------
// CloseTempFile  (0x000e5550) - if the record's temp file opens (read), close
// and delete it, then clear the record's flag. Returns 1 once the record was
// processed (0 only for a null record). Free __stdcall helper (callee-cleans).
RVA(0x000e5550, 0x9a)
int __stdcall CloseTempFile_e5550(SaveSlot* p) {
    if (p == 0) {
        return 0;
    }
    CFileIO file;
    if (file.Open(p->m_savePath, 0, 0)) {
        file.Close();
        CFile::Remove(p->m_savePath);
    }
    p->m_type = 0;
    return 1;
}

// SetMaxLevel (0x0e5620): clamped update of the highest-reached level. Out-of-line
// (retail emits it standalone; the inline member folded away and never emitted).
// @early-stop
// regalloc coin-flip: retail pins the arg `v` in edx and m_maxLevel in eax; cl
// picks the opposite (v in eax, m_maxLevel in edx). The branch structure and every
// compare/store are byte-identical - only the eax<->edx pairing differs, and it is
// not source-steerable here (SetCurLevel, the near-identical sibling, is 100%).
RVA(0x000e5620, 0x27)
void CSaveGame::SetMaxLevel(i32 v) {
    if (v < 0x21) {
        if (static_cast<u32>(v) > m_maxLevel) {
            m_maxLevel = v;
            return;
        }
        if (m_maxLevel > 0x24) {
            m_maxLevel = v;
            return;
        }
    }
    if (m_maxLevel <= 0x24) {
        return;
    }
    if (static_cast<u32>(v) <= m_maxLevel) {
        return;
    }
    m_maxLevel = v;
}

// SetCurLevel (0x0e5660): clamped update of the current level; re-stamp the save
// magic at level 0x20. Out-of-line (retail emits it standalone; the inline member
// folded away).
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
        SetMagic(); // 0xe56b0 (retail calls the magic-stamp here, not Init 0xe4d50)
    }
}

// ---------------------------------------------------------------------------
// CSaveGame::CheckMagic  (0x000e5690)
RVA(0x000e5690, 0xf)
i32 CSaveGame::CheckMagic() {
    i32 v = m_magic;
    return v == 0x42a;
}

// ---------------------------------------------------------------------------
// CSaveGame::SetMagic  (0x000e56b0) - stamp the save-header magic 0x42a at +0x20.
// Spatially re-homed from src/Stub/BoundaryLowerThunks.cpp (was CSettere56b0::Set);
// dissolved onto CSaveGame (m_magic@+0x20 is the field CheckMagic reads).
RVA(0x000e56b0, 0x8)
void CSaveGame::SetMagic() {
    m_magic = 0x42a;
}

// ---------------------------------------------------------------------------
// TempFileExists  (0x000e5700) - probe whether the record's flagged temp file
// can be opened for read: if bit0 is set and the path opens, close it and return
// 1, else 0. Free __cdecl helper (caller cleans the argument).
RVA(0x000e5700, 0x9e)
int TempFileExists_e5700(SaveSlot* p) {
    if (p != 0 && (p->m_type & 1)) {
        CFileIO file;
        if (file.Open(p->m_savePath, 0, 0)) {
            file.Close();
            return 1;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// LabelSaveSlot (0xe3e80, save dialog variant, re-homed from src/Stub/ApiCallers.cpp -
// it walks this file's CSaveGame::GetSlot() records). __cdecl(hWnd, item, id3..id6):
// label the slot's short name (m_name) into id3, "(Empty)" when IsSlotOccupied (0x2694 ->
// 0xe5700 slot-occupancy probe) fails; then set the four control enables. Here the first
// two enables are unconditional, the last two track occupancy. (Its GAME_INFO twin
// LabelGameInfoSlot @0x9e2d0 moved to LoadGameMenu.cpp - REHOME package D7.)
RVA(0x000e3e80, 0x86)
void LabelSaveSlot(HWND hWnd, SaveSlot* item, i32 id3, i32 id4, i32 id5, i32 id6) {
    i32 flag;
    if (TempFileExists_e5700(item)) {
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

// ---------------------------------------------------------------------------
// (The `BlitHost`/`BlitDrawOwner`/`BlitRectSrc` interleaver views are DISSOLVED,
// 2026-07-15: 0x0d00a0 was CPlay::PostSetup - vtable slot 37 (+0x94), the NEW virtual
// bound at that slot in ??_7CPlay / ??_7CDemo / ??_7CMulti. The body is now a real
// CPlay method homed to Play.cpp; `this` is a CPlay* (m_c -> CGameLevel::m_planeCtx
// rect, m_4 -> CGruntzMgr::m_chatLog CFontConfig draw sink). This `play` interleaver no longer
// lives in the savegame TU.)

// Class-metadata annotations (EOF-hosted).
SIZE(SaveSlot, 0x100);   // 0x100-byte slot record (m_slots[] array stride)
SIZE_UNKNOWN(CSaveGame); // fully modeled but tail not proven; owner may upgrade
