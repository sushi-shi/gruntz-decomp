#include <Mfc.h> // CObList/CString machinery (reloc-masked); /GX EH frame

#include <rva.h>
// MenuStateAssets.cpp - CMenuState::LoadAssets (0x09fe50, 835 B), the MENU game-
// state asset loader.  Sibling of CHelpState::LoadAssets / GameLevelState loaders:
// chains the base namespace loader, registers the "MENU" IMAGEZ+SOUNDZ namespaces
// through the m_c->m_10 (vtable +0x48) / m_c->m_28 registries, primes the state
// core (m_c->m_4 IsReady/Init), then heap-allocates the menu HUD object (CObList +
// two CString members) and wires its MENU_CURSOR/SELECT/ACTIVATE/MENU keys + the
// MENU_ACTIVATE / MENU_MENU sound cues.  The destructible CObList/CString members
// of the heap object give the routine its /GX exception frame.
//
// Only offsets / code bytes are load-bearing; every engine callee is a reloc-
// masked external (no body).

// --- the sound-cue lookup sub-table (the BootySndTable idiom): Find(name, &out)
// writes the matched entry to *out; the entry's m_10->m_28 is the cached token ---
struct MenuSndEntryInner {
    char m_pad00[0x28];
    i32 m_28; // +0x28
};
struct MenuSndEntry {
    char m_pad00[0x10];
    MenuSndEntryInner* m_10; // +0x10
};
struct MenuSndTable {
    void Find(char* name, MenuSndEntry** out); // FUN_001b8438 __thiscall, out-param
};

// --- the registries reached through this->m_c (image @ +0x10 virtual, sound @
// +0x28 non-virtual with an embedded Find sub-table at +0x10) ---
struct MenuImageRegistry {
    i32 Has(char* name); // FUN_00555550 __thiscall, ret found
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6();
    virtual void v7();
    virtual void v8();
    virtual void v9();
    virtual void v10();
    virtual void v11();
    virtual void v12();
    virtual void v13();
    virtual void v14();
    virtual void v15();
    virtual void v16();
    virtual void v17();
    virtual void Install(void* set, char* name, char* key); // slot 18 (+0x48)
};
struct MenuSoundRegistry {
    i32 Has(char* name);                            // FUN_004583c0 __thiscall
    void Install(void* set, char* name, char* key); // FUN_00557ee0 __thiscall
    char m_pad00[0x10];
    MenuSndTable m_10; // +0x10  Find sub-table
};
struct MenuStateCore {      // this->m_c->m_4
    i32 IsReady();          // FUN_00558d20 __thiscall
    i32 Init(i32 a, i32 b); // FUN_00558cb0 __thiscall
};
struct MenuAssetMgr {        // this->m_c
    char m_pad00[0x4];
    MenuStateCore* m_4;      // +0x04
    char m_pad08[0x10 - 0x8];
    MenuImageRegistry* m_10; // +0x10  image registry (vtable +0x48 install)
    char m_pad14[0x28 - 0x14];
    MenuSoundRegistry* m_28; // +0x28  sound registry
};
struct MenuRegObj {              // the registered STATEZ_MENU object (m_2c)
    void* LookupSet(char* name); // FUN_0053bae0 __thiscall, ret set ptr
};
struct MenuRegSet {                   // this->m_8
    MenuRegObj* Register(char* name); // FUN_0053c030 __thiscall (CHelpState idiom)
};
struct MenuCursorSub { // this->m_4->m_4
    char m_pad00[0x4];
    void* m_4; // +0x04  passed to the layout init
};
struct MenuRoot {       // this->m_4
    char m_pad00[0x4];
    MenuCursorSub* m_4; // +0x04
    void Hide(i32 z);   // FUN @ 0x4034ef __thiscall
};

// The global mgr singleton (*0x64556c): m_30->m_28 (+0x10) is the shared sound
// table the MENU_MENU cue is resolved from.
struct MenuMgrSndHost {
    char m_pad00[0x28];
    MenuSoundRegistry* m_28; // +0x28
};
struct MenuMgrSettings {
    char m_pad00[0x30];
    MenuMgrSndHost* m_30; // +0x30
};
DATA(0x0024556c)
extern MenuMgrSettings* g_menuMgrSettings;
DATA(0x0022f37c)
extern i32 g_severusCounterA; // 0x6bf37c

