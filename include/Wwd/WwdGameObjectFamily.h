#ifndef GRUNTZ_WWD_WWDGAMEOBJECTFAMILY_H
#define GRUNTZ_WWD_WWDGAMEOBJECTFAMILY_H

#include <Ints.h>
#include <Mfc.h> // CString (+0xdc) / CObList (+0x1dc)
#include <rva.h>
#include <Gruntz/ResolveNode.h>      // CResolveNode - the REAL base (+0x00..+0x67)
#include <Gruntz/AniAdvanceCursor.h> // CAniAdvanceCursor - A's +0x1a0 member
#include <Gruntz/WwdGridIter.h>      // WwdRegion - the embedded +0x9c spatial-grid node
#include <DDrawMgr/AnimWorkerObj.h>  // AnimWorkerObj - the owned +0x7c..+0x90 workers
#include <Wwd/WwdObjMgr.h>           // ex Globals.h

class
    CDDrawSurfacePair; // slots 11-14 params (render ctx + blit pairs; <DDrawMgr/DDrawSurfacePair.h>)
class CWwdGameObject;  // the flat dispatch model (CWwdGameObject factory pair return type)
class CDDrawWorker;    // CDDrawWorker/CDDrawWorker ARE CDDrawWorker (<DDrawMgr/DDrawWorker.h>)

class CImage;      // the cached frame element (<Image/CImage.h>)
struct LeafCue;    // the leaf-scan cache value (<Gruntz/LeafCue.h>)
class CAniElement; // ApplyGeometryDirect's geometry source (<Gruntz/AniElement.h>)

#define WORKER_FREE(p)                                                                             \
    do {                                                                                           \
        if (p) {                                                                                   \
            delete (p);                                                                            \
            (p) = 0;                                                                               \
        }                                                                                          \
    } while (0)

struct CGameObject : public CResolveNode {
public:
    virtual ~CGameObject() OVERRIDE; // 0x15b4f0 (out-of-line, WwdFactoryObject.cpp;
                                     // body = { Unload(); } + member/base folds)
    virtual i32 IsLoaded() OVERRIDE; // slot 5  @0x15b370 (m_7c && m_0c && m_04 != -1)
    // slot 7 - release the four workers + disarm the live/shadow dirty-rect
    // sentinels. INLINE so the family dtors + sibling Unloads fold the content
    // (retail ~A/~F/~C inline it; deep contexts spill to `call 0x15b5d0`).
    // The out-of-line copy (the vtable slot) lands at 0x15b5d0.
    RVA(0x0015b5d0, 0x7c)
    virtual void Unload() OVERRIDE {
        WORKER_FREE(m_7c);
        WORKER_FREE(m_80);
        WORKER_FREE(m_88);
        WORKER_FREE(m_collideWorker);
        m_c0 = static_cast<i32>(0x80000000);
        m_d8 = -1;
        m_screenX = static_cast<i32>(0x80000000);
        m_dirtyRect.left = static_cast<i32>(0x80000000);
        m_dirtyArmed = -1;
        // (void per the CLoadable slot; retail's eax residue is the INT_MIN the
        // stores materialize)
    }
    // slots 8 (GetClassId @0x154a00) and 9 (SetPosition @0x164790) INHERITED.
    // slot 10 - the factories' 4-arg build dispatch (the flat model's Setup
    // @0x150d60 is the body; renamed onto the family in the flat-merge stage).
    virtual i32 Setup(i32 a1, i32 a2, i32 a3, i32 a4); // slot 10 @0x150d60
    // slots 11-14 - per-object render + dirty-rect blit hooks: PURE in this base
    // (retail table holds __purecall @0x11fec0); every concrete kind overrides.
    virtual void Render(CDDrawSurfacePair* ctx) = 0;                                     // slot 11
    virtual void BltDirty(CDDrawSurfacePair* a, CDDrawSurfacePair* b) = 0;               // slot 12
    virtual void BltDirtyEx(CDDrawSurfacePair* a, CDDrawSurfacePair* b, i32 c) = 0;      // slot 13
    virtual void BltDirtyRegions(CDDrawSurfacePair* a, CDDrawSurfacePair* b, i32 c) = 0; // slot 14
    // slot 15 - the 4-arg play/serialize dispatch (the flat model's Play
    // @0x151150 is the body).
    virtual i32 Play(i32 ar, i32 mode, i32 a3, void* self); // slot 15 @0x151150

