#include <Gruntz/Demo.h>
#include <Gruntz/DemoHelpers.h> // CDemoSetup / Orient3 (the TU's helper types)
#include <Io/FileMem.h>         // the serialize stream (CFileMemBase == the real CFileMemBase)
#include <Gruntz/GruntzMgr.h> // CGruntzMgr / CGameMgr::m_gameWnd -> CGameWnd::m_hwnd (Render's exit post)
#include <Gruntz/FixedPtrArray32.h>  // the game-controller poll list (g_actorList)
#include <DinMgr2/DirectInputMgr2.h> // CInputDevBase (m_currentKeys press-edge flags)
#include <fstream.h> // the REAL CRT ifstream/ofstream/ios (their dtors ARE in the CRT libs)
#include <string.h>  // strlen (inline repne scas in the editor dialog proc)
#include <rva.h>

#include <Bute/ButeMgr.h>         // CButeMgr (Parse @0x3cc20)
#include <Bute/SymTab.h>          // the shared CSymTab (ResolveQualified 0x13be40)
#include <Gruntz/AnimWorker.h>    // shared Owner / Worker views + Worker_DefaultPump
#include <Gruntz/GameLevel.h>     // canonical CGameLevel + CDDrawWorkerHost (RecomputePlaneCoords)
#include <Gruntz/SerialArchive.h> // CFileMemBase (the inherited CWapX::Chain arg; ex SerialObjRef.h)
#include <Gruntz/SerialRecords.h>     // CTriRecord
#include <DDrawMgr/DDrawChildGroup.h> // the ONE CDDrawChildGroup shape (CreateSprite @0x1597b0)
#include <Gruntz/UserLogic.h>         // the dispatched CUserLogic leaves' slot layout
#include <Gruntz/GameLevel.h>         // m_world->m_level (CGameLevel; ex-CWorldState view dissolved)
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Ints.h>
#include <stdlib.h> // rand (0x11fee0, the engine LCG)

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

RVA(0x0003bfa0, 0x42)
i32 CDemo::LoadGameAssetNamespaces(i32 ctx, i32 a1, i32 a2) {
    (reinterpret_cast<CGruntzMgr*>(ctx))->m_strWorldFile.Empty();
    if (CPlay::LoadGameAssetNamespaces(ctx, a1, a2) == 0) {
        return 0;
    }
    m_520 = 0x124f80;
    return 1;
}

RVA(0x0003c030, 0x22)
i32 CDemo::Vslot15() {
    PostMessageA(m_mgr->m_gameWnd->m_hwnd, 0x111, 0x8027, 0);
    return 1;
}

// ---------------------------------------------------------------------------
// CDemoSetup (the attract/demo-mode actor seeding; @identity-TODO owner genuinely
// unrecovered) is modeled in <Gruntz/DemoHelpers.h>, not as a .cpp-local view.
// SetupDemoActors @0x3c070 - seed the two attract-mode demo HUD sprites
// ("DemoMover", "DemoSign") through the bound world's sprite factory.
RVA(0x0003c070, 0x47)
i32 CDemoSetup::SetupDemoActors() {
    m_c->m_childGroup->CreateSprite(1, 0, 0, 0, "DemoMover", 0x40003);
    m_c->m_childGroup->CreateSprite(1, 0, 0, 0x270f, "DemoSign", 0x40003);
    return 1;
}

class CParseSource;

RVA(0x0003c0e0, 0xfb)
i32 CDemo::BuildWorldLevelPath(i32 unused) { // slot-42 override (ex BuildWorldLevelKey)
    m_world->m_level->ReleaseChildren();
    CString key;
    key.Format("WORLDZ\\LEVEL%i", 1);
    CParseSource* node = m_levelBank->ResolveQualified(key, reinterpret_cast<void*>(0x575744));
    if (node == 0) {
        return 0;
    }
    if (m_world->m_level->LoadFromSource(node) == 0) {
        return 0;
    }
    m_world->m_level->NotifyAllPlanes();
    m_world->m_level->m_flags |= 4; // the CLoadable +0x08 flag word
    return 1;
}

