#include <Mfc.h>            // CPtrList/CString machinery (reloc-masked); /GX EH frame
#include <Bute/SymParser.h> // canonical CSymParser + CSymTab (ResolvePath @0x13c030/0x13bae0)
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <DDrawMgr/DDrawSubMgrPages.h>

#include <rva.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameMode.h>  // canonical CMenuState : CState (the one true shape)
#include <Gruntz/ResMgr.h>    // canonical CImageRegistry (this->m_c->m_10)
#include <Gruntz/GruntzMgr.h> // canonical CGruntzMgr (m_4 owner; RestoreVideoMode)
#include <Gruntz/ChatBox.h>   // canonical CChatBox (MenuHudObj facet)
class CDDrawWorkerRegistry {
public:
    i32 HasKeyEqual_155550(const char* k);
    i32 RemoveKeysEqual_155360(const char* a, const char* b);
    void Method_155630(i32 h, char* t, i32* o);
}; // 0x155550/0x155360/0x155630
// MenuStateAssets.cpp - CMenuState::LoadAssets (0x09fe50, 835 B), the MENU game-
// state asset loader.  Sibling of CHelpState::LoadAssets / GameLevelState loaders:
// chains the base namespace loader, registers the "MENU" IMAGEZ+SOUNDZ namespaces
// through the m_c->m_10 (vtable +0x48) / m_c->m_28 registries, primes the state
// core (m_c->m_4 IsReady/Init), then heap-allocates the menu HUD object (CPtrList +
// two CString members) and wires its MENU_CURSOR/SELECT/ACTIVATE/MENU keys + the
// MENU_ACTIVATE / MENU_MENU sound cues.  The destructible CPtrList/CString members
// of the heap object give the routine its /GX exception frame.
//
// Only offsets / code bytes are load-bearing; every engine callee is a reloc-
// masked external (no body).

// The image registry reached through this->m_c->m_10 is the canonical CImageRegistry
// (<Gruntz/ResMgr.h>): non-virtual Has + the vtable-slot-18 (+0x48) Install. Shared,
// so no local view.

// --- the sound-cue lookup: the sound registry's embedded name->cue map (+0x10)
// exposes a by-name Find (0x1b8438) whose value is a domain-specific cue entry (the
// entry's m_10->m_28 is the cached DS token). The generic CMapStringToOb does not
// model this Find, so the sound registry keeps a small TYPED local view here - a
// MenuSndEntry**-typed Find, so the resolve sites take no out-param cast. ---
struct MenuSndEntryInner {
    char m_pad00[0x28];
    i32 m_28; // +0x28
};
struct MenuSndEntry {
    char m_pad00[0x10];
    MenuSndEntryInner* m_10; // +0x10
};
struct MenuCueMap {
    void Find(const char* name, MenuSndEntry** out); // 0x1b8438 __thiscall, typed out-param
};
struct MenuAssetMgr { // this->m_c  (the CResMgr resource/level manager)
    char m_pad00[0x4];
    CDDrawSubMgrPages* m_4; // +0x04  state core (IsReady/Init)
    char m_pad08[0x10 - 0x8];
    CImageRegistry* m_10; // +0x10  image registry (canonical <Gruntz/ResMgr.h>)
    char m_pad14[0x28 - 0x14];
    CDDrawSubMgrLeafScan* m_28; // +0x28  sound registry (cue map)
};
struct MenuRegObj { // the registered STATEZ_MENU object (m_2c)
    // LookupSet @0x13bae0 IS CSymTab::ResolvePath; cast at the call.
};
struct MenuRegSet { // this->m_8
    // Register @0x13c030 IS CSymParser::ResolvePath; cast at the call.
};
struct MenuCursorSub { // this->m_4->m_4
    char m_pad00[0x4];
    void* m_4; // +0x04  passed to the layout init
};
struct MenuRoot { // this->m_4
    char m_pad00[0x4];
    MenuCursorSub* m_4; // +0x04
    // Hide @0x34ef IS CGruntzMgr::RestoreVideoMode; cast at the call.
};

