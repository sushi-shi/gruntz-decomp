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
 * Provenance: NAMES from RTTI. NO layout recovered — left as name-only stubs.
 */

namespace WAP32
{
    // CWapObj — WAP base object. Likely the root of the engine object tree.
    class CWapObj { /* .?AVCWapObj@@ */ };

    // CWapX — WAP extension / COM-ish wrapper.
    class CWapX { /* .?AVCWapX@@ */ };
}

#endif /* WAP32_CWAPOBJ_H */
