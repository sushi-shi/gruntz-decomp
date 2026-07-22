#ifndef GRUNTZ_PREVIEWSTATE_H
#define GRUNTZ_PREVIEWSTATE_H

#include <rva.h>
#include <Mfc.h>          // full CString (the +0x1bc value member)
#include <Gruntz/State.h> // the CState base this screen state derives (real vtable)

class CPreviewState : public CState {
public:
    i32 Enter(void* mgr, i32 a1, i32 a2); // 0x0de030
    // LoadGameAssetNamespaces (0x0f9ea0) inherited from CState (called cast-free).
    i32 Tick(); // 0x0de200
    // RetireScene (0x0fa8f0) is a CState base method (inherited); the cast-free calls
    // in the .cpp bind ?RetireScene@CState@@ - no local decl needed.
    void Cancel();                                         // 0x0de590
    void LoadLevelPreviewScreen();                         // 0x0de420
    i32 LoadScreen(char* name, i32 doFlip, i32 a2, i32 a3); // 0x0fab90
    void ResetPreview();                                    // 0x0de140 (retail dead code)
    i32 NextScreenCmd(i32 param);                    // 0x0de190
    i32 Refade();                                    // 0x0de2c0
    i32 RefadeVirtual();                             // 0x0de340
    i32 OnKey(i32 key, i32 param);                   // 0x0de3c0

    // CPreviewState-specific fields, past the CState base (which ends at +0x1b4):
    char m_pad1b4[0x1b8 - 0x1b4];
    u32 m_1b8;     // +0x1b8  countdown timer
    CString m_1bc; // +0x1bc  scratch screen-name string (PREVIEW%i / \SCREENZ\%s)
    i32 m_1c0;     // +0x1c0  preview counter
};
SIZE_UNKNOWN();

#endif // GRUNTZ_PREVIEWSTATE_H