// The global mgr singleton (*0x24556c): its resource holder's +0x28 sound registry
// carries the shared cue map the MENU_MENU cue is resolved from. That holder slot
// (CSpriteFactoryHolder::m_28) is a genuinely heterogeneous void* - other TUs view it
// as a sound-set (HbSndSet) or a mute gate - so it is cast to the sound-registry view
// at this one use-site (the authentic proven-heterogeneous-slot cast).
DATA(0x0024556c)
extern CGameRegistry* g_menuMgrSettings;
DATA(0x002bf37c) // VA-typo fix: 0x22f37c -> 0x2bf37c (canonical, Play.cpp); same global
extern i32 g_resourceInstallActive;

// The heap-allocated MENU HUD object (0x7c bytes): a CPtrList at +0x24 and two
// CString members at +0x44/+0x48, then its own sub-init (FUN_004010c8).  The
// destructible members give LoadAssets its EH frame.
struct MenuHudObj {
    MenuHudObj();
    char m_pad00[0x24];
    CPtrList m_24; // +0x24 (0x1c bytes -> ends 0x40)
    i32 m_40;     // +0x40
    CString m_44; // +0x44
    CString m_48; // +0x48
    char m_pad4c[0x7c - 0x4c];
    // Init @0x10c8 IS CChatBox::Init; cast at the call.
    // AddKey @0x182df0 IS CChatBox::AdvanceRow0; cast at the call.
    // AddKey2 @0x182e60 IS CChatBox::AdvanceRow1; cast at the call.
};
inline MenuHudObj::MenuHudObj() {
    ((CChatBox*)this)->Init();
}

// FUN_00182ab0 __cdecl: lay the menu out into the asset mgr from the cursor sub-
// object + a RECT.  FUN_00402fcc __cdecl: commit the menu HUD object (ret BOOL).
i32 MenuLayout(MenuAssetMgr* mgr, void* sub, RECT* rc, i32 a, i32 b, i32 c); // 0x182ab0
i32 MenuCommit(MenuHudObj* obj, i32 idx);                                    // 0x402fcc

// CMenuState is the canonical <Gruntz/GameMode.h> `CMenuState : CState`. The MENU
// asset loader reaches the CState base region through the SAME facets the game-state
// hierarchy documents (CState.h: the +0x04 owner and +0x0c CSpriteFactoryHolder holder are downcast
// to each TU's local facet views): m_4 (CGruntzMgr owner) -> MenuRoot cursor gate,
// m_8 (CBankMgr) -> MenuRegSet, m_c (CSpriteFactoryHolder) -> MenuAssetMgr resource holder, m_2c
// (CResSource) -> the STATEZ_MENU MenuRegObj. m_1b4 (CGMMenuUI) is the heap MenuHudObj
// the routine builds. Only offsets / code bytes are load-bearing.

// @early-stop
// frame-layout / regalloc wall (~89.5%): complete + correct body - instruction
// selection, the guarded registry-call chain, the heap MenuHudObj CPtrList+2 CString
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
    m_4->RestoreVideoMode(0);
    m_2c = (CResSource*)((CSymParser*)m_8)->ResolvePath("STATEZ_MENU");
    if (m_2c == 0) {
        return 0;
    }

    if (!((CDDrawWorkerRegistry*)((MenuAssetMgr*)m_c)->m_10)->HasKeyEqual_155550("MENU")) {
        void* set = SymTab2c()->ResolvePath("IMAGEZ");
        if (set == 0) {
            return 0;
        }
        g_resourceInstallActive = 1;
        ((MenuAssetMgr*)m_c)->m_10->Install(set, "MENU", "_");
        g_resourceInstallActive = 0;
    }

    if (!((MenuAssetMgr*)m_c)->m_28->HasKeyEqual_1583c0("MENU")) {
        void* set = SymTab2c()->ResolvePath("SOUNDZ");
        if (set == 0) {
            return 0;
        }
        ((MenuAssetMgr*)m_c)->m_28->ScanTree_157ee0((DirNode*)set, "MENU", "_");
    }

    if (!((MenuAssetMgr*)m_c)->m_4->Method_158d20()) {
        if (!((MenuAssetMgr*)m_c)->m_4->Method_158cb0(0, 0x30000)) {
            return 0;
        }
    }

    RECT rc;
    rc.left = 0;
    rc.top = 8;
    rc.right = 0x27f;
    rc.bottom = 0x1df;
    m_1b4 = (CChatBox*)new MenuHudObj();

    if (!MenuLayout((MenuAssetMgr*)m_c, ((MenuRoot*)m_4)->m_4->m_4, &rc, 0x14, 0xa, 1)) {
        return 0;
    }

    if (m_1b4->AdvanceRow0((void*)"MENU_CURSOR", 0x64, 0x20)) {
        m_1b4->AdvanceRow1((void*)"MENU_CURSOR", 0x64, 0x20);
    }
    ((MenuHudObj*)m_1b4)->m_44 = "MENU_SELECT";
    ((MenuHudObj*)m_1b4)->m_48 = "MENU_ACTIVATE";

    MenuSndEntry* e;
    ((MenuAssetMgr*)m_c)->m_28->m_10.Lookup("MENU_ACTIVATE", (CObject*&)e);
    if (e != 0) {
        ((MenuAssetMgr*)m_c)->m_28->m_10.Lookup("MENU_ACTIVATE", (CObject*&)e);
        m_1b8 = e->m_10->m_28;
    } else {
        m_1b8 = 0;
    }

    if (!MenuCommit((MenuHudObj*)m_1b4, -1)) {
        return 0;
    }

    MenuSndEntry* fm;
    ((CDDrawSubMgrLeafScan*)g_menuMgrSettings->m_world->m_28)
        ->m_10.Lookup("MENU_MENU", (CObject*&)fm);
    m_1bc = (CMenuMusic*)fm;
    return 1;
}