    // 0x15b650: per-tick notify - under flag bit 0x8 decrement the +0x128 budget
    // (latch the worker's error state on underflow); else fire the +0x80
    // notifier's m_notify with the owner. (Ex CWwdFactoryObject::Notify.)
    void Notify(void* p); // 0x15b650

    // The engine base-object ctor (0x15b390; declared-only here - the body is the
    // CWwdGameObjBaseCtor view's /GX ctor in WwdFactoryObject.cpp, a fold pending
    // that view's dissolution). ReadPlaneObjects runs it on a raw new(0x1dc) block.
    void Construct(void* owner, i32 id, i32 z); // 0x15b390

    // The 0x150xxx live method set (src/Wwd/WwdGameObject.cpp) - the ex-flat
    // CGameObject/CWwdGameObject models' methods, homed at their field level.
    i32 Serialize(i32 ar);                  // 0x151320
    i32 WriteSnapshot(i32 dst, i32 unused); // 0x151c00 (ret 8; 2nd arg unused)
    i32 SerializeObjectState(i32 a1);       // 0x151780  resolve deserialized worker names
    i32 ResolveLinkedObject(i32 gate);      // 0x151b90  cache the linked object
                                            //   (m_carrier) from the key m_184
    i32 EnsureWorker80(CGameObject* src);   // 0x150eb0  lazy worker @+0x80 (Hit)
    i32 EnsureWorker88(CGameObject* src);   // 0x150f90  lazy worker @+0x88 (Attack)
    i32 EnsureWorker90(CGameObject* src);   // 0x151070  lazy worker @+0x90 (collide)
    void AddLogicHit(char* key);            // 0x150f50
    void AddLogicAttack(char* key);         // 0x151030
    void AddLogicBump(char* key);           // 0x151110
    i32 NotifyHooked(void* arg);            // 0x151d20  hooked notify via the +0x7c aux

