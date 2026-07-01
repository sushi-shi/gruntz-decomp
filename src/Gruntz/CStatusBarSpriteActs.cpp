// CStatusBarSpriteActs.cpp - CStatusBarSprite's activation-name registration
// (C:\Proj\Gruntz).
//
// CStatusBarSprite : CUserLogic (RTTI vtable 0x5e7fc4; ctor 0x10c230, in
// src/Stub/CStatusBarSprite.cpp). Its per-frame activation handler is bound here
// by RegisterActs (the 0x18d shared-name-registry archetype) into the class's own
// coordinate registry singleton (g_statusBarSpriteActReg @0x64e670 - the slot
// just below the sibling tile-trigger registries in .data). Like CWormholeActs.cpp,
// this lives in a dedicated TU so it can pull the shared
// <Gruntz/ActNameRegistry.h> view of the registry globals without colliding with
// the stub TU's class model.
#include <Gruntz/ActNameRegistry.h> // the shared activation-name registry archetype
#include <Gruntz/UserLogic.h>

#include <rva.h>

class CStatusBarSprite : public CUserLogic {
public:
    CStatusBarSprite(CGameObject* obj); // 0x10c230
    static void InitActReg();           // 0x10c430
    static void RegisterActs();         // 0x10c610
    i32 AdvanceAnim();                  // 0x10c810 (the per-frame handler PMF; body in the stub TU)

    i32 m_40; // +0x40  geometry id (m_38->m_1b4 snapshot)
};
SIZE_UNKNOWN(CStatusBarSprite);

// The handler entry the per-class registry yields: its first dword receives the
// per-frame handler PMF (AdvanceAnim, a 4-byte code ptr on this single-inheritance
// class).
typedef i32 (CStatusBarSprite::*StatusBarSpriteHandler)();
struct CStatusBarSpriteActEntry {
    StatusBarSpriteHandler m_fn;
};
SIZE_UNKNOWN(CStatusBarSpriteActEntry);

// The class's activation-coordinate registry singleton (@0x64e670). Same
// [2000,2010] fixed-range CLeafActReg shape as the sibling tile triggers, built by
// the shared registry ctor (0x408710). ResolveEntry folds the VActLookup archetype
// inline; the slow Insert is __thiscall on m_coll2.
struct CStatusBarSpriteActReg {
    void* m_vptr;       // +0x00
    CActColl2* m_coll2; // +0x04
    i32 m_lo;           // +0x08
    i32 m_hi;           // +0x0c
    char* m_base;       // +0x10
    char* m_cur;        // +0x14
    i32 m_stride;       // +0x18
    char m_pad1c[0x20 - 0x1c];
    i32 m_scratch; // +0x20

    void Construct(i32 lo, i32 hi); // 0x408710 (__thiscall ret 8)

    char* ResolveEntry(i32 id) {
        m_scratch = 0;
        if (id >= m_lo && id <= m_hi) {
            return m_base + (id - m_lo) * m_stride;
        }
        if (((CActColl*)this)->Find(id, 0)) {
            return m_base + (id - m_lo) * m_stride;
        }
        void* item = g_actCache;
        g_actAllocResult = (void*)ActAlloc();
        m_coll2->Insert(this, item, 0xc);
        return m_cur;
    }
};
SIZE_UNKNOWN(CStatusBarSpriteActReg);
DATA(0x0024e670)
extern CStatusBarSpriteActReg g_statusBarSpriteActReg; // 0x64e670

// CStatusBarSprite::CStatusBarSprite @0x10c230 - fold the shared CUserLogic(obj)
// init, name the bound object "GAME_STATUSBARSPRITE", snapshot the geometry id,
// apply the single-image-ani geometry, bind the "A" bute node and lock the draw
// order to 0xf4240.
// @early-stop
// eh-ctor-vptr-restamp-position wall (docs/patterns/eh-ctor-vptr-restamp-position.md):
// body byte-identical; residual is the /GX leaf-vptr re-stamp position + EH-state ids.
RVA(0x0010c230, 0x178)
CStatusBarSprite::CStatusBarSprite(CGameObject* obj) : CUserLogic(obj) {
    m_38->ApplyName("GAME_STATUSBARSPRITE");
    m_40 = m_38->m_1b4;
    m_38->ApplyLookupGeometry("GAME_SINGLEIMAGEANI", 0);
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find(s_actKeyA);
    if (m_10->m_74 != 0xf4240) {
        m_10->m_74 = 0xf4240;
        m_10->m_08 |= 0x20000;
    }
}

// CStatusBarSprite::InitActReg @0x10c430 - construct the class's activation-
// coordinate registry singleton (g_statusBarSpriteActReg @0x64e670) over the fixed
// range [2000, 2010] via the shared registry ctor (0x408710). Free init thunk.
RVA(0x0010c430, 0x15)
void CStatusBarSprite::InitActReg() {
    g_statusBarSpriteActReg.Construct(2000, 2010);
}

// CStatusBarSprite::RegisterActs @0x10c610 - bind the per-frame handler (AdvanceAnim
// @0x10c810) to the activation key "A" via the shared name registry. The SAME
// archetype as CWarpStonePad::RegisterActs, driving the status-bar-sprite registry.
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (every call/immediate/branch/offset + the `mov [entry],offset
// AdvanceAnim` handler store match retail); residual is the slot-vs-id callee-saved
// register choice cascading into the free-loop count materialization. Deferred.
RVA(0x0010c610, 0x18d)
void CStatusBarSprite::RegisterActs() {
    i32 id = (i32)g_buteTree.Find(s_actKeyA);
    if (id == 0) {
        id = g_nextActId;
        g_buteTree.Insert(s_actKeyA, (void*)id);
        char* slot = ActNameLookup(id);
        i32 n = g_nameRegScratch;
        void** list = g_nameRegCurList;
        while (n-- != 0) {
            if (list != 0) {
                ((CActName*)list)->Free();
            }
            list++;
        }
        ((CActName*)slot)->Assign(s_actKeyA);
        g_nextActId++;
    }
    ((CStatusBarSpriteActEntry*)g_statusBarSpriteActReg.ResolveEntry(id))->m_fn =
        &CStatusBarSprite::AdvanceAnim;
}
