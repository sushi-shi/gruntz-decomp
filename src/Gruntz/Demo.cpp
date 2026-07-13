// Demo.cpp - the demo/attract-mode feature file (C:\Proj\Gruntz): ONE original obj
// `[0x3bfa0 .. 0x3dee1]` (dossier #16, waveM-judgment). Holds the CDemo playback
// methods (slot 1 / Render / the slot-21 re-post), the demo actor seeding, the
// world-level key resolver, the auto-scroll camera director, the Orient3 rotation
// steppers, the two bute-config debug editor DialogProcs + their record/teardown
// helpers, CButeMgr::Parse, and the state-0 anim-worker dispatch family
// (Handler03d2b0..Handler03ddf0).
//
// Merge evidence (dossier #16): total fn-granularity text weave across the ex
// demo / gruntzmgrtransition / demosetup / worldlevelkey / democameratools /
// animworkerhandlers / trirecordserialize / orphanleaves / butemgrparse units
// (democam fns bracket the animworker fns and vice versa), and the two 9-frag
// {0,1,1}-triple static groups (slots i530-538 @0x3c520, i541-549 @0x3cfe0 -
// "animworkerhandlersx18") sit INSIDE the block at their statics' source
// positions, BRACKETING democameratools fns - one obj. The initialized-.data
// private band 0x20d008..0x20d148 is contiguous-monotone across the ex-units.
// /GX per the EH prologues at 0x3c0e0 (BuildWorldLevelKey) + 0x3cc20 (Parse).
//
// CDemo's identity + layout live in the canonical <Gruntz/Demo.h>; its /GX dtor
// is in PlayDtor.cpp. Field names are placeholders (m_<hexoffset>); only OFFSETS
// + emitted code bytes are load-bearing (campaign doctrine).
#include <Gruntz/Demo.h>
#include <Io/FileMem.h>       // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Gruntz/GruntzMgr.h> // CGruntzMgr / CGameMgr::m_gameWnd -> CGameWnd::m_hwnd (Render's exit post)
#include <Gruntz/AttractActor.h> // the shared per-frame g_actorList view
#include <fstream.h> // the REAL CRT ifstream/ofstream/ios (their dtors ARE in the CRT libs)
#include <rva.h>

#include <Bute/ButeMgr.h>         // CButeMgr (Parse @0x3cc20)
#include <Bute/SymTab.h>          // the shared CSymTab (ResolveQualified 0x13be40)
#include <Gruntz/AnimWorker.h>    // shared Owner / Worker views + Worker_DefaultPump
#include <Gruntz/GameLevel.h>     // canonical CGameLevel + CLevelPlane (RecomputePlaneCoords)
#include <Gruntz/SerialObjRef.h>  // CSerialArchive (Read @+0x2c / Write @+0x30)
#include <Gruntz/SerialRecords.h> // CTriRecord
#include <Gruntz/SpriteFactory.h> // the ONE CSpriteFactory shape (CreateSprite @0x1597b0)
#include <Gruntz/UserLogic.h>     // the dispatched CUserLogic leaves' slot layout
#include <Gruntz/WorldState.h>    // canonical CWorldState + LevelMgr
#include <Globals.h>
#include <Ints.h>
#include <stdlib.h> // rand (0x11fee0, the engine LCG)

// The real CUserLogic game-object leaves each contiguous handler builds in its
// state-0 case. Sizes are proven from the `new` immediates.
#include <Gruntz/ExitTrigger.h>
#include <Gruntz/FortressFlag.h>
#include <Gruntz/GruntCreationPoint.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntStartingPoint.h>
#include <Gruntz/SecretLevelTrigger.h>
#include <Gruntz/SecretTeleporterTrigger.h>
#include <Gruntz/Teleporter.h>
#include <Gruntz/Warlord.h>
#include <Gruntz/Wormhole.h>

// PostMessageA reached through the IAT slot (the engine's ff15 indirect); the exit
// WM_COMMANDs (0x8023 / 0x8027). Symbol bound by Attract.cpp's DATA; extern here.
typedef i32(WINAPI* PostMessageFn)(void* hwnd, u32 msg, u32 wparam, i32 lparam);
// extern "C" so the fn-ptr reloc emits the canonical `_g_pPostMessageA` (the single
// name bound at 0x2c44c8 by sbi_rectonly); the C++-mangled spelling never bound and
// shadowed gruntzmgrtransition's correct copy of the duplicate CDemo::Vslot15 (0x3c030).
extern "C" PostMessageFn g_pPostMessageA;

// The per-frame attract actor list (DAT_00645574; bound in Globals.cpp) and the
// per-frame time delta (DAT_00645584; bound in Attract.cpp). Extern here (reloc-masked).
extern "C" u32 g_frameDelta;

