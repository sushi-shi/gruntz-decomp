#include <Gruntz/SerialArchive.h> // CSerialArchive (the inherited CWapX::Chain arg; ex SerialObjRef.h)
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Io/FileMem.h> // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/GruntPowerupSprite.h>
#include <Gruntz/LightFxMgr.h> // CLightFxMgr (g_gameReg->m_logicPump @+0x78; m_tables[])
#include <Wap32/ZVec.h>
#include <Gruntz/Grunt.h> // CGrunt - the registry grunt-table slot (was the CGruntEntry view)
#include <Gruntz/TypeKeyColl.h> // the REAL registry class at 0x6bf650 (its fields were the shredded g_type* globals)

DATA(0x00244d30)
extern CIndicatorActReg g_powerupActReg; // 0x644d30

// ~CGruntPowerupSprite @0x012370 - the CUserLogic-folded /GX leaf dtor.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CGruntPowerupSprite() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
// @rva-symbol: ??1CGruntPowerupSprite@@UAE@XZ 0x00012370 0x44

RVA(0x0007fdb0, 0x166)
CGruntPowerupSprite::CGruntPowerupSprite(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_38->ApplyName("GAME_LIGHTING_POWERUP");
    m_value = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);
    if (m_object->m_sortKey != 0x15) {
        m_object->m_sortKey = 0x15;
        m_object->m_flags |= 0x20000;
    }
    m_38->m_stateFlags |= 1;
}

RVA(0x0007ffa0, 0x15)
void CGruntPowerupSprite::InitActReg() {
    g_powerupActReg.Construct(2000, 2010);
}

RVA(0x00080020, 0x102)
void CGruntPowerupSprite::FireActivation(i32 id) {
    if ((reinterpret_cast<CPowerupActEntry*>(g_powerupActReg.ResolveEntry(id)))->m_fn != 0) {
        (this->*(reinterpret_cast<CPowerupActEntry*>(g_powerupActReg.ResolveEntry(id)))->m_fn)();
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
    i32 id = reinterpret_cast<i32>(g_buteTree.Find("A"));
    if (id == 0) {
        id = g_typeCounter;
        g_buteTree.Insert("A", reinterpret_cast<void*>(id));
        char* slot = ActNameLookup(id);
        i32 n = g_typeColl.m_grown;
        void** list = reinterpret_cast<void**>(g_typeColl.m_alloc);
        while (n-- != 0) {
            if (list != 0) {
                (reinterpret_cast<CString*>(list))->CString::~CString();
            }
            list++;
        }
        (reinterpret_cast<CString*>(slot))->operator=("A");
        g_typeCounter++;
    }
    (reinterpret_cast<CPowerupActEntry*>(g_powerupActReg.ResolveEntry(id)))->m_fn = static_cast<i32 (CUserLogic::*)()>(&CGruntPowerupSprite::Update);
}

RVA(0x00080380, 0x6c)
i32 CGruntPowerupSprite::SetCell(i32 x, i32 y, i32 powerup) {
    m_cellX = x;
    m_cellY = y;
    m_powerupId = powerup;
    i32 rec = reinterpret_cast<i32>(g_gameReg->m_logicPump->m_tables[powerup]);
    CWwdGameObjectA* r = m_object;
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
    CGrunt* e = (reinterpret_cast<CGrunt**>((reinterpret_cast<char*>(g_gameReg->m_cmdGrid) + 0x1c)))[m_cellX * 15 + m_cellY];
    if (e != 0) {
        m_object->m_screenX = e->m_object->m_screenX;
        m_object->m_screenY = e->m_object->m_screenY;
    }
    return 0;
}

RVA(0x00080490, 0xbe)
i32 CGruntPowerupSprite::SerializeMove(CGruntArchive* ar, i32 mode, i32 a3, i32 a4) {
    if (CUserLogic::SerializeMove(ar, mode, a3, a4) == 0) {
        return 0;
    }
    if (Chain(ar, mode, a3, reinterpret_cast<CGameObject*>(a4)) == 0) {
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
            CWwdGameObjectA* r = m_object;
            i32 v = reinterpret_cast<i32>(g_gameReg->m_logicPump->m_tables[id]);
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
