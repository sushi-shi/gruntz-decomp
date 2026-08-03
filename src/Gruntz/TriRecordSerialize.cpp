#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialRecords.h>
#include <Io/FileMem.h>

RVA(0x00058ee0, 0x5c)
i32 CPairRecord::Serialize(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d) {
    switch (tag) {
        case SERIAL_SAVE:
            ar->Write(&m_start, sizeof(m_start));
            ar->Write(&m_duration, sizeof(m_duration));
            break;
        case SERIAL_LOAD:
            ar->Read(&m_start, sizeof(m_start));
            ar->Read(&m_duration, sizeof(m_duration));
            break;
    }
    return 1;
}
