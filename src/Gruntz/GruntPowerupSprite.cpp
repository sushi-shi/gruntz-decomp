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
#include <Gruntz/GruntPowerupSprite.h>

// The (de)serialization archive is the shared CSerialArchive (Read @ +0x2c / Write
// @ +0x30), pulled in via the header - the former per-TU PupArchive view is folded.

// The serializable sub-object overlaid at CUserLogic+0x34; its own serializer is
// reached as `lea ecx,[this+0x34]; call` through the 0x1aff thunk (the SAME helper
// the in-game-text leaf uses). External; no body.
class PupSubObj {
public:
    i32 SerializeSub(void* ar, i32 mode, i32 a, i32 b); // 0x001aff (thunk)
};

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
// (saving the old node into m_prevAnimSetNode). Returns 1.
RVA(0x00080380, 0x6c)
i32 CGruntPowerupSprite::SetCell(i32 x, i32 y, i32 powerup) {
    m_cellX = x;
    m_cellY = y;
    m_powerupId = powerup;
    i32 rec = *(i32*)((char*)g_mgrSettings->m_78 + powerup * 4 + 0x14);
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
// but cl loads g_mgrSettings into a different register (edx vs retail's eax) AFTER the
// index add where retail schedules it between the lea-chain and the add, cascading
// a register-name swap through the indexed entry load - not source-steerable.
// Every instruction matches modulo register names. Deferred to the final sweep.
RVA(0x00080410, 0x51)
i32 CGruntPowerupSprite::Update() {
    ((CIndicatorSyncHelper*)((char*)m_38 + 0x1a0))->Sync(g_6bf3bc);
    CGruntEntry* e = ((CGruntEntry**)((char*)g_mgrSettings->m_68 + 0x1c))[m_cellX * 15 + m_cellY];
    if (e != 0) {
        m_object->m_screenX = e->m_renderable->m_screenX;
        m_object->m_screenY = e->m_renderable->m_screenY;
    }
    return 0;
}

// CGruntPowerupSprite::Serialize @0x080490 - the serialize override. Chain the base
// CUserLogic::SerializeChain and the +0x34 sub-object, then round-trip the own state:
// m_cellX/m_cellY (8 B) + m_powerupId (4 B). mode 4 = write, mode 7 = read. On read, re-resolve
// the powerup's bute-set record (g_mgrSettings->m_78[m_powerupId*4 + 0x14]) into the bound renderable.
RVA(0x00080490, 0xbe)
i32 CGruntPowerupSprite::Serialize(CSerialArchive* ar, i32 mode, i32 a3, i32 a4) {
    if (SerializeChain((i32)ar, mode, a3, a4) == 0) {
        return 0;
    }
    if (((PupSubObj*)&m_34)->SerializeSub(ar, mode, a3, a4) == 0) {
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
            i32 v = *(i32*)((char*)g_mgrSettings->m_78 + id * 4 + 0x14);
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
