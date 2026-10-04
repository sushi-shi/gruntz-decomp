#ifndef GRUNTZ_GRUNTZ_GRUNTZMGRMACROS_H
#define GRUNTZ_GRUNTZ_GRUNTZMGRMACROS_H

#include <string>

#define IS_STANDARD_VIDEO_MODE (m_modeSize.cx == SCREEN_W_PX && m_modeSize.cy == SCREEN_H_PX)

#define MODAL_REPORT_AT(format, x, y)                                                                  {                                                                                                      std::string s;                                                                                         s = formatText((format), (x), (y));                                                                      g_gameReg->EnterModalUI(s.c_str());                                               }

#endif
