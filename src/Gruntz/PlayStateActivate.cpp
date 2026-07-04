// PlayStateActivate.cpp - CPlay::OnActivate (0x0cb800), the PLAY level-state
// activation (vtable slot 8; reached directly by CTriggerMgr::NotifyCell). Re-homed
// from the artifact "GameLevelState" placeholder of BacklogStateLoaders.cpp: RVA
// 0x0cb800 sits inside CPlay's method band (between CPlay::ModeCleanup @0x0cb740 and
// CPlay::PresentAndFlush @0x0cba10), so the real owner is CPlay (<Gruntz/Play.h>).
//
// It chains the base activate, hides the cursor, registers the TILEZ/IMAGEZ(LEVEL)/
// IMAGEZ(GRUNTZ) namespaces through the m_c->m_10 registrar (vtable slot +0x4c),
// then runs the level-specific init chain (m_2dc map mgr, m_c sub-objects) and kicks
// the state timer. __thiscall; every callee is a reloc-masked external.
//
// Following CPlay's documented loader idiom (<Gruntz/Play.h>): the routine casts
// `this` once to a typed activation facet (the +0xc resource root, +0x28/+0x30
// bank sources, +0x2dc guts, the region gates), keeping the sub-object views local.
#include <Mfc.h> // ShowCursor (afx-first)

#include <Gruntz/Play.h>       // the real CPlay : CState (method owner)
#include <Gruntz/WwdGameReg.h> // the canonical WwdGameReg singleton (g_gameReg)
#include <rva.h>
#include <Globals.h> // g_glsResetMgr (DAT_00645570)

// The global empty C string (0x6293f4).
extern "C" char g_emptyString[];

// The namespace registrar reached via m_c->m_10. This is a FOREIGN engine class:
// its ??_7 and the intermediate slots 0..18 are unreconstructed engine code, so the
// honest model is the ONE dispatched slot. Its vtable slot +0x4c registers a
// looked-up image set under a name + separator (returns -1 on failure), modeled as a
// 4-byte PMF loaded from the vtable (`char m_pad00[0x4c]` documents the un-recovered
// slots) so the call falls out as `mov edx,[ecx]; call [edx+0x4c]`.
struct GLSRegistrarVtbl;
struct GLSRegistrar {
    GLSRegistrarVtbl* m_vtbl; // +0x00
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
// m_c->m_c: a FOREIGN engine class - its ??_7 and slots 0..12 are unreconstructed,
// so the honest model is the ONE dispatched slot +0x34 (a 4-byte PMF from the vtable).
struct GLSPolyCVtbl;
struct GLSPolyC {
    GLSPolyCVtbl* m_vtbl; // +0x00
    void CallSlot34(GLSSub14* a, void* b);
};
typedef void (GLSPolyC::*GLSPolyCFn)(GLSSub14* a, void* b);
struct GLSPolyCVtbl {
    char m_pad00[0x34];
    GLSPolyCFn Slot34; // +0x34
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
// GLSResetMgr is forward-declared in <Globals.h> (g_glsResetMgr @0x645570); complete
// it here so g_glsResetMgr->Reset() falls out.
struct GLSResetMgr {
    void Reset(); // FUN_00533110 __thiscall
};
// The game-manager singleton (0x64556c); mangled ?g_gameReg@@3PAUWwdGameReg@@A.
DATA(0x0024556c)
extern WwdGameReg* g_gameReg;
// cdecl level-init helper (g_gameReg, this->m_2dc, this->m_470).
void LevelInit2356(WwdGameReg* gameReg, GLSMapMgr* mapMgr, i32 a3); // reloc-masked

// The CPlay activation facet: `this` cast once, so the field accesses take CPlay's
// activation offsets without disturbing Play.cpp's Render-side member typing.
struct PlayActivate {
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
i32 CPlay::OnActivate() {
    PlayActivate* p = (PlayActivate*)this;
    if (!p->BaseOnActivate()) {
        return 0;
    }
    while (ShowCursor(FALSE) >= 0)
        ;

    i32 h = p->m_28->Lookup("TILEZ");
    if (!h) {
        return 0;
    }
    if (p->m_c->m_10->CallRegister(h, g_emptyString, "_") == -1) {
        return 0;
    }

    h = p->m_28->Lookup("IMAGEZ");
    if (!h) {
        return 0;
    }
    if (p->m_c->m_10->CallRegister(h, "LEVEL", "_") == -1) {
        return 0;
    }

    h = p->m_30->Lookup("IMAGEZ");
    if (!h) {
        return 0;
    }
    if (p->m_c->m_10->CallRegister(h, "GRUNTZ", "_") == -1) {
        return 0;
    }

    g_glsResetMgr->Reset();
    while (ShowCursor(FALSE) >= 0)
        ;

    p->m_c->m_4->m_14->m_2c->Step(0);
    LevelInit2356(g_gameReg, p->m_2dc, p->m_470);

    if (p->m_474 != 0) {
        p->Method1ae6();
    } else {
        p->m_c->m_24->Wire(p->m_c->m_4->m_14, p->m_c->m_8);
        p->m_c->m_c->CallSlot34(p->m_c->m_4->m_14, p->m_c->m_4->m_18);
    }

    p->m_2dc->Finalize();
    p->m_2dc->Activate2();
    p->m_510 = 2;
    p->m_c->m_4->Begin();
    p->StartTimer(0x50, 0x3e8, 0, 1);
    return 1;
}

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
SIZE_UNKNOWN(PlayActivate);