// The heap-allocated MENU HUD object (0x7c bytes): a CObList at +0x24 and two
// CString members at +0x44/+0x48, then its own sub-init (FUN_004010c8).  The
// destructible members give LoadAssets its EH frame.
struct MenuHudObj {
    MenuHudObj();
    char m_pad00[0x24];
    CObList m_24; // +0x24 (0x1c bytes -> ends 0x40)
    i32 m_40;     // +0x40
    CString m_44; // +0x44
    CString m_48; // +0x48
    char m_pad4c[0x7c - 0x4c];
    void Init();                            // FUN_004010c8 __thiscall (sub-init)
    i32 AddKey(char* name, i32 a, i32 b);   // FUN_00182df0 __thiscall
    void AddKey2(char* name, i32 a, i32 b); // FUN_00182e60 __thiscall
};
inline MenuHudObj::MenuHudObj() {
    Init();
}

// FUN_00182ab0 __cdecl: lay the menu out into the asset mgr from the cursor sub-
// object + a RECT.  FUN_00402fcc __cdecl: commit the menu HUD object (ret BOOL).
i32 MenuLayout(MenuAssetMgr* mgr, void* sub, RECT* rc, i32 a, i32 b, i32 c); // 0x182ab0
i32 MenuCommit(MenuHudObj* obj, i32 idx);                                    // 0x402fcc

class CMenuState {
public:
    i32 LoadAssets(i32 a1, i32 a2, i32 a3);
    i32 LoadGameAssetNamespaces(i32, i32, i32); // base loader; reloc-masked near call

    char m_pad00[0x4];
    MenuRoot* m_4;   // +0x04
    MenuRegSet* m_8; // +0x08
    MenuAssetMgr* m_c; // +0x0c
    char m_pad10[0x2c - 0x10];
    MenuRegObj* m_2c; // +0x2c
    char m_pad30[0x1b4 - 0x30];
    MenuHudObj* m_1b4; // +0x1b4
    i32 m_1b8;         // +0x1b8
    i32 m_1bc;         // +0x1bc
};

// @early-stop
// frame-layout / regalloc wall (~89.5%): complete + correct body - instruction
// selection, the guarded registry-call chain, the heap MenuHudObj CObList+2 CString
// construction + EH trylevel, the MENU_CURSOR/SELECT/ACTIVATE keys and the two sound
// finds all match retail (verified instruction-by-instruction; the 96 ARG_MISMATCH
// rows are the reloc-name scoring artifact).  Residual: retail frame-allocates 0x10
// of locals while this /O2 recompile allocates 0x14 (one extra slot), yielding a +4
// cascade across every [esp+N] operand, plus the base-loader arg push scheduling and
// the new-obj-vs-EH-state interleave - all allocator choices, not source-steerable.
// See docs/patterns/zero-register-pinning.md + identical-return-epilogue-tailmerge.md.
RVA(0x0009fe50, 0x343)
i32 CMenuState::LoadAssets(i32 a1, i32 a2, i32 a3) {
    if (a3 == 0) {
        return 0;
    }
    if (!LoadGameAssetNamespaces(a2, a3, a3)) {
        return 0;
    }
    m_4->Hide(0);
    m_2c = m_8->Register("STATEZ_MENU");
    if (m_2c == 0) {
        return 0;
    }

    if (!m_c->m_10->Has("MENU")) {
        void* set = m_2c->LookupSet("IMAGEZ");
        if (set == 0) {
            return 0;
        }
        g_severusCounterA = 1;
        m_c->m_10->Install(set, "MENU", "_");
        g_severusCounterA = 0;
    }

    if (!m_c->m_28->Has("MENU")) {
        void* set = m_2c->LookupSet("SOUNDZ");
        if (set == 0) {
            return 0;
        }
        m_c->m_28->Install(set, "MENU", "_");
    }

    if (!m_c->m_4->IsReady()) {
        if (!m_c->m_4->Init(0, 0x30000)) {
            return 0;
        }
    }

    RECT rc;
    rc.left = 0;
    rc.top = 8;
    rc.right = 0x27f;
    rc.bottom = 0x1df;
    m_1b4 = new MenuHudObj();

    if (!MenuLayout(m_c, m_4->m_4->m_4, &rc, 0x14, 0xa, 1)) {
        return 0;
    }

    if (m_1b4->AddKey("MENU_CURSOR", 0x64, 0x20)) {
        m_1b4->AddKey2("MENU_CURSOR", 0x64, 0x20);
    }
    m_1b4->m_44 = "MENU_SELECT";
    m_1b4->m_48 = "MENU_ACTIVATE";

    MenuSndEntry* e;
    m_c->m_28->m_10.Find("MENU_ACTIVATE", &e);
    if (e != 0) {
        m_c->m_28->m_10.Find("MENU_ACTIVATE", &e);
        m_1b8 = e->m_10->m_28;
    } else {
        m_1b8 = 0;
    }

    if (!MenuCommit(m_1b4, -1)) {
        return 0;
    }

    MenuSndEntry* fm;
    g_menuMgrSettings->m_30->m_28->m_10.Find("MENU_MENU", &fm);
    m_1bc = (i32)fm;
    return 1;
}
