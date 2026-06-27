// m2_MgrObjSerialize.cpp - serialize methods of a large persisted game-mgr object
// (C:\Proj\Gruntz). The object's Save writes its fields through a writer/archive
// whose Write(buf,len) is virtual slot 11 (vtbl+0x2c); the whole pass is gated on
// the CGruntzMgr settings singleton (_g_mgrSettings) being live. Offsets + code
// bytes are load-bearing; field/class names are best-guess placeholders.
#include <Mfc.h>
#include <Ints.h>
#include <rva.h>

// The settings singleton gate: g_mgrSettings->m_30 must be non-null to serialize.
struct CMgrSettingsGate {
    char m_pad0[0x30];
    void* m_30; // +0x30
};
extern "C" CMgrSettingsGate* g_mgrSettings; // _g_mgrSettings (VA 0x64556c)

// Win32 ShowCursor reached through an import pointer; the loading screen hides the
// cursor (calls until the display count goes negative).
extern "C" int(__stdcall* g_ShowCursor)(int);  // ?g_ShowCursor@@3P6GHH@ZA (0x6c44c4)
DATA(0x0024e35c) extern i32 g_64e35c;           // 0x64e35c "splash drawn" latch

// A sub-object (m_0c->m_04) whose Method158bc0 must be live to proceed.
struct CMgrReady {
    i32 Method158bc0();  // 0x158bc0
};

// The image-set the object loads into (m_0c->m_10); slot 19 (vtbl+0x4c) loads it.
struct CImageSet {
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
    virtual void v12();
    virtual void v13();
    virtual void v14();
    virtual void v15();
    virtual void v16();
    virtual void v17();
    virtual void v18();
    virtual i32 Load(char* path, const char* a, const char* b);  // slot 19 -> vtbl+0x4c
};

// The level-data object (m_0c) and the renderer it owns.
struct CLevelData {
    char pad00[4];
    CMgrReady* m_04;   // +0x04
    char pad08[0x10 - 0x08];
    CImageSet* m_10;   // +0x10
};

// The display owner (m_04): its m_8c/m_90 are blitted into the splash params.
struct CDisplayMgr {
    char pad00[0x8c];
    i32 m_8c;          // +0x8c
    i32 m_90;          // +0x90
};

// The resource locator (m_08): maps a logical name to a rez path.
struct CRezLocator {
    char* ResolvePath(const char* name);  // 0x13c030
};

// The on-screen splash params block built on the stack for EngStr_DrawText; its
// leading slot is the loaded caption CString.
struct SplashParams {
    CString text;  // +0x00
    i32 m_04;      // +0x04
    i32 m_08, m_0c, m_10, m_14;
};
void EngStr_DrawText(CLevelData* lvl, SplashParams* a, i32* b, i32 size, i32 e,
                     i32 f, i32 g, i32 h, i32 i);  // 0x115440

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
    i32 m_00;          // +0x00
    CDisplayMgr* m_04; // +0x04
    CRezLocator* m_08; // +0x08
    CLevelData* m_0c;  // +0x0c
    char m_pad10[0x1c - 0x10];
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
    i32 Init();  // 0xface0
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

// @early-stop
// /GX frame-packing artifact (~96%): the instruction stream is byte-faithful, but
// retail reserves `sub esp,0x14` and builds the splash block's tail two dwords in
// the transient arg-push area, where this cl reserves the whole block (`sub esp,
// 0x1c`), shifting every esp-relative displacement by 8; plus the EH scope-table
// symbol is named/represented differently by the delinker.  Logic is complete.
// 0xface0 - CMgrPersistObj::Init: hide the cursor, gate on the level being ready,
// draw the "loading imagez" splash (once), resolve the GAME_IMAGEZ rez and load it
// into the image-set, then mark the object started.
RVA(0x000face0, 0x17c)
i32 CMgrPersistObj::Init() {
    if (m_0c == 0)
        return 0;
    int(__stdcall * sc)(int) = g_ShowCursor;
    while (sc(0) >= 0)
        ;
    if (m_0c->m_04->Method158bc0() == 0)
        return 0;
    if (g_64e35c == 0) {
        SplashParams sp;
        sp.text.LoadString(0x81a9);
        sp.m_08 = m_04->m_8c;
        sp.m_0c = m_04->m_90;
        sp.m_10 = 0;
        sp.m_14 = 0;
        EngStr_DrawText(m_0c, &sp, &sp.m_04, 0x78, 1, 0xff, 0, 0xff, 1);
    }
    while (sc(0) >= 0)
        ;
    g_64e35c = 0;
    char* path = m_08->ResolvePath("GAME_IMAGEZ");
    if (path == 0)
        return 0;
    if (m_0c->m_10->Load(path, "GAME", "_") == -1)
        return 0;
    m_1a8 = 0;
    m_1ac = 1;
    m_1b0 = 0;
    return 1;
}
