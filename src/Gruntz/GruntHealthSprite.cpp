#include <Gruntz/Sprite.h> // CSprite - the bound object's +0x194 cached sprite (ex CGruntLayerHolder)
#include <Image/CImage.h> // complete CImage: the CObArray-element downcasts are static (CImage : CWapObj : CObject)
#include <Gruntz/GruntHealthSprite.h>
#include <Gruntz/Grunt.h> // CGrunt - the registry grunt-table slot (was the CGruntEntry view)
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Io/FileMem.h>           // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Gruntz/SerialArchive.h> // CSerialArchive (the inherited CWapX::Chain arg; ex SerialObjRef.h)
#include <Wap32/ZVec.h>
#include <Gruntz/TypeKeyColl.h> // the REAL registry class at 0x6bf650 (its fields were the shredded g_type* globals)
#include <Gruntz/TriggerMgr.h> // CTriggerMgr - m_cmdGrid (its m_grid CGrunt cells)

VTBL(CGruntHealthSprite, 0x001e7ba4);
DATA(0x00244d80)
extern CIndicatorActReg g_healthActReg; // 0x644d80

RVA(0x00011ef0, 0x4b)
CGruntHealthSprite::CGruntHealthSprite() {}

// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CGruntHealthSprite() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
#include <rva.h>
RVA_COMPGEN(0x00011fb0, 0x44, ??1CGruntHealthSprite@@UAE@XZ)

RVA(0x0007eb00, 0x170)
CGruntHealthSprite::CGruntHealthSprite(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_38->ApplyLookupSprite("GAME_GRUNTHEALTHSPRITE", 1);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_health = 0x64;
    if (m_object->m_sortKey != 0xdbba0) {
        m_object->m_sortKey = 0xdbba0;
        m_object->m_flags |= 0x20000;
    }
    m_60 = -0x19;
}

RVA(0x0007ecf0, 0x15)
void CGruntHealthSprite::InitActReg() {
    g_healthActReg.Construct(2000, 2010);
}

RVA(0x0007ed70, 0x102)
void CGruntHealthSprite::FireActivation(i32 id) {
    if ((reinterpret_cast<CHealthActEntry*>(g_healthActReg.ResolveEntry(id)))->m_fn != 0) {
        (this->*(reinterpret_cast<CHealthActEntry*>(g_healthActReg.ResolveEntry(id)))->m_fn)();
    }
}

// CGruntHealthSprite::RegisterActs @0x07eed0 - bind the class's per-frame handler
// (HealthUpdate @0x07f180) to the activation key "A". The key is first resolved
// to an id via the bute-tree name map (g_buteTree.Find); a fresh key gets the
// next id (g_buteTree.Insert), is interned into the shared name registry slot
// (ActNameLookup -> free the old name list, assign "A"), and bumps the counter.
// The id is then resolved to an entry in the class registry and the handler PMF
// stored there.
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md, topic:wall
// topic:regalloc): the logic is byte-faithful and the instruction SELECTION is
// exact end-to-end - the bute Find/Insert, the shared-name-registry resolve, the
// inlined CString-list free loop (the `while (n-- != 0)` spelling reproduces
// retail's `mov eax,N; mov ecx,eax; dec eax; test ecx; je; lea ebp,[eax+1]`
// count-recover per test-old-value-decrement-loop-while-postdec.md +
// predecrement-guard-lea-recover-count.md), operator=, the id-bump, and the
// per-class id->entry resolve with the `mov [entry],offset HealthUpdate` handler
// store. ~93%; the SOLE residual is which callee-saved register holds the name
// slot vs the live id - retail pins the slot in esi / id in edi, cl swaps them
// (slot->edi / id->esi), a whole-function allocation choice not source-steerable.
// Logic complete; deferred to the final sweep.
RVA(0x0007eed0, 0x18d)
void CGruntHealthSprite::RegisterActs() {
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
    (reinterpret_cast<CHealthActEntry*>(g_healthActReg.ResolveEntry(id)))->m_fn =
        static_cast<i32 (CUserLogic::*)()>(&CGruntHealthSprite::HealthUpdate);
}

