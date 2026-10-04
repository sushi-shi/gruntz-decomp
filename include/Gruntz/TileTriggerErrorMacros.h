#ifndef GRUNTZ_TILETRIGGERERRORMACROS_H
#define GRUNTZ_TILETRIGGERERRORMACROS_H

#include <Gruntz/GameRegistry.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/TileTriggerLogic.h>

#define REPORT_TILE_TRIGGER_ERROR(messageFormat, x, y, errorClass, errorSite)                      \
    do {                                                                                           \
        CString message;                                                                           \
        message.Format((messageFormat), (x), (y));                                                 \
        g_gameReg->ShowModalMessage(message);                                                      \
        g_gameReg->ReportError(IDX(errorClass), IDX(errorSite));                                   \
    } while (0)

#endif
