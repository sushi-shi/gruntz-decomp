#ifndef GRUNTZ_GRUNTZ_CINGAMEICON_H
#define GRUNTZ_GRUNTZ_CINGAMEICON_H

#include <Ints.h>

#include <Enums.h>
#include <Gruntz/ClockInterval.h>
#include <Gruntz/CurPlayer.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/LogicFnTable.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialCounter.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/UserLogic.h>

struct SoundCue;

GZ_ENUM_BEGIN(InGameIconGlitter)
    ICON_GLITTER_NONE = 0,
    ICON_GLITTER_CURSE_GREEN = 1,
    ICON_GLITTER_POWERUP_RED = 2
GZ_ENUM_END(InGameIconGlitter)

GZ_ENUM_BEGIN(InGameTextVisibility)
    INGAME_TEXT_ALWAYS = 0,
    INGAME_TEXT_EASY_ONLY = 1,
    INGAME_TEXT_NORMAL_ONLY = 2
GZ_ENUM_END(InGameTextVisibility)

class CInGameIcon : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*)  ;

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_INGAMEICON;
    }

public:
    CInGameIcon() {}
    CInGameIcon(CGameObject* obj);

    PickupType GetPickupType() const {
        return static_cast<PickupType>(m_object->m_smarts);
    }

    PickupType GetToyType() const {
        return static_cast<PickupType>(m_object->m_points);
    }

    i32 GetPlayerIndex() const {
        return m_object->m_score;
    }

    void SetPlayerIndex(i32 playerIndex) {
        m_object->m_score = playerIndex;
    }

    void SetupSprite(const char* cat);

    i32 HandleInput();
    virtual void FireActivation(i32 id)  ;

    i32 RefreshCell();
    i32 PeekCycle();
    i32 PlaceAt(i32 playerIndex, i32 unitIndex);
    i32 Reposition();

    SoundCue* m_cue;
    ClockInterval m_driftTiming;
    ClockInterval m_peekTiming;
    CWwdSpriteObject* m_glitterSprite;
};

#endif
