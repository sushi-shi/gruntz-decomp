// SaveRecordLoad.cpp - SaveRecord::Load (0x0faff0): streams a fixed record through
// the shared WAP32 CSerialArchive's +0x30 slot (CSerialArchive::Write, the canonical
// store entry). NB the SaveRecord::Load method name is the recovered-symbol
// placeholder; the archive object drives the actual transfer direction, only the
// +0x30 slot offset is load-bearing.
#include <Ints.h>
#include <rva.h>

#include <Gruntz/SerialArchive.h> // the ONE shared archive stream (Read@+0x2c / Write@+0x30)

struct SaveRecord {
    char pad00[0x0c];
    void* m_0c; // +0x0c (must be non-null)
    char pad10[0x1c - 0x10];
    i32 m_1c; // +0x1c
    i32 m_20;
    i32 m_24;
    char pad28[0x38 - 0x28];
    i32 m_38;
    i32 m_3c;
    i32 m_40;
    i32 m_44;
    i32 m_48;
    char m_4c[0x100]; // +0x4c
    i32 m_14c;
    i32 m_150;
    i32 m_154;
    i32 m_158;
    i32 m_15c;
    char pad160[0x168 - 0x160];
    char m_168[0x10];
    char m_178[0x10];
    char m_188[0x10];
    char m_198[0x10];
    i32 m_1a8;
    i32 m_1ac;

    i32 Load(CSerialArchive* s); // 0x0faff0
};

RVA(0x000faff0, 0x163)
i32 SaveRecord::Load(CSerialArchive* s) {
    if (!s) {
        return 0;
    }
    if (!m_0c) {
        return 0;
    }
    s->Write(&m_1c, 4);
    s->Write(&m_20, 4);
    s->Write(&m_24, 4);
    s->Write(&m_38, 4);
    s->Write(&m_3c, 4);
    s->Write(&m_40, 4);
    s->Write(&m_44, 4);
    s->Write(&m_48, 4);
    s->Write(m_4c, 0x100);
    s->Write(&m_14c, 4);
    s->Write(&m_150, 4);
    s->Write(&m_154, 4);
    s->Write(&m_158, 4);
    s->Write(&m_15c, 4);
    s->Write(m_168, 0x10);
    s->Write(m_178, 0x10);
    s->Write(m_188, 0x10);
    s->Write(m_198, 0x10);
    s->Write(&m_1a8, 4);
    s->Write(&m_1ac, 4);
    return 1;
}
SIZE_UNKNOWN(SaveRecord);
