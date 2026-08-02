#ifndef GRUNTZ_GRUNTZ_CINGAMEICON_H
#define GRUNTZ_GRUNTZ_CINGAMEICON_H

#include <rva.h>
#include <Gruntz/GameRegistry.h>

#include <Mfc.h>

#include <Clock64.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/GameRegMfcPtr.h>

#include <Gruntz/CurPlayer.h>

#include <Gruntz/SoundState.h>
#include <Gruntz/SerialCounter.h>
#include <Gruntz/LogicFnTable.h>

extern "C" u32 g_frameTime;

class LeafCue;

class CInGameIcon : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, CGameObject*) OVERRIDE;
    RVA(0x00011cb0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_INGAMEICON;
    }

public:
    CInGameIcon() {}
    CInGameIcon(CGameObject* obj);

    void SetupSprite(const char* cat);

    i32 HandleInput();
    virtual void FireActivation(i32 id) OVERRIDE;

    i32 RefreshCell();
    i32 PeekCycle();
    i32 PlaceAt(i32 idx, i32 gridBase);
    i32 Reposition();

    LeafCue* m_cue;
    Clock64 m_driftPos;
    Clock64 m_driftThresh;
    Clock64 m_peekTimer;
    Clock64 m_peekWindow;
    CWwdGameObjectA* m_glitterSprite;
};
SIZE(0x80);

#endif // GRUNTZ_GRUNTZ_CINGAMEICON_H
