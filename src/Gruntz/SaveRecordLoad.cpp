// SaveRecordLoad.cpp - SaveRecord::Load (0x0faff0): deserializes a fixed record
// from a stream object via its virtual Read(buf,size) (vtable slot +0x30).
#include <Ints.h>
#include <rva.h>

struct LoadStream {
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6();
    virtual void v7();
    virtual void v8();
    virtual void v9();
    virtual void v10();
    virtual void v11();
    virtual void Read(void* buf, i32 n); // slot +0x30
};

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

    i32 Load(LoadStream* s); // 0x0faff0
};

// 0x0faff0
RVA(0x000faff0, 0x163)
i32 SaveRecord::Load(LoadStream* s) {
    if (!s) {
        return 0;
    }
    if (!m_0c) {
        return 0;
    }
    s->Read(&m_1c, 4);
    s->Read(&m_20, 4);
    s->Read(&m_24, 4);
    s->Read(&m_38, 4);
    s->Read(&m_3c, 4);
    s->Read(&m_40, 4);
    s->Read(&m_44, 4);
    s->Read(&m_48, 4);
    s->Read(m_4c, 0x100);
    s->Read(&m_14c, 4);
    s->Read(&m_150, 4);
    s->Read(&m_154, 4);
    s->Read(&m_158, 4);
    s->Read(&m_15c, 4);
    s->Read(m_168, 0x10);
    s->Read(m_178, 0x10);
    s->Read(m_188, 0x10);
    s->Read(m_198, 0x10);
    s->Read(&m_1a8, 4);
    s->Read(&m_1ac, 4);
    return 1;
}
