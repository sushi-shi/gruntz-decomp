#include <rva.h>
#include <Gruntz/StatusBarItem.h>
// SBI_RectOnly.cpp - Gruntz CSBI_RectOnly (C:\Proj\Gruntz).
// The constructor is matched byte-exact.
//
// CSBI_RectOnly derives from CStatusBarItem. The retail ctor inlines the base
// ctor (zeroing m_4/m_24/m_28; the base's m_8=0 store is dropped as dead because
// the derived ctor sets m_8=1), then stores its own vptr, then m_8=1. That fold
// is exactly why CStatusBarItem's ctor is inline in the shared header (MSVC 5.0
// will not fold an out-of-line base ctor).

// base vftable (CStatusBarItem) anchored out-of-line in this TU.
class CSBI_RectOnly : public CStatusBarItem {
public:
    CSBI_RectOnly();
    virtual int SbiVfunc0() OVERRIDE;

    // Engine-label backlog stubs.
    void Stub_0ffde0();
    int winapi_107d00_SetRect();
    void LoadBattlezItemConfig(int);
    void LoadMainStatusBarSprite();
    void UpdateStatusBarTabHighlight(int, int, int);
    void LoadDestructButtonSprite(int);
    void BuildGameTabResumeButton(int);
    void BuildGameTabPauseButton();
    void LoadGooCookingSprite(int);
    void UpdateRezConveyorStatusBar();
    void LoadRezMachineConfig();
    void UpdateRezMachineSnoozeStatusBar();
    void LoadChipMachineConfig();
    void UpdateFallingItemStatusBar(int, int, int);
    void UpdateRezMachineWakeStatusBar();
    void LoadMultiplayerBattlezConfig(int);
};

CStatusBarItem::~CStatusBarItem() {}
int CStatusBarItem::SbiVfunc0() {
    return 0;
}

// ---------------------------------------------------------------------------
// CSBI_RectOnly::CSBI_RectOnly()
// Inlines the CStatusBarItem base ctor (the dead m_8=0 store is elided), stores
// its own vptr, then sets m_8 = 1.
RVA(0x00101fa0, 0x1b)
CSBI_RectOnly::CSBI_RectOnly() {
    m_8 = 1;
}

int CSBI_RectOnly::SbiVfunc0() {
    return 1;
}

// -------------------------------------------------------------------------
// Engine-label backlog stubs.
// -------------------------------------------------------------------------
// @confidence: med
// @source: rtti-vptr
// @stub
RVA(0x000ffde0, 0x5b1)
void CSBI_RectOnly::Stub_0ffde0() {}

// @confidence: low
// @source: winapi:SetRect
// @stub
RVA(0x00107d00, 0x591)
int CSBI_RectOnly::winapi_107d00_SetRect() {
    return 0;
}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x000fdc00, 0x5c2)
void CSBI_RectOnly::LoadBattlezItemConfig(int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000fe6b0, 0x145)
void CSBI_RectOnly::LoadMainStatusBarSprite() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000fe910, 0xb8e)
void CSBI_RectOnly::UpdateStatusBarTabHighlight(int, int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000ffb20, 0x13a)
void CSBI_RectOnly::LoadDestructButtonSprite(int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00102180, 0x5f)
void CSBI_RectOnly::BuildGameTabResumeButton(int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00102200, 0x37)
void CSBI_RectOnly::BuildGameTabPauseButton() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x001055b0, 0x109)
void CSBI_RectOnly::LoadGooCookingSprite(int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00105990, 0x398)
void CSBI_RectOnly::UpdateRezConveyorStatusBar() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x00105e40, 0x62c)
void CSBI_RectOnly::LoadRezMachineConfig() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00106660, 0x68)
void CSBI_RectOnly::UpdateRezMachineSnoozeStatusBar() {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x00106bb0, 0x7bc)
void CSBI_RectOnly::LoadChipMachineConfig() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00107590, 0xc4)
void CSBI_RectOnly::UpdateFallingItemStatusBar(int, int, int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00107a10, 0x62)
void CSBI_RectOnly::UpdateRezMachineWakeStatusBar() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00107ae0, 0x1aa)
void CSBI_RectOnly::LoadMultiplayerBattlezConfig(int) {}
