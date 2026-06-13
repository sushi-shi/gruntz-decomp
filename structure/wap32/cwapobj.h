#ifndef WAP32_CWAPOBJ_H
#define WAP32_CWAPOBJ_H

/*
 * WAP32 object-model bases.
 *
 * CWapObj is the WAP-managed base object (much of the engine's object graph is
 * expected to derive from it); CWapX is a WAP extension / COM-ish wrapper. Both
 * are confirmed shared across Gruntz / Claw / Get Medieval (REVERSE_ENGINEERING_
 * PLAN.md §11 lists CWapX/CWapObj as shared base classes).
 *
 * Provenance: NAMES from RTTI. NO layout recovered — all @todo.
 */

namespace WAP32
{
    /*
     * CWapObj — WAP base object.
     * .?AVCWapObj@@
     */
    class CWapObj
    {
    public:
        //@size: unknown @todo
        //@todo: vtable + fields unknown. Likely the root of the engine object tree.
    };

    /*
     * CWapX — WAP extension / COM-ish wrapper.
     * .?AVCWapX@@
     */
    class CWapX
    {
    public:
        //@size: unknown @todo
        //@todo: vtable + fields unknown.
    };
}

#endif /* WAP32_CWAPOBJ_H */
