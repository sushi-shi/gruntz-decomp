// BootyCheatState.cpp - the STATEZ_BOOTY cheat-screen game-state asset loader
// (0x18830), a /GX (eh) unit: the five destructible CString locals of the first-run
// cheat-table build give it the MSVC exception frame.
//
// The method chains the base-class asset loader, then on the FIRST activation
// (guarded by g_bootyCheatBuilt) builds the 25-entry cheat table at g_cheatTable
// (stride 0xa0): each entry reads its de-obfuscated "Text"/"Desc" from the
// [Cheat<id>] group whose id comes from BootyCheatz/"A<a>C<c>". The common path
// (both runs) registers the STATEZ_BOOTY/GAME/GRUNTZ namespaces, installs the
// BOOTY/GRUNTZ_WANDGRUNT sound + image sets, hides the cursor, pumps a message
// burst, then runs the five-stage init chain and stamps the mode fields.
//
// Only offsets / code bytes are load-bearing; the CButeMgr getters + CString
// library come from the real headers, every engine helper is a reloc-masked
// external (no body). Field names are placeholders (m_<hexoffset>).
// <Bute/ButeMgr.h> pulls <Mfc.h> (afx-first windows.h) - MUST precede any other
// header that reaches windows.h, so it comes first.
#include <Bute/ButeMgr.h>

#include <rva.h>

#include <string.h> // inline strcpy intrinsic (/O2) for the cheat-table copy
#include <Globals.h>
#include <Gruntz/ResMgr.h> // canonical CImageRegistry (the +0x10 image registrar)

// The global CButeMgr instance the cheat table reads from (0x6453d8).
DATA(0x002453d8)
extern CButeMgr g_buteMgr;
// The engine empty C-string the default text/desc temp copies from (0x6293f4).
extern "C" char g_emptyString[];
// The hardware-cursor hide fn-ptr (?g_ShowCursor@@3P6GHH@ZA, 0x6c44c4); the
// `mov edi,ds:g_ShowCursor; call edi` cached-ptr loop idiom (AppHelpers.cpp).
extern int(WINAPI* g_ShowCursor)(int);
// First-run guard (DAT_0062af10): 0 until the cheat table is built.
// The 25-entry cheat text/desc table (0x629f50 .. 0x62aef0, stride 0xa0). The
// loop pointer walks [base .. end); each entry's text lands at p-0x20, desc at p.
// The +0x1c0 mode-record seed (_g_645588).
DATA(0x00245588)
extern i32 g_645588;

// ---------------------------------------------------------------------------
// The registered namespace object (Register result): FindSub resolves a named
// child set, ResolvePath resolves a namespaced path (both reloc-masked
// __thiscall, no body).
struct BcRegObj {
    void* FindSub(char* name);     // FUN_0053a230
    void* ResolvePath(char* name); // FUN_0053bae0
};
struct BcRegSet {                   // this->m_8
    BcRegObj* Register(char* name); // FUN_0053c030
};
struct BcSoundRegistry {                            // this->m_c->m_28
    void Install(void* set, char* name, char* sep); // FUN_00557ee0
};
// The image registrar reached via m_c->m_10 is the canonical CImageRegistry
// (ResMgr.h): Install is its slot-18 (+0x48) virtual. Uses the real class - no local
// registrar view.
struct BcAssetCore { // this->m_c->m_8
    void Prepare();  // FUN_00559ef0
};
struct BcAssetRoot { // this->m_c
    char m_pad00[0x8];
    BcAssetCore* m_8; // +0x08
    char m_pad0c[0x10 - 0xc];
    CImageRegistry* m_10; // +0x10  the image/tile registry (Install slot 18)
    char m_pad14[0x28 - 0x14];
    BcSoundRegistry* m_28; // +0x28
};
struct BcPumpHost {            // this->m_4->m_4
    void Pump(i32 msg, i32 n); // FUN_0053d4e0
};
struct BcStateRoot { // this->m_4
    void Reset(i32); // FUN_...34ef __thiscall on m_4
    char m_pad00[0x4];
    BcPumpHost* m_4; // +0x04
};

class CBootyCheatState {
public:
    i32 LoadAssets(i32 a1, i32 a2, i32 a3);     // 0x18830
    i32 LoadGameAssetNamespaces(i32, i32, i32); // base loader FUN_000043a9

    // The five-stage tail init chain (__thiscall(this), reloc-masked).
    i32 Init1(); // FUN_...11c2
    i32 Init2(); // FUN_...39c7
    i32 Init3(); // FUN_...3b8e
    i32 Init4(); // FUN_...1681
    i32 Init5(); // FUN_...2d83

