#include <Gruntz/GruntzMapMgr.h>

#include <DDrawMgr/DDrawWorkerHost.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/ImageSets.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/TileCollisionKind.h>
#include <Io/FileMem.h>
#include <Wap32/CoordUnset.h>

#include <stddef.h>

// @early-stop
RVA(0x00082430, 0x161)
i32 CGruntzMapMgr::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    i32 payload
) {
    if (ar == NULL) {
        return 0;
    }
    switch (mode) {
        case SERIAL_LOAD: {

            ar->Read(&m_reserved90, sizeof(m_reserved90));
            i32 count;
            ar->Read(&count, sizeof(count));
            for (i32 fi = 0; fi < m_arr.GetSize(); fi++) {
                Coord* elem = static_cast<Coord*>(m_arr.GetData()[fi]);
                if (elem != NULL) {
                    CoordPoolNode* node = g_coordPool.NodeOf(elem);
                    node->m_next = g_coordPool.m_freeHead;
                    g_coordPool.m_freeHead = node;
                }
            }
            m_arr.SetSize(0, -1);
            m_arr.SetSize(count, -1);
            for (u32 ri = 0; ri < static_cast<u32>(count); ri++) {
                CoordPoolNode* node = g_coordPool.m_freeHead;
                Coord* elem = NULL;
                if (node->m_next != NULL) {
                    elem = &node->m_coord;
                    g_coordPool.m_freeHead = node->m_next;
                }
                ar->Read(elem, 8);
                m_arr.GetData()[ri] = elem;
            }
            break;
        }
        case SERIAL_SAVE: {

            ar->Write(&m_reserved90, sizeof(m_reserved90));
            i32 wn = m_arr.GetSize();
            ar->Write(&wn, sizeof(wn));
            for (u32 wi = 0; wi < static_cast<u32>(wn); wi++) {
                Coord* elem = static_cast<Coord*>(m_arr.GetData()[wi]);
                if (elem == NULL) {
                    return 0;
                }
                ar->Write(elem, 8);
            }
            break;
        }
    }

    return CMapMgr::SerializeDispatch(ar, mode, typeId, payload) != 0;
}

RVA(0x00082600, 0x73)
TileCollisionKind CGameLevel::LookupTile(i32 x, i32 y) {
    CDDrawWorkerHost* mp;
    if (x < 0) {
        x = 0;
    } else {
        mp = m_mainPlane;
        if (x >= mp->m_tileColumns) {
            x = mp->m_tileColumns - 1;
        }
    }
    if (y < 0) {
        y = 0;
    } else {
        mp = m_mainPlane;
        if (y >= mp->m_tileRows) {
            y = mp->m_tileRows - 1;
        }
    }
    mp = m_mainPlane;
    i32 tile = mp->m_tileHandles[mp->m_tileRowOffsets[y] + x];
    if (tile == UNINIT_FILL || tile == TILE_CLEAR) {
        return TILEKIND_PASSABLE;
    }
    CTileImageSet* set =
        static_cast<CTileImageSet*>(m_imageSets[tile & WWD_TILE_IMAGE_SET_INDEX_MASK]);
    return set->GetCollisionAt(0, 0);
}

RVA(0x00085480, 0x52)
void CGruntzMapMgr::Reset() {
    for (i32 i = 0; i < m_arr.GetSize(); i++) {
        Coord* elem = static_cast<Coord*>(m_arr.GetData()[i]);
        if (elem != NULL) {
            CoordPoolNode* node = g_coordPool.NodeOf(elem);
            node->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = node;
        }
    }
    m_arr.SetSize(0, -1);
    CMapMgr::Reset();
}

RVA(0x00085d10, 0xa7)
CGruntzMapMgr::~CGruntzMapMgr() {
    for (i32 i = 0; i < m_arr.GetSize(); i++) {
        Coord* elem = static_cast<Coord*>(m_arr.GetAt(i));
        if (elem != NULL) {
            CoordPoolNode* node = g_coordPool.NodeOf(elem);
            node->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = node;
        }
    }
    m_arr.SetSize(0, -1);
    CMapMgr::Reset();
}