RVA(0x0007f0d0, 0x6e)
i32 CGruntHealthSprite::SetHealthGlyph(i32 x, i32 y, i32 health) {
    m_cellX = x;
    m_cellY = y;
    i32 slot = 0x15 - static_cast<i32>((static_cast<double>(health) * 0.2 + 0.5));
    CWwdGameObjectA* obj = m_object;
    CSprite* map = obj->m_sprite;
    if (map) {
        CImage* glyph;
        if (slot >= map->m_minIndex && slot <= map->m_maxIndex) {
            glyph = static_cast<CImage*>(map->m_items.GetAt(slot));
        } else {
            glyph = 0;
        }
        obj->m_layer = glyph;
        obj->m_190 = slot;
    }
    m_health = health;
    return 1;
}

// CGruntHealthSprite::HealthUpdate @0x07f180 - the registered per-frame handler. Resolve
// the grunt for cell (m_cellX,m_cellY) from the game registry's grunt table; if absent,
// return 0. Otherwise poll the per-class stat getter (Vslot16, the grunt entry) for the
// current health; when it changed, round it to a glyph slot (0x15 - (int)(v*0.2+0.5)) and
// republish the glyph/slot through the bound renderable's [m_64..m_68]-gated +0x194 table
// (the SAME resolve as SetHealthGlyph), then stash the health. Finally sync the bound
// renderable's screen position from the grunt (y biased by this->m_60). Returns 0.
//
// @early-stop
// regalloc/scheduling wall (zero-register-pinning class, 96.67%): logic byte-faithful
// end-to-end (the grunt-table resolve, the Vslot16 stat dispatch, the *0.2+0.5 __ftol
// glyph round + the [m_64..m_68]-gated republish, the health stash, the screen-pos sync
// with the m_60 y-bias). The SOLE residual is the entry-lookup's whole-function register
// allocation: retail pins g_gameReg in edx and schedules reg->m_68 AFTER the index
// lea-chain (add ecx,eax; [eax+ecx*4+0x1c]); cl pins it in ecx and materializes reg->m_68
// BEFORE the chain (add eax,edx; [ecx+4*eax+0x1c]) - the same pin the sibling
// CGruntSelectedSprite::Update / CGruntPowerupSprite::Update carry. Not source-steerable
// (the `reg` local + idx-split + cellY-first add spellings + the permuter all leave it);
// see docs/patterns/zero-register-pinning.md. Deferred to the final sweep.
RVA(0x0007f180, 0xb4)
i32 CGruntHealthSprite::HealthUpdate() {
    CGruntzMgr* reg = g_gameReg;
    CGrunt* e = reg->m_cmdGrid->m_grid[m_cellX * 15 + m_cellY];
    if (e == 0) {
        return 0;
    }
    i32 result = Vslot16(e);
    if (m_health != result) {
        i32 slot = 0x15 - static_cast<i32>((static_cast<double>(result) * 0.2 + 0.5));
        CWwdGameObjectA* obj = m_object;
        CSprite* holder = obj->m_sprite;
        if (holder != 0) {
            CImage* glyph;
            if (slot >= holder->m_minIndex && slot <= holder->m_maxIndex) {
                glyph = static_cast<CImage*>(holder->m_items.GetAt(slot));
            } else {
                glyph = 0;
            }
            obj->m_layer = glyph;
            obj->m_190 = slot;
        }
        m_health = result;
    }
    m_object->m_screenX = e->m_object->m_screenX;
    m_object->m_screenY = m_60 + e->m_object->m_screenY;
    return 0;
}

RVA(0x0007f270, 0xa3)
i32 CGruntHealthSprite::SerializeMove(CGruntArchive* ar, i32 mode, i32 a3, i32 a4) {
    switch (mode) {
        case 4:
            ar->Write(&m_cellX, 8);
            ar->Write(&m_health, 4);
            ar->Write(&m_60, 4);
            break;
        case 7:
            ar->Read(&m_cellX, 8);
            ar->Read(&m_health, 4);
            ar->Read(&m_60, 4);
            break;
    }
    if (CUserLogic::SerializeMove(ar, mode, a3, a4) == 0) {
        return 0;
    }
    return Chain(ar, mode, a3, reinterpret_cast<CGameObject*>(a4)) != 0;
}
