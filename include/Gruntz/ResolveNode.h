#ifndef GRUNTZ_GRUNTZ_RESOLVENODE_H
#define GRUNTZ_GRUNTZ_RESOLVENODE_H

#include <Gruntz/Loadable.h> // canonical CLoadable : CWapObj : CObject (9-slot base)
#include <Ints.h>
#include <rva.h>

class CResolveNode : public CLoadable {
public:
    // Re-based onto the canonical 9-slot CLoadable: the m_04/m_08/m_0c header +
    // slots 5..8 come from CLoadable. This node OVERRIDES slot 5 (IsLoaded
    // @0x154a10) and slot 7 (Unload/reset @0x154a80); slots 6/8 carry the
    // CLoadable default bodies (0x001c08 / 0x154a00) and are INHERITED (audit:
    // redeclare-nothing now that CLoadable's own vtable 0x1efc30 is bound).
    // SetPosition (slot 9) is the node's own new virtual.
    virtual i32 IsLoaded() OVERRIDE; // [5] 0x154a10  (checks m_04!=-1 && m_0c)
    virtual i32 Unload() OVERRIDE;   // [7] 0x154a80  disarm the dirty-rect sentinels
    // slot 9 (new): set position + reset the draw state (x->m_5c, y->m_60, zero
    // the clip/plot fields, reseed m_48=0x32/m_50=1, cache owner->m_24). Body
    // 0x164790 (T obj); shared by the whole wide-object family (never overridden).
    virtual i32 SetPosition(i32 x, i32 y); // [9] 0x164790

    CResolveNode();                                    // 0x1549d0 (D pocket)
    CResolveNode(i32 owner, i32 field04, i32 field08); // 0x15b2c0 (I obj)
    // No-seed tag-ctor for the worker leaves (CDDrawWorkerBase family): constructs
    // the base WITHOUT the 0x1549d0 sentinel seeding - the leaf ctor spells its own
    // fused seed set, matching the factories' single-stamp inline shape (retail
    // 0x157150: one derived stamp, no base-ctor call). Inline + empty so the
    // intermediate vptr stamp dies as a dead store under the derived stamp.
    enum ENoSeed {
        NO_SEED
    };
    CResolveNode(ENoSeed) {}
    i32 Init(i32 owner, i32 field04, i32 resolveX, i32 resolveY, i32 field40, i32 field08);
    // ^ 0x1647e0 (T obj)

    // Dtor: disarm the live dirty-rect sentinels, then ~CLoadable (m_04/-1,
    // m_08/m_0c zero) + the CObject grand-base restamp fold in. Defined
    // OUT-OF-LINE in WwdFactoryObject.cpp (0x154a50) - the SAME TU as the
    // wide-object family dtors, so cl folds its content into ~E/~A/~F/~C
    // exactly as retail does, while the ResolveNode.cpp pocket's ??_G emits
    // retail's `call 0x154a50` against the extern (an inline def here would
    // make cl fold the body INTO that ??_G, which retail did not).
    virtual ~CResolveNode() OVERRIDE; // 0x154a50 (WwdFactoryObject.cpp)

    // vptr @+0x00 + m_04/m_08/m_0c inherited from CLoadable; own fields from +0x10.
    // (Names merged from the wide-object family's proven readers - the ex
    // WwdEdgeA/WwdEdgeB RAII scaffolding and the flat views' +0x10..+0x64 block.)
    i32 m_10; // +0x10  (SetPosition zeroes; plot state)
    i32 m_14; // +0x14  (SetPosition zeroes)
    // +0x18/+0x1c: the live dirty-rect / last-drawn position (RenderDot caches the
    // drawn column/row here; the 9-dword +0x18..+0x3c block is snapshotted to +0xb8).
    i32 m_lastX;              // +0x18
    i32 m_lastY;              // +0x1c
    i32 m_dirtyLeft;                 // +0x20  live dirty-rect left (INT_MIN = disarmed corner)
    i32 m_dirtyTop;                 // +0x24  live dirty-rect top
    char _pad28[0x30 - 0x28]; // +0x28..+0x2f
    // +0x30/+0x34: live dirty-rect size (a RenderDot dot plot sets 1x1; the
    // A/C blit slots read them as the rect extent).
    i32 m_dirtyW;             // +0x30
    i32 m_dirtyH;             // +0x34
    i32 m_dirtyArmed;                 // +0x38  live dirty-rect armed flag (-1 == disarmed)
    i32 m_3c;                 // +0x3c  = 0
    i32 m_stateFlags;                 // +0x40  (SetPosition zeroes)
    i32 m_44;                 // +0x44  (SetPosition zeroes)
    i32 m_48;                 // +0x48  (SetPosition reseeds 0x32)
    i32 m_drawFillArg;        // +0x4c  (SetPosition zeroes; flat name carried)
    i32 m_drawFillCmd;        // +0x50  (SetPosition reseeds 1; 0xb = decay fill-bar)
    i32 m_fillFraction;       // +0x54  fill fraction (0..256)
    i32 m_drawActive;         // +0x58  dirty/active flag (SetPosition zeroes)
    i32 m_screenX;            // +0x5c  screen/position X (INT_MIN = unset; the flat
                              //        CGameObject model's m_screenX - name converged)
    i32 m_screenY;            // +0x60  screen/position Y
    // +0x64..+0x73  the record clip rect - ONE member, wholly the node's: LevelTile-
    // Validation passes it BY VALUE (a RECT straddling a base boundary cannot exist),
    // the D-ctor seeds only .left (the INT_MIN sentinel), and the worker leaves'
    // +0x68..+0x73 is an untouched pad with their own m_refCount at +0x74 - so the
    // node ends at +0x74, not +0x68. (.left/.top/.right are also the checkpoint
    // config triple, slots 12..14.)
    RECT m_clip; // +0x64
};
SIZE_UNKNOWN(CResolveNode);
VTBL(CResolveNode, 0x001efbc0); // ??_7CResolveNode@@6B@ (10 slots; ex WwdBResolve dup)

#endif // GRUNTZ_GRUNTZ_RESOLVENODE_H
