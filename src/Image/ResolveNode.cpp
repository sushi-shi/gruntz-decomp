#include <Gruntz/ResolveNode.h>
#include <rva.h>

RVA(0x001549d0, 0x29)
CResolveNode::CResolveNode() {
    m_0c = 0;
    m_dirtyLeft = static_cast<i32>(0x80000000);
    m_dirtyArmed = -1;
    m_screenX = static_cast<i32>(0x80000000);
    m_clip.left = static_cast<i32>(0x80000000);
    m_level = 0;
    m_stateFlags = 0;
}

// CLoadable::GetClassId (0x154a00): the base-default class id - `xor eax,eax; ret`.
// Concrete kinds override with their own CLASSID_*; this un-phantoms the slot-8 default.
RVA(0x00154a00, 0x3)
i32 CLoadable::GetClassId() {
    return CLASSID_NONE;
}
