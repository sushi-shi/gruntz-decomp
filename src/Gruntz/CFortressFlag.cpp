// CFortressFlag.cpp - the fortress-flag game-object (C:\Proj\Gruntz).
//
// Two trace-discovered CFortressFlag methods, defined in ascending retail-RVA
// order:
//   ~CFortressFlag    @0x010e90 - the /GX leaf dtor (folds the CUserLogic teardown).
//   Serialize         @0x046410 - the Serialize override (chain + the tag-8 fixup).
//
// CFortressFlag : CUserLogic (the base hierarchy comes from <Gruntz/UserLogic.h>).
// Only offsets / code bytes are load-bearing; names are placeholders for the
// recovered engine identities.
#include <Gruntz/CFortressFlag.h>

// The +0x34 serializable sub-object Serialize chains into after the shared
// CUserLogic::SerializeChain (the SAME archetype as CSecretTeleporterTrigger::
// Serialize, UserLogic.cpp 0x010a10). Its Chain (0x8c00, via the 0x1aff thunk)
// is __thiscall ret 0x10; modeled NO-body so the call reloc-masks.
struct CSerialSub34 {
    i32 Chain(i32 a, i32 b, i32 c, i32 d); // 0x8c00
};

// The bound sprite/game-object (this->m_10). The tag-8 fixup reads its +0x124
// sprite-selector key and re-seeds the +0x4c/+0x50/+0x58 state trio. Modeled as
// a typed view of the same object the CUserLogic base holds at m_10.
struct CFlagSprite {
    char m_pad0[0x4c];
    i32 m_4c; // +0x4c  selected sprite handle (written from GetSel)
    i32 m_50; // +0x50  state (set to 0xa)
    char m_pad54[0x58 - 0x54];
    i32 m_58; // +0x58  state flag (set to 1)
    char m_pad5c[0x124 - 0x5c];
    i32 m_124; // +0x124 sprite-selector row key
};

// The level sprite-ref table (g_gameReg->m_74). GetSel(i, bAlt) (0xe23c0) returns
// the selected sprite handle for ref-row i; modeled NO-body so the call
// reloc-masks (the body lives in src/Gruntz/SpriteRefTable.cpp).
struct CSpriteRefTable {
    i32 GetSel(i32 i, i32 bAlt); // 0xe23c0
};

// One ref-index array slot: an 8-byte entry whose first dword is the ref-row
// index. The fixup indexes the array at g_gameReg+0x158 by (selector-row * 71)
// (retail: lea+shl+sub = *71, then the *8 element stride folds into the [...*8]
// addressing-mode scale), so the row multiply is materialized but the *8 is free.
struct WwdRefSlot {
    i32 m_idx; // +0x00  ref-row index (passed to GetSel)
    i32 m_04;  // +0x04
};

// The global game registry (WwdGameReg, RVA 0x24556c; wwdfile owns the DATA
// label). The tag-8 fixup reads the ref-index array at +0x158 and the level
// sprite-ref table at +0x74. Only the touched fields are modeled.
struct WwdGameReg {
    char m_pad0[0x74];
    CSpriteRefTable* m_74; // +0x74  level sprite-ref table
    char m_pad78[0x158 - 0x78];
    WwdRefSlot m_158[1]; // +0x158 base of the ref-index array
};
DATA(0x0024556c)
extern WwdGameReg* g_gameReg;

// CFortressFlag::~CFortressFlag @0x010e90 - the leaf adds no destructible members
// beyond CUserLogic, so its dtor folds the bare CUserLogic teardown: store the
// CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame. Byte-identical in shape to
// ~CSecretTeleporterTrigger @0x010ab0; the empty body is enough for cl.
RVA(0x00010e90, 0x44)
CFortressFlag::~CFortressFlag() {}

// CFortressFlag::Serialize @0x046410 - chain the shared CUserLogic serialize
// helper on `this`, and (only on success) the +0x34 sub-object's chain; both run
// the same (ar, tag, c, d) tuple. On the post-load tag (tag == 8), look the
// flag's sprite selector (m_10->m_124 * 71) up in g_gameReg's ref-index array,
// resolve it through the level sprite-ref table, and re-seed the bound sprite's
// state trio. Always returns 1 once the two chains succeed.
RVA(0x00046410, 0x92)
i32 CFortressFlag::Serialize(i32 ar, i32 tag, i32 c, i32 d) {
    if (!SerializeChain(ar, tag, c, d)) {
        return 0;
    }
    if (!((CSerialSub34*)((char*)this + 0x34))->Chain(ar, tag, c, d)) {
        return 0;
    }
    if (tag == 8) {
        CFlagSprite* spr = (CFlagSprite*)m_10;
        i32 idx = g_gameReg->m_158[spr->m_124 * 71].m_idx;
        i32 sel = g_gameReg->m_74->GetSel(idx, 0);
        spr = (CFlagSprite*)m_10;
        spr->m_58 = 1;
        spr->m_50 = 0xa;
        spr->m_4c = sel;
    }
    return 1;
}
