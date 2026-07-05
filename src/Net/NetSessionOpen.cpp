// NetSessionOpen.cpp - the multiplayer "BACKGND" session opener (0x0b77a0), re-homed
// from src/Stub/ApiCallers.cpp. Its only caller is CNetMgr::Stub_0b5460 (netmgr), and
// its members are proven real engine classes:
//   - m_524 (the connection) IS a CNetMgr: its Connect@0x1780b0 is
//     ?InitFromProvider@CNetMgr (netmgr, 96% reconstructed).
//   - m_c->m_4 (the sub) IS a CDDrawSubMgrPages: its Init@0x158dc0 is
//     ?Method_158dc0@CDDrawSubMgrPages (ddrawsubmgr) - the "view of something bigger".
// Kept as named views here (rather than the real classes) only to preserve the 100%
// byte match: Connect takes the app GUID BY VALUE (4 dwords) where CNetMgr::
// InitFromProvider's reconstructed signature is (void*, int x4); dissolving m_524 to
// CNetMgr would re-shape that call and must be re-verified before it lands. Offsets +
// code bytes are load-bearing.
#include <Win32.h> // GUID (the DirectPlay app GUID passed by value to Connect)
#include <Ints.h>
#include <rva.h>

// The DirectPlay application GUID (DAT_0060fab8), passed by value to Connect.
DATA(0x0020fab8)
extern GUID g_dplayAppGuid;
DATA(0x00248cf0)
extern i32 g_isHost_648cf0; // DAT_00648cf0: nonzero when hosting

// The connection object (m_524). IS a CNetMgr - Connect == CNetMgr::InitFromProvider @0x1780b0.
struct NetSessionConn {
    i32 Connect(i32 sessionFlags, GUID guid); // thiscall, RVA 0x1780b0 (CNetMgr::InitFromProvider)
};
// The sub-object (m_c->m_4). IS a CDDrawSubMgrPages - Init == Method_158dc0 @0x158dc0.
struct NetSessionSub {
    void Init(); // thiscall, RVA 0x158dc0 (CDDrawSubMgrPages::Method_158dc0, QAEHXZ)
};
struct NetSessionHolder {
    char m_pad0[4];
    NetSessionSub* m_4; // +0x04
};
// The session-setup coordinator (`this`). Reached via CNetMgr::Stub_0b5460.
struct NetSessionOpener {
    char m_pad0[0xc];
    NetSessionHolder* m_c; // +0x0c
    char m_pad10[0x524 - 0x10];
    NetSessionConn* m_524;                                  // +0x524  the CNetMgr connection
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
    m_c->m_4->Init();
    i32 sessionFlags = Build();
    if (!sessionFlags) {
        return 0;
    }
    if (!m_524->Connect(sessionFlags, g_dplayAppGuid)) {
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

SIZE_UNKNOWN(NetSessionConn);
SIZE_UNKNOWN(NetSessionSub);
SIZE_UNKNOWN(NetSessionHolder);
SIZE_UNKNOWN(NetSessionOpener);
