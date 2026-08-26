#include <Gruntz/ActReg.h>
#include <Gruntz/EyeCandyAni.h>
#include <Gruntz/LogicFnTable.h>
#include <Wap32/ZVec.h>

RVA_DYNINIT(0x000acb00, 0xa, CActRegPool<CEyeCandyAni>::s_table)
RVA_DYNINIT(0x000acb20, 0x15, CActRegPool<CEyeCandyAni>::s_table)
RVA_DYNINIT(0x000acb50, 0xe, CActRegPool<CEyeCandyAni>::s_table)
RVA_DYNINIT(0x000acb70, 0x1f, CActRegPool<CEyeCandyAni>::s_table)
template<> DATA(0x00246fb8)
CActReg CActRegPool<CEyeCandyAni>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
