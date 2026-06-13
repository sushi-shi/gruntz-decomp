// GameApp.cpp - WAP32 CGameApp (Brian Goble's engine).
// Matched: ??0CGameApp@@QAE@XZ @ RVA 0x13d590 (byte-exact code; the instance
// counter at 0x653c6c is a file-scope global here - same store sequence, the
// reloc just names a different symbol than the Ghidra DAT_ at that address).
#include "Wap32.h"

// File-scope instance counter (binary: global int @ 0x653c6c, bumped per ctor).
static int s_gameAppCount;

CGameApp::CGameApp()
{
    m_4   = 0;
    m_8   = 0;
    m_10  = 0;   // the optimiser schedules the +0x10 store before +0x0c
    m_c   = 0;
    m_240 = 0;
    m_248 = 0;
    m_24c = 0;
    m_250 = 0;
    s_gameAppCount++;
}

CGameApp::~CGameApp() {}
int CGameApp::Wap32GameAppVfunc0() { return 0; }
