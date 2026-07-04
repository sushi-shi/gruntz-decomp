#include <rva.h>
// ReconBatch2.cpp - reconstructed engine/game leaf methods recovered from the
// engine_unmatched worklist (matcher batch 2). Each function below is homed
// against a best-guess class shape (placeholder m_<hexoffset> field names; only
// the OFFSETS + emitted code bytes are load-bearing, campaign doctrine). All are
// plain /O2 /MT frameless leaves (no SEH/EH frame). External engine callees are
// modeled with NO body so their rel32 calls reloc-mask.
#include <Mfc.h> // real MFC (CPtrList/CString) + windows.h via afx.h (superset of Win32.h)

#include <DDrawMgr/DDSurface.h> // canonical CDDSurface (Blt @0x13ee60)
#include <Gruntz/Multi.h>       // real CMulti (the 0x64bd5c multiplayer singleton)
#include <sfman32.h>            // the *0x64e0b0 receiver (shared w/ SFSelectDevice)
#include <Globals.h>

// ===========================================================================
// 0x000be820 (73B) - EnableWindow(GetDlgItem(hDlg, id), obj->m_528) for ids
// 0x4d0 and 0x4d1. __cdecl(hDlg, obj). Clean Win32 idiom.
// ===========================================================================
struct DlgData_be820 {
    char m_pad0[0x528];
    i32 m_528; // +0x528 enable flag
};
RVA(0x000be820, 0x49)
void EnableButtons_be820(HWND hDlg, DlgData_be820* obj) {
    if (hDlg && obj) {
        EnableWindow(GetDlgItem(hDlg, 0x4d0), obj->m_528);
        EnableWindow(GetDlgItem(hDlg, 0x4d1), obj->m_528);
    }
}

// ===========================================================================
// 0x000bf580 (16B) - `g_pool.AddTail(arg0)` on the global recycled-node pool at
// 0x64aca8: the real MFC CPtrList `g_pool` (canonical ?g_pool@@3VCPtrList@@A, owned by
// LobbySync.cpp). Reachable directly now that this TU is MFC (wall broken); AddTail is
// the real NAFXCW CPtrList::AddTail (reloc-masked).
// ===========================================================================
DATA(0x0024aca8)
extern CPtrList g_pool;
RVA(0x000bf580, 0x10)
void AddTail_bf580(void* p) {
    g_pool.AddTail(p);
}

// ===========================================================================
// 0x000c0460 (46B) - scan 4 embedded 0x64-byte entries; return the first whose
// type==3, field4==0 and field10 > key (unsigned), else 0. __thiscall(1 arg).
// ===========================================================================
struct Entry_c0460 {
    i32 m_0; // +0x00 type
    i32 m_4; // +0x04
    char m_pad8[0x10 - 0x8];
    u32 m_10; // +0x10
    char m_pad14[0x64 - 0x14];
};
struct EntryOwner_c0460 {
    char m_pad0[0x20];
    Entry_c0460 m_entries[4]; // +0x20
    Entry_c0460* Find(u32 key);
};
// @early-stop
// regalloc wall (topic:wall topic:regalloc): structure byte-identical to retail
// (lea/loop/guards/ja/ret all match); residual is the key<->counter register
// swap (retail key=ecx/counter=edx, cl here key=edx/counter=ecx) driven by the
// key-load vs lea schedule order, not source-steerable, ~88.75%.
RVA(0x000c0460, 0x2e)
Entry_c0460* EntryOwner_c0460::Find(u32 key) {
    Entry_c0460* p = &m_entries[0];
    for (i32 i = 0; i < 4; i++, p++) {
        if (p && p->m_0 == 3 && p->m_4 == 0 && p->m_10 > key) {
            return p;
        }
    }
    return 0;
}

