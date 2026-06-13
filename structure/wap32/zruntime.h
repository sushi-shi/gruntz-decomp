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
 * field layouts are recovered yet for these — every body is @todo. Do not invent.
 */

namespace WAP32
{
    /*
     * zErrHandling — error / exception handling base.
     * .?AVzErrHandling@@
     */
    class zErrHandling
    {
    public:
        //@size: unknown @todo
        //@todo: vtable, fields, methods all unknown
    };

    /*
     * zPtrColl — pointer collection (intrusive container base). Many WAP-managed
     * collections likely derive from / contain this.
     * .?AVzPtrColl@@
     */
    class zPtrColl
    {
    public:
        //@size: unknown @todo
        //@todo: vtable, fields, methods all unknown
    };

    /*
     * zPTree — pointer tree (balanced / sorted ptr tree).
     * .?AVzPTree@@
     */
    class zPTree
    {
    public:
        //@size: unknown @todo
        //@todo: vtable, fields, methods all unknown
    };

    /*
     * _zvec — vector helper (engine math/geometry vector). Leading underscore in
     * the RTTI name suggests an internal/struct-ish type.
     * .?AV_zvec@@
     */
    class _zvec
    {
    public:
        //@size: unknown @todo
        //@todo: probably {x,y,...} components — types/count unknown
    };

    /*
     * _zdvec — "double" vector helper (companion to _zvec).
     * .?AV_zdvec@@
     */
    class _zdvec
    {
    public:
        //@size: unknown @todo
        //@todo: probably {x,y,...} double components — unknown
    };

    /*
     * zDArray<T> — dynamic array template. This is THE engine container template.
     *
     * RTTI proves exactly one instantiation in the binary:
     *   .?AV?$zDArray@P8CUserLogic@@AEHXZ@@
     *   = zDArray< int (CUserLogic::*)() >
     *   = a dynamic array of pointer-to-member-function (CUserLogic, returns int,
     *     takes void). THIS IS THE PUZZLE-LOGIC DISPATCH TABLE — see
     *     game/userlogic.h (CUserLogic).
     *
     * Only the element type of that one instantiation is known; the template's
     * own field layout (count/capacity/data ptr) is @todo.
     */
    template <typename T>
    class zDArray
    {
    public:
        //@size: unknown @todo
        //@todo: typical dynamic-array fields (data ptr, size, capacity) — layout
        //       not recovered.
        // T* m_data;       // @todo @offset unknown
        // int m_count;     // @todo @offset unknown
        // int m_capacity;  // @todo @offset unknown
    };
}

/*
 * The concrete logic-dispatch instantiation, as a convenience typedef.
 * CUserLogic is forward-declared here; see game/userlogic.h.
 */
class CUserLogic;
typedef int (CUserLogic::*UserLogicFn)();
//@rtti: .?AV?$zDArray@P8CUserLogic@@AEHXZ@@
typedef WAP32::zDArray<UserLogicFn> UserLogicDispatchTable;

#endif /* WAP32_ZRUNTIME_H */
