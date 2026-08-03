#include <rva.h>

#include <DDrawMgr/DDrawWorkerCache.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialCounter.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/Timer.h>
#include <Image/CImage.h>
#include <Io/FileMem.h>

#include <string.h>

// @early-stop
RVA(0x0009c650, 0x372)
i32 CTimer::Deserialize(CFileMemBase* s) {
    if (s == NULL) {
        return 0;
    }
    CDDrawSurfaceMgr* reg = g_gameReg->m_world;
    if (reg == NULL) {
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
        out = NULL;
        reg->m_imageRegistry->m_10map.Lookup(buf, out);
        m_sprite = static_cast<CDDrawWorker*>(out);
    } else {
        m_sprite = NULL;
    }

    s->Read(&m_active, 4);

    g_serialCounter++;
    s->Read(buf, 0x80);
    s->Read(&idx, 4);
    if (strlen(buf) != 0) {
        i32 i = idx;
        out = NULL;
        reg->m_imageRegistry->m_10map.Lookup(buf, out);
        CDDrawWorker* tt = static_cast<CDDrawWorker*>(out);
        CImage* r;
        if (tt != NULL && i >= tt->m_minIndex && i <= tt->m_maxIndex) {
            r = static_cast<CImage*>(tt->m_items.GetAt(i));
        } else {
            r = NULL;
        }
        m_frameMinTens = r;
    } else {
        m_frameMinTens = NULL;
    }

    g_serialCounter++;
    s->Read(buf, 0x80);
    s->Read(&idx, 4);
    if (strlen(buf) != 0) {
        i32 i = idx;
        out = NULL;
        reg->m_imageRegistry->m_10map.Lookup(buf, out);
        CDDrawWorker* tt = static_cast<CDDrawWorker*>(out);
        CImage* r;
        if (tt != NULL && i >= tt->m_minIndex && i <= tt->m_maxIndex) {
            r = static_cast<CImage*>(tt->m_items.GetAt(i));
        } else {
            r = NULL;
        }
        m_frameMinOnes = r;
    } else {
        m_frameMinOnes = NULL;
    }

    g_serialCounter++;
    s->Read(buf, 0x80);
    s->Read(&idx, 4);
    if (strlen(buf) != 0) {
        i32 i = idx;
        out = NULL;
        reg->m_imageRegistry->m_10map.Lookup(buf, out);
        CDDrawWorker* tt = static_cast<CDDrawWorker*>(out);
        CImage* r;
        if (tt != NULL && i >= tt->m_minIndex && i <= tt->m_maxIndex) {
            r = static_cast<CImage*>(tt->m_items.GetAt(i));
        } else {
            r = NULL;
        }
        m_frameSecTens = r;
    } else {
        m_frameSecTens = NULL;
    }

    g_serialCounter++;
    s->Read(buf, 0x80);
    s->Read(&idx, 4);
    if (strlen(buf) != 0) {
        i32 i = idx;
        out = NULL;
        reg->m_imageRegistry->m_10map.Lookup(buf, out);
        CDDrawWorker* tt = static_cast<CDDrawWorker*>(out);
        CImage* r;
        if (tt != NULL && i >= tt->m_minIndex && i <= tt->m_maxIndex) {
            r = static_cast<CImage*>(tt->m_items.GetAt(i));
        } else {
            r = NULL;
        }
        m_frameSecOnes = r;
    } else {
        m_frameSecOnes = NULL;
    }

    g_serialCounter++;
    s->Read(buf, 0x80);
    s->Read(&idx, 4);
    if (strlen(buf) != 0) {
        i32 i = idx;
        out = NULL;
        reg->m_imageRegistry->m_10map.Lookup(buf, out);
        CDDrawWorker* tt = static_cast<CDDrawWorker*>(out);
        CImage* r;
        if (tt != NULL && i >= tt->m_minIndex && i <= tt->m_maxIndex) {
            r = static_cast<CImage*>(tt->m_items.GetAt(i));
        } else {
            r = NULL;
        }
        m_frameColon = r;
    } else {
        m_frameColon = NULL;
    }

    s->Read(&m_running, 4);
    s->Read(&m_currentMs, 4);

    return 1;
}

RVA(0x0009cab0, 0x23)
CObject* CDDrawWorkerCache::Find(const char* key) {
    CObject* found = 0;
    m_workers.Lookup(key, found);
    return found;
}
