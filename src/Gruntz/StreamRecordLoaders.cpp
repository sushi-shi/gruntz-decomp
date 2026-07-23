#include <rva.h>
#include <Image/CImage.h> // complete CImage: the CObArray-element downcasts are static (CImage : CWapObj : CObject)
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Io/FileMem.h> // the serialize stream (CFileMemBase == the real CFileMemBase)
#include <DDrawMgr/DDrawWorkerRegistry.h> // name map at g_gameReg->m_world +0x10
#include <Gruntz/SerialCounter.h>
#include <Gruntz/GameRegistry.h>  // CGameRegistry (g_gameReg->m_world = CDDrawSurfaceMgr*)
#include <Gruntz/SerialArchive.h> // CFileMemBase (reader; Read @ vtable +0x2c)
#include <Gruntz/Timer.h>
#include <Gruntz/Sprite.h>             // CDDrawWorker (the looked-up, index-gated frame table)
#include <DDrawMgr/DDrawWorkerCache.h> // the +0x14 worker cache - Find (0x9cab0) is its method
#include <string.h>                    // inline strlen (repne scasb) over the scratch buffer

// @early-stop
// outparam-zeroinit-scheduling wall (same as CTriggerLoadRec): logic + offsets
// byte-exact, residual is only the 6 `out = 0` store positions (retail sinks, cl
// hoists). The idx-in-callee-saved-reg regalloc is steered by the `i32 i = idx;`
// copy. ~92%.
RVA(0x0009c650, 0x372)
i32 CTimer::Deserialize(CFileMemBase* s) {
    if (s == 0) {
        return 0;
    }
    CDDrawSurfaceMgr* reg = g_gameReg->m_world;
    if (reg == 0) {
        return 0;
    }

    char buf[0x80];
    CObject* out;
    i32 idx;

    s->Read(&m_baseX, 4);
    s->Read(&m_baseY, 4);

    g_serialCounter++;
    s->Read(buf, 0x80);
    if (strlen(buf) != 0) {
        out = 0;
        reg->m_imageRegistry->m_10map.Lookup(buf, out);
        m_sprite = static_cast<CDDrawWorker*>(out);
    } else {
        m_sprite = 0;
    }

    s->Read(&m_active, 4);

    g_serialCounter++;
    s->Read(buf, 0x80);
    s->Read(&idx, 4);
    if (strlen(buf) != 0) {
        i32 i = idx;
        out = 0;
        reg->m_imageRegistry->m_10map.Lookup(buf, out);
        CDDrawWorker* tt = static_cast<CDDrawWorker*>(out);
        CImage* r;
        if (tt != 0 && i >= tt->m_minIndex && i <= tt->m_maxIndex) {
            r = static_cast<CImage*>(tt->m_items.GetAt(i));
        } else {
            r = 0;
        }
        m_frameMinTens = r;
    } else {
        m_frameMinTens = 0;
    }

    g_serialCounter++;
    s->Read(buf, 0x80);
    s->Read(&idx, 4);
    if (strlen(buf) != 0) {
        i32 i = idx;
        out = 0;
        reg->m_imageRegistry->m_10map.Lookup(buf, out);
        CDDrawWorker* tt = static_cast<CDDrawWorker*>(out);
        CImage* r;
        if (tt != 0 && i >= tt->m_minIndex && i <= tt->m_maxIndex) {
            r = static_cast<CImage*>(tt->m_items.GetAt(i));
        } else {
            r = 0;
        }
        m_frameMinOnes = r;
    } else {
        m_frameMinOnes = 0;
    }

    g_serialCounter++;
    s->Read(buf, 0x80);
    s->Read(&idx, 4);
    if (strlen(buf) != 0) {
        i32 i = idx;
        out = 0;
        reg->m_imageRegistry->m_10map.Lookup(buf, out);
        CDDrawWorker* tt = static_cast<CDDrawWorker*>(out);
        CImage* r;
        if (tt != 0 && i >= tt->m_minIndex && i <= tt->m_maxIndex) {
            r = static_cast<CImage*>(tt->m_items.GetAt(i));
        } else {
            r = 0;
        }
        m_frameSecTens = r;
    } else {
        m_frameSecTens = 0;
    }

    g_serialCounter++;
    s->Read(buf, 0x80);
    s->Read(&idx, 4);
    if (strlen(buf) != 0) {
        i32 i = idx;
        out = 0;
        reg->m_imageRegistry->m_10map.Lookup(buf, out);
        CDDrawWorker* tt = static_cast<CDDrawWorker*>(out);
        CImage* r;
        if (tt != 0 && i >= tt->m_minIndex && i <= tt->m_maxIndex) {
            r = static_cast<CImage*>(tt->m_items.GetAt(i));
        } else {
            r = 0;
        }
        m_frameSecOnes = r;
    } else {
        m_frameSecOnes = 0;
    }

    g_serialCounter++;
    s->Read(buf, 0x80);
    s->Read(&idx, 4);
    if (strlen(buf) != 0) {
        i32 i = idx;
        out = 0;
        reg->m_imageRegistry->m_10map.Lookup(buf, out);
        CDDrawWorker* tt = static_cast<CDDrawWorker*>(out);
        CImage* r;
        if (tt != 0 && i >= tt->m_minIndex && i <= tt->m_maxIndex) {
            r = static_cast<CImage*>(tt->m_items.GetAt(i));
        } else {
            r = 0;
        }
        m_frameColon = r;
    } else {
        m_frameColon = 0;
    }

    s->Read(&m_running, 4);
    s->Read(&m_currentMs, 4);

    return 1;
}

RVA(0x0009cab0, 0x23)
i32 CDDrawWorkerCache::Find(const char* key) {
    i32 local = 0;
    CObject* localOb = 0;
    m_10.Lookup(key, localOb);
    local = reinterpret_cast<i32>(localOb);
    return local;
}
