// GruntSprintAnim.cpp - BuildGruntSprintAnimation @0x019920 (graduated from
// src/Stub/Backlog.cpp). The per-direction grunt "sprint" animation builder: for
// each of the 8 compass directions it asks the HUD sprite factory
// (g_gameReg->m_30->m_8) for a "SimpleAnimation" sprite, caches its first frame
// from the directional walk-cycle name "GRUNTZ_NORMALGRUNT_<DIR>_WALK", applies
// the "GAME_GRUNTSPRINT" cycle geometry, stamps a few fixed fields (the
// g_gameReg->m_74 sound handle at +0x4c, rate 10 at +0x50, the active flag 1 at
// +0x58) and the per-direction sub-tile origin from the sibling geometry helper
// (PerDirGeometry @0x019cd0). The 8 sprites are stored in this->m_204[0..7].
//
// It is /GX EH-framed: the directional name is assembled through three MFC
// CString temporaries ("GRUNTZ_NORMALGRUNT_" + dir + "_WALK"), whose destructors
// give it the exception frame - so it lives in an `eh` unit.
//
// CARCASS doctrine: the owning collection class (best-guess CGruntSprintAnim; the
// real RTTI owner is reloc-masked anyway) and the g_gameReg sub-objects are
// unmatched engine classes accessed by raw this+offset; every callee is an
// external reloc-masked __thiscall thunk; the GRUNTZ_*/GAME_*/compass strings are
// $SG/$S literals reloc-masked against the matched string symbols. Only the
// offsets / code bytes are load-bearing (campaign doctrine).

#include <Ints.h>
#include <Mfc.h> // CString (the /GX directional-name temps) + Win32

#include <rva.h>

struct CGsSprite;

// The HUD sprite factory reached via g_gameReg->m_30->m_8 (CreateSprite looks the
// template up by class-NAME, the 5th arg; __thiscall ret 0x18). Same shape as the
// IconLoaders factory.
struct CGsSpriteFactory {
    CGsSprite* CreateSprite(
        i32 kind,
        i32 px,
        i32 py,
        i32 hint,
        const char* name,
        i32 flags
    ); // 0x1597b0
};
struct CGsFactoryHolder {
    char m_pad0[0x8];
    CGsSpriteFactory* m_8; // +0x08
};
// The g_gameReg->m_74 lookup table (0xe23c0, thunk 0x4165): Lookup(idx, flag)
// returns the entry at [this + idx*4 + (flag ? 0x4c : 0x08)], 0 if idx >= 0x11.
struct CGsSoundTable {
    void* Lookup(i32 idx, i32 flag); // 0xe23c0
};
struct CGsGameReg {
    char m_pad0[0x30];
    CGsFactoryHolder* m_30; // +0x30  sprite-factory holder
    char m_pad34[0x74 - 0x34];
    CGsSoundTable* m_74; // +0x74  sound/resource lookup table
};
DATA(0x0024556c)
extern CGsGameReg* g_gameReg; // *0x64556c

// The created SimpleAnimation sprite. CacheFirstFrame (0x150540) caches the named
// first frame; ApplyLookupGeometry (0x1505b0) resolves its cycle geometry. Both
// reloc-masked __thiscall.
struct CGsSprite {
    void CacheFirstFrame(const char* name);             // 0x150540
    i32 ApplyLookupGeometry(const char* name, i32 def); // 0x1505b0

    char m_pad0[0x4c];
    void* m_4c; // +0x4c  sound handle
    i32 m_50;   // +0x50  rate
    char m_pad54[0x58 - 0x54];
    i32 m_58; // +0x58  active flag
    i32 m_5c; // +0x5c  sub-tile X
    i32 m_60; // +0x60  sub-tile Y
};

// The owning sprite-collection (best-guess name; RTTI owner reloc-masked).
class CGruntSprintAnim {
public:
    i32 BuildGruntSprintAnimation();                    // 0x019920
    void PerDirGeometry(i32 dir, i32* outX, i32* outY); // 0x019cd0 (thunk 0x3869)