// The ButeFileStream `defaults` arg CButeMgr::Parse hands the stream ctor (owner-TU
// def; VA 0x5f03e0). Holds the sentinel 0x1a4 (loaded + pushed as the void* arg).
extern "C" {
    DATA(0x001f03e0)
    void* g_pButeDefaults = (void*)0x1a4; // 0x5f03e0
}

// The first arg is the game-manager/entry context whose CString at +0xc8 (a
// pending-name latch) is cleared before delegating to the CPlay base entry.
SIZE_UNKNOWN(CDemoEnterCtx);
struct CDemoEnterCtx {
    char p0[0xc8];
    CString m_c8; // +0xc8
};

// 0x3bfa0 - CDemo::Vfunc1 (slot 1): clear the entry context's pending name, run
// the CPlay base slot-1 (CPlay::Vfunc1 == the mode/object initializer at 0xc7ec0);
// on failure return 0, else latch m_520 and return 1.
RVA(0x0003bfa0, 0x42)
i32 CDemo::Vfunc1(i32 ctx, i32 a1, i32 a2) {
    ((CDemoEnterCtx*)ctx)->m_c8.Empty();
    if (CPlay::Vfunc1(ctx, a1, a2) == 0) {
        return 0;
    }
    m_520 = 0x124f80;
    return 1;
}

// CDemo::Vslot15 (slot 21, 0x3c030): post WM_COMMAND 0x8027 to the owner HWND (the
// CState/CWorld back-ptr chain m_4w()->m_4->m_4), the demo-state re-post twin of
// CPlay::Vslot15. (Re-homed from GruntzMgrTransition.cpp - its birth position is
// this obj's head run.)
RVA(0x0003c030, 0x22)
i32 CDemo::Vslot15() {
    g_pPostMessageA((HWND)m_4w()->m_4->m_4, 0x111, 0x8027, 0);
    return 1;
}

// ---------------------------------------------------------------------------
// CDemoSetup - the attract/demo-mode actor seeding. this->m_c is the bound world
// object; its +0x8 is the sprite factory.
struct CDemoWorld {
    char m_pad0[0x8];
    CSpriteFactory* m_8; // +0x8
};

class CDemoSetup {
public:
    i32 SetupDemoActors(); // 0x3c070
    char m_pad0[0xc];
    CDemoWorld* m_c; // +0xc  bound world
};

// SetupDemoActors @0x3c070 - seed the two attract-mode demo HUD sprites
// ("DemoMover", "DemoSign") through the bound world's sprite factory.
RVA(0x0003c070, 0x47)
i32 CDemoSetup::SetupDemoActors() {
    m_c->m_8->CreateSprite(1, 0, 0, 0, "DemoMover", 0x40003);
    m_c->m_8->CreateSprite(1, 0, 0, 0x270f, "DemoSign", 0x40003);
    return 1;
}

// ---------------------------------------------------------------------------
// BuildWorldLevelKey @0x3c0e0 - resolve the WORLDZ\LEVEL%i record for the active
// world. Resets the level record, formats its namespace key, resolves it, and on
// success runs the record's load hook + NotifyAllPlanes and raises its dirty bit.
// The loaded level record IS the canonical CGameLevel: slot 15 LoadFromSource
// (+0x3c) / slot 17 ReleaseChildren (+0x44) through the real vtable, plus the
// non-virtual NotifyAllPlanes (0x160f40); the dirty-flag word is CLoadable's m_08.
class CParseSource;

RVA(0x0003c0e0, 0xfb)
i32 CWorldState::BuildWorldLevelKey(i32 unused) {
    m_0c->m_24->ReleaseChildren();
    CString key;
    key.Format("WORLDZ\\LEVEL%i", 1);
    i32 node = m_28->ResolveQualified(key, (void*)0x575744);
    if (node == 0) {
        return 0;
    }
    if (m_0c->m_24->LoadFromSource((CParseSource*)node) == 0) {
        return 0;
    }
    m_0c->m_24->NotifyAllPlanes();
    m_0c->m_24->m_08 |= 4;
    return 1;
}

