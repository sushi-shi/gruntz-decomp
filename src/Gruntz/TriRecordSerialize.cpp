// TriRecordSerialize.cpp - the Serialize override of a 12-byte three-i32 record
// (orphan COMDAT @0x3c8f0). Transfers m_0/m_4/m_8 through the archive Read (slot
// +0x2c) / Write (slot +0x30) dispatch, keyed on the tag (4 = write, 7 = read).
// Placeholder class name; only OFFSETS + code bytes are load-bearing.
#include <Gruntz/SerialObjRef.h> // CSerialArchive (Read @+0x2c / Write @+0x30)
#include <rva.h>
#include <Gruntz/SerialRecords.h>

RVA(0x0003c8f0, 0x76)
i32 CTriRecord::Serialize(CSerialArchive* ar, i32 tag, i32 c, i32 d) {
    switch (tag) {
        case 4:
            ar->Write(&m_0, 4);
            ar->Write(&m_4, 4);
            ar->Write(&m_8, 4);
            break;
        case 7:
            ar->Read(&m_0, 4);
            ar->Read(&m_4, 4);
            ar->Read(&m_8, 4);
            break;
    }
    return 1;
}

// 0x58ee0: the Serialize override of a 16-byte two-i64 record (m_0 / m_8).

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
SIZE_UNKNOWN(CPairRecord);
SIZE_UNKNOWN(CTriRecord);