// ===========================================================================
// 0x000c2a80 (19B) - call two sibling __thiscall methods on `this`
// (Sub0c2ab0(3) then Sub0c40b0()).
// ===========================================================================
struct Host_c2a80 {
    void Sub0c2ab0(i32 a);
    void Sub0c40b0();
    void Run();
};
RVA(0x000c2a80, 0x13)
void Host_c2a80::Run() {
    Sub0c2ab0(3);
    Sub0c40b0();
}

// ===========================================================================
// 0x000c4b30 (31B) - resolve an options slot for a global config id; return the
// stored value or -1 when absent.
// ===========================================================================
struct OptionsSlotHost_c4b30 {
    i32* FindOptionsSlot(i32 key); // 0x2e00 (FindOptionsSlot)
};
// The multiplayer game-state singleton at 0x64bd5c is the real CMulti (xref-proven;
// <Gruntz/Multi.h>, included above now that this TU is MFC -- the old <Win32.h> wall is
// broken). This TU owns the canonical DATA; Resolve reads CMulti::m_hostIndex directly.
DATA(0x0024bd5c)
extern CMulti* g_64bd5c;
struct OptOwner_c4b30 {
    char m_pad0[0x5c];
    OptionsSlotHost_c4b30* m_5c; // +0x5c
    i32 Resolve();
};
RVA(0x000c4b30, 0x1f)
i32 OptOwner_c4b30::Resolve() {
    i32* slot = m_5c->FindOptionsSlot(g_64bd5c->m_hostIndex);
    if (slot == 0) {
        return -1;
    }
    return *slot;
}

// ===========================================================================
// 0x000f9840 / 0x000de140 (cleanup pair, CGameModeBase) - stop the owned sub-
// resource (SoundStream::Stop), clear/prune the sub-manager map, then run the
// base teardown (CGameModeBase::BaseCleanup). m_c->m_28 (the map holder) is
// reloaded each statement, not cached.
// ===========================================================================
// Local placeholder view of the real SoundStream (include/Dsndmgr/SoundStream.h);
// 0x137a80 is canonically SoundStream::Stop (minervainner / SoundStreamTeardown.cpp),
// so name the method Stop here too (was the mislabel "Free").
class SoundStream {
public:
    void Stop(); // 0x137a80 (= SoundStream::Stop)
};
// The map holder: ClearMap lives in the CDDrawMapHolder base; the keyed
// prune RemoveKeysEqual_157c70 is on the CDDrawSubMgrLeafScan view. m_2c is the
// freeable inner at +0x2c.
SIZE_UNKNOWN(CDDrawMapHolder);
class CDDrawMapHolder {
public:
    char m_pad0[0x2c];
    SoundStream* m_2c; // +0x2c
    void ClearMap();   // 0x157bc0
};
class CDDrawSubMgrLeafScan {
public:
    i32 RemoveKeysEqual_157c70(const char* a, const char* b); // 0x157c70
};
struct Holder_f9840 {
    char m_pad0[0x28];
    CDDrawMapHolder* m_28; // +0x28
};
class CGameModeBase {
public:
    char m_pad0[0xc];
    Holder_f9840* m_c;   // +0x0c
    void BaseCleanup();  // 0x3f53 thunk -> CGameModeBase::BaseCleanup
    void Reset();        // 0x0f9840
    void ResetPreview(); // 0x0de140
};
// @early-stop
// regalloc wall (topic:wall topic:regalloc): same m_28-intermediate register
// micro-diff as Reset; logic + named callees + the "_" string literal match
// retail, ~98.8%.
RVA(0x000de140, 0x33)
void CGameModeBase::ResetPreview() {
    if (m_c->m_28->m_2c != 0) {
        m_c->m_28->m_2c->Stop();
    }
    ((CDDrawSubMgrLeafScan*)m_c->m_28)->RemoveKeysEqual_157c70(s_PREVIEW_6135e8, "_");
    BaseCleanup();
}