// 0x3c220 - CDemo::Render (slot 5, +0x14): the demo/attract per-frame render poll.
// Runs the CPlay base render, scans the g_actorList for the 0x100 exit-request flag
// (posting WM_COMMAND 0x8023 to the top-level HWND on the first match), counts the
// idle timer m_520 down by the frame delta, and posts WM_COMMAND 0x8027 when it
// expires. Returns 1.
// @early-stop
// asm now byte-IDENTICAL (55 insns): the prior residual included a real signedness
// bug - the m_520 timer compare emitted signed jl where retail uses unsigned jb;
// fixed by casting the g_frameDelta>=m_520 test to u32. The only remaining residual
// is the PostMessageA IAT-absolute scoring artifact (target bakes the bare 0x6c44c8,
// no symbol) - reloc-masked truth, not a code diff. docs/matching-patterns.md.
RVA(0x0003c220, 0xa4)
i32 CDemo::Render() {
    CPlay::Render();
    CFixedPtrArray32* list = g_actorList;
    i32 n = list->m_count;
    for (i32 i = 0; i < n; i++) {
        if (list->m_items[i]->m_currentKeys & 0x100) {
            PostMessageA(m_mgr->m_gameWnd->m_hwnd, 0x111, 0x8023, 0);
            break;
        }
    }
    if (static_cast<u32>(g_frameDelta) >= static_cast<u32>(m_520)) {
        m_520 = 0;
    } else {
        m_520 -= g_frameDelta;
    }
    if (m_520 == 0) {
        PostMessageA(m_mgr->m_gameWnd->m_hwnd, 0x111, 0x8027, 0);
    }
    return 1;
}

RVA(0x0003c300, 0x183)
// @early-stop
// Structure exact (switch mode-dispatch sub/je/dec/jne matches, x87 scale-multiply
// + plane loop + RecomputePlaneCoords all correct). Residual is a regalloc butterfly:
// retail pins the level in esi (freeing edi for the plane-loop counter i), cl pins it
// in edi (spilling i to a 3rd stack slot -> sub esp,0xc vs 0x8) + holds curY in
// edx/[esp+0x20] vs retail's ecx/[esp+0x1c]. Not source-steerable; ~73% code-correct.
i32 DemoAutoScrollStep(CGameObject* owner) {
    AnimWorkerObj* st = owner->m_7c;
    switch (reinterpret_cast<i32>(st->m_1c)) {
        case 1: {
            // step the current scroll position one unit toward the target.
            CGameLevel* gh = (static_cast<CDDrawSurfaceMgr*>(st->m_0c))->m_level;
            i32 curX = gh->m_mainPlane->m_originX;
            i32 curY = gh->m_mainPlane->m_originY;
            if (curX < st->m_scrollTargetX) {
                curX++;
            } else if (curX > st->m_scrollTargetX) {
                curX--;
            }
            if (curY < st->m_scrollTargetY) {
                curY++;
            } else if (curY > st->m_scrollTargetY) {
                curY--;
            }
            // apply the (optionally parallax-scaled) coords to the main plane + recompute.
            CDDrawWorkerHost* mg = gh->m_mainPlane;
            float fx = static_cast<float>(curX);
            float fy = static_cast<float>(curY);
            if (!(mg->m_flags & 1)) {
                fx *= mg->m_scaleX;
                fy *= mg->m_scaleY;
            }
            mg->m_scaledX = fx;
            mg->m_scaledY = fy;
            mg->RecomputePlaneCoords();
            // apply the same coords to every plane layer (the m_planes CObArray;
            // the element cast is the devs' own - CObArray stores CObject*).
            for (i32 i = 0; i < gh->m_planes.GetSize(); i++) {
                CDDrawWorkerHost* p = static_cast<CDDrawWorkerHost*>(gh->m_planes[i]);
                float px = static_cast<float>(curX);
                float py = static_cast<float>(curY);
                if (!(p->m_flags & 1)) {
                    px *= p->m_scaleX;
                    py *= p->m_scaleY;
                }
                p->m_scaledX = px;
                p->m_scaledY = py;
                p->RecomputePlaneCoords();
            }
            // reached the target -> back to mode 0.
            if (st->m_scrollTargetX == curX && st->m_scrollTargetY == curY) {
                st->m_1c = 0;
            }
            return 1;
        }
        case 0: {
            // pick a fresh random per-axis target within the main plane's wrap range.
            i32 rx = (static_cast<CDDrawSurfaceMgr*>(st->m_0c))->m_level->m_mainPlane->m_wrapW;
            st->m_scrollTargetX = (rx == -1) ? (rand() % 2 - 1) : (rand() % (rx + 1));
            i32 ry = (static_cast<CDDrawSurfaceMgr*>(st->m_0c))->m_level->m_mainPlane->m_wrapH;
            st->m_scrollTargetY = (ry == -1) ? (rand() % 2 - 1) : (rand() % (ry + 1));
            st->m_1c = reinterpret_cast<void*>(1);
            break;
        }
    }
    return 1;
}