// 0x3c220 - CDemo::Render (slot 5, +0x14): the demo/attract per-frame render poll.
// Runs the CPlay base render, scans the g_actorList for the 0x100 exit-request flag
// (posting WM_COMMAND 0x8023 to the top-level HWND on the first match), counts the
// idle timer m_520 down by the frame delta, and posts WM_COMMAND 0x8027 when it
// expires. Returns 1.
// @early-stop
// 99.55%: every opcode/offset/branch is byte-identical. The residual is (1) the
// PostMessageA IAT-absolute scoring artifact (target bakes the bare 0x6c44c8, no
// symbol) and (2) a register-coloring coin-flip (pm <-> ebx/edi vs the 0x100 mask) -
// the documented regalloc back-edge wall (docs/patterns/zero-register-pinning.md).
// Not source-steerable; deferred to the final sweep.
RVA(0x0003c220, 0xa4)
i32 CDemo::Render() {
    CPlay::Render();
    PostMessageFn pm = g_pPostMessageA;
    AttractActorList* list = g_actorList;
    i32 n = list->m_count;
    for (i32 i = 0; i < n; i++) {
        if (list->m_data[i]->m_2ac & 0x100) {
            pm(m_4->m_gameWnd->m_hwnd, 0x111, 0x8023, 0);
            break;
        }
    }
    if (g_frameDelta >= m_520) {
        m_520 = 0;
    } else {
        m_520 -= g_frameDelta;
    }
    if (m_520 == 0) {
        pm(m_4->m_gameWnd->m_hwnd, 0x111, 0x8027, 0);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// 0x3c300 - the demo/attract auto-scroll camera director (__cdecl(owner)).
// ---------------------------------------------------------------------------
// @identity-TODO: owner->m_7c is the demo/level render context (owns the plane
// list + main plane via m_c->m_24; Ghidra carved neither, no RTTI). The plane
// objects are the canonical CLevelPlane; the surrounding holders are modeled by
// their proven offsets until the context class is recovered.
SIZE_UNKNOWN(ScrollGeomHolder);
struct ScrollGeomHolder {
    char p0[0x38];
    CLevelPlane** m_38; // +0x38  plane-layer array
    i32 m_3c;           // +0x3c  plane-layer count
    char p40[0x5c - 0x40];
    CLevelPlane* m_5c; // +0x5c  main plane
};
SIZE_UNKNOWN(ScrollViewHolder);
struct ScrollViewHolder {
    char p0[0x24];
    ScrollGeomHolder* m_24; // +0x24
};
SIZE_UNKNOWN(AutoScrollState);
struct AutoScrollState {
    char p0[0xc];
    ScrollViewHolder* m_c; // +0xc
    char p10[0x1c - 0x10];
    i32 m_1c; // +0x1c  mode (0 = pick target, 1 = scroll toward it)
    char p20[0x4c - 0x20];
    i32 m_4c, m_50; // +0x4c  per-axis scroll target
};
SIZE_UNKNOWN(ScrollOwner);
struct ScrollOwner {
    char p0[0x7c];
    AutoScrollState* m_7c; // +0x7c
};

// @early-stop
// Structure exact (switch mode-dispatch sub/je/dec/jne matches, x87 scale-multiply
// + plane loop + RecomputePlaneCoords all correct). Residual is a regalloc butterfly:
// retail pins m_c->m_24 in esi (freeing edi for the plane-loop counter i), cl pins it
// in edi (spilling i to a 3rd stack slot -> sub esp,0xc vs 0x8) + holds curY in
// edx/[esp+0x20] vs retail's ecx/[esp+0x1c]. Not source-steerable; ~73% code-correct.
RVA(0x0003c300, 0x183)
i32 DemoAutoScrollStep(ScrollOwner* owner) {
    AutoScrollState* st = owner->m_7c;
    switch (st->m_1c) {
        case 1: {
            // step the current scroll position one unit toward the target.
            ScrollGeomHolder* gh = st->m_c->m_24;
            i32 curX = gh->m_5c->m_originX;
            i32 curY = gh->m_5c->m_originY;
            if (curX < st->m_4c) {
                curX++;
            } else if (curX > st->m_4c) {
                curX--;
            }
            if (curY < st->m_50) {
                curY++;
            } else if (curY > st->m_50) {
                curY--;
            }
            // apply the (optionally parallax-scaled) coords to the main plane + recompute.
            CLevelPlane* mg = gh->m_5c;
            float fx = (float)curX;
            float fy = (float)curY;
            if (!(mg->m_flags & 1)) {
                fx *= mg->m_scaleX;
                fy *= mg->m_scaleY;
            }
            mg->m_scaledX = fx;
            mg->m_scaledY = fy;
            mg->RecomputePlaneCoords();
            // apply the same coords to every plane layer.
            for (i32 i = 0; i < gh->m_3c; i++) {
                CLevelPlane* p = gh->m_38[i];
                float px = (float)curX;
                float py = (float)curY;
                if (!(p->m_flags & 1)) {
                    px *= p->m_scaleX;
                    py *= p->m_scaleY;
                }
                p->m_scaledX = px;
                p->m_scaledY = py;
                p->RecomputePlaneCoords();
            }
            // reached the target -> back to mode 0.
            if (st->m_4c == curX && st->m_50 == curY) {
                st->m_1c = 0;
            }
            return 1;
        }
        case 0: {
            // pick a fresh random per-axis target within the main plane's wrap range.
            i32 rx = st->m_c->m_24->m_5c->m_wrapW;
            st->m_4c = (rx == -1) ? (rand() % 2 - 1) : (rand() % (rx + 1));
            i32 ry = st->m_c->m_24->m_5c->m_wrapH;
            st->m_50 = (ry == -1) ? (rand() % 2 - 1) : (rand() % (ry + 1));
            st->m_1c = 1;
            break;
        }
    }
    return 1;
}

// CGrunt_IsSameType @0x3c7f0 - a free __cdecl comparator returning whether two
// grunts share the same type record (their +0x8 sub-object ptr).
class CGrunt;
RVA(0x0003c7f0, 0x18)
bool CGrunt_IsSameType(CGrunt* a, CGrunt* b) {
    return *(void**)((char*)a + 8) == *(void**)((char*)b + 8);
}

// ---------------------------------------------------------------------------
// 0x3c850 / 0x3c8a0 - twin orientation-step helpers (ORPHAN: no callers, no RTTI).
// __thiscall(count) over a 3-int orientation {facing, sub, dir}; each step looks up
// the next {facing, sub, dir} in a table indexed by (facing*3 + sub). The two tables
// (0x60d008 / 0x60d078) are the CW/CCW rotation transitions.
// @orphan - identity genuinely unrecovered (no xref/RTTI); modeled on a local helper.
// ---------------------------------------------------------------------------
DATA(0x0020d008)
extern const i32 g_rotTableA_60d008[30]; // 10 * {facing, sub, dir}
DATA(0x0020d078)
extern const i32 g_rotTableB_60d078[30];

SIZE_UNKNOWN(Orient3);
struct Orient3 {
    i32 m_0, m_4, m_8;
    void StepA(i32 count); // 0x3c850
    void StepB(i32 count); // 0x3c8a0
};

// @early-stop
// Counter-register regalloc wall: retail pins the loop counter in edi (push edi at
// entry, callee-saved) which frees edx for the m_4 temp + esi for the `e` pointer,
// so it materializes `e` once (lea edx,[eax*4+tbl]; mov esi,edx) and reads e[0..2]
// via [esi]. cl assigns count to edx (scratch), reads e[0] directly via [tbl+eax*4]
// then re-leas the base for e[1..2]. Same instrs, ~correct; not source-steerable (a
// count-copy local didn't flip it). Logic + the table DIR32 reloc are exact.
RVA(0x0003c850, 0x38)
void Orient3::StepA(i32 count) {
    if (count > 0) {
        do {
            const i32* e = &g_rotTableA_60d008[(m_0 * 3 + m_4) * 3];
            m_0 = e[0];
            m_4 = e[1];
            m_8 = e[2];
        } while (--count);
    }
}

// @early-stop
// Same counter-register regalloc wall as StepA (edi vs edx); logic + table reloc exact.
RVA(0x0003c8a0, 0x38)
void Orient3::StepB(i32 count) {
    if (count > 0) {
        do {
            const i32* e = &g_rotTableB_60d078[(m_0 * 3 + m_4) * 3];
            m_0 = e[0];
            m_4 = e[1];
            m_8 = e[2];
        } while (--count);
    }
}

// ---------------------------------------------------------------------------
// CTriRecord::Serialize @0x3c8f0 - the Serialize override of a 12-byte three-i32
// record. Transfers m_0/m_4/m_8 through the archive Read (slot +0x2c) / Write
// (slot +0x30) dispatch, keyed on the tag (4 = write, 7 = read).
RVA(0x0003c8f0, 0x76)
i32 CTriRecord::Serialize(CSerialArchive* ar, i32 tag, i32 c, i32 d) {
    switch (tag) {
        case 4:
            ar->Write(&m_0, 4);
            ar->Write(&m_4, 4);
            ar->Write(&m_8, 4);
            break;
        case 7:
            ar->Read(&m_0, 4);
            ar->Read(&m_4, 4);
            ar->Read(&m_8, 4);
            break;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// The two bute-config debug editor DialogProcs. INT_PTR CALLBACK, /GX. Identical
// shape, differing only in the key strings + the DAT_ edit buffers (0x62c458/
// 0x62c450 vs 0x63f790/0x643790):
//   WM_INITDIALOG(0x110): CString from the lowercase key ("attributez.txt" /
//     "dwrects.txt") via g_pButeDefaults [0x169fb0], read the buffer,
//     SetDlgItemTextA(0x435, ...).
//   WM_COMMAND(0x111): wParam==1(OK): GetDlgItemTextA(hDlg,0x435, DAT buf, 0xffff/
//     0x4000); strlen->DAT len; CString from the mixed-case key [0x16a670];
//     CButeMgr::Parse(CString, 1) on g_resButeMgr [thunk 0x38e6 -> 0x3cc20, below];
//     EndDialog(hDlg,1). wParam==2 (Cancel): EndDialog(hDlg,0).
// ---------------------------------------------------------------------------
// @early-stop
// 0x3c990 ("Attributez.txt" editor). BLOCKER: the CString/ResButeMgr helper callees
// (0x169fb0/0x16a670/0x16ab20/0x16aa50/0x16a8e0/0x16a510/0x16a3b0/0x16a240/0x169d70/
// 0x1b9d4c) are unreconstructed, and the /GX EH trylevel machine ([esp+0x6c]=1/-1/0)
// is driven by the exact CString-temp lifetimes - needs real MFC CString locals + the
// real ResButeMgr method identities first (finicky EH; a focused bute-editor pass).
RVA(0x0003c990, 0x1bc)
i32 Gap_03c990(void) {
    return 0;
}

// ---------------------------------------------------------------------------
// 0x3cbc0 / 0x3cbf0: compiler-generated member destructors for the bute editor's
// embedded C++ iostream members - each runs the two-phase virtual-base teardown of
// an fstream (the derived stream clear, then the shared ios virtual-base dtor).
// PROVEN library (disasm): 0x169d70 = ??1ios@@UAE@XZ (stamps ??_7ios@@6B@ @0x5f03bc,
// decrements the ios refcount), 0x16a240 = ??1ifstream@@UAE@XZ, 0x16a8e0 =
// ??1ofstream@@UAE@XZ. Named CButeIosSub-family per the established library_labels
// reloc-alias convention (0x169be0/0x169d70 already bound to CButeIosSub); the shared
// base Dtor reuses that alias, the two derived stream clears are reloc-aliased below
// (config/library_labels.csv) - so all four rel32 calls bind to their real library rvas.
// The comment above had the identity exactly right and then invented symbols for it
// anyway: CButeIosSub::Dtor / CSubObjC::Clear / CSubObj8::Clear were a "reloc-alias"
// stand-in family for ??1ios@@UAE@XZ / ??1ifstream@@UAE@XZ / ??1ofstream@@UAE@XZ. But an
// alias is a symbol NOTHING defines (objdiff masks the reloc; the linker would not), while
// the three REAL names are right there in the CRT libs. So call them: <fstream.h> is the
// toolchain's own header, and the qualified `p->ifstream::~ifstream()` form emits the same
// direct rel32 retail does. The double call (derived dtor, then the shared `ios`
// virtual-base dtor, BOTH with ecx = this+off - retail reloads ecx from esi unchanged) is
// MSVC's two-phase teardown of a virtual-base member; the void* launder keeps cl from
// inserting a vbase adjustment between them.
// `this` IS the stream object's base in both helpers - retail's `lea esi,[ecx+0xc]` /
// `lea esi,[ecx+0x8]` are MSVC's VIRTUAL-BASE ADJUSTMENTS, not member offsets (~ifstream
// reads its vbtable at [ecx-0xc], ~ofstream at [ecx-0x8]; the adjust is per-class). So the
// stream sits at offset 0 and cl computes the adjusted `this` itself; the shared `ios`
// virtual-base dtor then runs on that SAME adjusted pointer.
struct COwnerWithSubs {
    void DtorSubC(); // 0x3cbc0  (`this` is an ifstream; vbase adjust 0xc)
    void DtorSub8(); // 0x3cbf0  (`this` is an ofstream; vbase adjust 0x8)
};

RVA(0x0003cbc0, 0x14)
void COwnerWithSubs::DtorSubC() {
    ((ifstream*)(void*)this)->ifstream::~ifstream(); // 0x16a240 ??1ifstream@@UAE@XZ
    ((ios*)(void*)((char*)this + 0xc))->ios::~ios(); // 0x169d70 ??1ios@@UAE@XZ
}

RVA(0x0003cbf0, 0x14)
void COwnerWithSubs::DtorSub8() {
    ((ofstream*)(void*)this)->ofstream::~ofstream(); // 0x16a8e0 ??1ofstream@@UAE@XZ
    ((ios*)(void*)((char*)this + 0x8))->ios::~ios(); // 0x169d70 ??1ios@@UAE@XZ
}

// ---------------------------------------------------------------------------
// CButeMgr::Parse(CString, int) @0x3cc20 - the .att file-parse entry point: opens
// a CRT istream-derived source stream over the named file, checks it opened OK
// (ios::state & (failbit|badbit)), resets the three keyed stores, runs the
// recursive group parse, then closes + deletes the stream. /GX EH-framed.
//
// The stream is a multiply/virtually-derived CRT iostream class: its `ios`
// virtual base carries the state word at +8 (the `& 6` open-failure probe) and
// the vtable whose slot-0 scalar-deleting dtor runs the `delete`. Modeled as a
// real virtual-base hierarchy so MSVC lowers the vbtable lookups + the vbase-
// adjusted virtual delete; the engine ctor/Sync (0x169fb0 / 0x16a3b0) are
// external no-body callees (reloc-masked).
class streambuf; // the CRT <streambuf.h> foreign type (opaque here)
struct ButeIos {
    virtual ~ButeIos(); // vptr @ +0
    streambuf* m_bp;    // +0x04  the CRT streambuf the ios owns
    int m_state;        // +0x08  io_state (& 6 == failbit|badbit)
    char m_pad0c[0x50 - 0xc];
};
SIZE(ButeIos, 0x50); // CRT ios virtual base view (vptr, streambuf, state, ...)

// The file source stream `new`-d at +0xa0: vbptr @ +0, two derived dwords, then
// the `ios` virtual base @ +0xc -> sizeof 0x5c. The ctor (0x169fb0) takes the
// filename, an open-mode word (0x21), and the defaults pointer; MSVC appends the
// hidden most-derived vbase flag (=1). Sync (0x16a3b0) finalizes before delete.
struct ButeFileStream : virtual ButeIos {
    int m_d4;                                                       // +0x04
    int m_d8;                                                       // +0x08
    ButeFileStream(const char* fileName, int mode, void* defaults); // 0x169fb0
    void Sync();                                                    // 0x16a3b0
    virtual ~ButeFileStream() OVERRIDE; // external; the delete runs the vbase vtable's slot-0
};
SIZE(ButeFileStream, 0x5c); // vbptr + 2 dwords + the ios virtual base @0xc
// NO VTBL: its ctor/dtor are declared-only (the CRT bodies), so cl emits no ??_7 for
// this class in ANY form - not the plain ??_7ButeFileStream@@6B@ and not a through-base
// ??_7ButeFileStream@@6BButeIos@@@ (llvm-nm over every base obj). The old
// RELOC_VTBL(ButeFileStream, 0x001f03c4) therefore masked nothing, and the rva it named
// is not even a primary vtable: 0x1f03c4 carries RTTI .?AVifstream@@ at **base_off 12**,
// i.e. the ios VIRTUAL-BASE secondary table of the CRT ifstream - whose mangled name
// embeds the base and can never be a plain ??_7<class>@@6B@ row.
// @identity-TODO: ButeFileStream IS the CRT `ifstream` (FID-labelled iostream methods;
// the +0x14/+0xc/+0x8 constants in the users are vbase adjustments, not member offsets).
// Folding it onto the real <fstream.h> class is the remaining work.

// @early-stop
// 98.84% - logic + instruction-selection byte-exact (verified base-vs-target
// llvm-objdump); two residuals, both proven artifacts:
//   (1) the /GX prologue `push <scopetable>`: base references its local unwind
//       table `$L17759 + 0`, retail references the shared `Unwind@005d9ca8 +
//       0xb`. objdiff masks the symbol but the addend (0x0 vs 0xb) differs -
//       the per-unit unwind-record-layout wall (retail packed this fn's unwind
//       record at +0xb of a shared table; an isolated single-fn unit emits it
//       at +0 of a local symbol; not source-steerable).
//   (2) one `mov ecx,esi` (ParseGroup's `this`-setup) scheduled at slot 1 of
//       the third inlined Reset()'s store run (mine) vs slot 3 (retail) - a
//       pure MSVC5 list-scheduler tie-break; same opcode/operands, identical
//       everywhere else. Tried moving the `result=true` init + an explicit
//       tree pointer; neither flips it (one regressed to 96.3%).
RVA(0x0003cc20, 0x14e)
bool CButeMgr::Parse(CString filename, int streamBase) {
    ButeFileStream* s = new ButeFileStream(filename, 0x21, g_pButeDefaults);
    m_stream = s;
    if ((s->m_state & 6) != 0) {
        return false;
    }

    Init();
    m_streamBase = streamBase;
    m_str108 = filename;

    m_tree.Reset();
    m_tree48.Reset();
    m_tree74.Reset();

    bool result = true;
    if (!ParseGroup()) {
        m_0d = 1;
        result = false;
    }

    // m_stream is a genuinely heterogeneous void* slot (NextChar reads it as
    // CButeStream* / void** / int); the reload-through-cast here reproduces retail's
    // two member reloads (using the typed local `s` would keep it in a register and
    // diverge). Documented reinterpret of the multi-view slot, not a placeholder.
    ((ButeFileStream*)m_stream)->Sync();
    delete (ButeFileStream*)m_stream;
    return result;
}

// @early-stop
// 0x3cdd0 ("dwrects.txt" editor) - twin of 0x3c990 (0x4000-byte edit buffer variant);
// same CString/ResButeMgr + /GX-trylevel blocker.
RVA(0x0003cdd0, 0x19f)
i32 Gap_03cdd0(void) {
    return 0;
}

// ---------------------------------------------------------------------------
// The state-0 anim-worker dispatch family (0x3d2b0..0x3ddf0) - each handler is a
// __cdecl FREE function (the owner is a stack arg, ecx is never `this`); it reads
// owner->m_7c (the worker), then runs a message pump keyed on the worker's state
// tag worker->m_1c:
//   state 0      -> `new <leaf>(owner)`; activate it (sub->vtbl[0x18]); stow
//                   it at worker->m_18; advance the state tag to 0x3e8.
//   state 0x1d   -> sub->vtbl[0x2c]()      state 0x1e -> sub->vtbl[0x28]()
//   state 0x50   -> sub->vtbl[0x38]()      state 0x53 -> sub->vtbl[0x3c]()
//   state 0x52   -> sub->vtbl[0x30]()      state 0x51 -> sub->vtbl[0x34]()
//   state 0x3e8  -> idle (no-op).          default     -> the engine default pump
//                   (0x16e4f0, __cdecl, taking the sub-record).
// The handlers are byte-identical bar the sub-record TYPE (the `new` size + ctor
// target). The switch key worker->m_1c is UNSIGNED (u32) so MSVC5 emits the range
// checks as unsigned ja/jbe, matching retail byte-for-byte (a signed i32 key emits
// jg/jle and caps each at 97.86%; see docs/patterns/switch-key-unsigned-ja-vs-jg.md).
//
// The family's siblings live with their classes: Handler03a200 (CCursorSnapSprite)
// in CursorSnapSprite.cpp, Handler046990 (CExplosion) in FortressFlag.cpp, the
// grunt-HUD cluster in GruntIndicatorWorkerHandlers.cpp.
RVA(0x0003d2b0, 0xf1)
i32 Handler03d2b0(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            CUserLogic* sub = new CGruntStartingPoint((CGameObject*)owner);
            sub->Activate();
            rec->m_18 = sub;
            break;
        }
        case 0x1d:
            rec->m_18->UserLogicVfunc9();
            break;
        case 0x1e:
            rec->m_18->UserLogicVfunc8();
            break;
        case 0x50:
            rec->m_18->UserLogicVfuncC();
            break;
        case 0x53:
            rec->m_18->UserLogicVfuncD();
            break;
        case 0x52:
            rec->m_18->UserLogicVfuncA();
            break;
        case 0x51:
            rec->m_18->UserLogicVfuncB();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_18);
            break;
    }
    return 1;
}

RVA(0x0003d3f0, 0xf1)
i32 Handler03d3f0(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            CUserLogic* sub = new CExitTrigger((CGameObject*)owner);
            sub->Activate();
            rec->m_18 = sub;
            break;
        }
        case 0x1d:
            rec->m_18->UserLogicVfunc9();
            break;
        case 0x1e:
            rec->m_18->UserLogicVfunc8();
            break;
        case 0x50:
            rec->m_18->UserLogicVfuncC();
            break;
        case 0x53:
            rec->m_18->UserLogicVfuncD();
            break;
        case 0x52:
            rec->m_18->UserLogicVfuncA();
            break;
        case 0x51:
            rec->m_18->UserLogicVfuncB();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_18);
            break;
    }
    return 1;
}

