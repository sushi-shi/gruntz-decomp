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
    RVA(0x000fab90, 0xaa)
    i32 LoadScreen(char* name, i32 doFlip, i32 unused3, i32 unused4) {
        if (m_world == 0) {
            return 0;
        }
        if (m_symParser == 0) {
            return 0;
        }
        if (m_stateBank == 0) {
            return 0;
        }
        char buf[64];
        sprintf(buf, "\\SCREENZ\\%s", name);
        CParseSource* sym = SymTab2c()->ResolveQualified(buf, IMGTAG_XCP);
        if (sym == 0) {
            return 0;
        }
        if (m_world->m_drawTarget->LoadPageImage(sym, 1) == 0) {
            return 0;
        }
        if (doFlip != 0) {
            m_world->m_drawTarget->m_frontPair->m_surface->Flip(0);
        }
        return 1;
    }
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
