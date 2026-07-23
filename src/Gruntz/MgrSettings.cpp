#include <Mfc.h>
#include <Image/CImage.h> // complete CImage: the CObArray-element downcasts are static (CImage : CWapObj : CObject)
#include <Gruntz/GameRegMfcPtr.h>
#include <Io/FileMem.h> // the serialize stream (CFileMemBase == the real CFileMemBase)
#include <Ints.h>
#include <rva.h>
#include <Gruntz/WarpStoneFly.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialCounter.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GruntzMgr.h>         // the *0x24556c singleton (CGruntzMgr)
#include <DDrawMgr/DDrawSurfaceMgr.h> // g_gameReg->m_world (ex CMgrActiveHolder view)
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Gruntz/Sprite.h> // CDDrawWorker - the looked-up, index-gated record (ex CMgrLookupRec view)
#include <string.h>        // strlen / memset (inlined to repne scasb / rep stos)

// @early-stop
// 89.1% - logic byte-faithful: the mode-4/7 dispatch, the eight scalar Read/Write
// virtual calls, g_serialCounter bump, inline strlen/memset, the Lookup + indexed
// record resolve, and the AnyValueMatches reverse-probe all match. Residual is one
// regalloc choice in the lookup range-check: retail keeps `index` in callee-saved
// esi across the Lookup and materializes the out-init 0 transiently, while cl pins
// the constant 0 in esi and spills `index` (docs/patterns/zero-register-pinning.md
// + pin-local-for-callee-saved-reg). Not source-steerable (& and || forms both
// normalize to the same fail-first regalloc). Logic complete; final-sweep deferred.
RVA(0x00109e00, 0x245)
i32 CWarpStoneFly::Sync(CFileMemBase* arc, i32 mode, i32 a3, i32 a4) {
    if (arc == 0) {
        return 0;
    }
    CDDrawSurfaceMgr* lvl = g_gameReg->m_world;
    if (lvl == 0) {
        return 0;
    }
    if (mode != 4) {
        if (mode == 7) {
            // READ the scalar block, then resolve the object reference.
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
            if (strlen(name) == 0) {
                m_sprite = 0;
                return 1;
            }
            CObject* out = 0;
            lvl->m_imageRegistry->m_10map.Lookup(name, out);
            CDDrawWorker* rec = static_cast<CDDrawWorker*>(out);
            if (rec == 0 || index < rec->m_minIndex || index > rec->m_maxIndex) {
                m_sprite = 0;
            } else {
                m_sprite = static_cast<CImage*>(rec->m_items.GetAt(index));
            }
            return 1;
        }
    } else {
        // WRITE the scalar block, then the resolved object's name + index.
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
    }
    return 1;
}
