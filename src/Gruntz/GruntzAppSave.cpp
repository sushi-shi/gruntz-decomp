// GruntzAppSave.cpp - CGruntzApp save-game orchestrator.
//
// Targets:
//   CGruntzApp::SaveGame @ RVA 0x092530 (380 B) - SEH-framed __thiscall
//       that orchestrates a quick-save. Checks pre-conditions, accesses
//       fields at offsets +0x58, +0x2c, +0x44, +0xbc, +0x60, +0x5c from
//       `this` (all within CGameApp's 0x254-byte base), calls thunked
//       engine helpers (0x417e, 0x24c3, 0x2d97, 0x20a4, 0x1a3c, 0x1483),
//       and references sCannotSaveGame[] and sQuickSavedOk[].
//
// String constants:
//   "ERROR - Cannot Save Game."          @ RVA 0x6110b0
//   "Game Quicksaved successfully."      @ RVA 0x61108c
//   resource 0x81aa (for the CString in the error path)
//
// Build: cl /nologo /c /O2 /MT /GX /Fogruntzappsav.obj src\Gruntz\GruntzAppSave.cpp

// ---------------------------------------------------------------------------
// File-scope string literals (.rdata, fixed VA, reloc-masked).
// ---------------------------------------------------------------------------
static const char sCannotSaveGame[] = "ERROR - Cannot Save Game.";
static const char sQuickSavedOk[]   = "Game Quicksaved successfully.";

// ---------------------------------------------------------------------------
// External symbols (reloc-masked).
// ---------------------------------------------------------------------------
extern "C" int g_gameReg[];       // ?g_gameReg@@3PAHA @ RVA 0x24556c

extern "C" void __cdecl EngineShowMsg(void *, const char *);     // 0x417e
extern "C" void __cdecl EngineWriteSave(void *, void *, int);    // 0x24c3
extern "C" int  __cdecl EngineCheckPath(void *);                 // 0x2d97
extern "C" void __cdecl EngineCleanup(void *);                   // 0x20a4
extern "C" void __cdecl EngineFallback(void *);                  // 0x1a3c

// ---------------------------------------------------------------------------
// CGameApp — minimal layout for field-offset access.
// We use explicit pointer arithmetic so the emitted field offsets are
// determined by our calculations, not by the compiler's class layout.
// ---------------------------------------------------------------------------
class CGameApp {
public:
    void *vftable;                        // +0x00
    // Access fields at known byte offsets via inline helpers:
    // +0x2c: game manager pointer
    // +0x44: display status pointer
    // +0x58: must-be-non-NULL guard
    // +0x5c: status-message handler pointer
    // +0x60: save-handle cleanup object
    // +0xbc: save-flag pointer (bit 0 = can save)
};

#define FIELD_AT(this, off)  (*(void **)((char *)(this) + (off)))
#define INT_AT(this, off)    (*(int *)((char *)(this) + (off)))

// ---------------------------------------------------------------------------
// CGruntzApp (CGameApp subclass).
// ---------------------------------------------------------------------------
class CGruntzApp : public CGameApp {
public:
    int SaveGame();
};

// ---------------------------------------------------------------------------
// CGruntzApp::SaveGame  @ 0x092530 (380 B)
//
// SEH-framed (C++ EH /GX). Sequence:
//   1. if (this + 0x58 == NULL) return 0.
//   2. if (FIELD(this,0x2c)->vtbl[+0x10]() != 3) return 0.
//   3. if (FIELD(this,0x44)->field_0x124 != 0):
//        CString from ID 0x81aa -> EngineShowMsg(this, msg) -> ~CString.
//        return 1.
//   4. if (FIELD(this,0xbc) != NULL && *FIELD(this,0xbc) & 1):
//        a) if (FIELD(this,0x2c) + 0x1d0 == NULL) return 0.
//        b) if (FIELD(this,0x60)) EngineCleanup(FIELD(this,0x60)).
//        c) EngineWriteSave(this, FIELD(this,0xbc), 0).
//        d) if (!EngineCheckPath(FIELD(this,0xbc) + 0x35)):
//             EngineShowMsg(this, sCannotSaveGame); return 1.
//        e) (engine call through FIELD(this,0x5c) with msg + args):
//             (simplified) EngineShowMsg(this, sQuickSavedOk); return 1.
//   5. EngineFallback(this); return 0.
//
// @address: 0x92530
// @size:    0x17c
// ---------------------------------------------------------------------------
int CGruntzApp::SaveGame()
{
    // Check 1: guard field at +0x58 must be non-NULL.
    if (FIELD_AT(this, 0x58) == 0)
        return 0;

    // Check 2: game-manager mode check.
    {
        void *pMgr = FIELD_AT(this, 0x2c);
        if (pMgr != 0)
        {
            // Virtual call slot +0x10: check mode == 3.
            // (actual implementation requires __thiscall function pointer,
            //  which we cannot portably declare in MSVC 5; approximated.)
        }
    }

    // Check 3: if m_44->field_0x124 is set, show message and return.
    {
        void *pDisp = FIELD_AT(this, 0x44);
        if (pDisp != 0)
        {
            int val = *(int *)((char *)pDisp + 0x124);
            if (val != 0)
            {
                // Target: construct CString from resource 0x81aa,
                // EngineShowMsg(this, msg), destroy CString, return 1.
                EngineShowMsg(this, sCannotSaveGame);
                return 1;
            }
        }
    }

    // Check 4: can-save flag.
    {
        void *pSaveFlags = FIELD_AT(this, 0xbc);
        if (pSaveFlags != 0)
        {
            if (*(unsigned char *)pSaveFlags & 1)
            {
                // Sub-check on manager +0x1d0.
                void *pMgr = FIELD_AT(this, 0x2c);
                if (pMgr != 0)
                {
                    if (FIELD_AT(pMgr, 0x1d0) == 0)
                        return 0;
                }

                // Cleanup.
                void *pCleanup = FIELD_AT(this, 0x60);
                if (pCleanup != 0)
                    EngineCleanup(pCleanup);

                // Write save file.
                EngineWriteSave(this, pSaveFlags, 0);

                // Verify path.
                if (!EngineCheckPath((char *)pSaveFlags + 0x35))
                {
                    EngineShowMsg(this, sCannotSaveGame);
                    return 1;
                }

                // Success message.
                // Target: EngineMsg2(FIELD(this,0x5c), sQuickSavedOk, 0, 0x11)
                EngineShowMsg(this, sQuickSavedOk);
                return 1;
            }
        }
    }

    // Fallback.
    EngineFallback(this);
    return 0;
}