// ===========================================================================
// 0x000f8ec0 (80B) - if the init flag is set, walk key codes 1..0x7f dispatching
// vtable slot +0x34 on a global receiver; set the scratch word each iteration.
// __cdecl(); returns 0 if the flag is clear else 1.
// ===========================================================================
DATA(0x0024dace)
extern WORD g_sfCfgA2;
DATA(0x0024dacc)
extern WORD g_sfCfgA0;
DATA(0x0024dd28)
extern WORD g_sfDeviceId;
RVA(0x000f8ec0, 0x50)
i32 SfDeviceInitKeys() {
    if (g_sfReady == 0) {
        return 0;
    }
    g_sfCfgA2 = 0;
    for (i32 i = 1; i <= 0x7f; i++) {
        g_sfCfgA0 = (WORD)i;
        g_sfDevice->SF_GetLoadedBankPathname(g_sfDeviceId, &g_sfCfgA0);
    }
    g_sfCfgA0 = 1;
    return 1;
}

// @early-stop
// regalloc wall (topic:wall topic:regalloc): logic byte-faithful (every load/
// call/offset + the 3 named callees match); residual is the m_28 intermediate
// register choice in the pointer chain (retail reuses eax: eax->eax->ecx; cl
// here picks fresh ecx/edx) - a 2-3 byte modrm-field micro-diff, ~98.7%.
RVA(0x000f9840, 0x29)
void CGameModeBase::Reset() {
    if (m_c->m_28->m_2c != 0) {
        m_c->m_28->m_2c->Stop();
    }
    m_c->m_28->ClearMap();
    BaseCleanup();
}

// ===========================================================================
// 0x000faec0 (103B) - per-frame present/refresh of the bound view. If the
// suppress flag is set, clear it and return; else Refresh the sub-view, blit the
// front surface with arg0, Flip the back surface, then Refresh again.
// __thiscall(arg0). m_c->m_4 is reloaded each statement (not cached across).
//
// NOT a one-class-many-views over-split: it is a genuine 4-deep pointer chain into
// distinct real DDraw objects (every sub-object call is direct/reloc-masked, so the
// typing below is matching-neutral):
//   SubView_faec0.Refresh  = CDDrawWorkerMgr::Method_158c70 (0x158c70, ddrawsubmgr, EXACT)
//   CView.m_2c .Flip       = CDDSurface::Flip               (0x13e850, directdrawmgr, EXACT)
//   CView.m_2c .CopyRect   = CFileImage::ShadeRect          (0x13f460, lutshaderect, src-claim ~67%)
// unidentified: PresentHost_faec0 (the `this` owner) + the Mid/CView hops -- the
// only inbound edge is ILT thunk 0x1ec9 (no clean ctor/new trace).
// ===========================================================================
struct SurfCtl_faec0 { // +0x2c surface controller (CDDSurface for Flip / CFileImage for the blit)
    void CopyRect(i32 a, i32 b); // 0x13f460 (= CFileImage::ShadeRect)
    void Flip(i32 a);            // 0x13e850 (= CDDSurface::Flip)
};
struct CView_faec0 {
    char m_pad0[0x2c];
    SurfCtl_faec0* m_2c; // +0x2c
};
struct SubView_faec0 { // = CDDrawWorkerMgr (Refresh = Method_158c70 @0x158c70)
    char m_pad0[0x10];
    CView_faec0* m_10;            // +0x10 (Flip target host)
    CView_faec0* m_14;            // +0x14 (blit target host + Refresh arg)
    void Refresh(CView_faec0* v); // 0x158c70
};
struct Mid_faec0 {
    char m_pad0[0x4];
    SubView_faec0* m_4; // +0x04
};
struct
    PresentHost_faec0 { // unidentified owner of Present @0xfaec0 (only inbound edge: ILT thunk 0x1ec9)
    char m_pad0[0xc];
    Mid_faec0* m_c; // +0x0c
    void Present(i32 arg0);
};
RVA(0x000faec0, 0x67)
void PresentHost_faec0::Present(i32 arg0) {
    if (g_suppress_64e360 != 0) {
        g_suppress_64e360 = 0;
        return;
    }
    m_c->m_4->Refresh(m_c->m_4->m_14);
    m_c->m_4->m_14->m_2c->CopyRect(arg0, 0);
    m_c->m_4->m_10->m_2c->Flip(0);
    m_c->m_4->Refresh(m_c->m_4->m_14);
}

