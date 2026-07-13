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
#include <Bute/SymParser.h> // canonical CSymParser (pulls CSymTab)

#include <rva.h>

#include <string.h> // inline strcpy intrinsic (/O2) for the cheat-table copy
#include <Globals.h>
#include <Gruntz/ResMgr.h>    // canonical CImageRegistry (the +0x10 image registrar)
#include <Gruntz/GameMode.h>  // the REAL owner: CBootyState (0x18830 IS its vtable slot 1)
#include <Gruntz/GruntzMgr.h> // CState::m_4 is CGruntzMgr (RestoreVideoMode @0x8ddd0)
#include <DDrawMgr/DDrawSubMgrLeafScan.h> // canonical CDDrawSubMgrLeafScan (ScanTree_157ee0)
class CDDrawSubMgrPages {
public:
    void Method_159ef0();
}; // 0x159ef0

// BcRegSet::Register @0x13c030 IS CSymParser::ResolvePath (canonical <Bute/SymParser.h>).

// The global CButeMgr instance the cheat table reads from (0x6453d8).
DATA(0x002453d8)
extern CButeMgr g_buteMgr;
// The engine empty C-string the default text/desc temp copies from (0x6293f4).
extern "C" char g_emptyString[];
// The hardware-cursor hide fn-ptr (?::ShowCursor@@3P6GHH@ZA, 0x6c44c4); the
// `mov edi,ds:g_ShowCursor; call edi` cached-ptr loop idiom (AppHelpers.cpp).
// The 25-entry cheat text/desc table (0x629f50 .. 0x62aef0, stride 0xa0) + its end
// sentinel g_cheatTableEnd (0x62aef0). .bss (built at runtime); DEFINED here (owner
// TU), reference externs stay in <Globals.h>. (REHOME DD-G)
DATA(0x00229f50)
char g_cheatTable[0xfa0]; // 0x629f50  (25 entries x 0xa0 stride)
DATA(0x0022aef0)
char g_cheatTableEnd[4]; // 0x62aef0  (loop end sentinel = &g_cheatTable[0xfa0])
// First-run guard (DAT_0062af10): 0 until the cheat table is built. DEFINED here
// (owner TU); a plain `extern` stays in Globals.h.
DATA(0x0022af10)
i32 g_bootyCheatBuilt = 0; // 0x22af10
// The 25-entry cheat text/desc table (0x629f50 .. 0x62aef0, stride 0xa0). The
// loop pointer walks [base .. end); each entry's text lands at p-0x20, desc at p.
// The +0x1c0 mode-record seed (_g_645588).
DATA(0x00245588)
extern "C" i32 g_645588; // DEFINED in Projectile.cpp (extern "C" = canonical linkage)

// (the six Bc* sub-views are GONE too - each was a facet of a class CState already names:
//    BcStateRoot   -> CState::m_4  is CGruntzMgr*, and its "Reset(0)" @thunk 0x34ef is
//                     really CGruntzMgr::RestoreVideoMode (0x8ddd0) - a real method.
//    BcPumpHost    -> CGruntzMgr's +0x04 == the CGameWnd it inherits from WAP32::CGameMgr
//                     (m_gameWnd), and its "Pump" is CGameWnd::PumpMessages (0x13d4e0).
//    BcRegSet      -> CState::m_8 is CBankMgr*; its Register @0x13c030 IS
//                     CSymParser::ResolvePath.
//    BcAssetRoot   -> CState::m_c is CSpriteFactoryHolder*, whose +0x08/+0x10/+0x28 are
//    BcAssetCore /  the sprite factory, the CImageRegistry and the CSndHost.
//    BcSoundRegistry
// The casts that remain below are the honest ones: those three CSpriteFactoryHolder slots
// are genuinely multi-faceted (the header already documents the render facet reaching m_8
// as CRenderer and m_28 as CSoundRegistry), and CBankMgr-vs-CSymParser is unproven.)