    i32 m_sortKey;  // +0x74  the manager z-order sort key (Setup stores a3;
                    //         CDDrawChildGroup::InsertSorted orders the list by it)
    i32 m_posCache; // +0x78  CObList POSITION cache (InsertSorted stores the node;
                    //         TickKillCues/RemoveAndDelete unlink through it)
    // The worker/partner PAIR scheme (CollideBroadcast @0x159f00 is the proof): each
    // lazily-built handler worker at +0x80/+0x88/+0x90 has its partner slot right
    // after it - the OTHER object of the pending event, stored just before the
    // worker's m_notify fires. (+0x94 is the flat model's m_hitOther.)
    AnimWorkerObj* m_7c;            // +0x7c  the owned worker/logic record
    AnimWorkerObj* m_80;            // +0x80  lazily-built Hit handler worker
    CGameObject* m_84;              // +0x84  Hit partner (RECT-phase mask1 hit)
    AnimWorkerObj* m_88;            // +0x88  lazily-built Attack handler worker
    CGameObject* m_8c;              // +0x8c  Attack partner (RECT mask2 / BOX mask2b hit)
    AnimWorkerObj* m_collideWorker; // +0x90  lazily-built Bump/collide handler worker
    CGameObject* m_hitOther;        // +0x94  collide partner (stored just before
                                    //        m_collideWorker's m_collideNotify fires - BroadPhase)
    CGameObject* m_carrier;         // +0x98  latched carrier (a category-0x80 platform
                                    //        object; StepAxisAlt stores it + sets flags
                                    //        bit4; CMovingLogic::Update then advances
                                    //        m_screenX/Y by the carrier's m_deltaX/Y).
                                    //        Also the serialized linked object (ResolveLinkedObject
                                    //        resolves it from the key m_184).
    WwdRegion m_region;             // +0x9c..+0xb7  the embedded spatial-grid region
                                    //        node: m_x/m_y (+0xac/+0xb0) are the position
                                    //        copies Setup refreshes, m_object (+0xb4) the
                                    //        self back-pointer (the CWwdSlot9c* records
                                    //        below are its placement-ctor views)
    i32 m_b8;                       // +0xb8  shadow dirty-rect x (prev-frame copy of +0x18)
    i32 m_bc;                       // +0xbc  shadow dirty-rect y
    i32 m_c0;                       // +0xc0  shadow dirty-rect corner (INT_MIN sentinel)
    char _pc4[0xd0 - 0xc4];
    i32 m_d0;     // +0xd0  shadow dirty-rect size x
    i32 m_d4;     // +0xd4  shadow dirty-rect size y
    i32 m_d8;     // +0xd8  shadow dirty-rect armed flag (-1 == disarmed)
    CString m_dc; // +0xdc  the object's name (dtor 0x1b9cde folds in ~E)
    // +0xe0..+0x18b  the serialized state block (field knowledge merged from the
    // flat CGameObject model - same offsets, one object).
    i32 m_e0; // +0xe0
    // +0xe4  movement-resolution mode (CGameLevel::DispatchMove kinds 1..8):
    // 7 = direct set (no tile collision; CProjectile seeds it), 1/2/5 -> handler A,
    // 3 -> B, 4 -> C, 8 -> B/C by direction, 6 -> D (two-probe recovery); the
    // handlers transition 1 <-> 4 <-> 6 as moves land/fall/block.
    i32 m_moveMode;     // +0xe4
    u32 m_collCategory; // +0xe8  collision category bits (0x80 = carrier/platform;
                        //        BroadPhase tests other->m_collCategory & t->m_collMask)
    i32 m_ec;           // +0xec  CollideBroadcast RECT-phase receive mask
    i32 m_f0;           // +0xf0  CollideBroadcast BOX-phase receive mask (the
                        //        entrance-sprite ctor seeds 1)
    u32 m_collMask;     // +0xf4  which categories this object collides with
    i32 m_strideX;      // +0xf8  tile-probe stride X (the move steppers' scan step)
    i32 m_strideY;      // +0xfc  tile-probe stride Y
    i32 m_100;          // +0x100
    i32 m_104;          // +0x104
    i32 m_108;          // +0x108
    i32 m_10c;          // +0x10c
    i32 m_110;          // +0x110
    i32 m_114;          // +0x114  (teleporter spawn: source-tile coordinate mirror)
    i32 m_118;          // +0x118  CSpotLight ctor: pi/0 mode gate
    i32 m_11c;          // +0x11c  CSpotLight ctor: settings-table index
    i32 m_120;          // +0x120  CSpotLight ctor: SpotLightTime override; ALSO the
                        //         damage amount CollideBroadcast subtracts from the
                        //         other party's m_placeMode budget
    i32 m_124;          // +0x124  sprite-selector row key (leaf ctors -> ApplyLookupSprite)
    i32 m_placeMode;    // +0x128  visibility/place mode (1 or 2; the on-screen gate
                        //         discriminator); ALSO the flag-bit-8 damage budget
                        //         CollideBroadcast decrements (worker error state on
                        //         underflow)
    i32 m_12c;          // +0x12c  CSpotLight ctor: m_58 scale gate
    i32 m_130;          // +0x130  (CUFO ctor: seeds the spotlight's m_120)
    // +0x134..+0x140  signed per-side collision extents around (m_screenX, m_screenY):
    // left/top/right/bottom. Trigger ctors store TILE spans (world box = pos +/-
    // extent<<5 +/- 7); the movement steppers read them as PIXEL offsets (L stored
    // negative). 0x80000000 = unset (the collision pumps skip the object).
    RECT m_extent;     // +0x134  L/T/R/B (a REAL RECT - the broad-phase overlap
                       //         helpers take it BY VALUE as tagRECT); .bottom is
                       //         the feet line (WalkColumnDown ground-snaps from it)
    RECT m_area;       // +0x144  derived activation/stand box (world-space in the
                       //         trigger initializers; .top is a platform's stand
                       //         surface row; CollideBroadcast's oi-side test box)
    RECT m_switchRect; // +0x154  the tile-switch registrar rect (BY-VALUE arg of
                       //         RegisterSwitchLogic; CollideBroadcast's oj-side box)
    i32 m_164;         // +0x164
    i32 m_168;         // +0x168
    i32 m_16c;         // +0x16c
    i32 m_170;         // +0x170
    i32 m_deltaX;      // +0x174  per-frame movement delta X (carrier-ride advance)
    i32 m_deltaY;      // +0x178  per-frame movement delta Y
    i32 m_17c;         // +0x17c
    i32 m_180;         // +0x180
    i32 m_184;         // +0x184  serialized linked-object key (ResolveLinkedObject -> m_carrier)
    i32 m_188;         // +0x188  object id (the manager's CMapPtrToPtr key -
                       //         g_wwdObjIdCounter stamp; warlord battle-event id)
};
SIZE_UNKNOWN(); // base subobject; the concrete kinds carry the sizes

