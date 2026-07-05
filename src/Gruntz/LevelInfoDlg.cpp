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
#include <string.h>           // strrchr/strchr (out-of-line) + strcat (inline /Oi)

#include <rva.h>
#include <Globals.h>

// The 11-entry area-name table (questz "Stage %d of <area>"). An array of char*
// indexed by (level-1)/4; modeled by-address so the load is reloc-masked.

// The preview-image loader singleton (m_14-style image manager) + the loaded
// preview image slot the dialog blits.
struct CPreviewMgr {
    void* LoadImage(void* pData, i32 fmt, i32 flags); // 0x1751f0 __thiscall
};

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
            g_previewImage = g_previewMgr->LoadImage(&readBuf[0xe], 2, 0);
            SetDlgItemTextA(hDlg, 0x4b3, title);
        }
    }
}

SIZE_UNKNOWN(CPreviewMgr);
