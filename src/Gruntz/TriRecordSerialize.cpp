#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialRecords.h>
#include <Io/FileMem.h>

RVA(0x00058ee0, 0x5c)
i32 CPairRecord::Serialize(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d) {
    switch (tag) {
        case SERIAL_SAVE:
            ar->Write(&m_start, 8);
            ar->Write(&m_duration, 8);
            break;
        case SERIAL_LOAD:
            ar->Read(&m_start, 8);
            ar->Read(&m_duration, 8);
            break;
    }
    return 1;
}
