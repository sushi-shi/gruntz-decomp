#include <rva.h>

#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/LogicRecordRegistry.h>
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

static inline CDDrawWorker* LookupWorker(CMapStringToOb& map, LPCTSTR name) {
    CObject* found = NULL;
    map.Lookup(name, found);
    return static_cast<CDDrawWorker*>(found);
}

RVA(0x0009c650, 0x372)
i32 CTimer::Deserialize(CFileMemBase* s) {
    if (s == NULL) {
        return 0;
    }
    CDDrawSurfaceMgr* reg = g_gameReg->m_world;
    if (reg == NULL) {
        return 0;
    }

    char buf[SERIAL_NAME_LEN];
    i32 idx;

    s->Read(&m_baseX, sizeof(m_baseX));
    s->Read(&m_baseY, sizeof(m_baseY));

    g_serialCounter++;
    s->Read(buf, SERIAL_NAME_LEN);
    if (strlen(buf) != 0) {
        m_sprite = LookupWorker(reg->m_imageRegistry->m_workersByName, buf);
    } else {
        m_sprite = NULL;
    }

    s->Read(&m_active, sizeof(m_active));

    g_serialCounter++;
    s->Read(buf, SERIAL_NAME_LEN);
    s->Read(&idx, sizeof(idx));
    if (strlen(buf) != 0) {
        i32 i = idx;
        CDDrawWorker* tt = LookupWorker(reg->m_imageRegistry->m_workersByName, buf);
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
    s->Read(buf, SERIAL_NAME_LEN);
    s->Read(&idx, sizeof(idx));
    if (strlen(buf) != 0) {
        i32 i = idx;
        CDDrawWorker* tt = LookupWorker(reg->m_imageRegistry->m_workersByName, buf);
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
    s->Read(buf, SERIAL_NAME_LEN);
    s->Read(&idx, sizeof(idx));
    if (strlen(buf) != 0) {
        i32 i = idx;
        CDDrawWorker* tt = LookupWorker(reg->m_imageRegistry->m_workersByName, buf);
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
    s->Read(buf, SERIAL_NAME_LEN);
    s->Read(&idx, sizeof(idx));
    if (strlen(buf) != 0) {
        i32 i = idx;
        CDDrawWorker* tt = LookupWorker(reg->m_imageRegistry->m_workersByName, buf);
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
    s->Read(buf, SERIAL_NAME_LEN);
    s->Read(&idx, sizeof(idx));
    if (strlen(buf) != 0) {
        i32 i = idx;
        CDDrawWorker* tt = LookupWorker(reg->m_imageRegistry->m_workersByName, buf);
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

    s->Read(&m_running, sizeof(m_running));
    s->Read(&m_currentMs, sizeof(m_currentMs));

    return 1;
}

RVA(0x0009cab0, 0x23)
CLogicRecord* CLogicRecordRegistry::FindTemplate(const char* key) {
    CObject* found = NULL;
    ASSERT(key != NULL);
    m_templatesByName.Lookup(key, found);
    return static_cast<CLogicRecord*>(found);
}