RVA(0x0003d530, 0xf1)
i32 Handler03d530(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            CUserLogic* sub = new CGruntCreationPoint((CGameObject*)owner);
            sub->Activate();
            rec->m_18 = sub;
            break;
        }
        case 0x1d:
            rec->m_18->UserLogicVfunc9();
            break;
        case 0x1e:
            rec->m_18->UserLogicVfunc8();
            break;
        case 0x50:
            rec->m_18->UserLogicVfuncC();
            break;
        case 0x53:
            rec->m_18->UserLogicVfuncD();
            break;
        case 0x52:
            rec->m_18->UserLogicVfuncA();
            break;
        case 0x51:
            rec->m_18->UserLogicVfuncB();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_18);
            break;
    }
    return 1;
}

RVA(0x0003d670, 0xf1)
i32 Handler03d670(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            CUserLogic* sub = new CWormhole((CGameObject*)owner);
            sub->Activate(); // slot 6 (+0x18): activate
            rec->m_18 = sub;
            break;
        }
        case 0x1d:
            rec->m_18->UserLogicVfunc9(); // slot 11 (+0x2c)
            break;
        case 0x1e:
            rec->m_18->UserLogicVfunc8(); // slot 10 (+0x28)
            break;
        case 0x50:
            rec->m_18->UserLogicVfuncC(); // slot 14 (+0x38)
            break;
        case 0x53:
            rec->m_18->UserLogicVfuncD(); // slot 15 (+0x3c)
            break;
        case 0x52:
            rec->m_18->UserLogicVfuncA(); // slot 12 (+0x30)
            break;
        case 0x51:
            rec->m_18->UserLogicVfuncB(); // slot 13 (+0x34)
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_18);
            break;
    }
    return 1;
}

