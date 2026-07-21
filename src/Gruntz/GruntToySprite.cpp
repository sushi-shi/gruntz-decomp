#include <Gruntz/Sprite.h> // CSprite - the bound object's +0x194 cached sprite (ex CGruntLayerHolder)
#include <Image/CImage.h> // complete CImage: the CObArray-element downcasts are static (CImage : CWapObj : CObject)
#include <Gruntz/GruntToySprite.h>
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Io/FileMem.h>           // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Gruntz/SerialArchive.h> // CSerialArchive (the inherited CWapX::Chain arg; ex SerialObjRef.h)
#include <Wap32/ZVec.h>
#include <Gruntz/Grunt.h> // CGrunt - the registry grunt-table slot (was the CGruntEntry view)
#include <Gruntz/TypeKeyColl.h> // the REAL registry class at 0x6bf650 (its fields were the shredded g_type* globals)
#include <Gruntz/TriggerMgr.h> // CTriggerMgr - m_cmdGrid (its m_grid CGrunt cells)

DATA(0x00244d58)
extern CIndicatorActReg g_toyActReg; // 0x644d58

// ~CGruntToySprite @0x0122b0 - the CUserLogic-folded /GX leaf dtor (see header).
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CGruntToySprite() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
#include <rva.h>
RVA_COMPGEN(0x000122b0, 0x44, ??1CGruntToySprite@@UAE@XZ)

RVA(0x0007f350, 0x16a)
CGruntToySprite::CGruntToySprite(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_38->ApplyLookupSprite("GAME_STATUSBAR_TABZ_STATZTAB_SMALL", 0);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->m_stateFlags |= 1;
    if (m_object->m_sortKey != 0xdbba0) {
        m_object->m_sortKey = 0xdbba0;
        m_object->m_flags |= 0x20000;
    }
    m_lastLayer = 0;
}

RVA(0x0007f540, 0x15)
void CGruntToySprite::InitActReg() {
    g_toyActReg.Construct(2000, 2010);
}

RVA(0x0007f5c0, 0x102)
void CGruntToySprite::FireActivation(i32 id) {
    if ((reinterpret_cast<CToyActEntry*>(g_toyActReg.ResolveEntry(id)))->m_fn != 0) {
        (this->*(reinterpret_cast<CToyActEntry*>(g_toyActReg.ResolveEntry(id)))->m_fn)();
    }
}

// CGruntToySprite::RegisterActs @0x07f720 - bind the class's per-frame handler
// (Update @0x07f960) to the activation key "A" (the SAME activation-name-intern
// archetype as CGruntHealthSprite::RegisterActs; see that TU for the full notes).
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (every call/immediate/branch/offset + the `mov [entry],offset
// Update` handler store match retail); residual is the slot-vs-id callee-saved
// register choice cascading into the free-loop count materialization. Deferred.
RVA(0x0007f720, 0x18d)
void CGruntToySprite::RegisterActs() {
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
    (reinterpret_cast<CToyActEntry*>(g_toyActReg.ResolveEntry(id)))->m_fn =
        static_cast<i32 (CUserLogic::*)()>(&CGruntToySprite::Update);
}

RVA(0x0007f920, 0x21)
i32 CGruntToySprite::SetCell(i32 x, i32 y) {
    m_cellX = x;
    m_cellY = y;
    m_38->m_stateFlags &= ~1;
    return 1;
}

RVA(0x0007f960, 0x85)
i32 CGruntToySprite::Update() {
    CGrunt* e = g_gameReg->m_cmdGrid->m_grid[m_cellX * 15 + m_cellY];
    if (e == 0) {
        return 0;
    }
    i32 layer = e->m_198;
    if (m_lastLayer != layer) {
        CWwdGameObjectA* r = m_object;
        m_lastLayer = layer;
        CSprite* h = r->m_sprite;
        if (h != 0) {
            CImage* mapped;
            if (layer >= h->m_minIndex && layer <= h->m_maxIndex) {
                mapped = static_cast<CImage*>(h->m_items.GetAt(layer));
            } else {
                mapped = 0;
            }
            r->m_layer = mapped;
            r->m_190 = layer;
        }
    }
    m_object->m_screenX = e->m_object->m_screenX;
    m_object->m_screenY = e->m_object->m_screenY - 0x20;
    return 0;
}

RVA(0x0007fa20, 0x89)
i32 CGruntToySprite::SerializeMove(CGruntArchive* ar, i32 mode, i32 a3, i32 a4) {
    switch (mode) {
        case 4:
            ar->Write(&m_cellX, 8);
            ar->Write(&m_lastLayer, 4);
            break;
        case 7:
            ar->Read(&m_cellX, 8);
            ar->Read(&m_lastLayer, 4);
            break;
    }
    if (CUserLogic::SerializeMove(ar, mode, a3, a4) == 0) {
        return 0;
    }
    return Chain(ar, mode, a3, reinterpret_cast<CGameObject*>(a4)) != 0;
}
