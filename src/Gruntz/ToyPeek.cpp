#include <rva.h>

#include <Gruntz/ToyPeek.h>

#include <Bute/ButeTree.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Io/FileMem.h>
#include <Rez/FrameClock.h>

RVA_COMPGEN(0x00011c10, 0x1e, ??_GCToyPeek@@UAEPAXI@Z)
RVA_COMPGEN(0x00011c40, 0x44, ??1CToyPeek@@UAE@XZ)

// @early-stop
RVA(0x00098140, 0x18e)
CToyPeek::CToyPeek(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_startClock.m_v = 0;
    m_countdown.m_v = 0;
    m_object->m_screenY -= 0x18;
    if (m_object->m_sortKey != 0xdbba0) {
        m_object->m_sortKey = 0xdbba0;
        m_object->m_flags |= 0x20000;
    }
    m_wwdObject->ApplyLookupSprite("GAME_STATUSBAR_TABZ_STATZTAB_SMALLICONZ", m_object->m_smarts);
    m_countdown.m_v = 0x1388;
    m_startClock.m_v = static_cast<u32>(g_frameTime);
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
}

// @interleaver SerializeMove - 152 B lone body at 0x983e0, between RefreshCell
// (ingameicon) and PeekCycle (ingameicon): a first-use placement.
RVA(0x000983e0, 0x98)
i32 CToyPeek::SerializeMove(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* pObj
) {
    if (CUserLogic::SerializeMove(ar, mode, typeId, pObj) == 0) {
        return 0;
    }
    if (Chain(ar, mode, typeId, pObj) == 0) {
        return 0;
    }

    switch (mode) {
        case SERIAL_SAVE:
            ar->Write(&m_startClock, sizeof(m_startClock));
            ar->Write(&m_countdown, sizeof(m_countdown));
            break;
        case SERIAL_LOAD:
            ar->Read(&m_startClock, sizeof(m_startClock));
            ar->Read(&m_countdown, sizeof(m_countdown));
            break;
    }
    return 1;
}

VTBL(CToyPeek, 0x001e7204);
