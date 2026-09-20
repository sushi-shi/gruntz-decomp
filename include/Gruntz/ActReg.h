#ifndef GRUNTZ_GRUNTZ_ACTREG_H
#define GRUNTZ_GRUNTZ_ACTREG_H

#include <rva.h>

#include <Enums.h>
#include <Gruntz/ActRegistry.h>
#include <Gruntz/UserLogic.h>
#include <ZTools/BitVec.h>
#include <ZTools/ZDArray.h>

GZ_ENUM_CONST_BEGIN(ActIdRange)
    ACT_ID_FIRST = 2000,
    ACT_ID_LAST = 2010
GZ_ENUM_CONST_END(ActIdRange)

typedef i32 (CUserLogic::*CActHandler)();
typedef zDArray<CActHandler> CActReg;

template<class Tag> struct CActRegPool {
    static CActReg s_table;
};

#endif // GRUNTZ_GRUNTZ_ACTREG_H
