#include <rva.h>

#include <DDrawMgr/DDrawWorker.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/LogicRecordRegistry.h>
#include <DDrawMgr/WorkerLookup.h>
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
        m_sprite = reg->FindWorker(buf);
    } else {
        m_sprite = NULL;
    }

    s->Read(&m_active, sizeof(m_active));

    g_serialCounter++;
    s->Read(buf, SERIAL_NAME_LEN);
    s->Read(&idx, sizeof(idx));
    if (strlen(buf) != 0) {
        m_frameMinTens = reg->FindFrame(buf, idx);
    } else {
        m_frameMinTens = NULL;
    }

    g_serialCounter++;
    s->Read(buf, SERIAL_NAME_LEN);
    s->Read(&idx, sizeof(idx));
    if (strlen(buf) != 0) {
        m_frameMinOnes = reg->FindFrame(buf, idx);
    } else {
        m_frameMinOnes = NULL;
    }

    g_serialCounter++;
    s->Read(buf, SERIAL_NAME_LEN);
    s->Read(&idx, sizeof(idx));
    if (strlen(buf) != 0) {
        m_frameSecTens = reg->FindFrame(buf, idx);
    } else {
        m_frameSecTens = NULL;
    }

    g_serialCounter++;
    s->Read(buf, SERIAL_NAME_LEN);
    s->Read(&idx, sizeof(idx));
    if (strlen(buf) != 0) {
        m_frameSecOnes = reg->FindFrame(buf, idx);
    } else {
        m_frameSecOnes = NULL;
    }

    g_serialCounter++;
    s->Read(buf, SERIAL_NAME_LEN);
    s->Read(&idx, sizeof(idx));
    if (strlen(buf) != 0) {
        m_frameColon = reg->FindFrame(buf, idx);
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
