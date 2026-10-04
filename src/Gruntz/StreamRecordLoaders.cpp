#include <StdAfx.h>

#include <Ints.h>

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

i32 CTimer::Deserialize(CFileMemBase* s) {
    if (s == NULL) {
        return 0;
    }
    CDDrawSurfaceMgr* reg = g_gameReg->World();
    if (reg == NULL) {
        return 0;
    }

    char buf[SERIAL_NAME_LEN];
    i32 idx;

    s->Read(&m_baseX, sizeof(m_baseX));
    s->Read(&m_baseY, sizeof(m_baseY));

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

CLogicRecord* CLogicRecordRegistry::FindTemplate(const std::string& key) {
    return MapFind<CLogicRecord>(m_templatesByName, key);
}