class CWwdGameObjectA : public CGameObject {
public:
    virtual ~CWwdGameObjectA() OVERRIDE; // slot 1  0x15b790 (out-of-line, I obj)
    // slot 7 override: null the geometry cache then the base release pass.
    // INLINE so ~A folds it (retail ~A = { Unload(); } + folds); the out-of-line
    // copy (the vtable slot) lands at 0x15b980.
    RVA(0x0015b980, 0x96)
    virtual void Unload() OVERRIDE {
        m_18c = -1;
        m_190 = -1;
        m_layer = 0;
        m_194 = 0;
        CGameObject::Unload(); // the E pass (0x15b5d0 content)
    }
    virtual i32 GetClassId() OVERRIDE; // slot 8  @0x15b760 (5 = CLASSID_SERIALREF)
    virtual i32 Setup(i32 a1, i32 a2, i32 a3, i32 a4) OVERRIDE; // slot 10 @0x15b940 (Init)
    virtual void Render(CDDrawSurfacePair* ctx) OVERRIDE;       // slot 11 @0x15ba20 (ret 4)
    virtual void BltDirty(CDDrawSurfacePair* a, CDDrawSurfacePair* b) OVERRIDE; // slot 12 @0x150660
    virtual void BltDirtyEx(CDDrawSurfacePair* a, CDDrawSurfacePair* b, i32 c)
        OVERRIDE; // slot 13 @0x1506b0
    virtual void BltDirtyRegions(CDDrawSurfacePair* a, CDDrawSurfacePair* b, i32 c)
        OVERRIDE; // slot 14 @0x1508a0
    virtual i32 Play(i32 ar, i32 mode, i32 a3, void* self)
        OVERRIDE; // slot 15 @0x150a70 (Dispatch: route by mode - 4 -> ReadState,
                  // 7 -> SerializeSpriteName - then the base Play body)

    // The created-sprite frame-cache method set (this kind's +0x194/+0x198 tail).
    void ApplyLookupSprite(const char* key, i32 flag);                  // 0x1504d0
    void ApplyName(const char* name);                                   // 0x150540
    i32 ApplyLookupGeometry(const char* key, i32 flag);                 // 0x1505b0
    i32 LookupAnimSprite(const char* name);                             // 0x150610
    void ApplyGeometryDirect(CAniElement* srcSprite, i32 applyDefault); // 0x58b60
    i32 Test();                      // 0x1509c0  on-screen visibility cull (the m_198 extent)
    i32 SerializeSpriteName(i32 a1); // 0x150c30  (A-tail frame-cache reader; Play mode-7 route)
    i32 ReadState(i32 src);          // 0x150b00

    i32 m_18c;       // +0x18c  (WwdFile stamp -1; the C kind reads its low byte as dot color)
    i32 m_190;       // +0x190  cached frame NUMBER (WwdFile stamp -1)
    union {          // +0x194  role-union (the flat model's proof): a WwdFile-loaded
                     //         object keeps its source-def record (class-name string
                     //         at +0x24); a CreateSprite'd object caches the looked-up
                     //         sprite (ApplyName/ApplyLookupSprite) / its CDDrawWorker
                     //         (ActionArea's pulse ramp SetAllTypes/SetAllField18)
        char* m_194; // source-def record
        CDDrawWorker* m_sprite;   // cached sprite (frame-cache role)
        CDDrawWorker* m_imageSet; // cached image set (color/brightness role)
    };
    CImage* m_layer; // +0x198  cached frame POINTER (the flat model name)
    union {          // +0x19c  role-union (mirrors +0x194): resolved sound-cue value
                     //         (ReadState -> FindKeyOfValue) vs the cached anim
                     //         sprite (LookupAnimSprite); WwdFile stamps 0
        LeafCue* m_19c;
        CDDrawWorker* m_19cSprite;
    };
    CAniAdvanceCursor m_1a0; // +0x1a0..+0x1db  the anim/command cursor (its
                             //  ~CAniAdvanceCursor folds inline in ~A/~B - the
                             //  retail 0x5f0128 member restamp)
};
SIZE(0x1dc);

