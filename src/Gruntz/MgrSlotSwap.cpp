// MgrSlotSwap.cpp - 0x1128b0 (__thiscall) on a small game object holding a token at
// +0x34 plus the (group m_08, index m_0c) coordinates. If the token is live it swaps
// the token currently parked in the global registry's plane table
// (g_mgrSettings->m_30->m_24->m_5c->m_20[ m_24[m_0c] + m_08 ]) with its own, notifies
// the registry's m_70 sub-manager, and adopts the previously-parked token. An empty
// token reports the 0x8009/0x451 diagnostic and returns 0. Every callee + the global
// are reloc-masked.
#include <Ints.h>
#include <rva.h>

// The registry plane table: a value plane (m_20) indexed by an offset plane (m_24).
struct RegPlane {
    char m_pad0[0x20];
    i32* m_20; // +0x20  value table
    i32* m_24; // +0x24  per-group offset table
};

struct RegLevel { // g_mgrSettings->m_30->m_24
    char m_pad0[0x5c];
    RegPlane* m_5c; // +0x5c
};

struct RegM30 {
    char m_pad0[0x24];
    RegLevel* m_24; // +0x24
};

struct RegSubMgr { // g_mgrSettings->m_70
    void Notify(i32 group, i32 index, i32 token); // 0x33f0 (thiscall)
};

struct WwdGameReg {
    void Report(i32 code, i32 param); // 0x346d (thiscall diagnostic)
    char m_pad0[0x30];
    RegM30* m_30; // +0x30
    char m_pad34[0x70 - 0x34];
    RegSubMgr* m_70; // +0x70
};

extern WwdGameReg* g_mgrSettings;
DATA(0x0024556c)
extern WwdGameReg* g_mgrSettings; // ?g_mgrSettings (VA 0x64556c)

// The owning object: group/index coordinates + the parked token.
struct CSlotHolder {
    i32 DoSwap(); // 0x1128b0
    char m_pad0[0x08];
    i32 m_08; // +0x08  group
    i32 m_0c; // +0x0c  index
    char m_pad10[0x34 - 0x10];
    i32 m_34; // +0x34  token
};

// ---------------------------------------------------------------------------
RVA(0x001128b0, 0x88)
i32 CSlotHolder::DoSwap() {
    i32 oldTok = this->m_34;
    if (oldTok == 0) {
        g_mgrSettings->Report(0x8009, 0x451);
        return 0;
    }
    i32 newTok = g_mgrSettings->m_30->m_24->m_5c
                     ->m_20[g_mgrSettings->m_30->m_24->m_5c->m_24[this->m_0c]
                            + this->m_08];
    g_mgrSettings->m_30->m_24->m_5c
        ->m_20[g_mgrSettings->m_30->m_24->m_5c->m_24[this->m_0c] + this->m_08]
        = oldTok;
    g_mgrSettings->m_70->Notify(this->m_08, this->m_0c, oldTok);
    this->m_34 = newTok;
    return 1;
}
