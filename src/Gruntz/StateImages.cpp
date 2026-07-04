// StateImages.cpp - a CState/CAttract-family vfunc-8 image loader (RVA 0xa09a0).
//
// Resolves the "IMAGEZ" namespace in the state's symbol table and asks the active
// DDraw worker registry (mgr->m_workerReg) to load it (vtable slot +0x4c); on success it
// runs the per-state image hook (this vtable slot +0x18) and forces the cursor
// hidden. Field names are placeholders; only offsets + code bytes are load-bearing.
#include <rva.h>
#include <Win32.h> // WINAPI (windows.h) for the g_ShowCursor import-pointer type

#include <Bute/SymTab.h>
#include <DDrawMgr/DDrawWorkerMgr.h> // the ONE CDDrawWorkerMgr shape (Method_158ee0)

// The engine helper at 0xface0 (returns nonzero when the state may load images).
i32 Unmatched_0face0(); // ?Unmatched_0face0@@YA...XZ

// The cursor-show counter, cached in a game-owned function pointer (ff 15).
DATA(0x006c44c4)
extern i32(WINAPI* g_ShowCursor)(i32);

// The active DDraw worker registry (mgr->m_workerReg): only slot +0x4c (load namespace)
// is reached; the lower slots pad the vtable to the right index.
SIZE_UNKNOWN(WorkerReg);
class WorkerReg {
public:
    virtual void Slot00();
    virtual void Slot04();
    virtual void Slot08();
    virtual void Slot0C();
    virtual void Slot10();
    virtual void Slot14();
    virtual void Slot18();
    virtual void Slot1C();
    virtual void Slot20();
    virtual void Slot24();
    virtual void Slot28();
    virtual void Slot2C();
    virtual void Slot30();
    virtual void Slot34();
    virtual void Slot38();
    virtual void Slot3C();
    virtual void Slot40();
    virtual void Slot44();
    virtual void Slot48();
    virtual i32 LoadNamespace(void* tree, char* a, char* b); // +0x4c
};

SIZE_UNKNOWN(StateMgr);
struct StateMgr {
    char m_pad00[0x4];
    CDDrawWorkerMgr* m_workerMgr; // +0x04
    char m_pad08[0x10 - 0x8];
    WorkerReg* m_workerReg; // +0x10 active worker registry
};

SIZE_UNKNOWN(CImageState);
class CImageState {
public:
    virtual void V00();
    virtual void V04();
    virtual void V08();
    virtual void V0C();
    virtual void V10();
    virtual void V14();
    virtual i32 RunImageHook(); // slot 6 (+0x18)
    char m_pad04[0x0c - 0x04];
    StateMgr* m_stateMgr; // +0x0c
    char m_pad10[0x2c - 0x10];
    CSymTab* m_symTab; // +0x2c state symbol table
    i32 LoadStateImages();
};

RVA(0x000a09a0, 0x6a)
i32 CImageState::LoadStateImages() {
    if (Unmatched_0face0() == 0) {
        return 0;
    }
    void* tree = m_symTab->ResolvePath("IMAGEZ");
    if (tree == 0) {
        return 0;
    }
    if (m_stateMgr->m_workerReg->LoadNamespace(tree, "MENU", "_") == -1) {
        return 0;
    }
    if (RunImageHook() == 0) {
        return 0;
    }
    i32(WINAPI * sc)(i32) = g_ShowCursor;
    i32 r = sc(1);
    while (r < 0) {
        r = sc(1);
    }
    return 1;
}

// CBootyState::OnActivate2 (slot-8 vfunc, RVA 0x1c8a0): hide the cursor, load the
// BOOTY + GRUNTZ image namespaces, then either pop the secret-bonus toast (when
// m_activation==200) or fade the "bg" title and show the level-complete toast.
// This is the REAL CBootyState (vtable 0x5e9cec) slot 8 - NOT its sibling
// CMultiBootyState (0x5e9bdc); GameMode.h declares this slot @0x01c8a0 as
// "declared-only; StateImages". NOT folded onto the canonical CBootyState : CState
// (<Gruntz/GameMode.h>): (1) this is a pure-Win32 TU (<Win32.h> -> windows.h FIRST) and
// GameMode.h pulls <Mfc.h> -> afx (C1189 wall); (2) the canonical CBootyState declares
// no data members (its fields are reached by offset), so the +0xc/+0x2c/+0x30/+0x1bc
// members below have no canonical name to fold to yet (P2 identity-recovery). Kept as a
// minimal REAL-named Win32-safe partial. C1189 + memberless-canonical wall.
class CBootyState {
public:
    i32 FadeInTitle(char* name, i32 a, i32 b, i32 c, i32 d, i32 e); // 0x0fa1f0
    void ShowSecretBonusMessage();                                  // 0x0fa540
    void ShowLevelCompleteMessage();                                // 0x0fa120
    void RetireScene(i32 a, i32 b, i32 c, i32 d);                   // 0x0fa8f0
    i32 OnActivate2();

    char m_pad00[0x0c];
    StateMgr* m_stateMgr; // +0x0c
    char m_pad10[0x2c - 0x10];
    CSymTab* m_bootySymTab;  // +0x2c BOOTY symbol table
    CSymTab* m_gruntzSymTab; // +0x30 GRUNTZ symbol table
    char m_pad34[0x1bc - 0x34];
    i32 m_activation; // +0x1bc activation discriminator
};

RVA(0x0001c8a0, 0xec)
i32 CBootyState::OnActivate2() {
    if (Unmatched_0face0() == 0) {
        return 0;
    }
    i32(WINAPI * sc)(i32) = g_ShowCursor;
    i32 r = sc(0);
    while (r >= 0) {
        r = sc(0);
    }
    void* booty = m_bootySymTab->ResolvePath("IMAGEZ");
    if (booty == 0) {
        return 0;
    }
    if (m_stateMgr->m_workerReg->LoadNamespace(booty, "BOOTY", "_") == -1) {
        return 0;
    }
    void* gruntz = m_gruntzSymTab->ResolvePath("IMAGEZ");
    if (gruntz == 0) {
        return 0;
    }
    if (m_stateMgr->m_workerReg->LoadNamespace(gruntz, "GRUNTZ", "_") == -1) {
        return 0;
    }
    if (m_activation != 200) {
        if (FadeInTitle("bg", 0, 0, 0, 0, 1) == 0) {
            return 0;
        }
        ShowLevelCompleteMessage();
    } else {
        ShowSecretBonusMessage();
    }
    m_stateMgr->m_workerMgr->Method_158ee0();
    RetireScene(0x50, 0x3e8, 0, 1);
    return 1;
}
