// CGruntPowerupSprite.cpp - the "grunt has a powerup" indicator sprite
// (C:\Proj\Gruntz). A CUserLogic-derived game object; methods in ascending
// retail-RVA order:
//   ~CGruntPowerupSprite  @0x012370 - the /GX leaf dtor (CUserLogic teardown).
//   SetCell               @0x080380 - stash the cell + powerup id, bind the bute
//                                     sprite, return 1.
//   Update                @0x080410 - sync + track the grunt's screen position.
//
// The 0x44 is a DESTRUCTOR (stamps CUserLogic 0x5e705c then CUserBase 0x5e70b4,
// tears down the +0x18 link via ~EngStr @0x16d2a0), NOT a ctor - identical in
// shape to ~CTimeBomb @0x012a70.
#include <Gruntz/CGruntPowerupSprite.h>

// ~CGruntPowerupSprite @0x012370 - the CUserLogic-folded /GX leaf dtor.
RVA(0x00012370, 0x44)
CGruntPowerupSprite::~CGruntPowerupSprite() {}

// CGruntPowerupSprite::InitActReg @0x07ffa0 - construct the class's activation-
// coordinate registry singleton (g_powerupActReg @0x644d30) over [2000, 2010]
// via the shared registry ctor (FUN_00408710). Free init thunk; reloc-masked.
RVA(0x0007ffa0, 0x15)
void CGruntPowerupSprite::InitActReg() {
    g_powerupActReg.Construct(2000, 2010);
}

// CGruntPowerupSprite::RegisterActs @0x080180 - bind the class's per-frame handler
// (Update @0x080410) to the activation key "A" (the SAME activation-name-intern
// archetype as CGruntHealthSprite::RegisterActs; see that TU for the full notes).
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (every call/immediate/branch/offset + the `mov [entry],offset
// Update` handler store match retail); residual is the slot-vs-id callee-saved
// register choice cascading into the free-loop count materialization. Deferred.
RVA(0x00080180, 0x18d)
void CGruntPowerupSprite::RegisterActs() {
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
    ((CPowerupActEntry*)g_powerupActReg.ResolveEntry(id))->m_fn = &CGruntPowerupSprite::Update;
}

// SetCell @0x080380 - stash the grunt cell (x,y) and powerup id, seed the bound
// renderable's display fields (visible=1, state=7, bute-set record from the
// registry's +0x78 table indexed by the powerup id), clear bit 0 of the +0x38
// game object's flags, then point the +0x14 aux's bute node at the "A" node
// (saving the old node into m_30). Returns 1.
RVA(0x00080380, 0x6c)
i32 CGruntPowerupSprite::SetCell(i32 x, i32 y, i32 powerup) {
    m_54 = x;
    m_58 = y;
    m_5c = powerup;
    i32 rec = *(i32*)(g_gameReg->m_78 + powerup * 4 + 0x14);
    CGruntRenderable* r = (CGruntRenderable*)m_10;
    r->m_58 = 1;
    r->m_50 = 7;
    r->m_4c = rec;
    m_38->m_40 &= ~1;
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find("A");
    return 1;
}

// Update @0x080410 - sync the +0x38 object's helper, then if the grunt for cell
// (m_54,m_58) is present copy its screen position into the bound renderable so
// the powerup icon tracks the grunt. Returns 0.
//
// @early-stop
// regalloc/scheduling wall (zero-register-pinning class): the logic is byte-exact
// but cl loads g_gameReg into a different register (edx vs retail's eax) AFTER the
// index add where retail schedules it between the lea-chain and the add, cascading
// a register-name swap through the indexed entry load - not source-steerable.
// Every instruction matches modulo register names. Deferred to the final sweep.
RVA(0x00080410, 0x51)
i32 CGruntPowerupSprite::Update() {
    ((CIndicatorSyncHelper*)((char*)m_38 + 0x1a0))->Sync(g_indicatorSync);
    CGruntEntry* e = ((CGruntEntry**)(g_gameReg->m_68 + 0x1c))[m_54 * 15 + m_58];
    if (e != 0) {
        m_10->m_5c = e->m_10->m_5c;
        m_10->m_60 = e->m_10->m_60;
    }
    return 0;
}
