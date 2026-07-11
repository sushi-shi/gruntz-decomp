// ShadeSelector.h - the shade-descriptor selector at RVA 0x14dd90.
//
// 0x14dd90 (__thiscall) copies a caller-supplied ShadeDescr* (or, when null, the
// mode's global default from g_shadeDescr208..220) into the object's +0x1c field and
// latches the mode at +0x14. It is called on the shade-state sub-object that several
// classes carry at offset 0 (CImageFrame::m_format, CDDrawShadeBlit m_owned, CImageSet
// etc.), so callers reach it as ((ShadeSelector*)obj)->Select(mode, descr). The body
// lives in ShadeDescrTable.cpp; this header is the single shared shape (was a per-TU
// .cpp-local view in ShadeDescrTable.cpp / WwdGameObject.cpp / cimage's Notify alias).
#ifndef GRUNTZ_DDRAWMGR_SHADESELECTOR_H
#define GRUNTZ_DDRAWMGR_SHADESELECTOR_H

#include <rva.h>

struct ShadeDescr;

SIZE_UNKNOWN(ShadeSelector);
class ShadeSelector {
public:
    char m_pad[0x14];
    i32 m_mode;                               // +0x14  latched mode id
    i32 m_18;                                 // +0x18  (role unproven)
    ShadeDescr* m_descr;                      // +0x1c  selected descriptor
    void Select(int mode, ShadeDescr* descr); // 0x14dd90
};

#endif // GRUNTZ_DDRAWMGR_SHADESELECTOR_H
