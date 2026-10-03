#ifndef GRUNTZ_GRUNTCONFIGMACROS_H
#define GRUNTZ_GRUNTCONFIGMACROS_H

#define LOAD_GRUNT_TOOL_REACH()                                                                        {                                                                                                      i32 radius = g_buteMgr.GetInt(m_animSetName.c_str(), "ToolAA", 1);                                         m_reachRect = MakeRect(-radius, -radius, radius, radius);                                      }

#endif
