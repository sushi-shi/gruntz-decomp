#include <Gruntz/CursorSnapSprite.h>
#include <Bute/ButeTree.h>

#include <Gruntz/AnimWorker.h>
#include <Gruntz/UserLogic.h>
#include <rva.h>
#include <rva.h>
#include <Gruntz/SerialArchive.h>

RVA(0x00011880, 0x47)
i32 CCursorSnapSprite::SerializeMove(CFileMemBase* ar, i32 tag, i32 c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, d) != 0;
}

RVA_COMPGEN(0x000118f0, 0x1e, ??_GCCursorSnapSprite@@UAEPAXI@Z)
RVA_COMPGEN(0x00011920, 0x44, ??1CCursorSnapSprite@@UAE@XZ)

RVA(0x0003a200, 0xf1)
i32 CursorSnapWorkerPump(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_animWorker;
    switch (static_cast<u32>(rec->ActKey())) {
        case 0: {
            rec->SetActKey(0x3e8);
            CUserLogic* sub = new CCursorSnapSprite(owner);
            sub->Activate();
            rec->m_logic = sub;
            break;
        }
        case 0x1d:
            rec->m_logic->OnObjectRemoved();
            break;
        case 0x1e:
            rec->m_logic->OnLeaveActiveRegion();
            break;
        case 0x50:
            rec->m_logic->PrepareSave();
            break;
        case 0x53:
            rec->m_logic->AfterLoadReferences();
            break;
        case 0x52:
            rec->m_logic->AfterLoad();
            break;
        case 0x51:
            rec->m_logic->AfterSave();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_logic);
            break;
    }
    return 1;
}

// @early-stop
RVA(0x0003a340, 0x16e)
CCursorSnapSprite::CCursorSnapSprite(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_38->ApplyName("GAME_CURSORSNAPSPRITE");
    m_value = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry("GAME_SINGLEIMAGEANI", 0);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
    m_38->m_flags |= 2;
    m_38->m_stateFlags |= 1;
}

RVA(0x0003a5b0, 0x102)
void CCursorSnapSprite::FireActivation(i32 id) {
    CActHandler* e = (CActRegPool<CCursorSnapSprite>::s_table.ResolveEntry(id));
    if ((*e) != 0) {
        CActHandler* e2 = (CActRegPool<CCursorSnapSprite>::s_table.ResolveEntry(id));
        (this->*((*e2)))();
    }
}

VTBL(CCursorSnapSprite, 0x001e8074);
