#include <Win32.h> // ShowCursor (reloc-masked)

#include <rva.h>
// BacklogStateLoaders.cpp - game-state asset/activate loaders + small free
// helpers graduated out of the src/Stub/Backlog.cpp aggregate into their own
// frameless /O2 (base) unit.  Each carries its real owning class; only offsets /
// code bytes are load-bearing (helpers are reloc-masked externals).

// ---------------------------------------------------------------------------
// CHelpState::LoadAssets (0x095090) - the help-screen game-state asset loader.
// First chains the base-class asset loader (LoadGameAssetNamespaces, reloc-masked
// external), forces the cursor hidden, registers the "STATEZ_HELP" namespace
// through m_8 (cached at m_2c), then pumps a fixed message burst through m_4->m_4.
// ---------------------------------------------------------------------------
struct CHelpAssetSet {          // m_8 points here
    void* Register(char* name); // FUN_0053c030, __thiscall, returns the registered object
};
struct CHelpMsgPump {          // m_4->m_4 points here
    void Pump(i32 msg, i32 n); // FUN_0053d4e0, __thiscall message burst
};
struct CHelpAssetRoot { // m_4 / arg1 points here
    char m_pad00[0x4];
    CHelpMsgPump* m_4; // +0x4
};
class CHelpState {
public:
    i32 LoadAssets(i32, i32, i32);
    i32 LoadGameAssetNamespaces(i32, i32, i32); // base loader; reloc-masked external call

    char m_pad00[0x4];
    CHelpAssetRoot* m_4; // +0x4
    CHelpAssetSet* m_8;  // +0x8
    char m_pad0c[0x2c - 0xc];
    void* m_2c; // +0x2c  the registered "STATEZ_HELP" object (0 on failure)
};

// ---------------------------------------------------------------------------
// GameLevelState::OnActivate_vfunc8 (0x0cb800) - the PLAY level state activation:
// chain the base activate, hide the cursor, register the TILEZ/IMAGEZ(LEVEL)/
// IMAGEZ(GRUNTZ) namespaces through the m_c->m_10 registrar (vtable slot +0x4c),
// then run the level-specific init chain (m_2dc map mgr, m_c sub-objects) and kick
// the state timer.  __thiscall; all callees are reloc-masked externals.
// ---------------------------------------------------------------------------
extern "C" char g_emptyString[]; // 0x6293f4 (engine global empty string)

// The namespace registrar reached via m_c->m_10: vtable slot +0x4c registers a
// looked-up image set under a name + separator (returns -1 on failure).  Modeled
// as a pointer-to-member so MSVC emits `mov edx,[ecx]; call [edx+0x4c]`; the class
// must be COMPLETE before the PMF typedef (pmf-complete-class-4byte.md).
struct GLSRegistrarVtbl;
struct GLSRegistrar {
    GLSRegistrarVtbl* m_vtbl;
    i32 CallRegister(i32 handle, char* name, char* sep);
};
typedef i32 (GLSRegistrar::*GLSRegFn)(i32 handle, char* name, char* sep);
struct GLSRegistrarVtbl {
    char m_pad00[0x4c];
    GLSRegFn Register; // +0x4c
};
inline i32 GLSRegistrar::CallRegister(i32 handle, char* name, char* sep) {
    return (this->*(m_vtbl->Register))(handle, name, sep);
}
struct GLSNamespace {         // m_28 / m_30
    i32 Lookup(char* szName); // FUN_0013bae0 __thiscall
};
struct GLSSub2c {
    void Step(i32); // FUN_0013e760 __thiscall
};
struct GLSSub14 {
    char m_pad00[0x2c];
    GLSSub2c* m_2c; // +0x2c
};
struct GLSSubA {  // m_c->m_4
    void Begin(); // FUN_00558e90 __thiscall
    char m_pad00[0x14];
    GLSSub14* m_14; // +0x14
    void* m_18;     // +0x18
};
struct GLSObj24 {                    // m_c->m_24
    void Wire(GLSSub14* a, void* b); // FUN_0015dc90 __thiscall
};
struct GLSPolyCVtbl;
struct GLSPolyC { // m_c->m_c
    GLSPolyCVtbl* m_vtbl;
    void CallSlot34(GLSSub14* a, void* b);
};
typedef void (GLSPolyC::*GLSSlot34Fn)(GLSSub14* a, void* b);
struct GLSPolyCVtbl {
    char m_pad00[0x34];
    GLSSlot34Fn Slot34; // +0x34
};
inline void GLSPolyC::CallSlot34(GLSSub14* a, void* b) {
    (this->*(m_vtbl->Slot34))(a, b);
}
struct GLSAssetRoot { // this->m_c
    char m_pad00[0x4];
    GLSSubA* m_4;       // +0x04
    void* m_8;          // +0x08
    GLSPolyC* m_c;      // +0x0c
    GLSRegistrar* m_10; // +0x10
    char m_pad14[0x24 - 0x14];
    GLSObj24* m_24; // +0x24
};
struct GLSMapMgr {    // this->m_2dc
    void Finalize();  // FUN_0040125d __thiscall
    void Activate2(); // FUN_004021b7 __thiscall
};
// A singleton mgr (DAT_00645570) reset before the second cursor-hide.
struct GLSResetMgr {
    void Reset(); // FUN_00533110 __thiscall
};
extern GLSResetMgr* g_glsResetMgr;
// The game-manager singleton (0x64556c); mangled ?g_gameReg@@3PAUWwdGameReg@@A.
struct WwdGameReg;
DATA(0x0064556c)
extern WwdGameReg* g_gameReg;
// cdecl level-init helper (g_gameReg, this->m_2dc, this->m_470).
void LevelInit2356(WwdGameReg* gameReg, GLSMapMgr* mapMgr, i32 a3); // reloc-masked

