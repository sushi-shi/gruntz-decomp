#include <Mfc.h>
#include <Image/CImage.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Io/FileMem.h>
#include <Ints.h>
#include <rva.h>
#include <Gruntz/WarpStoneFly.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialCounter.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GruntzMgr.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Gruntz/Sprite.h>
#include <string.h>

RVA(0x00109e00, 0x245)
i32 CWarpStoneFly::Sync(CFileMemBase* arc, i32 mode, i32 typeId, i32 pObj) {
    if (arc == 0) {
        return 0;
    }
    CDDrawSurfaceMgr* lvl = g_gameReg->m_world;
    if (lvl == 0) {
        return 0;
    }
    switch (mode) {
        case 7: {

            arc->Read(&m_arrivalMode, 4);
            arc->Read(&m_targetX, 4);
            arc->Read(&m_targetY, 4);
            arc->Read(&m_currentX, 8);
            arc->Read(&m_currentY, 8);
            arc->Read(&m_velocityScale, 8);
            arc->Read(&m_xDirection, 8);
            arc->Read(&m_yDirection, 8);
            g_serialCounter++;

            char name[0x80];
            i32 index;
            arc->Read(name, 0x80);
            arc->Read(&index, 4);
            if (strlen(name) != 0) {
                i32 i = index;
                CObject* out = 0;
                lvl->m_imageRegistry->m_10map.Lookup(name, out);
                CDDrawWorker* rec = static_cast<CDDrawWorker*>(out);
                CImage* r;
                if (rec != 0 && i >= rec->m_minIndex && i <= rec->m_maxIndex) {
                    r = static_cast<CImage*>(rec->m_items.GetAt(i));
                } else {
                    r = 0;
                }
                m_sprite = r;
            } else {
                m_sprite = 0;
            }
            return 1;
        }
        case 4: {

            arc->Write(&m_arrivalMode, 4);
            arc->Write(&m_targetX, 4);
            arc->Write(&m_targetY, 4);
            arc->Write(&m_currentX, 8);
            arc->Write(&m_currentY, 8);
            arc->Write(&m_velocityScale, 8);
            arc->Write(&m_xDirection, 8);
            arc->Write(&m_yDirection, 8);
            g_serialCounter++;

            CImage* obj = m_sprite;
            char name[0x80];
            i32 index = 0;
            memset(name, 0, 0x80);
            if (obj != 0) {
                lvl->m_imageRegistry->AnyValueMatches(obj, name, &index);
            }
            arc->Write(name, 0x80);
            arc->Write(&index, 4);
            break;
        }
    }
    return 1;
}