    char m_pad00[0x4];
    BcStateRoot* m_4; // +0x04
    BcRegSet* m_8;    // +0x08
    BcAssetRoot* m_c; // +0x0c
    char m_pad10[0x2c - 0x10];
    BcRegObj* m_2c; // +0x2c  STATEZ_BOOTY
    BcRegObj* m_30; // +0x30  GRUNTZ
    BcRegObj* m_34; // +0x34  GAME
    char m_pad38[0x1b8 - 0x38];
    i32 m_1b8; // +0x1b8
    char m_pad1bc[0x1c0 - 0x1bc];
    i32 m_1c0; // +0x1c0
    i32 m_1c4; // +0x1c4
    i32 m_1c8; // +0x1c8
    i32 m_1cc; // +0x1cc
};

// @source: string-xref
// @early-stop
// epilogue tail-merge layout wall (~86%): logic complete + verified vs retail - the
// inline /GX prologue, the whole first-run cheat-table build (idiv %3 + reciprocal
// /3, the 5-CString EH frame, the Format/GetIntDef/GetStringDef/op= chain, the
// inline-strcpy rep-movs, the signed `jl` loop bound), the STATEZ_BOOTY/GAME/GRUNTZ
// registration, the sound/image installs, the ShowCursor cached-ptr loop, the Pump,
// and the 5-stage Init chain (`test;je` guards) all byte-match. Residual: retail
// routes ALL 12 returns to ONE shared epilogue, emitting per-site `jne skip; xor
// eax,eax; jmp <epilogue>` for the 6 pointer-null guards (flat `if(!x)return 0` with
// a cross-jumped epilogue tail); flat source here does NOT cross-jump (MSVC inlines
// 12 full epilogues, 54%), and the `goto fail` shared-exit that recovers the merge
// emits `je fail` (86%) not the retail per-site xor+jmp. The merge-vs-inline choice
// is the allocator's block-layout decision, not source-steerable. See
// docs/patterns/identical-return-epilogue-tailmerge.md (reverse direction).
RVA(0x00018830, 0x380)
i32 CBootyCheatState::LoadAssets(i32 a1, i32 a2, i32 a3) {
    if (!LoadGameAssetNamespaces(a1, a2, a3)) {
        goto fail;
    }

    if (g_bootyCheatBuilt == 0) {
        CString bootyCheatz("BootyCheatz");
        CString empty(g_emptyString);
        CString grp;
        CString text;
        CString desc;
        i32 i = 0;
        for (char* p = g_cheatTable; (i32)p < (i32)g_cheatTableEnd; p += 0xa0) {
            grp.Format("A%dC%d", i / 3 + 1, i % 3 + 1);
            i32 id = g_buteMgr.GetIntDef(bootyCheatz, grp, 1);
            grp.Format("Cheat%i", id);
            text = *g_buteMgr.GetStringDef(grp, "Text", &empty);
            desc = *g_buteMgr.GetStringDef(grp, "Desc", &empty);
            strcpy(p - 0x20, text);
            strcpy(p, desc);
            i++;
        }
        g_bootyCheatBuilt = 1;
    }

    m_4->Reset(0);

    m_2c = m_8->Register("STATEZ_BOOTY");
    if (!m_2c) {
        goto fail;
    }
    m_34 = m_8->Register("GAME");
    if (!m_34) {
        goto fail;
    }
    m_30 = m_8->Register("GRUNTZ");
    if (!m_30) {
        goto fail;
    }

    m_c->m_8->Prepare();

    {
        void* soundz = m_2c->FindSub("SOUNDZ");
        if (!soundz) {
            goto fail;
        }
        m_c->m_28->Install(soundz, "BOOTY", "_");

        void* wand = m_30->ResolvePath("SOUNDZ_WANDGRUNT");
        if (!wand) {
            goto fail;
        }
        m_c->m_28->Install(wand, "GRUNTZ_WANDGRUNT", "_");

        void* imagez = m_2c->FindSub("IMAGEZ");
        if (!imagez) {
            goto fail;
        }
        m_c->m_10->Install(imagez, "BOOTY", "_");
    }

    {
        int(WINAPI * sc)(int) = g_ShowCursor;
        while (sc(0) >= 0) {
        }
    }

    m_4->m_4->Pump(0x100, 0x40);

    m_1b8 = 0;
    if (!Init1()) {
        goto fail;
    }
    if (!Init2()) {
        goto fail;
    }
    if (!Init3()) {
        goto fail;
    }
    if (!Init4()) {
        goto fail;
    }
    if (!Init5()) {
        goto fail;
    }

    m_1c8 = 0x21;
    m_1cc = 0;
    m_1c0 = g_645588;
    m_1c4 = 0;
    return 1;

fail:
    return 0;
}

SIZE_UNKNOWN(BcAssetCore);
SIZE_UNKNOWN(BcAssetRoot);
SIZE_UNKNOWN(BcPumpHost);
SIZE_UNKNOWN(BcRegObj);
SIZE_UNKNOWN(BcRegSet);
SIZE_UNKNOWN(BcSoundRegistry);
SIZE_UNKNOWN(BcStateRoot);
SIZE_UNKNOWN(CBootyCheatState);
