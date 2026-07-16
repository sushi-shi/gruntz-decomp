// GruntPowerupSprite.cpp - the "grunt has a powerup" indicator sprite
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
#include <Gruntz/SerialObjRef.h>
#include <Gruntz/GameRegPtr.h>
#include <Io/FileMem.h> // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Gruntz/MovingLogicBase.h> // CMovingLogicBase::Serialize (0x16e7f0) - shared serialize chain
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/GruntPowerupSprite.h>
#include <Gruntz/LightFxMgr.h> // CLightFxMgr (g_gameReg->m_logicPump @+0x78; m_tables[])
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
#include <Gruntz/Grunt.h> // CGrunt - the registry grunt-table slot (was the CGruntEntry view)
#include <Gruntz/TypeKeyColl.h> // the REAL registry class at 0x6bf650 (its fields were the shredded g_type* globals)

// DATA-bind the class registry singleton in the main_file .cpp (labels.py scans
// DATA() only in TU source, not headers).
DATA(0x00244d30)
CIndicatorActReg g_powerupActReg; // 0x644d30

// The (de)serialization archive is the shared CSerialArchive (Read @ +0x2c / Write
// @ +0x30), pulled in via the header - the former per-TU PupArchive view is folded.

// The serializable sub-object overlaid at CUserLogic+0x34; its own serializer is
// reached as `lea ecx,[this+0x34]; call` through the 0x1aff thunk (the SAME helper
// the in-game-text leaf uses). External; no body.

// ~CGruntPowerupSprite @0x012370 - the CUserLogic-folded /GX leaf dtor.
RVA(0x00012370, 0x44)
CGruntPowerupSprite::~CGruntPowerupSprite() {}

// --- CGruntPowerupSprite (0x07fdb0), vptr 0x5e76c4 --- the ctor anchors the
// ??_7CGruntPowerupSprite vtable in this TU. Folds the inline CUserLogic(obj) base.
RVA(0x0007fdb0, 0x166)
CGruntPowerupSprite::CGruntPowerupSprite(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_38->ApplyName("GAME_LIGHTING_POWERUP");
    m_geoId = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);
    if (m_object->m_latchedAnimId != 0x15) {
        m_object->m_latchedAnimId = 0x15;
        m_object->m_flags |= 0x20000;
    }
    m_38->m_stateFlags |= 1;
}

// CGruntPowerupSprite::InitActReg @0x07ffa0 - construct the class's activation-
// coordinate registry singleton (g_powerupActReg @0x644d30) over [2000, 2010]
// via the shared registry ctor (FUN_00408710). Free init thunk; reloc-masked.
RVA(0x0007ffa0, 0x15)
void CGruntPowerupSprite::InitActReg() {
    ((CZDArrayDerived*)&g_powerupActReg)->Construct(2000, 2010);
}

// CGruntPowerupSprite::RunAct @0x080020 - resolve the coordinate-registry entry for `id`
// (inline CActReg::ResolveEntry) and, if it holds a registered handler PMF, re-resolve the
// entry and dispatch the PMF on `this`. Two inline ResolveEntry expansions (side effects,
// no CSE across the guard).
RVA(0x00080020, 0x102)
void CGruntPowerupSprite::RunAct(i32 id) {
    if (((CPowerupActEntry*)g_powerupActReg.ResolveEntry(id))->m_fn != 0) {
        (this->*((CPowerupActEntry*)g_powerupActReg.ResolveEntry(id))->m_fn)();
    }
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
    i32 id = (i32)g_buteTree.Find(s_codeA);
    if (id == 0) {
        id = g_typeCounter;
        g_buteTree.Insert(s_codeA, (void*)id);
        char* slot = ActNameLookup(id);
        i32 n = g_typeColl.m_grown;
        void** list = (void**)g_typeColl.m_alloc;
        while (n-- != 0) {
            if (list != 0) {
                ((CString*)list)->CString::~CString();
            }
            list++;
        }
        ((CString*)slot)->operator=(s_codeA);
        g_typeCounter++;
    }
    ((CPowerupActEntry*)g_powerupActReg.ResolveEntry(id))->m_fn = &CGruntPowerupSprite::Update;
}

// SetCell @0x080380 - stash the grunt cell (x,y) and powerup id, seed the bound
// renderable's display fields (visible=1, state=7, bute-set record from the
// registry's +0x78 table indexed by the powerup id), clear bit 0 of the +0x38
// game object's flags, then point the +0x14 aux's bute node at the "A" node
// (saving the old node into m_prevAnimSetNode). Returns 1.
RVA(0x00080380, 0x6c)
i32 CGruntPowerupSprite::SetCell(i32 x, i32 y, i32 powerup) {
    m_cellX = x;
    m_cellY = y;
    m_powerupId = powerup;
    i32 rec = (i32)g_gameReg->m_logicPump->m_tables[powerup];
    CGameObject* r = m_object;
    r->m_drawActive = 1;
    r->m_drawFillCmd = 7;
    r->m_drawFillArg = rec;
    m_38->m_stateFlags &= ~1;
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    return 1;
}

// Update @0x080410 - sync the +0x38 object's helper, then if the grunt for cell
// (m_cellX,m_cellY) is present copy its screen position into the bound renderable so
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
    m_38->m_1a0.Advance(g_engineFrameDelta);
    CGrunt* e = ((CGrunt**)((char*)g_gameReg->m_cmdGrid + 0x1c))[m_cellX * 15 + m_cellY];
    if (e != 0) {
        m_object->m_screenX = e->m_object->m_screenX;
        m_object->m_screenY = e->m_object->m_screenY;
    }
    return 0;
}

// CGruntPowerupSprite::Serialize @0x080490 - the serialize override. Chain the base
// CUserLogic::SerializeChain and the +0x34 sub-object, then round-trip the own state:
// m_cellX/m_cellY (8 B) + m_powerupId (4 B). mode 4 = write, mode 7 = read. On read, re-resolve
// the powerup's bute-set record (g_gameReg->m_78[m_powerupId*4 + 0x14]) into the bound renderable.
RVA(0x00080490, 0xbe)
i32 CGruntPowerupSprite::Serialize(CSerialArchive* ar, i32 mode, i32 a3, i32 a4) {
    if (((CMovingLogicBase*)this)->Serialize((CSerialArchive*)((i32)ar), mode, a3, a4) == 0) {
        return 0;
    }
    if (((CSerialObjRef*)&m_34)->Chain(ar, mode, a3, (CGameObject*)a4) == 0) {
        return 0;
    }
    switch (mode) {
        case 4:
            ar->Write(&m_cellX, 8);
            ar->Write(&m_powerupId, 4);
            break;
        case 7: {
            ar->Read(&m_cellX, 8);
            ar->Read(&m_powerupId, 4);
            i32 id = m_powerupId;
            CGameObject* r = m_object;
            i32 v = (i32)g_gameReg->m_logicPump->m_tables[id];
            r->m_drawActive = 1;
            r->m_drawFillArg = v;
            r->m_drawFillCmd = 7;
            break;
        }
    }
    return 1;
}

SIZE_UNKNOWN(CGruntPowerupSprite);
SIZE_UNKNOWN(CPowerupActEntry);
SIZE_UNKNOWN(PupSubObj);
