#ifndef GRUNTZ_GRUNTZ_GRUNTZMGRMACROS_H
#define GRUNTZ_GRUNTZ_GRUNTZMGRMACROS_H

#define IS_STANDARD_VIDEO_MODE (m_modeSize.cx == SCREEN_W_PX && m_modeSize.cy == SCREEN_H_PX)

#define MODAL_REPORT_AT(format, x, y)                                                              \
    {                                                                                              \
        CString s;                                                                                 \
        s.Format((format), (x), (y));                                                              \
        g_gameReg->EnterModalUI(static_cast<LPCSTR>(s));                                           \
    }

#endif // GRUNTZ_GRUNTZ_GRUNTZMGRMACROS_H
