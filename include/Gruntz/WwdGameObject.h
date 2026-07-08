#ifndef GRUNTZ_WWDGAMEOBJECT_H
#define GRUNTZ_WWDGAMEOBJECT_H

#include <Ints.h>
#include <rva.h>
#include <Mfc.h> // real MFC CObject (the object's grand-base) + CObList (m_subList @+0x1dc)

// CWwdGameObject - a runtime "plane object" deserialized from WWD level data.
// WwdFile::ReadPlaneObjects (0x162af0) constructs one per record via the ctor
// at 0x15b390 (NOT reconstructed here - it lives in the eh-frame ctor TU). The
// object owns a sprite-animation worker at +0x7c (0x17c-byte, the same family
// as CDDrawWorkerCache's WwdAnimWorker, foreign vtable g_*Vtbl), a small
// command-dispatch sub-object at +0x1a0, and a back-pointer to its owning
// manager at +0x0c.
//
// Class identity is a role inference (no RTTI on the vtable @0x5f0020); only the
// this-OFFSETS and emitted code bytes are load-bearing (campaign doctrine), so
// field names are placeholders m_<hexoffset>.

// The owning manager reached through CWwdGameObject+0x0c. Its methods are
// reached as [[+0xc]+slot] - modeled as a typed vtable struct so the dispatch
// lowers to the exact `mov eax,[mgr+slot]; call` with no cast.
struct WwdMgr;

// Forward of the WwdAnimWorker+0x18 sub-object interface (defined in the .cpp); the
// worker's m_18 slot is typed as this so the [m_18][+0x8] dispatch needs no cast.
class WorkerSub;

// The animation/sprite worker at CWwdGameObject+0x7c. Foreign vtable; its
// virtuals are DECLARED only (never defined) so cl emits no ??_7 - the real
// vtable is the engine datum the ctor stamps. Declared as a polymorphic class
// so the virtual dispatch lowers to the exact `mov ecx,worker; call [vtbl+off]`
// __thiscall sequence (virtuals are __thiscall by default in MSVC 5.0).
SIZE_UNKNOWN(WwdAnimWorker);
class WwdAnimWorker {
public:
    virtual void Slot00();             // +0x00
    virtual void Slot04();             // +0x04
    virtual void Slot08();             // +0x08
    virtual void Slot0C();             // +0x0c
    virtual void Advance(void* owner); // +0x10
    virtual void Slot14();             // +0x14
    virtual void Slot18();             // +0x18
    virtual void Slot1C();             // +0x1c
    virtual void Slot20();             // +0x20
    virtual i32 Init(i32 a1, i32 a3);  // +0x24

    // Non-virtual play-step helper (0x164830, __thiscall, 4 args) Play tail-calls.
    i32 QueryWorkerType(i32 a1, i32 type, i32 a3, i32 a4);

    i32 m_04;
    i32 m_08; // +0x08  flag bits (bit0/bit1 read by Setup)
    char m_pad0c[0x18 - 0x0c];
    WorkerSub* m_18; // +0x18  sub-object with its own vtable ([m_18][+0x8] called)
    i32 m_1c;        // +0x1c  scratch state id (saved/forced 0x50..0x53/restored)
};

// The +0x1a0 command-dispatch sub-object. Ctor 0x15c290 (1 arg = owner),
// Find 0x15c900 (4 args). A tiny class so the __thiscall dispatch lowers to
// `lea ecx,[this+0x1a0]; call` with no cast.
SIZE(CmdMap, 0x3c);
struct CmdMap {
    char m_body[0x3c]; // embedded sub-object body (+0x1a0..+0x1dc)
};

// (The +0x1dc member is the real MFC CObList, folded below - the former
// WwdSubList/WwdSubNode/WwdSubDel views are dissolved. ResetAndSetup walks it with
// the real CObList::GetHeadPosition/GetNext + `delete` on each MFC CObject payload.)

// The owning manager (+0x0c) and the render context (RenderDot's arg) are typed
// engine objects whose full layouts live in WwdGameObject.cpp (the only TU that
// walks them); pointer members here need only forward declarations.
struct WwdMgr;
struct WwdRenderCtx;

