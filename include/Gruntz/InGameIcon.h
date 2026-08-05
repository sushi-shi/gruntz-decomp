#ifndef GRUNTZ_GRUNTZ_CINGAMEICON_H
#define GRUNTZ_GRUNTZ_CINGAMEICON_H

#include <rva.h>

#include <Mfc.h>

#include <Clock64.h>
#include <Enums.h>
#include <Gruntz/CurPlayer.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/LogicFnTable.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialCounter.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/UserLogic.h>

extern "C" u32 g_frameTime;

class LeafCue;

GZ_ENUM_BEGIN(InGameIconGlitter)
    ICON_GLITTER_NONE = 0,
    ICON_GLITTER_CURSE_GREEN = 1,
    ICON_GLITTER_POWERUP_RED = 2
GZ_ENUM_END(InGameIconGlitter)

// The help-box WWD Health field is a visibility policy for CInGameText.
GZ_ENUM_BEGIN(InGameTextVisibility)
    INGAME_TEXT_ALWAYS = 0,
    INGAME_TEXT_EASY_ONLY = 1,
    INGAME_TEXT_NORMAL_ONLY = 2
GZ_ENUM_END(InGameTextVisibility)

class CInGameIcon : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
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
    i32 m_reserved7c; // retail news 0x80 (push in CreateInGameIcon); position unproven
};
SIZE(0x80);

#endif // GRUNTZ_GRUNTZ_CINGAMEICON_H