class CGrunt;
RVA(0x0003c7f0, 0x18)
bool CGrunt_IsSameType(CGrunt* a, CGrunt* b) {
    return *reinterpret_cast<void**>((reinterpret_cast<char*>(a) + 8))
           == *reinterpret_cast<void**>((reinterpret_cast<char*>(b) + 8));
}

DATA(0x0020d008)
const i32 g_rotTableA_60d008[27] = {
    0, 1, 1, 0, 2, 2, 1, 2, 3, 0, 0, 8, 1, 1, 0, 2, 2, 4, 1, 0, 7, 2, 0, 6, 2, 1, 5,
}; // CW transitions
DATA(0x0020d078)
const i32 g_rotTableB_60d078[27] = {
    1, 0, 7, 0, 0, 8, 0, 1, 1, 2, 0, 6, 1, 1, 0, 0, 2, 2, 2, 1, 5, 2, 2, 4, 1, 2, 3,
}; // CCW transitions

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

RVA(0x0003c8f0, 0x76)
i32 CTriRecord::Serialize(CFileMemBase* ar, i32 tag, i32 c, i32 d) {
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
// The editor dialog's text buffer + length word (first-referenced here).
DATA(0x0022c450)
i32 g_buteEditLen;
DATA(0x0022c458)
char g_buteEditBuf[0x10000];

// 0x3c990: the "Attributez.txt" editor dialog proc (edit control 0x435).
// WM_INITDIALOG loads attributez.txt into the edit box through a SCOPED ifstream;
// IDOK writes the box back (Attributez.txt) through a SCOPED ofstream, re-parses
// g_buteMgr, and closes with EndDialog(hDlg, 1); IDCANCEL EndDialog(hDlg, 0).
// The two scoped streams are exactly what makes cl emit the ??_Difstream /
// ??_Dofstream vbase-destructor COMDATs retail carries at 0x3cbc0/0x3cbf0
// (byte-identical to a cl 5.0 probe; retail keeps them unreferenced - no /OPT:REF).
// @early-stop
// structural reconstruction (ex a `return 0` stub + two hand-written ??_D bodies);
// the /GX trylevel schedule over the stream/CString temps still needs a matcher pass.
RVA(0x0003c990, 0x1bc)
INT_PTR CALLBACK ButeAttributezDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    static_cast<void>(lParam);
    if (msg == 0x110) { // WM_INITDIALOG
        ifstream in("attributez.txt", ios::nocreate | ios::binary);
        if (in.fail()) {
            EndDialog(hDlg, 1);
        } else {
            in.read(g_buteEditBuf, 0xffff);
            g_buteEditLen = in.gcount();
            g_buteEditBuf[g_buteEditLen] = 0;
            SetDlgItemTextA(hDlg, 0x435, g_buteEditBuf);
            in.close();
        }
        return 1;
    }
    if (msg != 0x111) { // WM_COMMAND
        return 0;
    }
    if (wParam == 1) { // IDOK
        GetDlgItemTextA(hDlg, 0x435, g_buteEditBuf, 0xffff);
        ofstream out("Attributez.txt", ios::binary);
        g_buteEditLen = strlen(g_buteEditBuf);
        out.write(g_buteEditBuf, g_buteEditLen);
        out.close();
        g_buteMgr.Parse("Attributez.txt", 0);
        EndDialog(hDlg, 1);
        return 1;
    }
    if (wParam == 2) { // IDCANCEL
        EndDialog(hDlg, 0);
        return 1;
    }
    return 0;
}