// ---------------------------------------------------------------------------
// CWwdGameObject - the canonical runtime plane object (raw-offset access; only
// offsets are load-bearing). It DERIVES from the real MFC CObject (the WAP engine
// statically links MFC and uses CObject as its game-object grand-base): PROVEN by
// ??_7CObject@0x1e8cb4 whose slots 0/2/3/4 (0x1bef01 GetRuntimeClass / 0x0028ec
// Serialize / 0x00106e AssertValid / 0x00404034 Dump) are EXACTLY this object's
// manual-table slots 0/2/3/4, with slot 1 the dtor override (0x15b4c0). So slots
// 0-4 are inherited from CObject and only slots 5-16 are the derived's own (below).
// The table (?g_wwdGameObjectVtbl@@3PAXA, VA 0x5f0020, read from .rdata) holds
// NON-virtual method addresses the engine hand-placed (retail calls Play/Setup/
// Helper164790 DIRECTLY via rel32), so those stay plain methods; the 12 declared-only
// virtuals model only the table SHAPE so WriteSnapshot dispatches slot 8/16 through
// the real vptr with no cast. Slot RVAs are the binary's ground truth (never
// fabricated); unrecovered slots carry @rva. (Slot 1, the scalar-deleting dtor
// override @0x15b4c0, is realized by the /GX CWwdGameObjectE sibling in this TU.)
// ---------------------------------------------------------------------------
class CWwdGameObject : public CObject {
public:
    // slots 0-4 inherited from CObject; slots 5-16 are CWwdGameObject's own:
    virtual void Slot14();      // slot 5  @0x15b370  (worker-gate; reads [this+0x7c])
    virtual void Slot18();      // slot 6  @0x001c08 -> 0xd5da0  (unrecovered engine method)
    virtual void ReleaseSubs(); // slot 7  @0x15b5d0  ReleaseSubs_15b5d0
    virtual i32 Vfunc20();      // slot 8  @0x154a00  (xor eax,eax;ret) read by WriteSnapshot
    virtual i32 Slot24();       // slot 9  @0x164790  == Helper164790 (below; foreign owner)
    virtual i32 Slot28();       // slot 10 @0x150d60  == Setup (below)
    virtual void Slot2C();      // slot 11 @0x11fec0  __purecall
    virtual void Slot30();      // slot 12 @0x11fec0  __purecall
    virtual void Slot34();      // slot 13 @0x11fec0  __purecall
    virtual void Slot38();      // slot 14 @0x11fec0  __purecall
    virtual i32 Slot3C();       // slot 15 @0x151150  == Play (below)
    virtual i32
    Vfunc40(); // slot 16 @0x1bef01  const-getter (== inherited slot 0) read by WriteSnapshot

    // Dispatch entry (0x150a70) and the methods it routes to.
    i32 Dispatch(i32 a1, i32 type, i32 a3, i32 a4);             // 0x150a70
    i32 ReadState(i32 src);                                     // 0x150b00
    i32 Setup(i32 a1, i32 a2, i32 a3, i32 a4);                  // 0x150d60 (vtbl +0x28)
    i32 Play(i32 a1, i32 type, i32 a3, i32 a4);                 // 0x151150 (vtbl +0x3c)
    i32 Serialize(i32 ar);                                      // 0x151320
    i32 WriteSnapshot(i32 dst);                                 // 0x151c00
    i32 Init(i32 a1, i32 a2, i32 a3, i32 a4);                   // 0x15b940
    i32 ResetAndSetup(i32 a1, i32 a2, i32 a3, i32 a4);          // 0x1665e0
    RVA(0x0015c1d0, 0x26)
    i32 SetupFlagged(i32 a1, i32 a2, i32 a3, i32 a4, i32 flag) {
        *(char*)&m_dotColor = (char)flag;
        return Setup(a1, a2, a3, a4);
    }
    RVA(0x0015bc30, 0x16)
    i32 SetupDeferred(i32 a3, i32 a4) {
        return Setup(0, 0, a3, a4);
    }
    void RenderDot(WwdRenderCtx* a);                            // 0x1660f0

    // Sibling helpers (modeled as same-class methods so ecx=this matches).
    i32 Helper164790(i32 a2, i32 a1); // 0x164790  __thiscall
    i32 Sub150c30(i32 a1);            // 0x150c30
    i32 Sub151780(i32 a1);            // 0x151780

    // The three "resolve object reference" setters Sub151780 dispatches the
    // deserialized name lookups into (sibling __thiscall methods, reloc-masked).
    i32 Resolve150eb0(void* obj); // 0x150eb0
    i32 Resolve150f90(void* obj); // 0x150f90
    i32 Resolve151070(void* obj); // 0x151070