// (the CBootyCheatState `this`-view is GONE - it WAS CBootyState, and "CBootyCheatState"
// is not a type at all: 0x18830 is DATA-REFERENCED at ??_7CBootyState@@6B@+0x4, i.e. it is
// CBootyState's own vtable SLOT 1 (CState::Vfunc1, the asset/state loader). Every field the
// view modeled is a CState or CBootyState member at the same offset - m_4/m_8/m_c are
// CState's own +0x04/+0x08/+0x0c (CGruntzMgr / CBankMgr / CSpriteFactoryHolder), m_2c/m_30/
// m_34 its three bank slots, and m_1b8 / m_1c0..m_1cc land in CBootyState pads. The whole
// Bc* sub-view nest went with it. See <Gruntz/GameMode.h> for the proof.)

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
i32 CBootyState::Vfunc1(i32 a1, i32 a2, i32 a3) {
    if (!LoadGameAssetNamespaces(a1, a2, a3)) { // CState base loader (0xf9ea0), inherited
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

    m_4->RestoreVideoMode(0); // thunk 0x34ef -> CGruntzMgr::RestoreVideoMode (0x8ddd0)

    m_2c = (CResSource*)((CSymParser*)m_8)->ResolvePath("STATEZ_BOOTY");
    if (!m_2c) {
        goto fail;
    }
    m_gameBank = (CSymTab*)((CSymParser*)m_8)->ResolvePath("GAME");
    if (!m_gameBank) {
        goto fail;
    }
    m_gruntzBank = (CSymTab*)((CSymParser*)m_8)->ResolvePath("GRUNTZ");
    if (!m_gruntzBank) {
        goto fail;
    }

    ((CDDrawSubMgrPages*)m_c->m_8)->Method_159ef0();

    {
        void* soundz = SymTab2c()->FindSub("SOUNDZ");
        if (!soundz) {
            goto fail;
        }
        ((CDDrawSubMgrLeafScan*)m_c->m_28)->ScanTree_157ee0((CSymTab*)soundz, "BOOTY", "_");

        void* wand = m_gruntzBank->ResolvePath("SOUNDZ_WANDGRUNT");
        if (!wand) {
            goto fail;
        }
        ((CDDrawSubMgrLeafScan*)m_c->m_28)
            ->ScanTree_157ee0((CSymTab*)wand, "GRUNTZ_WANDGRUNT", "_");

        void* imagez = SymTab2c()->FindSub("IMAGEZ");
        if (!imagez) {
            goto fail;
        }
        m_c->m_10->Install(imagez, "BOOTY", "_");
    }

    {
        int(WINAPI * sc)(BOOL) = ::ShowCursor;
        while (sc(0) >= 0) {
        }
    }

    // The cast is GONE - the binary settled it. This call was bound to
    // CMoviePlayer::Pump (0x17c790), which 0x18830 does not call at all: retail does
    //     mov eax,[esi+0x4]   ; this->m_4      (CGruntzMgr)
    //     push 0x40 / push 0x100
    //     mov ecx,[eax+0x4]   ; m_4->m_gameWnd (+0x04)
    //     call 0x13d4e0       ; ?PumpMessages@CGameWnd@@QAEXIH@Z
    // so the callee is CGameWnd::PumpMessages - the real method of the class that actually
    // sits at that offset, already reconstructed in the `gamewnd` unit. Wrong callee AND a
    // wrong cast; naming the true owner dissolved both.
    m_4->m_gameWnd->PumpMessages(0x100, 0x40);

    m_1b8 = 0;
    // The five-stage build chain - all real, rva-bound methods of THIS class now (the
    // Init1..Init5 declared-only aliases they used to hide behind are gone).
    if (!BuildWarpStoneGlitterAnimation()) { // 0x19540
        goto fail;
    }
    if (!BuildGruntSprintAnimation()) { // 0x19920
        goto fail;
    }
    if (!LoadGruntEffectSprites()) { // 0x1a040
        goto fail;
    }
    if (!BuildBootyWalkingGruntz()) { // 0x1b450
        goto fail;
    }
    if (!BuildBootyPerfectAnimation()) { // 0x1c070
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
