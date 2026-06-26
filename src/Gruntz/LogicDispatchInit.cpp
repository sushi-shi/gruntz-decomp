// LogicDispatchInit.cpp - file-scope logic-dispatch-table static initializers
// (C:\Proj\Gruntz).
//
// Three init thunks that are byte-identical to CSimpleAnimation.cpp's
// InitSimpleAnimDispatch (@0x0abb90): each constructs a per-logic-class
// zDArray<int (CUserLogic::*)(void)> dispatch table over the index band
// [0x7d0, 0x7da]. They are the CRT static initializers the engine emits per
// game-object-logic class; the matching companion RegisterXLogic (the adjacent
// function that touches the same table + the zvec error globals 0x6bf464/0x6bf428)
// consumes each table.
//
// The precise owning leaf class for each table is not yet pinned (the registration
// keys live in .data, not a .rdata string constant), so these are named by their
// dispatch-table address; the proximity candidates are recorded per function. The
// bodies are owner-independent (the global is a reloc-masked DATA extern and the
// zDArray ctor call reloc-masks), so the byte match is exact regardless.
#include <Wap32/ZVec.h> // zDArray base

// The logic dispatch table type + its ctor (0x408710, the zDArray<T> ctor over an
// index band; reached through the 0x3742 ILT thunk - the SAME callee as
// InitSimpleAnimDispatch). Returns this.
struct LogicFnTable : public zDArray {
    LogicFnTable* Construct(i32 lo, i32 hi); // 0x408710
};

// The per-class dispatch tables (zDArray<methodptr> in .data).
DATA(0x002445e8)
extern LogicFnTable g_logicDispatch_6445e8; // 0x6445e8  (proximity: CWormhole | CGruntPuddle)
DATA(0x002447f8)
extern LogicFnTable g_logicDispatch_6447f8; // 0x6447f8  (proximity: CParticlez | CGrunt)
DATA(0x00246060)
extern LogicFnTable g_logicDispatch_646060; // 0x646060  (proximity: CEyeCandy | CFrontCandyAni)

// 0x000406d0 - construct the dispatch table at 0x6445e8 over [0x7d0, 0x7da].
RVA(0x000406d0, 0x15)
void InitLogicDispatch_6445e8() {
    g_logicDispatch_6445e8.Construct(0x7d0, 0x7da);
}

// 0x000472d0 - construct the dispatch table at 0x6447f8 over [0x7d0, 0x7da].
RVA(0x000472d0, 0x15)
void InitLogicDispatch_6447f8() {
    g_logicDispatch_6447f8.Construct(0x7d0, 0x7da);
}

// 0x000acb30 - construct the dispatch table at 0x646060 over [0x7d0, 0x7da].
RVA(0x000acb30, 0x15)
void InitLogicDispatch_646060() {
    g_logicDispatch_646060.Construct(0x7d0, 0x7da);
}
