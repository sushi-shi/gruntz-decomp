// NetSessionOpen.cpp - the multiplayer "BACKGND" session opener (0x0b77a0), re-homed
// from src/Stub/ApiCallers.cpp. Its only caller is CNetMgr::SetupMultiplayerSession (netmgr). Its
// two sub-objects are folded onto their real engine classes:
//   - m_524 (the connection) IS a CNetMgr - Connect@0x1780b0 == CNetMgr::InitFromProvider.
//   - m_c->m_4 (the sub)     IS a CDDrawSubMgrPages - Init@0x158dc0 == Method_158dc0.
//
// SIGNATURE VERDICT (disasm-verified, 0xb77dd): the connect call emits `sub esp,0x10`
// + four GUID-dword stores + `push descriptor` then `call 0x1780b0`, i.e. the app GUID
// is passed BY VALUE (4 dwords) after the descriptor. InitFromProvider's body confirms
// it: `a` is the descriptor (reads a+4 for the DirectPlay GUID pointer) and its four
// trailing dwords are recorded into the m_4 setup block - so the proven shape is
// InitFromProvider(void* descriptor, GUID appGuid). The canonical signature was
// reconciled to that (was void*,int x4 - an ABI-equal reconstruction), which keeps this
// call byte-exact. Offsets + code bytes are load-bearing.
#include <Net/NetMgr.h>                // CNetMgr (m_524) + GUID (via <Mfc.h>)
#include <DDrawMgr/DDrawSubMgrPages.h> // CDDrawSubMgrPages (m_c->m_4)
#include <Ints.h>
#include <rva.h>

// The DirectPlay application GUID (DAT_0060fab8), passed by value to InitFromProvider.
DATA(0x0020fab8)
extern GUID g_dplayAppGuid;
DATA(0x00248cf0)
extern i32 g_isHost_648cf0; // DAT_00648cf0: nonzero when hosting

struct NetSessionHolder {
    char m_pad0[4];
    CDDrawSubMgrPages* m_4; // +0x04  the sub-object (Init == Method_158dc0)
};
// The session-setup coordinator (`this`). Reached via CNetMgr::SetupMultiplayerSession.
struct NetSessionOpener {
    char m_pad0[0xc];
    NetSessionHolder* m_c; // +0x0c
    char m_pad10[0x524 - 0x10];
    CNetMgr* m_524;                                         // +0x524  the CNetMgr connection
    i32 m_528;                                              // +0x528  host/join role flag
    void Configure(char* name, i32 a, i32 b, i32 c, i32 d); // RVA 0x3445 (near-thunk)
    i32 Build();                                            // RVA 0x3db9
    i32 HostStart();                                        // RVA 0x39bd
    i32 JoinStart();                                        // RVA 0x2487
    i32 Open();
};

// __thiscall(): configure the session, build it, connect via DirectPlay using the app
// GUID, then host- or join-start depending on g_isHost_648cf0.
RVA(0x000b77a0, 0xb5)
i32 NetSessionOpener::Open() {
    if (!m_524) {
        return 0;
    }
    Configure("BACKGND", 0, 0, 1, 0);
    m_c->m_4->Method_158dc0();
    i32 descriptor = Build();
    if (!descriptor) {
        return 0;
    }
    if (!m_524->InitFromProvider((void*)descriptor, g_dplayAppGuid)) {
        return 0;
    }
    if (g_isHost_648cf0) {
        m_528 = 1;
        if (!HostStart()) {
            return 0;
        }
    } else {
        m_528 = 0;
        if (!JoinStart()) {
            return 0;
        }
    }
    return 1;
}

SIZE_UNKNOWN(NetSessionHolder);
SIZE_UNKNOWN(NetSessionOpener);
