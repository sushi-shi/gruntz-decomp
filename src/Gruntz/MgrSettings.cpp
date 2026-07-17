// MgrSettings.cpp - CMgrSettings::Serialize (C:\Proj\Gruntz).
//
// UN-MERGED back to its own TU (2026-07-13); see WarpStoneFly.cpp. This one also had a
// DIFFERENT COMPILER FLAG SET than the TU it was merged into: units.toml had it as
// flags="base" while SBI_RectOnly.cpp is flags="eh" (/GX). A TU is compiled with ONE
// flag set, so a "base" obj cannot live inside an "eh" TU without changing its codegen.
#include <Mfc.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Io/FileMem.h> // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Ints.h>
#include <rva.h>
#include <Gruntz/MgrSettings.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GruntzMgr.h>         // the *0x24556c singleton (CGruntzMgr)
#include <DDrawMgr/DDrawSurfaceMgr.h> // g_gameReg->m_world (ex CMgrActiveHolder view)
#include <Gruntz/Sprite.h> // CSprite - the looked-up, index-gated record (ex CMgrLookupRec view)
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
i32 CMgrSettings::Serialize(CSerialArchive* arc, i32 mode, i32 a3, i32 a4) {
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
            arc->Read(&m_00, 4);
            arc->Read(&m_04, 4);
            arc->Read(&m_08, 4);
            arc->Read(&m_10, 8);
            arc->Read(&m_18, 8);
            arc->Read(&m_20, 8);
            arc->Read(&m_28, 8);
            arc->Read(&m_30, 8);
            g_serialCounter++;

            char name[0x80];
            i32 index;
            arc->Read(name, 0x80);
            arc->Read(&index, 4);
            if (strlen(name) == 0) {
                m_38 = 0;
                return 1;
            }
            CObject* out = 0;
            lvl->m_imageRegistry->m_10map.Lookup(name, out);
            CSprite* rec = (CSprite*)out;
            if (rec == 0 || index < rec->m_firstFrame || index > rec->m_lastFrame) {
                m_38 = 0;
            } else {
                m_38 = rec->m_frames.m_pData[index];
            }
            return 1;
        }
    } else {
        // WRITE the scalar block, then the resolved object's name + index.
        arc->Write(&m_00, 4);
        arc->Write(&m_04, 4);
        arc->Write(&m_08, 4);
        arc->Write(&m_10, 8);
        arc->Write(&m_18, 8);
        arc->Write(&m_20, 8);
        arc->Write(&m_28, 8);
        arc->Write(&m_30, 8);
        g_serialCounter++;

        CImage* obj = m_38;
        char name[0x80];
        i32 index = 0;
        memset(name, 0, 0x80);
        if (obj != 0) {
            lvl->m_imageRegistry->AnyValueMatches_155630(obj, name, &index);
        }
        arc->Write(name, 0x80);
        arc->Write(&index, 4);
    }
    return 1;
}
