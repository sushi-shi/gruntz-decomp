#include <Gruntz/ResolveNode.h>
#include <rva.h>

RVA(0x001549d0, 0x29)
CResolveNode::CResolveNode() {
    m_0c = 0;
    m_20 = static_cast<i32>(0x80000000);
    m_38 = -1;
    m_screenX = static_cast<i32>(0x80000000);
    m_clip.left = static_cast<i32>(0x80000000);
    m_3c = 0;
    m_stateFlags = 0;
}
