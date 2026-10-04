#ifndef GRUNTZ_DDRAWMGR_RESOLVENODEMACROS_H
#define GRUNTZ_DDRAWMGR_RESOLVENODEMACROS_H

#define SET_POSITION_AND_RESET_RENDER_PASSES(x, y)                                                 \
    m_renderPassesRemaining = 2;                                                                   \
    return CResolveNode::SetPosition(x, y)

#endif // GRUNTZ_DDRAWMGR_RESOLVENODEMACROS_H
