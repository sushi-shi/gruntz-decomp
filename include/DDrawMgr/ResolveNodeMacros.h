#ifndef GRUNTZ_DDRAWMGR_RESOLVENODEMACROS_H
#define GRUNTZ_DDRAWMGR_RESOLVENODEMACROS_H

#define SET_RESOLVE_POSITION_REFERENCED(x, y)                                                      \
    m_refCount = 2;                                                                                \
    return CResolveNode::SetPosition(x, y)

#endif // GRUNTZ_DDRAWMGR_RESOLVENODEMACROS_H