// The two 0x14-byte fns at 0x3cbc0/0x3cbf0 are the compiler-generated
// ??_Difstream@@QAEXXZ / ??_Dofstream@@QAEXXZ vbase destructors the dialog proc's
// unwind funclets make cl emit (COMDATs; bound below by RVA_COMPGEN). The old
// hand-written COwnerWithSubs::DtorSubC/DtorSub8 bodies - and their banned
// this+0xc/+0x8 vbase-offset casts - are GONE: no dev ever wrote that code.
RVA_COMPGEN(0x0003cbc0, 0x14, ??_Difstream@@QAEXXZ)
RVA_COMPGEN(0x0003cbf0, 0x14, ??_Dofstream@@QAEXXZ)

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
    // the third ctor arg is the header default `= filebuf::openprot` - the caller
    // materializes it as a load+push of ?openprot@filebuf@@2HB (retail 0x5f03e0)
    ifstream* s = new ifstream(filename, ios::in | ios::nocreate);
    m_stream = s;
    if (s->fail()) { // ios state & (failbit|badbit), behind the vbase adjust
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

    // m_stream is the base istream* member; here the concrete stream IS the ifstream `s`
    // just constructed, so downcast to it to call ifstream::sync + run its dtor via delete
    // (authentic downcast to the known concrete type, not a placeholder). Reloading THROUGH
    // the member - not the typed local `s`, which cl would keep in a register - is what
    // reproduces retail's two member reloads. The istream subobject sits at offset 0 of
    // ifstream (MSVC5), so the downcast is zero-adjust: byte-neutral vs the old void* model.
    (static_cast<ifstream*>(m_stream))->sync();
    delete static_cast<ifstream*>(m_stream);
    return result;
}

// @early-stop
// 0x3cdd0 ("dwrects.txt" editor) - twin of 0x3c990 (0x4000-byte edit buffer variant);
// same CString/ResButeMgr + /GX-trylevel blocker.
RVA(0x0003cdd0, 0x19f)
i32 EditDwRectsTxt(void) {
    return 0;
}

RVA(0x0003d2b0, 0xf1)
i32 GruntStartingPointWorkerPump(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_7c;
    switch (reinterpret_cast<u32>(rec->m_1c)) {
        case 0: {
            rec->m_1c = reinterpret_cast<void*>(0x3e8);
            CUserLogic* sub = new CGruntStartingPoint(owner);
            sub->Activate();
            rec->m_logic = sub;
            break;
        }
        case 0x1d:
            rec->m_logic->UserLogicVfunc9();
            break;
        case 0x1e:
            rec->m_logic->UserLogicVfunc8();
            break;
        case 0x50:
            rec->m_logic->UserLogicVfuncC();
            break;
        case 0x53:
            rec->m_logic->UserLogicVfuncD();
            break;
        case 0x52:
            rec->m_logic->UserLogicVfuncA();
            break;
        case 0x51:
            rec->m_logic->UserLogicVfuncB();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_logic);
            break;
    }
    return 1;
}

RVA(0x0003d3f0, 0xf1)
i32 ExitTriggerWorkerPump(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_7c;
    switch (reinterpret_cast<u32>(rec->m_1c)) {
        case 0: {
            rec->m_1c = reinterpret_cast<void*>(0x3e8);
            CUserLogic* sub = new CExitTrigger(owner);
            sub->Activate();
            rec->m_logic = sub;
            break;
        }
        case 0x1d:
            rec->m_logic->UserLogicVfunc9();
            break;
        case 0x1e:
            rec->m_logic->UserLogicVfunc8();
            break;
        case 0x50:
            rec->m_logic->UserLogicVfuncC();
            break;
        case 0x53:
            rec->m_logic->UserLogicVfuncD();
            break;
        case 0x52:
            rec->m_logic->UserLogicVfuncA();
            break;
        case 0x51:
            rec->m_logic->UserLogicVfuncB();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_logic);
            break;
    }
    return 1;
}

