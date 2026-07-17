#include <rva.h>

// The serialize stream is the REAL CFileMemBase (<Gruntz/SerialArchive.h> typedefs
// CSerialArchive onto it); a fwd decl of the OLD placeholder name here would
// re-declare a distinct class and silently out-rank the typedef (MSVC5).
class CFileMemBase;
typedef CFileMemBase CSerialArchive;
class CImage; // the menu-bar drawable (m_frame; RenderFrameClipped 0x153810)
struct CSprite;

class CActionOptionsMenuBar {
public:
    CActionOptionsMenuBar();
    void Init(i32 a, i32 b, i32 x, i32 y, i32 gx, i32 gy);
    void Clear();
    i32 Activate(i32 a);
    i32 Refresh();
    i32 Render();
    i32 HitClick(i32 mx, i32 my);
    i32 HitHover(i32 mx, i32 my);
    void Deactivate();
    i32 Serialize(CSerialArchive* ar);
    void Dtor(); // in-place dtor (DestroyGroup teardown, reloc-masked)
    i32 LoadAssets();

    void Forward(i32 a, i32 b);         // 0x49b86 (reloc-masked) - forward (x,y) to the overlay
    int Deserialize(CSerialArchive* s); // 0x00009bb0

    i32 m_gridX;               // +0x00  grid X
    i32 m_gridY;               // +0x04  grid Y
    i32 m_screenX;             // +0x08  screen X (clamped)
    i32 m_screenY;             // +0x0c  screen Y (adjusted)
    CImage* m_frame;           // +0x10  menu-bar frame 1 (the drawable)
    i32 m_button0State;        // +0x14  button[0] state
    i32 m_button1State;        // +0x18  button[1] state
    CImage* m_button0Frame;    // +0x1c  button[0] resolved frame
    CImage* m_button1Frame;    // +0x20  button[1] resolved frame
    i32 m_button0Icon;         // +0x24  button[0] icon
    i32 m_button1Icon;         // +0x28  button[1] icon
    i32 m_active;              // +0x2c  active flag
    CSprite* m_normChipSprite; // +0x30  norm-chip sprite
    CSprite* m_highChipSprite; // +0x34  high-chip sprite
    CSprite* m_greyChipSprite; // +0x38  grey-chip sprite
    i32 m_loaded;              // +0x3c  loaded flag
};
