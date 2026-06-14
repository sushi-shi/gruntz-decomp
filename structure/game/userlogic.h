#ifndef GAME_USERLOGIC_H
#define GAME_USERLOGIC_H

/*
 * The puzzle-logic system: CUserBase / CUserLogic + the member-fn-ptr dispatch.
 *
 * CUserLogic is the engine's scriptable "user logic" object. The one concrete
 * template instantiation in the whole binary's RTTI is:
 *
 *   .?AV?$zDArray@P8CUserLogic@@AEHXZ@@  =  zDArray< int (CUserLogic::*)() >
 *
 * i.e. a dynamic array of pointer-to-member-functions of CUserLogic (each
 * returning int, taking void) — THE LOGIC DISPATCH TABLE. See ../wap32/zruntime.h
 * (UserLogicDispatchTable typedef).
 *
 * The CTile*Trigger*Logic family (triggers.h) are the concrete logic types that
 * plug into this dispatch system. CMovingLogic / CGiantRockLogic /
 * CCoveredPowerupLogic are other CUserLogic-flavored behaviors.
 *
 * Provenance: NAMES from RTTI; the dispatch-table shape from the template RTTI.
 * NO field layout recovered — name-only stubs.
 */

#include "../wap32/zruntime.h"

// CUserBase — base of the user-logic hierarchy.
class CUserBase { /* .?AVCUserBase@@ */ };

// CUserLogic — scriptable logic object; holds/uses a member-fn-ptr dispatch table
// (a zDArray<UserLogicFn>; the dispatch member's offset is unknown). Base is
// probably CUserBase. Forward-declared in ../wap32/zruntime.h so the dispatch-table
// typedef can name it. Layout unknown — name-only stub.
class CUserLogic { /* .?AVCUserLogic@@ */ };

#endif /* GAME_USERLOGIC_H */
