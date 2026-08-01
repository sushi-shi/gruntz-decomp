#include <rva.h>

class CFileMemBase;

class CImage;
class CDDrawWorker;

class CActionOptionsMenuBar {
public:
    CActionOptionsMenuBar();
    i32 Init(i32 a, i32 b, i32 x, i32 y, i32 gx, i32 gy);
    void Clear();
    i32 Activate(i32 a);
    i32 Refresh();
    i32 Render();
    i32 HitClick(i32 mx, i32 my);
    i32 HitHover(i32 mx, i32 my);
    void Deactivate();
    i32 Serialize(CFileMemBase* ar);
    i32 LoadAssets();

    int Deserialize(CFileMemBase* s);

    i32 m_gridX;
    i32 m_gridY;
    i32 m_screenX;
    i32 m_screenY;
    CImage* m_frame;

    i32 m_buttonState[2];
    CImage* m_buttonFrame[2];
    i32 m_buttonIcon[2];
    i32 m_active;
    CDDrawWorker* m_normChipSprite;
    CDDrawWorker* m_highChipSprite;
    CDDrawWorker* m_greyChipSprite;
    i32 m_loaded;
};
SIZE_UNKNOWN();
