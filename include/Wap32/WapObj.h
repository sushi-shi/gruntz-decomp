#ifndef WAP32_CWAPOBJ_H
#define WAP32_CWAPOBJ_H

#include <rva.h>
#include <Ints.h>
#include <Wap32/Object.h> // CObject - the 5-slot engine grand-base (vtbl 0x5e8cb4)

// VTBL_ABSENT: the engine grand-base is never standalone-constructed - its default
// bodies (IsLoaded @0x13b6-thunked, IsReady @0x1c08-thunked) are dispatched only
// through derived vtables; no ??_7CWapObj exists in the image (every analysed
// vtable is otherwise covered).
VTBL_ABSENT(CWapObj);
class CWapObj : public CObject {
public:
    // slot 5 (@+0x14) default @0x0013b6: `return m_10 > 0`. Derived classes
    // (CLoadable::IsLoaded, CGameLevel::IsLoaded, ...) override; CImage keeps it.
    virtual i32 IsLoaded();
    // slot 6 (@+0x18) default @0x001c08: `return 1`. CImageSet1/2/3 override; the
    // rest of the family (CImage, CResolveNode, the workers, ...) keep it.
    virtual i32 IsReady();
};
SIZE(0x4);

#endif // WAP32_CWAPOBJ_H
