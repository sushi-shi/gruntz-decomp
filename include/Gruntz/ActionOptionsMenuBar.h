#include <rva.h>

#include <Enums.h>
#include <Gruntz/PickupType.h>

// The two action chips are either absent, drawable, selected, or greyed out.
// These values also select the normal/highlight/grey sprite set in Refresh.
GZ_ENUM_BEGIN(ActionOptionButtonState)
    ACTIONOPTION_HIDDEN = 0,
    ACTIONOPTION_NORMAL = 1,
    ACTIONOPTION_SELECTED = 2,
    ACTIONOPTION_DISABLED = 3
GZ_ENUM_END(ActionOptionButtonState)

// HitHover identifies which of the two action chips contains the cursor.  The
// values are the retail return codes consumed by CTriggerMgr::TriggerCell.
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
        i32 gx,
        i32 gy
    );
    void Clear();
    i32 Activate(i32 a);
    i32 Refresh();
    i32 Render();
    i32 HitClick(i32 mx, i32 my);
    ActionOptionHit HitHover(i32 mx, i32 my);
    void Deactivate();
    i32 Serialize(CFileMemBase* ar);
    i32 LoadAssets();

    int Deserialize(CFileMemBase* s);

    i32 m_gridX;
    i32 m_gridY;
    i32 m_screenX;
    i32 m_screenY;
    CImage* m_frame;

    ActionOptionButtonState m_buttonState[2];
    CImage* m_buttonFrame[2];
    PickupType m_buttonIcon[2];
    i32 m_active;
    CDDrawWorker* m_normChipSprite;
    CDDrawWorker* m_highChipSprite;
    CDDrawWorker* m_greyChipSprite;
    i32 m_loaded;
};
