// ShadeSelector.h - the shade-descriptor selector at RVA 0x14dd90.
//
// 0x14dd90 (__thiscall) copies a caller-supplied ShadeDescr* (or, when null, the
// mode's global default from g_shadeDescr208..220) into the object's +0x1c field and
// latches the mode at +0x14. The body lives in ShadeDescrTable.cpp; this header is the
// single shared shape (was a per-TU .cpp-local view in ShadeDescrTable.cpp /
// WwdGameObject.cpp / cimage's Notify alias).
//
// @identity-TODO: ShadeSelector is very probably NOT its own class - it is a FACET of
// CDDrawShadeBlit (<DDrawMgr/DDrawShadeBlit.h>). The 0x14dd90 disasm writes exactly
// [ecx+0x14] = mode and [ecx+0x1c] = descr, which are CDDrawShadeBlit::m_drawType and
// ::m_palDescr; m_mode/m_18/m_descr below are m_drawType/m_light/m_palDescr, offset for
// offset. Every caller reaches it on a CImage::m_owned (a CDDrawShadeBlit) - the old
// "CImageFrame::m_format" that this comment used to cite WAS that same +0x30 owned
// sprite. Not folded here only because 0x14dd90's body is BOUND under
// ?Select@ShadeSelector@@; dissolving it re-mangles that rva (a separate fold), so the
// ((ShadeSelector*)owned) casts at the call sites stay honest until then.
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
