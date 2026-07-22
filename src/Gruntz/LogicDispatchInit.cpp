// LogicDispatchInit.cpp - file-scope logic-dispatch-table static initializer
// (C:\Proj\Gruntz).
//
// One init thunk, byte-identical to CSimpleAnimation.cpp's InitSimpleAnimDispatch
// (@0x0abb90): constructs a per-logic-class zDArray<int (CUserLogic::*)(void)>
// dispatch table over the index band [0x7d0, 0x7da]. It is the CRT static
// initializer the engine emits per game-object-logic class; the matching companion
// RegisterXLogic (the adjacent function that touches the same table + the zvec
// error globals 0x6bf464/0x6bf428) consumes the table.
//
// Its two former siblings were re-homed to their ORIGINAL TUs (wave3-I):
//   InitLogicDispatch_6445e8 @0x0406d0 -> Wormhole.cpp (the wormhole-trio obj)
//   InitLogicDispatch_6447f8 @0x0472d0 -> FortressFlag.cpp (the ff+particlez+
//                                         explosion obj)
//
// @identity-TODO: the 646060 table's owning leaf class is not yet pinned
// (proximity: CEyeCandy | CFrontCandyAni; frag i513 @0xacb10 sits in the
// frontcandyani region 0xabfa0-0xad527).
#include <Wap32/ZVec.h>          // _zdvec base
#include <Gruntz/LogicFnTable.h> // the shared CLogicActTable dispatch-table shape

DATA_SYMBOL(0x00246060, 0x24, ?g_eyeCandyDispatch@@3UCLogicActTable@@A)

RVA(0x000acb30, 0x15)
void InitLogicDispatch_646060() {
    g_eyeCandyDispatch.Construct(0x7d0, 0x7da);
}