class CWwdGameObject : public CWwdGameObjectA {
public:
    virtual ~CWwdGameObject() OVERRIDE; // 0x15bd10 (out-of-line, I obj)
    virtual i32 IsLoaded() OVERRIDE;    // slot 5  0x15bcd0 (`return m_7c != 0`)
    // slot 7 override: destroy the child list, then the A/E release pass.
    // INLINE so ~B folds it; the out-of-line copy lands at 0x15bf00.
    RVA(0x0015bf00, 0xa1)
    virtual void Unload() OVERRIDE {
        Clear(); // 0x166810 destroy the m_1dc children + RemoveAll
        m_1f8 = 0;
        m_18c = -1;
        m_190 = -1;
        m_layer = 0;
        m_194 = 0;
        CGameObject::Unload(); // the E pass (deep contexts spill to `call 0x15b5d0`)
    }
    virtual i32 GetClassId() OVERRIDE; // slot 8  0x15bce0 (0x1b)
    // slot 10/11-14 overrides (bodies in WwdGameObjectRender.cpp).
    virtual i32 Setup(i32 a1, i32 a2, i32 a3, i32 a4) OVERRIDE; // slot 10 0x1665e0
    virtual void Render(CDDrawSurfacePair* ctx) OVERRIDE;       // slot 11 0x1668b0 (broadcast)
    virtual void BltDirty(CDDrawSurfacePair* a, CDDrawSurfacePair* b) OVERRIDE; // slot 12 0x1668e0
    virtual void BltDirtyEx(CDDrawSurfacePair* a, CDDrawSurfacePair* b, i32 c)
        OVERRIDE; // slot 13 0x166910
    virtual void BltDirtyRegions(CDDrawSurfacePair* a, CDDrawSurfacePair* b, i32 c)
        OVERRIDE; // slot 14 0x166950
    // slot 15 INHERITED from A (retail table: 0x150a70).

    void Clear();                        // 0x166810 (destroy m_1dc list + RemoveAll)
    i32 AddChild(CGameObject* child);    // 0x1667e0
    i32 RemoveChild(CGameObject* child); // 0x166850
    i32 WalkChildWorkers();              // 0x166880 (per-child worker cb + count)
    // The child-object factory pair (bodies in WwdGameObjectRender.cpp; the ex-CWwdObjMgrL
    // view is dissolved): build a child CWwdGameObjectA and publish it into m_1dc.
    CWwdGameObject* CreateObject(int a1, int a2, int a3, int a4, int a5, int a6); // 0x166640
    CWwdGameObject*
    CreateNamed(int a1, int a2, int a3, int a4, const char* name, int a6); // 0x166780

    CObList m_1dc; // +0x1dc  real MFC CObList (0x1c bytes; head @ +0x1e0 = m_pNodeHead;
                   // AddTail/RemoveAt = 0x1b5af6/0x1b5c2c; member dtor = ~CObList 0x1b5a2b)
    i32 m_1f8;     // +0x1f8
};
SIZE(0x1fc);

class CWwdGameObjectF : public CGameObject {
public:
    virtual ~CWwdGameObjectF() OVERRIDE; // slot 1  0x15bad0 (out-of-line, I obj)
    virtual i32 IsLoaded() OVERRIDE;     // slot 5  @0x15ba40 (own copy of E's)
    // slot 7 override: the E release pass (a full inline copy in retail).
    // INLINE so ~F folds it; the out-of-line copy lands at 0x15bc50.
    RVA(0x0015bc50, 0x7c)
    virtual void Unload() OVERRIDE {
        CGameObject::Unload(); // a full inline copy of the E pass in retail
    }
    virtual i32 GetClassId() OVERRIDE;                    // slot 8  @0x15ba60 (0x16)
    virtual void Render(CDDrawSurfacePair* ctx) OVERRIDE; // slot 11 @0x15ba70 (ret 4 - empty)
    virtual void BltDirty(CDDrawSurfacePair* a, CDDrawSurfacePair* b) OVERRIDE; // slot 12 @0x15ba80
    virtual void BltDirtyEx(CDDrawSurfacePair* a, CDDrawSurfacePair* b, i32 c)
        OVERRIDE; // slot 13 @0x15ba90
    virtual void BltDirtyRegions(CDDrawSurfacePair* a, CDDrawSurfacePair* b, i32 c)
        OVERRIDE; // slot 14 @0x15baa0
    // slot 16 (new) - the F kind's 2-arg build (the 0x159440 factory's `call
    // [eax+0x40]` pushes two args; body 0x15bc30 == the flat SetupDeferred(a3, a4)).
    virtual i32 SetupDeferred(i32 a3, i32 a4); // slot 16 @0x15bc30 (new)
};
SIZE(0x18c);

