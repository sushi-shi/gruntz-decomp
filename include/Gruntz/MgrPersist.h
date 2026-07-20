#ifndef GRUNTZ_GRUNTZ_MGRPERSIST_H
#define GRUNTZ_GRUNTZ_MGRPERSIST_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/String.h>        // MFC CString (SplashParams::text; the loaded caption)
#include <Gruntz/SerialArchive.h> // CSerialArchive typedef (== CFileMemBase); NEVER fwd-decl

class CDDrawSurfaceMgr; // m_levelData @+0x0c: the world/resource holder (== CState::m_c;
class CGruntzMgr;       // m_displayMgr @+0x04: the game-manager (== CState::m_4; its
class CSymParser;       // m_rezLocator @+0x08: the rez path resolver (ResolvePath)

struct SplashParams {
    CString text; // +0x00
    i32 m_04;     // +0x04
    i32 m_08, m_0c, m_10, m_14;
};

struct CMgrPersistObj {
    i32 m_00;                      // +0x00
    CGruntzMgr* m_displayMgr;      // +0x04  == CState::m_4 (m_8c/m_90 video mode)
    CSymParser* m_rezLocator;      // +0x08  rez path resolver
    CDDrawSurfaceMgr* m_levelData; // +0x0c  == CState::m_c (world/resource holder)
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