// ===========================================================================
// 0x000fafa0 (59B) - __stdcall(4) validity gate: returns 0 if arg0 is null;
// for kind 4 / 7 validates arg0 through a per-kind checker (return 0 on fail);
// otherwise (and on success) returns 1.
// ===========================================================================
i32 __stdcall Check4_2ce8(i32 h); // 0x0faff0 (kind 4)
i32 __stdcall Check7_36bb(i32 h); // 0x0fb1c0 (kind 7)
// @early-stop
// regalloc wall (topic:wall topic:regalloc): the switch body (cmp 4 je / cmp 7
// jne / kind7 inline / kind4 trailing) is byte-identical to retail; residual is
// a0 landing in ecx (cl) vs eax (retail) - so the a0==0 return needs an extra
// xor eax, and the push/cmp register encodings shift. The switch dispatch claims
// eax for `kind`; not source-steerable. ~93.3%.
RVA(0x000fafa0, 0x3b)
i32 __stdcall Validate_fafa0(i32 a0, i32 kind, i32 a2, i32 a3) {
    if (a0 == 0) {
        return 0;
    }
    switch (kind) {
        case 4:
            if (Check4_2ce8(a0) == 0) {
                return 0;
            }
            break;
        case 7:
            if (Check7_36bb(a0) == 0) {
                return 0;
            }
            break;
    }
    return 1;
}

// ===========================================================================
// 0x001104f0 (86B) - one-shot init: if already initialized (m_20) return 0; else
// scatter the 8 __stdcall args into member fields, set m_20=1, m_1c=0; return 1.
// ===========================================================================
struct Init8_1104f0 {
    i32 m_0;
    i32 m_4;
    i32 m_8;
    i32 m_c;
    i32 m_10;
    i32 m_14;
    i32 m_18;
    i32 m_1c;
    i32 m_20;
    i32 m_24;
    i32 m_28;
    i32 Init(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7);
};
RVA(0x001104f0, 0x56)
i32 Init8_1104f0::Init(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7) {
    if (m_20) {
        return 0;
    }
    m_4 = a1;
    m_8 = a2;
    m_c = a3;
    m_10 = a4;
    m_24 = a0;
    m_18 = a6;
    m_28 = a7;
    m_1c = 0;
    m_14 = a5;
    m_20 = 1;
    return 1;
}

// ===========================================================================
// 0x00112840 (12B) - `return LoadSwitchDownSprite() != 0;`. The trailing
// neg/sbb/neg is the int->bool normalize (docs/patterns/int-to-bool-normalize).
// ===========================================================================
i32 LoadSwitchDownSprite_2e0f();
RVA(0x00112840, 0xc)
i32 Probe_112840() {
    return LoadSwitchDownSprite_2e0f() != 0;
}

// ===========================================================================
// 0x00115630 (10B) - forward to a Font method on the medium-font global
// (tail call: `mov ecx,&g_mediumFont; jmp Font::Method`).
// ===========================================================================
class Font {
public:
    Font* Ctor179700(); // tail-called on the global (retail: ??0Font@@QAE@XZ)
};
DATA(0x0024eae8)
extern Font g_mediumFont;
RVA(0x00115630, 0xa)
Font* Forward_115630() {
    return g_mediumFont.Ctor179700();
}

