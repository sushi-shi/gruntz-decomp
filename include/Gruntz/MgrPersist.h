// MgrPersist.h - CMgrPersistObj, the persisted game-manager/settings object
// whose Save/Load (0xfb1c0 / 0xfaff0, ex MgrObjSerialize.cpp + SaveRecordLoad.cpp -
// two views of this one object, now folded) stream its fields through the shared
// WAP32 CSerialArchive slots (Read @+0x2c / Write @+0x30). Init (0xface0) drives the
// "loading imagez" splash + GAME_IMAGEZ load.
//
// Identity note: Init @0xface0 is MISATTRIBUTED here - it is genuinely
// CState::InputVirtual (slot 8, data-ref @0x1ea23c == ??_7CState@@6B@+0x20; SYMBOL'd
// as such in Attract.cpp so the leaf overrides' base calls bind). Its object's
// +0x04/+0x08/+0x0c prefix (m_displayMgr/m_rezLocator/m_levelData) IS the CState
// m_4/m_8/m_c prefix (the game-manager / bank-mgr / world-holder back-pointers),
// which is why it reads like a CState facet. The full body-shape fold of Init onto
// CState is DEFERRED (it needs CState's serialize-interior + m_8 type reconciled);
// this object is modeled here as a real (placeholder-named) header type, not a
// .cpp-local view. Save/Load are its own genuine methods. Only OFFSETS + code bytes
// are load-bearing; field/class names are best-guess placeholders.
#ifndef GRUNTZ_GRUNTZ_MGRPERSIST_H
#define GRUNTZ_GRUNTZ_MGRPERSIST_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/String.h>        // MFC CString (SplashParams::text; the loaded caption)
#include <Gruntz/SerialArchive.h> // CSerialArchive typedef (== CFileMemBase); NEVER fwd-decl
                                  // it under an elaborated struct name (see that header)

struct CSpriteFactoryHolder; // m_levelData @+0x0c: the world/resource holder (== CState::m_c;
                             // its +0x04 CDDrawSubMgrPages worker + +0x10 CImageRegistry)
class CGruntzMgr;            // m_displayMgr @+0x04: the game-manager (== CState::m_4; its
                             // +0x8c/+0x90 live video mode feed the splash block)
class CSymParser;            // m_rezLocator @+0x08: the rez path resolver (ResolvePath)

// The on-screen splash params block built on the stack for EngStr_DrawText; its
// leading slot is the loaded caption CString (its dtor forces Init's /GX frame).
// (EngStr_DrawText itself: the ONE canonical lean decl in <Wap32/EngStr.h> - the
// former (CSpriteFactoryHolder*, SplashParams*, i32*) spelling here mangled to a
// symbol the definition never emits, leaving the caller's reloc UNBOUND.)
struct SplashParams {
    CString text; // +0x00
    i32 m_04;     // +0x04
    i32 m_08, m_0c, m_10, m_14;
};

// The persisted object. Only the serialized fields are named. NB the Save/Load
// method names are recovered-symbol placeholders; the archive object drives the
// actual transfer direction, only the +0x2c/+0x30 slot offsets are load-bearing.
struct CMgrPersistObj {
    i32 m_00;                          // +0x00
    CGruntzMgr* m_displayMgr;          // +0x04  == CState::m_4 (m_8c/m_90 video mode)
    CSymParser* m_rezLocator;          // +0x08  rez path resolver
    CSpriteFactoryHolder* m_levelData; // +0x0c  == CState::m_c (world/resource holder)
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

    i32 Save(CSerialArchive* w); // 0x0fb1c0 (+0x2c slot pass)
    i32 Load(CSerialArchive* s); // 0x0faff0 (+0x30 slot pass; ex SaveRecord::Load)
    i32 Init();                  // 0x0face0 (misattributed CState::InputVirtual; SYMBOL'd)
};

#endif // GRUNTZ_GRUNTZ_MGRPERSIST_H
