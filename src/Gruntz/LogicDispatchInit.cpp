#include <Gruntz/ActReg.h>
#include <Gruntz/EyeCandyAni.h>
#include <Gruntz/LogicFnTable.h>
#include <Wap32/ZVec.h>

RVA_DYNINIT(0x000acb10, 0xa, int)
RVA_DYNINIT(0x000acb30, 0x15, int)
RVA_DYNINIT(0x000acb60, 0xe, int)
RVA_DYNINIT(0x000acb80, 0x1f, int)
template<> DATA(0x00246060)
CActReg CActRegPool<CEyeCandyAni>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
