#ifndef GRUNTZ_PREVIEWSTATE_H
#define GRUNTZ_PREVIEWSTATE_H

#include <string>

#include <Ints.h>

#include <Enums.h>
#include <Gruntz/State.h>

class CPreviewState : public CState {
public:
    i32 Enter(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId);

    i32 Tick();

    void Cancel();
    void LoadLevelPreviewScreen();
    i32 LoadScreen(const std::string& name, i32 doFlip, i32 unused3, i32 unused4);
    void ResetPreview();
    i32 NextScreenCmd(i32 unused);
    i32 AcceptPreviewCommand(i32 unused);
    i32 Refade();
    i32 RefadeVirtual();
    i32 OnKey(i32 key, i32 unused);
    virtual i32 OnLButtonDown(i32 unused, i32 x, i32 y)  ;

    char m_pad1b4[0x1b8 - 0x1b4];
    u32 m_previewCountdownMs;
    std::string m_previewName;
    i32 m_previewIndex;
};

#endif