RVA(0x0003d530, 0xf1)
i32 GruntCreationPointWorkerPump(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_7c;
    switch (reinterpret_cast<u32>(rec->m_1c)) {
        case 0: {
            rec->m_1c = reinterpret_cast<void*>(0x3e8);
            CUserLogic* sub = new CGruntCreationPoint(owner);
            sub->Activate();
            rec->m_logic = sub;
            break;
        }
        case 0x1d:
            rec->m_logic->UserLogicVfunc9();
            break;
        case 0x1e:
            rec->m_logic->UserLogicVfunc8();
            break;
        case 0x50:
            rec->m_logic->UserLogicVfuncC();
            break;
        case 0x53:
            rec->m_logic->UserLogicVfuncD();
            break;
        case 0x52:
            rec->m_logic->UserLogicVfuncA();
            break;
        case 0x51:
            rec->m_logic->UserLogicVfuncB();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_logic);
            break;
    }
    return 1;
}

RVA(0x0003d670, 0xf1)
i32 WormholeWorkerPump(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_7c;
    switch (reinterpret_cast<u32>(rec->m_1c)) {
        case 0: {
            rec->m_1c = reinterpret_cast<void*>(0x3e8);
            CUserLogic* sub = new CWormhole(owner);
            sub->Activate(); // slot 6 (+0x18): activate
            rec->m_logic = sub;
            break;
        }
        case 0x1d:
            rec->m_logic->UserLogicVfunc9(); // slot 11 (+0x2c)
            break;
        case 0x1e:
            rec->m_logic->UserLogicVfunc8(); // slot 10 (+0x28)
            break;
        case 0x50:
            rec->m_logic->UserLogicVfuncC(); // slot 14 (+0x38)
            break;
        case 0x53:
            rec->m_logic->UserLogicVfuncD(); // slot 15 (+0x3c)
            break;
        case 0x52:
            rec->m_logic->UserLogicVfuncA(); // slot 12 (+0x30)
            break;
        case 0x51:
            rec->m_logic->UserLogicVfuncB(); // slot 13 (+0x34)
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_logic);
            break;
    }
    return 1;
}

RVA(0x0003d7b0, 0xf1)
i32 GruntPuddleWorkerPump(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_7c;
    switch (reinterpret_cast<u32>(rec->m_1c)) {
        case 0: {
            rec->m_1c = reinterpret_cast<void*>(0x3e8);
            CUserLogic* sub = new CGruntPuddle(owner);
            sub->Activate();
            rec->m_logic = sub;
            break;
        }
        case 0x1d:
            rec->m_logic->UserLogicVfunc9();
            break;
        case 0x1e:
            rec->m_logic->UserLogicVfunc8();
            break;
        case 0x50:
            rec->m_logic->UserLogicVfuncC();
            break;
        case 0x53:
            rec->m_logic->UserLogicVfuncD();
            break;
        case 0x52:
            rec->m_logic->UserLogicVfuncA();
            break;
        case 0x51:
            rec->m_logic->UserLogicVfuncB();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_logic);
            break;
    }
    return 1;
}

RVA(0x0003d8f0, 0xf1)
i32 TeleporterWorkerPump(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_7c;
    switch (reinterpret_cast<u32>(rec->m_1c)) {
        case 0: {
            rec->m_1c = reinterpret_cast<void*>(0x3e8);
            CUserLogic* sub = new CTeleporter(owner);
            sub->Activate();
            rec->m_logic = sub;
            break;
        }
        case 0x1d:
            rec->m_logic->UserLogicVfunc9();
            break;
        case 0x1e:
            rec->m_logic->UserLogicVfunc8();
            break;
        case 0x50:
            rec->m_logic->UserLogicVfuncC();
            break;
        case 0x53:
            rec->m_logic->UserLogicVfuncD();
            break;
        case 0x52:
            rec->m_logic->UserLogicVfuncA();
            break;
        case 0x51:
            rec->m_logic->UserLogicVfuncB();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_logic);
            break;
    }
    return 1;
}