// ===========================================================================
// 0x0011e8dc (7B) - __thiscall vptr re-stamp: store the base dtor vtable
// (g_wapObjectDtorVtbl) into [this]. TERMINAL manual stamp (not convertible to
// `: public Wap::CObject`): this 7-byte fn IS the entire retail restamp - there is
// no ctor for cl to fold an auto-stamp into, and Obj_11e8dc is a placeholder.
// ===========================================================================
DATA(0x001e8cb4)
extern void* g_wapObjectDtorVtbl;
struct Obj_11e8dc {
    void* m_vptr; // +0x00
    void StampVtbl();
};
RVA(0x0011e8dc, 0x7)
void Obj_11e8dc::StampVtbl() {
    m_vptr = &g_wapObjectDtorVtbl;
}
// ===========================================================================
// 0x0016f6e0 (118B) - __stdcall(src, dst): while the descriptor-defined flag bit
// of src is clear, read an 8-byte record from src, transform it (Fn16f7f0), and
// write it to dst; remember src->m_8; finally Finish dst with the last m_8.
// ===========================================================================
struct Desc_16f6e0 {
    char m_pad0[4];
    i32 m_4; // +0x04 byte offset of the flag within the object
};
struct Src_16f6e0 {
    Desc_16f6e0* m_0; // +0x00 descriptor
    char m_pad4[0x8 - 0x4];
    i32 m_8;                           // +0x08
    void ReadRecord(void* buf, i32 n); // 0x16a510
};
struct Dst_16f6e0 {
    void WriteRecord(void* buf, i32 n); // 0x16ab20
    void Finish(i32 last);              // 0x16aab0
};
void Blowfish_encipher(unsigned int* lo, unsigned int* hi); // 0x16f7f0
// @early-stop
// regalloc wall (topic:wall topic:regalloc, const-materialize-into-reg-vs-
// immediate): the whole control flow + record read/encipher/write + Blowfish
// reloc match retail; residual is that retail pins the test mask 1 in bl
// (`movb $1,%bl; testb %bl,mem`) and re-zeros the record buffer with a fresh
// `xor edx` inside the loop, while cl tests with an immediate `$1` and hoists the
// zero into ebx outside the loop. Not source-steerable, ~84.9%.
RVA(0x0016f6e0, 0x76)
void __stdcall Copy_16f6e0(Src_16f6e0* src, Dst_16f6e0* dst) {
    i32 last = 0;
    while ((*((char*)src + src->m_0->m_4 + 8) & 1) == 0) {
        unsigned int rec[2];
        rec[0] = 0;
        rec[1] = 0;
        src->ReadRecord(rec, 8);
        last = src->m_8;
        Blowfish_encipher(&rec[0], &rec[1]);
        dst->WriteRecord(rec, 8);
    }
    dst->Finish(last);
}

// ===========================================================================
// 0x001816a0 (28B) - if a held item handle is present, remove it from the owned
// list and clear it. Sibling of 0x00181660 (same class: m_2c list, m_40 handle).
// ===========================================================================
class CPoolItemA;
class CDDrawPtrCollections {
public:
    void RemoveItemA(CPoolItemA* h);                            // 0x142160
    CPoolItemA* MakeAndAddB(i32 a, i32 b, i32 c, i32 d, i32 e); // 0x142e60
};
struct Worker181x_181x {
    void* m_vptr;                // +0x00
    char m_pad04[0x2c - 0x04];   // +0x04..0x2b
    CDDrawPtrCollections* m_2c;  // +0x2c  owned item collection
    char m_pad30[0x38 - 0x30];   // +0x30..0x37
    CDDSurface* m_38;            // +0x38
    char m_pad3c[0x40 - 0x3c];   // +0x3c..0x3f
    CPoolItemA* m_40;            // +0x40  held item handle (0 = none)
    char m_pad44[0x48 - 0x44];   // +0x44..0x47
    i32 m_48;                    // +0x48
    char m_pad4c[0x2060 - 0x4c]; // +0x4c..0x205f
    i32 m_2060;                  // +0x2060 count
    i32 m_2064;                  // +0x2064
    i32 m_2068;                  // +0x2068
    void DropItem();             // 0x1816a0
    void AddItem();              // 0x181660
};
// 0x00181660 (64B) - if active (m_2060>0 && m_48), allocate+register an item via
// the owned collection and Blt it onto m_38; store the new handle into m_40.
RVA(0x00181660, 0x40)
void Worker181x_181x::AddItem() {
    if (m_2060 > 0 && m_48 != 0) {
        CPoolItemA* h = m_2c->MakeAndAddB(m_2064, m_2068, 0, 0, -1);
        m_40 = h;
        ((CDDSurface*)h)->Blt(m_38);
    }
}

