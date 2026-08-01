#include <Gruntz/GruntzMapMgr.h>

#include <Io/FileMem.h>

#include <Gruntz/FreeNodePool.h>

VTBL(CGruntzMapMgr, 0x001e9bb4);

// @early-stop
RVA(0x00082430, 0x161)
i32 CGruntzMapMgr::Visit(CFileMemBase* ar, i32 mode, i32 typeId, i32 pObj) {
    if (ar == 0) {
        return 0;
    }
    switch (mode) {
        case 7: {

            ar->Read(&m_90, 4);
            i32 count;
            ar->Read(&count, 4);
            for (i32 fi = 0; fi < m_arr.GetSize(); fi++) {
                void* elem = m_arr.GetData()[fi];
                if (elem != 0) {
                    CoordPoolNode* node = g_coordPool.NodeOf(elem);
                    node->m_next = g_coordPool.m_freeHead;
                    g_coordPool.m_freeHead = node;
                }
            }
            m_arr.SetSize(0, -1);
            m_arr.SetSize(count, -1);
            for (u32 ri = 0; ri < static_cast<u32>(count); ri++) {
                CoordPoolNode* node = g_coordPool.m_freeHead;
                void* elem = 0;
                if (node->m_next != 0) {
                    elem = &node->m_coord;
                    g_coordPool.m_freeHead = node->m_next;
                }
                ar->Read(elem, 8);
                m_arr.GetData()[ri] = elem;
            }
            break;
        }
        case 4: {

            ar->Write(&m_90, 4);
            i32 wn = m_arr.GetSize();
            ar->Write(&wn, 4);
            for (u32 wi = 0; wi < static_cast<u32>(wn); wi++) {
                void* elem = m_arr.GetData()[wi];
                if (elem == 0) {
                    return 0;
                }
                ar->Write(elem, 8);
            }
            break;
        }
    }

    return CMapMgr::Visit(ar, mode, typeId, pObj) != 0;
}

RVA(0x00085480, 0x52)
void CGruntzMapMgr::Reset() {
    for (i32 i = 0; i < m_arr.GetSize(); i++) {
        void* elem = m_arr.GetData()[i];
        if (elem != 0) {
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
        void* elem = m_arr.GetAt(i);
        if (elem != 0) {
            CoordPoolNode* node = g_coordPool.NodeOf(elem);
            node->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = node;
        }
    }
    m_arr.SetSize(0, -1);
    CMapMgr::Reset();
}
