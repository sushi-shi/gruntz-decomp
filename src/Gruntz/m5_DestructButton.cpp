// m5_DestructButton.cpp - CSBI_RectOnly::UpdateDestructButton (RVA 0x10bc30).
//
// Arms the "destruct button" status-bar warning: latch the snapshot timer on the
// active CPlay (from g_mgrSettings), seed the warning-delay window from the
// StatusBar/DestructButtonWarningDelay bute setting, then drop the item to mode 0.
// Field names are placeholders; only offsets + code bytes are load-bearing.
#include <Bute/ButeMgr.h> // canonical CButeMgr (one shape)
#include <rva.h>

#include <Ints.h>

// CPlay (active game-mode object): only ArmSnapshot is reached here.
class CPlay {
public:
    i32 ArmSnapshot(i32 active, i32 dur); // 0x0d9240
};

struct MgrSettings {
    char m_pad00[0x2c];
    CPlay* m_2c; // +0x2c active CPlay
};
extern "C" MgrSettings* g_mgrSettings; // 0x64556c

// The CButeMgr text-config singleton (?g_buteMgr@@3VCButeMgr@@A @ 0x6453d8).
// GetDwordDef (0x1721e0) is on the canonical CButeMgr (include/Bute/ButeMgr.h).
DATA(0x006453d8)
extern CButeMgr g_buteMgr;

DATA(0x00645588)
extern i32 g_645588; // free-running clock global

class CSBI_RectOnly {
public:
    void SetMode(i32 mode);             // 0x0014ba
    void UpdateDestructButton(i32 arg); // 0x10bc30
    char m_pad000[0x558];
    i32 m_558; // +0x558
    i32 m_55c; // +0x55c
    i32 m_560; // +0x560
    i32 m_564; // +0x564
    i32 m_568; // +0x568
    i32 m_56c; // +0x56c
};

RVA(0x0010bc30, 0x78)
void CSBI_RectOnly::UpdateDestructButton(i32 arg) {
    CPlay* play = g_mgrSettings->m_2c;
    m_558 = 1;
    m_55c = 2;
    m_568 = g_buteMgr.GetDwordDef("StatusBar", "DestructButtonWarningDelay", 0x32);
    m_56c = 0;
    m_560 = g_645588;
    m_564 = 0;
    play->ArmSnapshot(1, arg);
    SetMode(0);
}
