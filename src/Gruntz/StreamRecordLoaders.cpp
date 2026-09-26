#include <rva.h>

#include <DDrawMgr/DDrawWorker.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/LogicRecordRegistry.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialWorkerRefMacros.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/Timer.h>
#include <Image/CImage.h>
#include <Io/FileMem.h>

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

    s->Read(&m_basePosition.m_x, sizeof(m_basePosition.m_x));
    s->Read(&m_basePosition.m_y, sizeof(m_basePosition.m_y));

    SERIAL_READ_WORKER(s, reg, buf, m_sprite);

    s->Read(&m_active, sizeof(m_active));

    SERIAL_READ_FRAME(s, reg, buf, idx, m_frameMinTens);

    SERIAL_READ_FRAME(s, reg, buf, idx, m_frameMinOnes);

    SERIAL_READ_FRAME(s, reg, buf, idx, m_frameSecTens);

    SERIAL_READ_FRAME(s, reg, buf, idx, m_frameSecOnes);

    SERIAL_READ_FRAME(s, reg, buf, idx, m_frameColon);

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
