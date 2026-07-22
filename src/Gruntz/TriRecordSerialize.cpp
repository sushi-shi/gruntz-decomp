#include <Gruntz/SerialArchive.h> // CSerialArchive (the inherited CWapX::Chain arg; ex SerialObjRef.h)
#include <Io/FileMem.h>          // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <rva.h>
#include <Gruntz/SerialRecords.h>

RVA(0x00058ee0, 0x5c)
i32 CPairRecord::Serialize(CSerialArchive* ar, i32 tag, i32 c, i32 d) {
    switch (tag) {
        case 4:
            ar->Write(&m_0, 8);
            ar->Write(&m_8, 8);
            break;
        case 7:
            ar->Read(&m_0, 8);
            ar->Read(&m_8, 8);
            break;
    }
    return 1;
}
