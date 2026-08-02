#include <rva.h>

#include <Gruntz/ResolveNode.h>

RVA(0x001549d0, 0x29)
CResolveNode::CResolveNode() {
    m_ownerCtx = 0;
    m_dirty.m_rect.left = static_cast<i32>(0x80000000);
    m_dirty.m_armed = -1;
    m_screenX = static_cast<i32>(0x80000000);
    m_clip.left = static_cast<i32>(0x80000000);
    m_level = 0;
    m_stateFlags = 0;
}

RVA(0x00154a00, 0x3)
i32 CLoadable::GetClassId() {
    return CLASSID_NONE;
}
RVA(0x00154a10, 0x16)
i32 CResolveNode::IsLoaded() {
    if (m_ownerCtx != 0 && m_id != -1) {
        return 1;
    }
    return 0;
}