RVA(0x0003da30, 0xf1)
i32 SecretTeleporterWorkerPump(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_7c;
    switch (reinterpret_cast<u32>(rec->m_1c)) {
        case 0: {
            rec->m_1c = reinterpret_cast<void*>(0x3e8);
            CUserLogic* sub = new CSecretTeleporterTrigger(owner);
            sub->Activate();
            rec->m_logic = sub;
            break;
        }
        case 0x1d:
            rec->m_logic->UserLogicVfunc9();
            break;
        case 0x1e:
            rec->m_logic->UserLogicVfunc8();
            break;
        case 0x50:
            rec->m_logic->UserLogicVfuncC();
            break;
        case 0x53:
            rec->m_logic->UserLogicVfuncD();
            break;
        case 0x52:
            rec->m_logic->UserLogicVfuncA();
            break;
        case 0x51:
            rec->m_logic->UserLogicVfuncB();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_logic);
            break;
    }
    return 1;
}

RVA(0x0003db70, 0xf4)
i32 WarlordWorkerPump(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_7c;
    switch (reinterpret_cast<u32>(rec->m_1c)) {
        case 0: {
            rec->m_1c = reinterpret_cast<void*>(0x3e8);
            CUserLogic* sub = new CWarlord(reinterpret_cast<i32>(owner));
            sub->Activate();
            rec->m_logic = sub;
            break;
        }
        case 0x1d:
            rec->m_logic->UserLogicVfunc9();
            break;
        case 0x1e:
            rec->m_logic->UserLogicVfunc8();
            break;
        case 0x50:
            rec->m_logic->UserLogicVfuncC();
            break;
        case 0x53:
            rec->m_logic->UserLogicVfuncD();
            break;
        case 0x52:
            rec->m_logic->UserLogicVfuncA();
            break;
        case 0x51:
            rec->m_logic->UserLogicVfuncB();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_logic);
            break;
    }
    return 1;
}

RVA(0x0003dcb0, 0xf1)
i32 FortressFlagWorkerPump(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_7c;
    switch (reinterpret_cast<u32>(rec->m_1c)) {
        case 0: {
            rec->m_1c = reinterpret_cast<void*>(0x3e8);
            CUserLogic* sub = new CFortressFlag(owner);
            sub->Activate();
            rec->m_logic = sub;
            break;
        }
        case 0x1d:
            rec->m_logic->UserLogicVfunc9();
            break;
        case 0x1e:
            rec->m_logic->UserLogicVfunc8();
            break;
        case 0x50:
            rec->m_logic->UserLogicVfuncC();
            break;
        case 0x53:
            rec->m_logic->UserLogicVfuncD();
            break;
        case 0x52:
            rec->m_logic->UserLogicVfuncA();
            break;
        case 0x51:
            rec->m_logic->UserLogicVfuncB();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_logic);
            break;
    }
    return 1;
}

RVA(0x0003ddf0, 0xf1)
i32 SecretLevelTriggerWorkerPump(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_7c;
    switch (reinterpret_cast<u32>(rec->m_1c)) {
        case 0: {
            rec->m_1c = reinterpret_cast<void*>(0x3e8);
            CUserLogic* sub = new CSecretLevelTrigger(owner);
            sub->Activate();
            rec->m_logic = sub;
            break;
        }
        case 0x1d:
            rec->m_logic->UserLogicVfunc9();
            break;
        case 0x1e:
            rec->m_logic->UserLogicVfunc8();
            break;
        case 0x50:
            rec->m_logic->UserLogicVfuncC();
            break;
        case 0x53:
            rec->m_logic->UserLogicVfuncD();
            break;
        case 0x52:
            rec->m_logic->UserLogicVfuncA();
            break;
        case 0x51:
            rec->m_logic->UserLogicVfuncB();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_logic);
            break;
    }
    return 1;
}

