#ifndef WAP32_ZRUNTIME_H
#define WAP32_ZRUNTIME_H

/*
 * WAP32 "z"-runtime — the engine's own base/runtime/container library.
 *
 * This is the lowercase-`z` substrate that Brian Goble's WAP32 engine statically
 * links into Gruntz (and Claw / Get Medieval — see REVERSE_ENGINEERING_PLAN.md §11).
 * It is DISTINCT from MFC's CArray/CObArray; it is the engine's intrinsic container
 * + error-handling layer.
 *
 * Provenance: class NAMES are from RTTI (mangled names kept verbatim below). NO
 * field layouts are recovered yet for these — left as name-only stubs (no layout
 * invented).
 */

namespace WAP32
{
    // zErrHandling — error / exception handling base.
    class zErrHandling { /* .?AVzErrHandling@@ */ };

    // zPtrColl — pointer collection (intrusive container base). Many WAP-managed
    // collections likely derive from / contain this.
    class zPtrColl { /* .?AVzPtrColl@@ */ };

    // zPTree — pointer tree (balanced / sorted ptr tree).
    class zPTree { /* .?AVzPTree@@ */ };

    // _zvec — vector helper (engine math/geometry vector).
    class _zvec { /* .?AV_zvec@@ */ };

    // _zdvec — "double" vector helper (companion to _zvec).
    class _zdvec { /* .?AV_zdvec@@ */ };

    // zDArray<T> — dynamic array template. THE engine container template. RTTI
    // proves exactly one instantiation in the binary:
    //   .?AV?$zDArray@P8CUserLogic@@AEHXZ@@ = zDArray< int (CUserLogic::*)() >
    //   = the puzzle-logic dispatch table (see game/userlogic.h, CUserLogic).
    // Only that element type is known; the template's own field layout is unknown,
    // so it is left empty (no layout invented).
    template <typename T>
    class zDArray { /* .?AV?$zDArray@...@@ */ };
}

// The concrete logic-dispatch instantiation, as a convenience typedef.
// CUserLogic is forward-declared here; see game/userlogic.h.
class CUserLogic;
typedef int (CUserLogic::*UserLogicFn)();
//@rtti: .?AV?$zDArray@P8CUserLogic@@AEHXZ@@
typedef WAP32::zDArray<UserLogicFn> UserLogicDispatchTable;

#endif /* WAP32_ZRUNTIME_H */
