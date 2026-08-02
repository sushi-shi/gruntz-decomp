#ifndef GRUNTZ_PREVIEWSTATE_H
#define GRUNTZ_PREVIEWSTATE_H

#include <rva.h>
#include <Mfc.h>
#include <Gruntz/State.h>

class CPreviewState : public CState {
public:
    i32 Enter(CGruntzMgr* mgr, i32 areaArg, i32 a2);

    i32 Tick();

    void Cancel();
    void LoadLevelPreviewScreen();
    i32 LoadScreen(char* name, i32 doFlip, i32 unused3, i32 unused4);
    void ResetPreview();
    i32 NextScreenCmd(i32 param);
    i32 Refade();
    i32 RefadeVirtual();
    i32 OnKey(i32 key, i32 param);

    char m_pad1b4[0x1b8 - 0x1b4];
    u32 m_previewCountdownMs;
    CString m_previewName;
    i32 m_previewIndex;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_PREVIEWSTATE_H
