// HelpState.cpp - CHelpState, the help-screen game state (CHelpState : CState,
// RTTI .?AVCHelpState@@, vtbl@0x1e9dfc). Real-class TU merged from the former
// StateLeaf8cf30.cpp (the /GX dtor + vtables) and the CHelpState::LoadAssets
// fragment of BacklogStateLoaders.cpp (the accepted dual-view is now one TU).
//
//   0x08cf30  ~CHelpState - the /GX leaf dtor: stamp the derived vtable (0x5e9dfc =
//             ??_7CHelpState), run the member teardown (0x1357) under the EH frame,
//             then chain the CState vtable (0x5ea21c) + base dtor.
//   0x095090  LoadAssets  - chain the base asset loader (reloc-masked), hide the
//             cursor, resolve the "STATEZ_HELP" bank through the inherited m_8
//             (CBankMgr, cached at m_2c), then pump a fixed message burst through
//             the owner's game window (m_4->m_gameWnd).
//
// Only offsets + code bytes are load-bearing; every engine callee is a reloc-masked
// external. The overridden CState slots are declared so the emitted ??_7CHelpState
// carries CHelpState's overrides (the other slots inherit CState's).
#include <Mfc.h> // ShowCursor (afx-first)
#include <Bute/SymTab.h>
#include <Bute/SymParser.h>

#include <Gruntz/BankMgr.h>   // CBankMgr::Lookup (inherited m_8) -> CResSource
#include <Gruntz/GruntzMgr.h> // CGruntzMgr m_4 + m_gameWnd->PumpMessages (pulls State.h/Wap32.h)
#include <rva.h>

// The 11 overridden CState slots (vtbl@0x1e9dfc; the other slots inherited). The
// override bodies live in the class's other (unmatched) TUs; declared-only here.
class CHelpState : public CState {
public:
    virtual ~CHelpState() OVERRIDE;              // slot 0  (0x8cf30)
    virtual i32 Vfunc1(i32, i32, i32) OVERRIDE;  // slot 1
    virtual void ReleaseResources() OVERRIDE;    // slot 2
    virtual GameStateId Update() OVERRIDE;       // slot 4
    virtual i32 Render() OVERRIDE;               // slot 5
    virtual i32 Vslot06() OVERRIDE;              // slot 6
    virtual i32 InputVirtual() OVERRIDE;         // slot 8
    virtual i32 Vslot09(i32) OVERRIDE;           // slot 9
    virtual i32 FrameSlot28(i32) OVERRIDE;       // slot 10
    virtual i32 Vslot0c(i32, i32) OVERRIDE;      // slot 12
    virtual i32 Vslot0e(i32, i32, i32) OVERRIDE; // slot 14

    void Teardown();                            // 0x1357  member teardown body
    i32 LoadAssets(i32, i32, i32);              // 0x95090
    i32 LoadGameAssetNamespaces(i32, i32, i32); // base loader; reloc-masked external
};

RVA(0x0008cf30, 0x55)
CHelpState::~CHelpState() {
    Teardown();
}

RVA(0x00095090, 0x6e)
i32 CHelpState::LoadAssets(i32 a1, i32 a2, i32 a3) {
    if (!LoadGameAssetNamespaces(a1, a2, a3)) {
        return 0;
    }
    while (ShowCursor(0) >= 0)
        ;
    m_2c = (CResSource*)((CSymParser*)m_8)->ResolvePath("STATEZ_HELP");
    if (!m_2c) {
        return 0;
    }
    m_4->m_gameWnd->PumpMessages(0x100, 0x40);
    return 1;
}

SIZE_UNKNOWN(CHelpState);