    // +0x00 is the CObject base vptr (the 17-slot table); m_04 at +0x04.
    i32 m_04;         // +0x04
    i32 m_flags;      // +0x08  bit flags (|=0x800000 / 0x1000000)
    WwdMgr* m_mgr;    // +0x0c  owning manager
    i32 m_10;         // +0x10
    i32 m_14;         // +0x14
    i32 m_lastX;      // +0x18  last-drawn column (cached by RenderDot)
    i32 m_lastY;      // +0x1c  last-drawn row
    i32 m_20;         // +0x20
    i32 m_24;         // +0x24
    i32 m_28;         // +0x28
    i32 m_2c;         // +0x2c
    i32 m_30;         // +0x30  set 1 on a successful plot
    i32 m_34;         // +0x34  set 1 on a successful plot
    i32 m_clipResult; // +0x38  clip result (0 plotted / -1 rejected)
    char m_pad3c[0x40 - 0x3c];
    i32 m_40; // +0x40
    i32 m_44; // +0x44
    i32 m_48; // +0x48
    char m_pad4c[0x50 - 0x4c];
    i32 m_50;         // +0x50
    i32 m_54;         // +0x54
    i32 m_58;         // +0x58
    i32 m_posX;       // +0x5c  position column
    i32 m_posY;       // +0x60  position row
    i32 m_clipLeft;   // +0x64  clip rect (0x80000000 = unbounded)
    i32 m_clipTop;    // +0x68
    i32 m_clipRight;  // +0x6c
    i32 m_clipBottom; // +0x70
    i32 m_74;         // +0x74
    char m_pad78[0x7c - 0x78];
    WwdAnimWorker* m_worker; // +0x7c  sprite-animation worker
    void* m_80;              // +0x80  object ref (serialized by name)
    i32 m_84;                // +0x84
    void* m_88;              // +0x88  object ref
    i32 m_8c;                // +0x8c
    void* m_90;              // +0x90  object ref
    i32 m_94;                // +0x94
    void* m_98;              // +0x98  linked object (reads its +0x188)
    char m_pad9c[0xac - 0x9c];
    i32 m_ac;               // +0xac  copy of m_posX
    i32 m_b0;               // +0xb0  copy of m_posY
    CWwdGameObject* m_self; // +0xb4  = this
    char m_b8[0x24];        // +0xb8  serialized state block
    char* m_name;           // +0xdc  CString name (handle = buffer pointer)
    i32 m_e0;               // +0xe0
    i32 m_e4;               // +0xe4
    i32 m_e8;               // +0xe8
    i32 m_ec;               // +0xec
    i32 m_f0;               // +0xf0
    i32 m_f4;               // +0xf4
    i32 m_f8;               // +0xf8
    i32 m_fc;               // +0xfc
    i32 m_100;              // +0x100
    i32 m_104;              // +0x104
    i32 m_108;              // +0x108
    i32 m_10c;              // +0x10c
    i32 m_110;              // +0x110
    i32 m_114;              // +0x114
    i32 m_118;              // +0x118
    i32 m_11c;              // +0x11c
    i32 m_120;              // +0x120
    i32 m_124;              // +0x124
    i32 m_128;              // +0x128
    i32 m_12c;              // +0x12c
    i32 m_130;              // +0x130
    i32 m_134;              // +0x134  block head (0x80000000 sentinel)
    i32 m_138;              // +0x138
    i32 m_13c;              // +0x13c
    i32 m_140;              // +0x140
    i32 m_144;              // +0x144  block head (0x80000000 sentinel)
    i32 m_148;              // +0x148
    i32 m_14c;              // +0x14c
    i32 m_150;              // +0x150
    i32 m_154;              // +0x154  block head (0x80000000 sentinel)
    i32 m_158;              // +0x158
    i32 m_15c;              // +0x15c
    i32 m_160;              // +0x160
    i32 m_164;              // +0x164
    i32 m_168;              // +0x168
    i32 m_16c;              // +0x16c
    i32 m_170;              // +0x170
    i32 m_174;              // +0x174
    i32 m_178;              // +0x178
    i32 m_17c;              // +0x17c
    i32 m_180;              // +0x180
    i32 m_184;              // +0x184
    i32 m_188;              // +0x188
    i32 m_dotColor;         // +0x18c  low byte = dot color / setup flag
    i32 m_190;              // +0x190
    void* m_194;            // +0x194  resolved object ref
    i32 m_198;              // +0x198
    void* m_19c;            // +0x19c  resolved object ref
    CmdMap m_cmdMap;        // +0x1a0  command-dispatch sub-object
    CObList m_subList;      // +0x1dc  MFC CObList of owned sub-objects
};

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---

#endif // GRUNTZ_WWDGAMEOBJECT_H