class GameLevelState {
public:
    i32 OnActivate_vfunc8();
    i32 BaseOnActivate();                // base vfunc8 (reloc-masked)
    void Method1ae6();                   // FUN_00401ae6 __thiscall (m_474 != 0 arm)
    void StartTimer(i32, i32, i32, i32); // FUN_00401843 __thiscall

    char m_pad00[0xc];
    GLSAssetRoot* m_c; // +0x0c
    char m_pad10[0x28 - 0x10];
    GLSNamespace* m_28; // +0x28
    char m_pad2c[0x30 - 0x2c];
    GLSNamespace* m_30; // +0x30
    char m_pad34[0x2dc - 0x34];
    GLSMapMgr* m_2dc; // +0x2dc
    char m_pad2e0[0x470 - 0x2e0];
    i32 m_470; // +0x470
    i32 m_474; // +0x474
    char m_pad478[0x510 - 0x478];
    i32 m_510; // +0x510
};

RVA(0x000cb800, 0x191)
i32 GameLevelState::OnActivate_vfunc8() {
    if (!BaseOnActivate()) {
        return 0;
    }
    while (ShowCursor(FALSE) >= 0)
        ;

    i32 h = m_28->Lookup("TILEZ");
    if (!h) {
        return 0;
    }
    if (m_c->m_10->CallRegister(h, g_emptyString, "_") == -1) {
        return 0;
    }

    h = m_28->Lookup("IMAGEZ");
    if (!h) {
        return 0;
    }
    if (m_c->m_10->CallRegister(h, "LEVEL", "_") == -1) {
        return 0;
    }

    h = m_30->Lookup("IMAGEZ");
    if (!h) {
        return 0;
    }
    if (m_c->m_10->CallRegister(h, "GRUNTZ", "_") == -1) {
        return 0;
    }

    g_glsResetMgr->Reset();
    while (ShowCursor(FALSE) >= 0)
        ;

    m_c->m_4->m_14->m_2c->Step(0);
    LevelInit2356(g_gameReg, m_2dc, m_470);

    if (m_474 != 0) {
        Method1ae6();
    } else {
        m_c->m_24->Wire(m_c->m_4->m_14, m_c->m_8);
        m_c->m_c->CallSlot34(m_c->m_4->m_14, m_c->m_4->m_18);
    }

    m_2dc->Finalize();
    m_2dc->Activate2();
    m_510 = 2;
    m_c->m_4->Begin();
    StartTimer(0x50, 0x3e8, 0, 1);
    return 1;
}

RVA(0x00095090, 0x6e)
i32 CHelpState::LoadAssets(i32 a1, i32 a2, i32 a3) {
    if (!LoadGameAssetNamespaces(a1, a2, a3)) {
        return 0;
    }
    while (ShowCursor(0) >= 0)
        ;
    m_2c = m_8->Register("STATEZ_HELP");
    if (!m_2c) {
        return 0;
    }
    m_4->m_4->Pump(0x100, 0x40);
    return 1;
}

// ---------------------------------------------------------------------------
// BuildNamedGruntTable (0x0c16b0) - seeds the 4 named-grunt CString globals (the
// contiguous array at 0x64bdb0) with their default names via
// CString::operator=(LPCSTR) (FUN_001b9d4c, reloc-masked __thiscall).
// ---------------------------------------------------------------------------
struct EngStrAssign {
    char* m_pszData;
    void operator=(const char* s); // CString::operator=, FUN_001b9d4c
};
// 4 contiguous CString globals at 0x64bdb0 (defined in the engine's data).
DATA(0x0064bdb0)
extern EngStrAssign g_gruntNames[4];

RVA(0x000c16b0, 0x3d)
void BuildNamedGruntTable() {
    g_gruntNames[0] = "Beefy";
    g_gruntNames[1] = "Zed";
    g_gruntNames[2] = "Serra";
    g_gruntNames[3] = "Jebediah";
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CHelpAssetRoot);
SIZE_UNKNOWN(CHelpAssetSet);
SIZE_UNKNOWN(CHelpMsgPump);
SIZE_UNKNOWN(CHelpState);
SIZE_UNKNOWN(EngStrAssign);
SIZE_UNKNOWN(GLSAssetRoot);
SIZE_UNKNOWN(GLSMapMgr);
SIZE_UNKNOWN(GLSNamespace);
SIZE_UNKNOWN(GLSObj24);
SIZE_UNKNOWN(GLSPolyC);
SIZE_UNKNOWN(GLSPolyCVtbl);
SIZE_UNKNOWN(GLSRegistrar);
SIZE_UNKNOWN(GLSRegistrarVtbl);
SIZE_UNKNOWN(GLSResetMgr);
SIZE_UNKNOWN(GLSSub14);
SIZE_UNKNOWN(GLSSub2c);
SIZE_UNKNOWN(GLSSubA);
SIZE_UNKNOWN(GameLevelState);
