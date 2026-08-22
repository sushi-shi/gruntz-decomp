#ifndef GRUNTZ_STATUSBARITEMINLINE_H
#define GRUNTZ_STATUSBARITEMINLINE_H

#define INITIALIZE_STATUS_BAR_ITEM(owner, tab, host)                                               \
    m_owner = owner;                                                                               \
    m_tab = tab;                                                                                   \
    m_host = host;                                                                                 \
    m_redrawFrames = 0;                                                                            \
    SetEnabled(1);

#endif // GRUNTZ_STATUSBARITEMINLINE_H