    char m_pad0[0x204];
    CGsSprite* m_204[8]; // +0x204  the 8 directional sprint sprites
};

// ===========================================================================
// BuildGruntSprintAnimation @0x019920
// ===========================================================================
// @early-stop
// regalloc/scheduling wall (~80%): complete + correct, verified instruction-by-
// instruction vs retail. The prologue, /GX exception frame (sub esp,0x18; the EH
// state thread at [esp+0x30]/0x34/0x3c), the g_gameReg->m_74 lookup, the whole
// CString jump-table build ("GRUNTZ_NORMALGRUNT_" + dir + "_WALK"), the two
// operator+ temps, both CacheFirstFrame/ApplyLookupGeometry receivers, and the
// this=ebp / array=esi induction all match byte-for-byte (modulo reloc names).
// Residual: in the {m_58,m_50,m_4c} field-store block retail caches the sprite
// pointer in eax across the three stores (one `mov eax,[esi]`), while this /O2
// recompile re-reads m_204[i-1] (=[esi]) per store because the store to
// spr->m_58 may alias the array slot - MSVC's reload-vs-cache choice here is a
// register-pressure artifact (all four callee-saved regs are pinned to
// this/array/const-1/counter, so `spr` is memory-homed; homing it in a `spr`
// local instead flips array<->spr and regresses to ~73%). Plus the inline
// jump-table base (jmpl *0x1c4(,%eax,4) vs our $L DIR32 reloc) and the loop-bound
// strength-reduction form ((i-1)<8 vs i<=8) are documented scoring/scheduling
// walls. See docs/patterns/pin-local-for-callee-saved-reg.md +
// jumptable-data-overlap.md + zero-register-pinning.md.
RVA(0x00019920, 0x1c2)
i32 CGruntSprintAnim::BuildGruntSprintAnimation() {
    void* h = g_gameReg->m_74->Lookup(0, 0);
    if (!h) {
        return 0;
    }

    for (i32 i = 1; i <= 8; i++) {
        m_204[i - 1] = g_gameReg->m_30->m_8->CreateSprite(0, 0, 0, 2, "SimpleAnimation", 3);
        if (m_204[i - 1] == 0) {
            return 0;
        }

        CString dir;
        switch (i - 1) {
            case 0:
                dir = "NORTH";
                break;
            case 1:
                dir = "NORTHEAST";
                break;
            case 2:
                dir = "EAST";
                break;
            case 3:
                dir = "SOUTHEAST";
                break;
            case 4:
                dir = "SOUTH";
                break;
            case 5:
                dir = "SOUTHWEST";
                break;
            case 6:
                dir = "WEST";
                break;
            case 7:
                dir = "NORTHWEST";
                break;
        }

        m_204[i - 1]->CacheFirstFrame("GRUNTZ_NORMALGRUNT_" + dir + "_WALK");
        m_204[i - 1]->ApplyLookupGeometry("GAME_GRUNTSPRINT", 0);
        m_204[i - 1]->m_58 = 1;
        m_204[i - 1]->m_50 = 0xa;
        m_204[i - 1]->m_4c = h;

        i32 outX, outY;
        PerDirGeometry(i, &outX, &outY);
        m_204[i - 1]->m_5c = outX;
        m_204[i - 1]->m_60 = outY;
    }
    return 1;
}

// class-metadata sweep: grunt/game-object family size annotations (SIZE_UNKNOWN = retail size TBD, at .cpp EOF).
SIZE_UNKNOWN(CGruntSprintAnim);
SIZE_UNKNOWN(CGsFactoryHolder);
SIZE_UNKNOWN(CGsGameReg);
SIZE_UNKNOWN(CGsSoundTable);
SIZE_UNKNOWN(CGsSprite);
SIZE_UNKNOWN(CGsSpriteFactory);
