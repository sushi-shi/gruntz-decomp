#ifndef GRUNTZ_LOGICRECORDSTATE_H
#define GRUNTZ_LOGICRECORDSTATE_H

#include <Enums.h>

// The logic-record act-key state constants the dispatch compilands switch
// on (LogicRecordDispatch.cpp and StaticHazard.cpp's CreateStaticHazard).
GZ_ENUM_CONST_BEGIN(LogicRecordState)
    LOGICREC_INIT = 0,
    LOGICREC_OP_1D = 0x1d,
    LOGICREC_OP_1E = 0x1e,
    LOGICREC_OP_50 = 0x50,
    LOGICREC_OP_51 = 0x51,
    LOGICREC_OP_52 = 0x52,
    LOGICREC_OP_53 = 0x53,
    LOGICREC_BUILT = 0x3e8,
GZ_ENUM_CONST_END(LogicRecordState)

#endif // GRUNTZ_LOGICRECORDSTATE_H
