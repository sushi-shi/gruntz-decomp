// MgrObjSerialize.cpp - serialize methods of a large persisted game-mgr object
// (C:\Proj\Gruntz). The object's Save streams its fields through the shared WAP32
// CSerialArchive's +0x2c slot (CSerialArchive::Read, virtual slot 11); the whole
// pass is gated on the CGruntzMgr settings singleton (_g_mgrSettings) being live.
// Offsets + code bytes are load-bearing; field/class names are best-guess placeholders.
#include <Mfc.h>
#include <Gruntz/CDDrawWorkerMgr.h> // canonical CDDrawWorkerMgr (m_levelData->m_ready)
#include <Gruntz/GruntzMgr.h>       // canonical CGruntzMgr (game-manager singleton; one true shape)
#include <Gruntz/SerialArchive.h>   // the ONE shared archive stream (Read@+0x2c / Write@+0x30)
#include <Ints.h>
#include <rva.h>

// The settings singleton gate: g_mgrSettings->m_world must be non-null to serialize.
extern "C" CGruntzMgr* g_mgrSettings; // _g_mgrSettings (VA 0x64556c)

// Win32 ShowCursor reached through an import pointer; the loading screen hides the
// cursor (calls until the display count goes negative).
extern "C" int(WINAPI* g_ShowCursor)(int); // ?g_ShowCursor@@3P6GHH@ZA (0x6c44c4)
DATA(0x0024e35c)
extern i32 g_64e35c; // 0x64e35c "splash drawn" latch

// The image-set the object loads into (m_levelData->m_imageSet). This is a FOREIGN
// engine class: its ??_7 and slots 0..18 are unreconstructed engine code, so the
// honest model is the ONE dispatched slot. Its vtable slot 19 (+0x4c, __thiscall)
// loads a resolved rez path under a name, modeled as a 4-byte member-function
// pointer loaded from the vtable (`char m_pad00[0x4c]` documents the un-recovered
// slots) so `m_imageSet->Load(...)` emits `mov eax,[ecx]; call [eax+0x4c]`. Class
// COMPLETE before the T::* typedef so the PMF stays 4 bytes
// (docs/patterns/pmf-complete-class-4byte.md).
struct CMgrImageSetVtbl;
struct CMgrImageSet {
    CMgrImageSetVtbl* m_vtbl;                           // +0x00
    i32 Load(char* path, const char* a, const char* b); // slot 19 (+0x4c) via vtbl
};
typedef i32 (CMgrImageSet::*CMgrImageSetLoadFn)(char* path, const char* a, const char* b);
SIZE_UNKNOWN(CMgrImageSetVtbl);
struct CMgrImageSetVtbl {
    char m_pad00[0x4c];
    CMgrImageSetLoadFn Load; // +0x4c
};
inline i32 CMgrImageSet::Load(char* path, const char* a, const char* b) {
    return (this->*(m_vtbl->Load))(path, a, b);
}

// The level-data object (m_levelData) and the renderer it owns.
struct CLevelData {
    char pad00[4];
    CDDrawWorkerMgr* m_ready; // +0x04 (the Method_158bc0 readiness gate)
    char pad08[0x10 - 0x08];
    CMgrImageSet* m_imageSet; // +0x10
};

// The display owner (m_displayMgr): its m_8c/m_90 are blitted into the splash params.
struct CDisplayMgr {
    char pad00[0x8c];
    i32 m_8c; // +0x8c
    i32 m_90; // +0x90
};

// The resource locator (m_rezLocator): maps a logical name to a rez path.
struct CRezLocator {
    char* ResolvePath(const char* name); // 0x13c030
};

// The on-screen splash params block built on the stack for EngStr_DrawText; its
// leading slot is the loaded caption CString.
struct SplashParams {
    CString text; // +0x00
    i32 m_04;     // +0x04
    i32 m_08, m_0c, m_10, m_14;
};
void EngStr_DrawText(
    CLevelData* lvl,
    SplashParams* a,
    i32* b,
    i32 size,
    i32 e,
    i32 f,
    i32 g,
    i32 h,
    i32 i
); // 0x115440