RVA(0x001816a0, 0x1c)
void Worker181x_181x::DropItem() {
    if (m_40) {
        m_2c->RemoveItemA(m_40);
        m_40 = 0;
    }
}

// ===========================================================================
// 0x00193340 (97B) - recursive tree walk. For each node call cb(node->m_c,
// node->m_10, ctx); recurse on the left child then iterate to the right child
// while the child's key (m_8) exceeds the node's. __thiscall(cb, ctx, node).
// ===========================================================================
typedef void(__cdecl* WalkCb_193340)(i32, i32, i32);
struct TNode_193340 {
    TNode_193340* m_0; // +0x00 left
    TNode_193340* m_4; // +0x04 right
    i32 m_8;           // +0x08 key
    i32 m_c;           // +0x0c
    i32 m_10;          // +0x10
};
struct Tree_193340 {
    char m_pad0[0x18];
    TNode_193340* m_18; // +0x18 root
    void Walk(WalkCb_193340 cb, i32 ctx, TNode_193340* node);
};
RVA(0x00193340, 0x61)
void Tree_193340::Walk(WalkCb_193340 cb, i32 ctx, TNode_193340* node) {
    while (1) {
        if (node == 0) {
            node = m_18;
            if (node == 0) {
                return;
            }
        }
        cb(node->m_c, node->m_10, ctx);
        TNode_193340* l = node->m_0;
        if (l != 0 && l->m_8 > node->m_8) {
            Walk(cb, ctx, l);
        }
        TNode_193340* r = node->m_4;
        if (r == 0 || r->m_8 <= node->m_8) {
            return;
        }
        node = r;
    }
}

// ===========================================================================
// 0x001b9b8d (6B) - getter that returns the address of a global descriptor
// (PTR_DAT_006156f4). `mov eax, OFFSET g; ret`.
// ===========================================================================
RVA(0x001b9b8d, 0x6)
void** Get_1b9b8d() {
    return &g_desc_6156f4;
}

SIZE_UNKNOWN(CView_faec0);
SIZE_UNKNOWN(Desc_16f6e0);
SIZE_UNKNOWN(DlgData_be820);
SIZE_UNKNOWN(Dst_16f6e0);
SIZE_UNKNOWN(EntryOwner_c0460);
SIZE_UNKNOWN(Entry_c0460);
SIZE_UNKNOWN(Holder_f9840);
SIZE_UNKNOWN(Host_c2a80);
SIZE_UNKNOWN(Init8_1104f0);
SIZE_UNKNOWN(Mid_faec0);
SIZE_UNKNOWN(Obj_11e8dc);
SIZE_UNKNOWN(OptOwner_c4b30);
SIZE_UNKNOWN(OptionsSlotHost_c4b30);
SIZE_UNKNOWN(PresentHost_faec0);
SIZE_UNKNOWN(Src_16f6e0);
SIZE_UNKNOWN(SubView_faec0);
SIZE_UNKNOWN(SurfCtl_faec0);
SIZE_UNKNOWN(TNode_193340);
SIZE_UNKNOWN(Tree_193340);
SIZE_UNKNOWN(Worker181x_181x);
