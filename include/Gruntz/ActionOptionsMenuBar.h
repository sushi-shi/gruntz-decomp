#include <rva.h>

#include <Enums.h>
#include <Gruntz/PickupType.h>

GZ_ENUM_BEGIN(ActionOptionButtonState)
    ACTIONOPTION_HIDDEN = 0,
    ACTIONOPTION_NORMAL = 1,
    ACTIONOPTION_SELECTED = 2,
    ACTIONOPTION_DISABLED = 3
GZ_ENUM_END(ActionOptionButtonState)

GZ_ENUM_BEGIN(ActionOptionHit)
    ACTIONOPTION_HIT_NONE = 0,
    ACTIONOPTION_HIT_PRIMARY = 2,
    ACTIONOPTION_HIT_SECONDARY = 3
GZ_ENUM_END(ActionOptionHit)

class CFileMemBase;

class CImage;
class CDDrawWorker;

class CActionOptionsMenuBar {
public:
    CActionOptionsMenuBar();
    i32 Init(
        ActionOptionButtonState primaryState,
        ActionOptionButtonState secondaryState,
        i32 x,
        i32 y,
        i32 playerIndex,
        i32 unitIndex
    );
    void Clear();
    i32 RefreshIfActive(i32 unusedDeltaMs);
    i32 Refresh();
    i32 Render();
    i32 UpdateHoverState(i32 mx, i32 my);
    ActionOptionHit HitTestButtons(i32 mx, i32 my);
    void Deactivate();
    i32 Serialize(CFileMemBase* ar);
    i32 LoadAssets();

    int Deserialize(CFileMemBase* s);

    b32 IsActive() const {
        return m_active;
    }

    i32 GetPlayerIndex() const {
        return m_playerIndex;
    }
    i32 GetUnitIndex() const {
        return m_unitIndex;
    }

    i32 m_playerIndex;
    i32 m_unitIndex;
    i32 m_screenX;
    i32 m_screenY;
    CImage* m_frame;

    ActionOptionButtonState m_buttonState[2];
    CImage* m_buttonFrame[2];
    PickupType m_buttonIcon[2];
    b32 m_active;
    CDDrawWorker* m_normChipSprite;
    CDDrawWorker* m_highChipSprite;
    CDDrawWorker* m_greyChipSprite;
    b32 m_loaded;
};
