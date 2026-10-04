#ifndef GRUNTZ_PREVIEWSTATE_H
#define GRUNTZ_PREVIEWSTATE_H

#include <rva.h>

#include <Enums.h>
#include <Gruntz/State.h>

class CPreviewState : public CState {
public:
    i32 LoadPreviewAssets(CGruntzMgr* gameManager, i32 levelIndex, i32 previousStateId);

    i32 UpdatePreview();

    void Cancel();
    void ShowNextPreviewScreen();
    i32 LoadPreviewImage(char* imageName, i32 present, i32 unused3, i32 unused4);
    void ReleasePreviewAssets();
    i32 BeginPreview(i32 unused);
    i32 AcceptPreviewCommand(i32 unused);
    i32 RestorePreviewGraphics();
    i32 RedrawPreview();
    i32 HandlePreviewKey(i32 virtualKey, i32 unused);
    virtual i32 OnLButtonDown(i32 unused, i32 x, i32 y) OVERRIDE;

    // @identity-TODO: unaccessed word required by m_previewCountdownMs's retail offset.
    char m_pad1b4[0x1b8 - 0x1b4];
    u32 m_previewCountdownMs;
    CString m_currentPreviewName;
    i32 m_nextPreviewIndex;
};

#endif // GRUNTZ_PREVIEWSTATE_H