class CWwdGameObjectC : public CGameObject {
public:
    virtual ~CWwdGameObjectC() OVERRIDE; // slot 1  0x15c070 (out-of-line, I obj)
    virtual i32 IsLoaded() OVERRIDE;     // slot 5  @0x15c000 (own copy of E's)
    // slot 7 override: clear the dot-color byte then the E release pass.
    // INLINE so ~C folds it; the out-of-line copy lands at 0x15c200.
    RVA(0x0015c200, 0x82)
    virtual void Unload() OVERRIDE {
        m_dotColor = 0;
        CGameObject::Unload();
    }
    virtual i32 GetClassId() OVERRIDE;                    // slot 8  @0x15c020 (6)
    virtual void Render(CDDrawSurfacePair* ctx) OVERRIDE; // slot 11 @0x1660f0 (RenderDot)
    virtual void BltDirty(CDDrawSurfacePair* a, CDDrawSurfacePair* b) OVERRIDE; // slot 12 @0x1661d0
    virtual void BltDirtyEx(CDDrawSurfacePair* a, CDDrawSurfacePair* b, i32 c)
        OVERRIDE; // slot 13 @0x1662a0
    virtual void BltDirtyRegions(CDDrawSurfacePair* a, CDDrawSurfacePair* b, i32 c)
        OVERRIDE; // slot 14 @0x1664a0
    // Slots 16-18 unique to the C variant (0x5effd0 is a 19-slot table).
    // slot 16 - the C kind's 5-arg build (the 0x159250 factory's `call [eax+0x40]`
    // pushes five args; body 0x15c1d0 == the flat SetupFlagged(a1..a4, flag)).
    virtual i32 SetupFlagged(i32 a1, i32 a2, i32 a3, i32 a4, i32 flag); // slot 16 @0x15c1d0
    virtual u8 GetDotColor();        // slot 17 @0x15c030 (`mov al,[this+0x18c]`)
    virtual void SetDotColor(u8 c8); // slot 18 @0x15c040 (byte store to +0x18c)

    u8 m_dotColor; // +0x18c (byte dot color / setup flag)
    char _p18d[0x190 - 0x18d];
};
SIZE(0x190);

class CWwdSlot9c {
public:
    char m_pad00[0x08]; // +0x00..0x07
    i32 m_08;           // +0x08
    i32 m_0c;           // +0x0c
    char m_pad10[0x18 - 0x10];
    i32 m_18;     // +0x18  -> obj+0xb4
    CWwdSlot9c(); // 0x15b2a0
};
SIZE_UNKNOWN();
class CWwdShadowRec { // the E-level shadow dirty-rect block (+0xb8)
public:
    CWwdShadowRec(); // 0x15b270
    char m_pad0[0x8];
    i32 m_8; // abs +0xc0 == CGameObject::m_c0 (INT_MIN sentinel)
    char m_pad0c[0x20 - 0xc];
    i32 m_20; // abs +0xd8 == CGameObject::m_d8 (-1 == disarmed)
};
SIZE_UNKNOWN();
class CWwdSlot9cA { // the A kind's +0x9c sibling record
public:
    CWwdSlot9cA(); // 0x15b2b0
    char m_pad0[0x8];
    i32 m_8;
    i32 m_c;
    char m_pad10[0x18 - 0x10];
    i32 m_18;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_WWD_WWDGAMEOBJECTFAMILY_H
