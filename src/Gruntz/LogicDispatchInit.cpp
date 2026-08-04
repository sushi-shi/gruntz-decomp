#include <Gruntz/ActReg.h>
#include <Gruntz/EyeCandyAni.h>
#include <Gruntz/LogicFnTable.h>
#include <Wap32/ZVec.h>

template<> DATA(0x00246060)
CActReg CActRegPool<CEyeCandyAni>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
