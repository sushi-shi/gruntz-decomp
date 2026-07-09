// LevelInfoDlg.cpp - the level-select dialog title/preview helpers.
//
// BuildLevelTitleString formats a level's display title (Questz/Battlez/Training/
// Custom variants) into a stack buffer via wsprintfA, opens the level file
// (CFileIO), reads its trailing 0x3843a-byte block, hands the embedded preview
// image to the g_previewMgr loader (g_previewImage), and pushes the title into
// the dialog item 0x4b3 (SetDlgItemTextA).
//
// /GX EH frame: the conditionally-constructed CString("Training") temp + the
// CFileIO local force the exception frame. Field names are placeholders
// (m_<hexoffset>); only the offsets + code bytes are load-bearing.

#include <Mfc.h>              // CString + <windows.h> (wsprintfA/SetDlgItemTextA) FIRST
#include <Io/FileStream.h>    // CFileIO (Open/Read/Seek/Close, all reloc-masked)
#include <Gruntz/LevelInfo.h> // the canonical CLevelInfo (arg3; shared with LoadConfig)
#include <Image/ImagePool.h>  // the canonical CImagePool (g_previewMgr; AddSurfaceOp)
#include <Image/Image.h>      // CRezImage (g_previewImage; the DIB StretchDIBits reads)
#include <Gruntz/GameRegistry.h> // g_gameReg (m_owner res-module handle, m_saveSink)
#include <string.h>           // strrchr/strchr (out-of-line) + strcat (inline /Oi)

#include <rva.h>
#include <Globals.h>

// The game registry singleton + the queried save-slot state (the previewed level).
DATA(0x0024556c)
extern CGameRegistry* g_gameReg;
DATA(0x0064c864)
extern i32 g_slotState;

// Defined below (same TU); the WM_INITDIALOG path calls it on the fresh preview mgr.
void BuildLevelTitleString(HWND hDlg, i32 bShow, CLevelInfo* lev);

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
            CRezImage* img = (CRezImage*)g_previewImage;
            if (img->m_bitCount == 8) {
                StretchDIBits(
                    ps.hdc, dx, dy, w, h, 0, 0, img->m_width, img->m_height, img->m_pixels,
                    (BITMAPINFO*)img, DIB_PAL_COLORS, SRCCOPY
                );
            } else {
                StretchDIBits(
                    ps.hdc, dx, dy, w, h, 0, 0, img->m_width, img->m_height, img->m_pixels,
                    (BITMAPINFO*)img, DIB_RGB_COLORS, SRCCOPY
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
            if (g_previewMgr->SetHandles(
                    (i32) * (HINSTANCE*)((char*)g_gameReg->m_owner + 0xc), (i32)g_previewMgr, 0
                )
                == 0) {
                return 0;
            }
            BuildLevelTitleString(
                (HWND)g_previewMgr, (i32)g_gameReg->m_saveSink, (CLevelInfo*)g_slotState
            );
            return 1;
        }
        case WM_COMMAND: {
            if (wParam != 2 && wParam != 1) {
                return 0;
            }
            if (g_previewMgr != 0) {
                if (g_previewImage != 0) {
                    g_previewMgr->Free((CRezImage*)g_previewImage);
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
void BuildLevelTitleString(HWND hDlg, i32 bShow, CLevelInfo* lev) {
    char title[0x80];
    char readBuf[0x3843a];

    if (!hDlg) {
        return;
    }
    if (!bShow) {
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
            (n > 0x24 && n < 0x29) ? (const char*)CString("Training") : g_areaNames[(n - 1) / 4]
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