// The archive stream is the shared WAP32 CSerialArchive (declared-only virtuals in
// <Gruntz/SerialArchive.h>). This pass dispatches through its +0x2c slot
// (CSerialArchive::Read, the canonical load entry): `mov eax,[w]; push len; push
// buf; mov ecx,w; call [eax+0x2c]`. NB the method name Save here is the recovered-
// symbol placeholder; the archive object drives the actual transfer direction, only
// the +0x2c slot offset is load-bearing.

// The persisted object. Only the serialized fields are named.
struct CMgrPersistObj {
    i32 m_00;                  // +0x00
    CDisplayMgr* m_displayMgr; // +0x04
    CRezLocator* m_rezLocator; // +0x08
    CLevelData* m_levelData;   // +0x0c
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

    i32 Save(CSerialArchive* w);
    i32 Init(); // 0xface0
};

// CMgrPersistObj::Save: gate on the writer + the settings singleton, then
// stream every persisted field through the writer's Write(buf,len) virtual.
RVA(0x000fb1c0, 0x168)
i32 CMgrPersistObj::Save(CSerialArchive* w) {
    if (w == 0) {
        return 0;
    }
    if (g_mgrSettings->m_world == 0) {
        return 0;
    }
    w->Read(&m_1c, 4);
    w->Read(&m_20, 4);
    w->Read(&m_24, 4);
    w->Read(&m_38, 4);
    w->Read(&m_3c, 4);
    w->Read(&m_40, 4);
    w->Read(&m_44, 4);
    w->Read(&m_48, 4);
    w->Read(m_4c, 0x100);
    w->Read(&m_14c, 4);
    w->Read(&m_150, 4);
    w->Read(&m_154, 4);
    w->Read(&m_158, 4);
    w->Read(&m_15c, 4);
    w->Read(m_168, 0x10);
    w->Read(m_178, 0x10);
    w->Read(m_188, 0x10);
    w->Read(m_198, 0x10);
    w->Read(&m_1a8, 4);
    w->Read(&m_1ac, 4);
    w->Read(&m_1b0, 4);
    return 1;
}

// @early-stop
// /GX frame-packing artifact (~96%): the instruction stream is byte-faithful, but
// retail reserves `sub esp,0x14` and builds the splash block's tail two dwords in
// the transient arg-push area, where this cl reserves the whole block (`sub esp,
// 0x1c`), shifting every esp-relative displacement by 8; plus the EH scope-table
// symbol is named/represented differently by the delinker.  Logic is complete.
// CMgrPersistObj::Init: hide the cursor, gate on the level being ready,
// draw the "loading imagez" splash (once), resolve the GAME_IMAGEZ rez and load it
// into the image-set, then mark the object started.
RVA(0x000face0, 0x17c)
i32 CMgrPersistObj::Init() {
    if (m_levelData == 0) {
        return 0;
    }
    int(WINAPI * sc)(int) = g_ShowCursor;
    while (sc(0) >= 0)
        ;
    if (m_levelData->m_ready->Method_158bc0() == 0) {
        return 0;
    }
    if (g_64e35c == 0) {
        SplashParams sp;
        sp.text.LoadString(0x81a9);
        sp.m_08 = m_displayMgr->m_8c;
        sp.m_0c = m_displayMgr->m_90;
        sp.m_10 = 0;
        sp.m_14 = 0;
        EngStr_DrawText(m_levelData, &sp, &sp.m_04, 0x78, 1, 0xff, 0, 0xff, 1);
    }
    while (sc(0) >= 0)
        ;
    g_64e35c = 0;
    char* path = m_rezLocator->ResolvePath("GAME_IMAGEZ");
    if (path == 0) {
        return 0;
    }
    if (m_levelData->m_imageSet->Load(path, "GAME", "_") == -1) {
        return 0;
    }
    m_1a8 = 0;
    m_1ac = 1;
    m_1b0 = 0;
    return 1;
}

SIZE_UNKNOWN(CDisplayMgr);
SIZE_UNKNOWN(CMgrImageSet);
SIZE_UNKNOWN(CLevelData);
SIZE_UNKNOWN(CMgrPersistObj);
SIZE_UNKNOWN(CRezLocator);
SIZE_UNKNOWN(SplashParams);