RVA(0x0003d7b0, 0xf1)
i32 Handler03d7b0(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            CUserLogic* sub = new CGruntPuddle((CGameObject*)owner);
            sub->Activate();
            rec->m_18 = sub;
            break;
        }
        case 0x1d:
            rec->m_18->UserLogicVfunc9();
            break;
        case 0x1e:
            rec->m_18->UserLogicVfunc8();
            break;
        case 0x50:
            rec->m_18->UserLogicVfuncC();
            break;
        case 0x53:
            rec->m_18->UserLogicVfuncD();
            break;
        case 0x52:
            rec->m_18->UserLogicVfuncA();
            break;
        case 0x51:
            rec->m_18->UserLogicVfuncB();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_18);
            break;
    }
    return 1;
}

RVA(0x0003d8f0, 0xf1)
i32 Handler03d8f0(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            CUserLogic* sub = new CTeleporter((CGameObject*)owner);
            sub->Activate();
            rec->m_18 = sub;
            break;
        }
        case 0x1d:
            rec->m_18->UserLogicVfunc9();
            break;
        case 0x1e:
            rec->m_18->UserLogicVfunc8();
            break;
        case 0x50:
            rec->m_18->UserLogicVfuncC();
            break;
        case 0x53:
            rec->m_18->UserLogicVfuncD();
            break;
        case 0x52:
            rec->m_18->UserLogicVfuncA();
            break;
        case 0x51:
            rec->m_18->UserLogicVfuncB();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_18);
            break;
    }
    return 1;
}

