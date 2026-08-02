#include <rva.h>

#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialRecords.h>
#include <Io/FileMem.h>

RVA(0x00058ee0, 0x5c)
i32 CPairRecord::Serialize(CFileMemBase* ar, i32 tag, i32 c, CGameObject* d) {
    switch (tag) {
        case 4:
            ar->Write(&m_start, 8);
            ar->Write(&m_duration, 8);
            break;
        case 7:
            ar->Read(&m_start, 8);
            ar->Read(&m_duration, 8);
            break;
    }
    return 1;
}
