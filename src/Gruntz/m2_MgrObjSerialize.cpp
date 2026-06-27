// m2_MgrObjSerialize.cpp - serialize methods of a large persisted game-mgr object
// (C:\Proj\Gruntz). The object's Save writes its fields through a writer/archive
// whose Write(buf,len) is virtual slot 11 (vtbl+0x2c); the whole pass is gated on
// the CGruntzMgr settings singleton (_g_mgrSettings) being live. Offsets + code
// bytes are load-bearing; field/class names are best-guess placeholders.
#include <Ints.h>
#include <rva.h>

// The settings singleton gate: g_mgrSettings->m_30 must be non-null to serialize.
struct CMgrSettingsGate {
    char m_pad0[0x30];
    void* m_30; // +0x30
};
extern "C" CMgrSettingsGate* g_mgrSettings; // _g_mgrSettings (VA 0x64556c)

// The archive/writer: Write(buf,len) is virtual slot 11 (vtbl+0x2c). The leading
// slots are never touched here, modeled only to place Write at the right offset.
struct CMgrWriter {
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
    virtual void Write(void* buf, i32 len); // slot 11 -> vtbl+0x2c
};

// The persisted object. Only the serialized fields are named.
struct CMgrPersistObj {
    char m_pad0[0x1c];
    i32 m_1c, m_20, m_24;
    char m_pad28[0x38 - 0x28];
    i32 m_38, m_3c, m_40, m_44, m_48;
    char m_4c[0x100]; // 0x4c..0x14c
    i32 m_14c, m_150, m_154, m_158, m_15c;
    char m_pad160[0x168 - 0x160];
    char m_168[0x10];
    char m_178[0x10];
    char m_188[0x10];
    char m_198[0x10];
    i32 m_1a8, m_1ac, m_1b0;

    i32 Save(CMgrWriter* w);
};

// 0xfb1c0 - CMgrPersistObj::Save: gate on the writer + the settings singleton, then
// stream every persisted field through the writer's Write(buf,len) virtual.
RVA(0x000fb1c0, 0x168)
i32 CMgrPersistObj::Save(CMgrWriter* w) {
    if (w == 0) {
        return 0;
    }
    if (g_mgrSettings->m_30 == 0) {
        return 0;
    }
    w->Write(&m_1c, 4);
    w->Write(&m_20, 4);
    w->Write(&m_24, 4);
    w->Write(&m_38, 4);
    w->Write(&m_3c, 4);
    w->Write(&m_40, 4);
    w->Write(&m_44, 4);
    w->Write(&m_48, 4);
    w->Write(m_4c, 0x100);
    w->Write(&m_14c, 4);
    w->Write(&m_150, 4);
    w->Write(&m_154, 4);
    w->Write(&m_158, 4);
    w->Write(&m_15c, 4);
    w->Write(m_168, 0x10);
    w->Write(m_178, 0x10);
    w->Write(m_188, 0x10);
    w->Write(m_198, 0x10);
    w->Write(&m_1a8, 4);
    w->Write(&m_1ac, 4);
    w->Write(&m_1b0, 4);
    return 1;
}