RVA(0x0003da30, 0xf1)
i32 Handler03da30(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            CUserLogic* sub = new CSecretTeleporterTrigger((CGameObject*)owner);
            sub->Activate();
            rec->m_18 = sub;
            break;
        }
        case 0x1d:
            rec->m_18->UserLogicVfunc9();
            break;
        case 0x1e:
            rec->m_18->UserLogicVfunc8();
            break;
        case 0x50:
            rec->m_18->UserLogicVfuncC();
            break;
        case 0x53:
            rec->m_18->UserLogicVfuncD();
            break;
        case 0x52:
            rec->m_18->UserLogicVfuncA();
            break;
        case 0x51:
            rec->m_18->UserLogicVfuncB();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_18);
            break;
    }
    return 1;
}

// CWarlord's ctor is the odd one out: it takes an int (??0CWarlord@@QAE@H@Z), so
// the owner pointer is passed as (i32)owner (retail `push edi`); size 0xb0.
RVA(0x0003db70, 0xf4)
i32 Handler03db70(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            CUserLogic* sub = new CWarlord((i32)owner);
            sub->Activate();
            rec->m_18 = sub;
            break;
        }
        case 0x1d:
            rec->m_18->UserLogicVfunc9();
            break;
        case 0x1e:
            rec->m_18->UserLogicVfunc8();
            break;
        case 0x50:
            rec->m_18->UserLogicVfuncC();
            break;
        case 0x53:
            rec->m_18->UserLogicVfuncD();
            break;
        case 0x52:
            rec->m_18->UserLogicVfuncA();
            break;
        case 0x51:
            rec->m_18->UserLogicVfuncB();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_18);
            break;
    }
    return 1;
}