// ---------------------------------------------------------------------------
// The tile-region initializer (0x0182ab0), re-homed from the ApiCaller stubs:
// CMenuState::LoadAssets (0x09fe50) drives it. Stashes the source host + geometry
// args; copies the given RECT, or defaults it to the source's tile extent
// (src->m_4->m_10->m_10/m_14 - 1). A placeholder region record whose concrete
// class is not yet recovered; offsets + code bytes load-bearing.
struct TileExtent {
    char m_pad0[0x10];
    i32 m_10; // +0x10 width
    i32 m_14; // +0x14 height
};
struct TileSrc {
    char m_pad0[0x10];
    TileExtent* m_10; // +0x10
};
struct TileSrcHost {
    char m_pad0[4];
    TileSrc* m_4; // +0x04
};
struct TileRegion {
    TileSrcHost* m_0; // +0x00
    i32 m_4;          // +0x04
    RECT m_8;         // +0x08 (left/top/right/bottom = m_8/m_c/m_10/m_14)
    i32 m_18;         // +0x18
    i32 m_1c;         // +0x1c
    i32 m_20;         // +0x20
    char m_pad24[0x40 - 0x24];
    i32 m_40; // +0x40
    i32 Init(TileSrcHost* src, i32 a, RECT* rc, i32 d, i32 e, i32 f);
};
SIZE_UNKNOWN(TileExtent);
SIZE_UNKNOWN(TileSrc);
SIZE_UNKNOWN(TileSrcHost);
SIZE_UNKNOWN(TileRegion);
RVA(0x00182ab0, 0x7b)
i32 TileRegion::Init(TileSrcHost* src, i32 a, RECT* rc, i32 d, i32 e, i32 f) {
    if (!src) {
        return 0;
    }
    m_0 = src;
    m_4 = a;
    m_20 = f;
    m_18 = d;
    m_1c = e;
    m_40 = 0;
    if (rc) {
        CopyRect(&m_8, rc);
        return 1;
    }
    m_8.left = 0;
    m_8.top = 0;
    m_8.right = src->m_4->m_10->m_10 - 1;
    m_8.bottom = src->m_4->m_10->m_14 - 1;
    return 1;
}

SIZE_UNKNOWN(MenuAssetMgr);
SIZE_UNKNOWN(MenuCueMap);
SIZE_UNKNOWN(MenuCursorSub);
SIZE_UNKNOWN(MenuHudObj);
SIZE_UNKNOWN(CGameRegistry);
SIZE_UNKNOWN(MenuRegObj);
SIZE_UNKNOWN(MenuRegSet);
SIZE_UNKNOWN(MenuRoot);
SIZE_UNKNOWN(MenuSndEntry);
SIZE_UNKNOWN(MenuSndEntryInner);
SIZE_UNKNOWN(MenuSndRegistry);
SIZE_UNKNOWN(MenuStateCore);