RVA(0x0003dcb0, 0xf1)
i32 Handler03dcb0(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            CUserLogic* sub = new CFortressFlag((CGameObject*)owner);
            sub->Activate();
            rec->m_18 = sub;
            break;
        }
        case 0x1d:
            rec->m_18->UserLogicVfunc9();
            break;
        case 0x1e:
            rec->m_18->UserLogicVfunc8();
            break;
        case 0x50:
            rec->m_18->UserLogicVfuncC();
            break;
        case 0x53:
            rec->m_18->UserLogicVfuncD();
            break;
        case 0x52:
            rec->m_18->UserLogicVfuncA();
            break;
        case 0x51:
            rec->m_18->UserLogicVfuncB();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_18);
            break;
    }
    return 1;
}

RVA(0x0003ddf0, 0xf1)
i32 Handler03ddf0(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            CUserLogic* sub = new CSecretLevelTrigger((CGameObject*)owner);
            sub->Activate();
            rec->m_18 = sub;
            break;
        }
        case 0x1d:
            rec->m_18->UserLogicVfunc9();
            break;
        case 0x1e:
            rec->m_18->UserLogicVfunc8();
            break;
        case 0x50:
            rec->m_18->UserLogicVfuncC();
            break;
        case 0x53:
            rec->m_18->UserLogicVfuncD();
            break;
        case 0x52:
            rec->m_18->UserLogicVfuncA();
            break;
        case 0x51:
            rec->m_18->UserLogicVfuncB();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_18);
            break;
    }
    return 1;
}

SIZE_UNKNOWN(CDemoWorld);
SIZE_UNKNOWN(CDemoSetup);
SIZE_UNKNOWN(COwnerWithSubs);
SIZE_UNKNOWN(CTriRecord);
