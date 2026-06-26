// CInGameIcon.cpp - the in-game HUD/cursor icon (a CUserLogic-derived game
// object). Five __thiscall methods reconstructed in ascending-RVA order:
//   0x011d00  ~CInGameIcon   (the bare CUserLogic teardown, /GX frame)
//   0x097680  HandleInput    (the cursor/key input handler -> game-state fields)
//   0x0986b0  PlaceAt        (place/click the icon into a tile cell)
//   0x098c90  Serialize      (CArchive round-trip of the icon state)
//   0x099b10  SetField54     (CMap-lookup the +0x54 id)
//
// CUserLogic / CUserBase / EngStr / CGameObject come from <Gruntz/UserLogic.h>;
// the game-manager singleton (g_gameReg) + the icon factory/records from the
// class header. Engine callees are reloc-masked (no body).
#include <Gruntz/CInGameIcon.h>

#include <rva.h>

#include <Bute/ButeMgr.h> // CButeTree (the bute store Setup queries)
#include <Wap32/ZVec.h>   // zDArray (the command-dispatch tables)

// The global bute store the icon Setup queries (g_buteTree.Find). Owned by
// another TU; declared extern so `ecx=&g_buteTree; call Find` reloc-masks.
extern CButeTree g_buteTree;

// ===========================================================================
// The two file-scope command-dispatch tables (zDArray<member-fn-ptr>) the icon
// registration thunks construct + populate. The ctor (zDArray::Construct, the
// 0x408710 method: stride-4 base init + the 0x5e70fc vptr stamp) reloc-masks.
// Their static-init thunks below build each table over the index band [0x7d0,
// 0x7da].
// ===========================================================================
struct LogicFnTable : public zDArray {
    LogicFnTable* Construct(i32 lo, i32 hi); // 0x408710 (zDArray<T> ctor, returns this)
};

DATA(0x002458b0)
extern LogicFnTable g_iconActionTable; // 0x6458b0
DATA(0x00245928)
extern LogicFnTable g_iconStateTable; // 0x645928

// --- the shared registration infrastructure (mirror of CInGameText's) --------
// The zvec error globals the inlined accessors touch on a bounds miss.
DATA(0x001f0464)
extern u32 g_zvecErrSentinel; // 0x6bf464
DATA(0x001f0428)
extern void* g_zvecErrToken;     // 0x6bf428
extern void* zErr_CaptureRetB(); // 0x16d990

DATA(0x0021aea8)
extern i32 g_iconRegCounter; // 0x61aea8  (running registration index)

// The scratch name-vec (zDArray<CString> @ 0x6bf650): the registration path
// IndexToPtr's it (growing + CString-constructing fresh slots) to stash the key.
struct NameVec : public zDArray {};
DATA(0x002bf650)
extern NameVec g_buteNameVec; // 0x6bf650

// The two registration key strings (.data constants).
DATA(0x0020a454)
extern const char s_iconKeyA[]; // 0x60a454
DATA(0x0020d1bc)
extern const char s_iconKeyB[]; // 0x60d1bc

// The handler member functions loaded into the dispatch slots (FUN_004023d3 /
// 0x403c06 into the action table; 0x40370b into the state table). Referenced by
// address so the stored DIR32 operand reloc-masks.
extern i32 IconAction_4023d3();
extern i32 IconAction_403c06();
extern i32 IconState_40370b();

// The zDArray<CString> accessor inlined WITH the per-slot CString-ctor fixup over
// the freshly-grown region (the zDArray::IndexToPtr body).
static inline i32 ResolveNameSlot(NameVec* v, i32 idx) {
    i32 r;
    v->m_grown = 0;
    if (idx >= v->m_lo && idx <= v->m_hi) {
        r = v->m_base + (idx - v->m_lo) * v->m_stride;
    } else if (v->GrowTo(idx, 0)) {
        r = v->m_base + (idx - v->m_lo) * v->m_stride;
    } else {
        i32 sentinel = g_zvecErrSentinel;
        g_zvecErrToken = zErr_CaptureRetB();
        v->m_err->Error(v, sentinel, 0xc);
        r = v->m_spare;
    }
    CString* slot = (CString*)v->m_alloc;
    i32 n = v->m_grown;
    while (n-- != 0) {
        if (slot) {
            slot->CString::CString();
        }
        slot++;
    }
    return r;
}

// The plain _zvec accessor inlined (no fixup) - the dispatch-table slot resolver.
static inline i32 ResolveSlot(_zvec* v, i32 idx) {
    i32 lo = v->m_lo;
    v->m_grown = 0;
    if (idx >= lo && idx <= v->m_hi) {
        return v->m_base + (idx - lo) * v->m_stride;
    }
    if (v->GrowTo(idx, 0)) {
        return v->m_base + (idx - v->m_lo) * v->m_stride;
    }
    i32 sentinel = g_zvecErrSentinel;
    g_zvecErrToken = zErr_CaptureRetB();
    v->m_err->Error(v, sentinel, 0xc);
    return v->m_spare;
}

// ===========================================================================
// CInGameIcon::~CInGameIcon  (0x011d00)
// ===========================================================================
// The leaf adds no destructible members, so its dtor folds the bare CUserLogic
// teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link
// (the embedded ~EngStr call), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. The empty body is enough.
RVA(0x00011d00, 0x44)
CInGameIcon::~CInGameIcon() {}

// ===========================================================================
// CInGameIcon::HandleInput  (0x097680)
// ===========================================================================
// Reads the owning CGameObject's input/command context (m_10): its command id
// (+0x124), a (key,sub) pair (+0x114/+0x118) and a sub-command (+0x130). For the
// 0x55 ("cursor-place") command in the 0x17..0x20 key band, it resolves a icon
// id from the per-player icon table (g_gameReg+0x158, 71*8 stride) and fires the
// factory probe; otherwise, for the 0x1e/0x13 commands it maps the sub-command
// through a small jump table to a fixed icon id and fires. On a hit it stamps the
// command id back into m_10 (+0x58/+0x50/+0x4c) and returns 1; the no-match
// paths return 0.
//
// @early-stop
// CODE BYTE-EXACT - residual is the jumptable-data-overlap scoring artifact
// (docs/patterns/jumptable-data-overlap.md): the dense 0x1e/0x13 switch lowers to
// a .rdata jump table that cl emits as local $L labels while the delinked target
// carries self-relocs, so objdiff under-counts the table region (~9%). The
// dispatch, the index table and every case body are byte-identical to retail
// (verified by raw byte-compare). Effectively matched; deferred only for the
// jump-table reloc-typing fix.
RVA(0x00097680, 0xf5)
i32 CInGameIcon::HandleInput() {
    CGameObject* obj = m_10;
    i32 cmd = *(i32*)((char*)obj + 0x124);
    i32 rec;
    if (cmd == 0x55) {
        i32 key = *(i32*)((char*)obj + 0x114);
        i32 sub = *(i32*)((char*)obj + 0x118);
        if (sub < 0x17 || sub > 0x20) {
            return 0;
        }
        i32 slot = key * 71;
        i32 icon = ((i32*)((char*)g_gameReg + 0x158))[slot * 2];
        if (icon < 0 || icon >= 0x11) {
            icon = 0;
        }
        rec = g_gameReg->m_74->GetByIndex(icon, 0);
        if (rec == 0) {
            rec = g_gameReg->m_74->GetByIndex(1, 0);
        }
    } else if (cmd == 0x1e || cmd == 0x13) {
        i32 icon;
        switch (*(i32*)((char*)obj + 0x130)) {
            case 1:
                icon = 0x10;
                break;
            case 2:
                icon = 1;
                break;
            case 3:
                icon = 0;
                break;
            case 4:
                icon = 0xc;
                break;
            case 5:
                icon = 2;
                break;
            case 6:
                icon = 3;
                break;
            default:
                icon = 7;
                break;
        }
        rec = g_gameReg->m_74->GetByIndex(icon, 0);
        if (rec == 0) {
            rec = g_gameReg->m_74->GetByIndex(1, 0);
        }
    } else {
        return 1;
    }
    CGameObject* o = m_10;
    *(i32*)((char*)o + 0x58) = 1;
    *(i32*)((char*)o + 0x50) = 0xa;
    *(i32*)((char*)o + 0x4c) = rec;
    return 1;
}

// ===========================================================================
// InitIconActionTable  (0x097800)
// ===========================================================================
// File-scope static-init thunk: construct the action dispatch table over the
// index band [0x7d0, 0x7da].
RVA(0x00097800, 0x15)
void InitIconActionTable() {
    g_iconActionTable.Construct(0x7d0, 0x7da);
}

// ===========================================================================
// RegisterIconActions  (0x0979e0)
// ===========================================================================
// Register two icon-action handlers into g_iconActionTable: key A -> 0x4023d3,
// key B -> 0x403c06. Each: bute-tree Find (Insert + cache the name into the
// scratch zDArray<CString> when absent, bump the counter), resolve the table
// slot for the key index, load the handler member-fn-ptr.
// ---------------------------------------------------------------------------
// @early-stop
// inlined zDArray/zvec IndexToPtr regalloc wall (the documented ZVec family, see
// ZVec.cpp + RegisterTextLogic): both register blocks + the CString-ctor fixup
// loops are reconstructed faithfully, but cl pins the index/this/base across the
// grow branches differently than retail. Logic + find/insert + the fn-ptr stores
// correct; the register assignment is not source-steerable.
RVA(0x000979e0, 0x2ac)
void RegisterIconActions() {
    i32 idxA = (i32)g_buteTree.Find(s_iconKeyA);
    if (idxA == 0) {
        g_buteTree.Insert(s_iconKeyA, (void*)g_iconRegCounter);
        i32 slot = ResolveNameSlot(&g_buteNameVec, g_iconRegCounter);
        *(CString*)slot = s_iconKeyA;
        g_iconRegCounter++;
    }
    i32 dslotA = ResolveSlot(&g_iconActionTable, idxA);
    *(void**)dslotA = (void*)&IconAction_4023d3;

    i32 idxB = (i32)g_buteTree.Find(s_iconKeyB);
    if (idxB == 0) {
        g_buteTree.Insert(s_iconKeyB, (void*)g_iconRegCounter);
        i32 slot = ResolveNameSlot(&g_buteNameVec, g_iconRegCounter);
        *(CString*)slot = s_iconKeyB;
        g_iconRegCounter++;
    }
    i32 dslotB = ResolveSlot(&g_iconActionTable, idxB);
    *(void**)dslotB = (void*)&IconAction_403c06;
}

// ===========================================================================
// InitIconStateTable  (0x097d60)
// ===========================================================================
// File-scope static-init thunk: construct the state dispatch table over the
// index band [0x7d0, 0x7da].
RVA(0x00097d60, 0x15)
void InitIconStateTable() {
    g_iconStateTable.Construct(0x7d0, 0x7da);
}

// ===========================================================================
// RegisterIconState  (0x097f40)
// ===========================================================================
// Register one icon-state handler into g_iconStateTable (key A -> 0x40370b):
// bute-tree Find (Insert + cache the name when absent, bump the counter),
// resolve the table slot for the key index, load the handler member-fn-ptr.
// ---------------------------------------------------------------------------
// @early-stop
// inlined zDArray/zvec IndexToPtr regalloc wall (the documented ZVec family, see
// ZVec.cpp + RegisterTextLogic ~96%): faithfully reconstructed; cl's index/this
// register assignment across the grow branches differs from retail and is not
// source-steerable. Logic + find/insert + the fn-ptr store correct.
RVA(0x00097f40, 0x18d)
void RegisterIconState() {
    i32 idx = (i32)g_buteTree.Find(s_iconKeyA);
    if (idx == 0) {
        g_buteTree.Insert(s_iconKeyA, (void*)g_iconRegCounter);
        i32 slot = ResolveNameSlot(&g_buteNameVec, g_iconRegCounter);
        *(CString*)slot = s_iconKeyA;
        g_iconRegCounter++;
    }
    i32 dslot = ResolveSlot(&g_iconStateTable, idx);
    *(void**)dslot = (void*)&IconState_40370b;
}

// ===========================================================================
// CInGameIcon::RefreshCell  (0x098340)
// ===========================================================================
// If the icon's tracked position has drifted at least g_iconDefault past the
// owning object's stored position (a 64-bit signed compare of {m_5c:m_58}
// against {m_64:m_60}), OR the owning object's tile cell is empty/off-grid,
// flag the +0x38 render object dirty (|= 0x10000). Returns 0.
RVA(0x00098340, 0x71)
i32 CInGameIcon::RefreshCell() {
    CGameObject* obj = m_10;
    i32 tileY = obj->m_5c >> 5;
    i32 tileX = (obj->m_60 + 0x18) >> 5;
    i64 delta = (i64)(u32)g_iconDefault - *(i64*)&m_58;
    if (delta < *(i64*)&m_60) {
        CIconTileGrid* grid = g_gameReg->m_70;
        i32 cell;
        if ((u32)tileY < (u32)grid->m_c && (u32)tileX < (u32)grid->m_10) {
            i32* row = grid->m_8[tileX];
            cell = row[tileY * 8 - tileY + 2];
        } else {
            cell = 0;
        }
        if (cell != 0) {
            return 0;
        }
    }
    CGameObject* r = *(CGameObject**)((char*)this + 0x38);
    *(i32*)((char*)r + 0x8) |= 0x10000;
    return 0;
}

// Clear the "occupied" bit (0x40000) in the tile cell the owning object stands
// on. The grid is g_gameReg->m_70: m_8[tileY] is a row base (each row a flat
// array of 0x1c-byte cells = 7 dwords), the cell for tileX sits at offset
// (tileX*71)*4 ... matching retail's `eax=tileX*8-tileX` then `<<2` and the
// `[m_8[tileY] + eax + 8]=0` / `[m_8[tileY] + eax] &= ~0x40000` pair.
static inline void ClearTileBit(CGameReg* reg, CGameObject* owner) {
    CIconTileGrid* grid = reg->m_70;
    i32 tileX = owner->m_60 >> 5;
    i32 tileY = owner->m_5c >> 5;
    if ((u32)tileY < (u32)grid->m_c && (u32)tileX < (u32)grid->m_10) {
        i32 rowByte = tileX * 4;
        i32 cellOff = (tileY * 8 - tileY) * 4;
        char* cell0 = (char*)*(i32**)((char*)grid->m_8 + rowByte);
        *(i32*)(cell0 + cellOff + 8) = 0;
        char* cell1 = (char*)*(i32**)((char*)grid->m_8 + rowByte);
        *(i32*)(cell1 + cellOff) &= ~0x40000;
    }
}

// ===========================================================================
// CInGameIcon::PlaceAt  (0x0986b0)
// ===========================================================================
// The cursor place/click handler. Resolves the per-player icon record from
// g_gameReg->m_68 [(arg0*15 + arg1) dword index, +0x1c base], binds its sprite
// set (LoadPickupSprites / LoadGruntTypeTable), optionally posts an input flush
// when the icon is on-screen, clears the owner's tile-occupancy bit, and (the
// full non-0x55 path) re-seeds the icon's animation/state fields from the bute
// store. Returns 1 on a successful place, 0 on a reject.
//
// @early-stop
// regalloc/scheduling wall (zero-register-pinning class): 780-byte dense body
// with 5 pinned register vars (ebx=1, ebp, edi, esi=this, edx=g_gameReg). The
// logic, the cell index, both call arg-sets, the on-screen bounds gates, the
// shared tile-clear and the bute re-seed tail are all reconstructed; MSVC's exact
// ebx/ebp/edi allocation across the two halves is not source-steerable. Deferred.
RVA(0x000986b0, 0x30c)
i32 CInGameIcon::PlaceAt(i32 arg0, i32 arg1) {
    CGameReg* reg = g_gameReg;
    if (reg->m_134 == 1 && arg0 != g_curPlayer && *(i32*)((char*)m_10 + 0x124) != 0x55) {
        return 0;
    }
    CGameObject* obj = m_10;
    if (*(i32*)((char*)obj + 0x124) == 0x55) {
        // ---- selection/preview path ----
        i32 param = *(i32*)((char*)obj + 0x118);
        i32 matchActive = 0;
        i32 flag = 1;
        if (*(i32*)((char*)obj + 0x114) == arg0) {
            matchActive = 1;
            flag = 0;
        }
        i32 sub = *(i32*)((char*)obj + 0x130);
        i32 idx = arg0 * 15 + arg1;
        CIconRecord* cell = ((CIconRecord**)((char*)reg->m_68 + 0x1c))[idx];
        i32 ok;
        if (cell == 0 || cell->m_1fc == 0) {
            ok = 0;
        } else if (matchActive) {
            ok = cell->LoadPickupSprites(param, flag, 0, sub, 0);
        } else {
            ok = cell->LoadGruntTypeTable(param, flag, sub, 0);
        }
        reg = g_gameReg;
        if (ok == 0) {
            return 0;
        }
        if (m_54 != 0) {
            CGameObject* o = m_10;
            if (o->m_5c < reg->m_144 && o->m_5c >= reg->m_13c && o->m_60 < reg->m_148
                && o->m_60 >= reg->m_140) {
                Eng_PostCmd(g_inputCtx, 0, 0, 0);
                reg = g_gameReg;
            }
        }
        ClearTileBit(reg, m_10);
        CGameObject* r = *(CGameObject**)((char*)this + 0x38);
        *(i32*)((char*)r + 0x8) |= 0x10000;
        return 1;
    }

    // ---- full place path (cmd != 0x55) ----
    i32 sub = *(i32*)((char*)obj + 0x130);
    i32 cmd = *(i32*)((char*)obj + 0x124);
    i32 idx = arg0 * 15 + arg1;
    CIconRecord* cell = ((CIconRecord**)((char*)reg->m_68 + 0x1c))[idx];
    i32 ok;
    if (cell == 0 || cell->m_1fc == 0) {
        ok = 0;
    } else {
        ok = cell->LoadPickupSprites(cmd, 0, 0, sub, 1);
    }
    reg = g_gameReg;
    if (ok == 0) {
        return 0;
    }
    if (cmd == 0x14) {
        CIconRecord* placed = ((CIconRecord**)((char*)reg->m_68 + 0x1c))[idx];
        if (placed != 0) {
            placed->m_38c = *(i32*)((char*)m_10 + 0x128);
            reg = g_gameReg;
        }
    }
    if (m_54 != 0) {
        CGameObject* o = m_10;
        if (o->m_5c < reg->m_144 && o->m_5c >= reg->m_13c && o->m_60 < reg->m_148
            && o->m_60 >= reg->m_140) {
            Eng_PostCmd(g_inputCtx, 0, 0, 0);
            reg = g_gameReg;
        }
    }
    ClearTileBit(reg, m_10);
    CGameObject* owner = *(CGameObject**)((char*)this + 0x38);
    if (*(i32*)((char*)owner + 0x120) > 0) {
        *(i32*)((char*)owner + 0x40) |= 1;
        void* aux = *(void**)((char*)this + 0x14);
        m_30 = *(void**)((char*)aux + 0x1c);
        *(void**)((char*)aux + 0x1c) = g_buteTree.Find(g_iconBute);
        owner = *(CGameObject**)((char*)this + 0x38);
        m_58 = *(i32*)((char*)owner + 0x120);
        m_5c = 0;
        m_60 = g_iconDefault;
        m_64 = 0;
        return 1;
    }
    CGameObject* rend = *(CGameObject**)((char*)this + 0x78);
    if (rend != 0) {
        *(i32*)((char*)rend + 0x8) |= 0x10000;
        *(CGameObject**)((char*)this + 0x78) = 0;
    }
    CGameObject* r = *(CGameObject**)((char*)this + 0x38);
    *(i32*)((char*)r + 0x8) |= 0x10000;
    return 1;
}

// ===========================================================================
// CInGameIcon::Serialize  (0x098c90)
// ===========================================================================
// The CArchive load/store of the icon state: guard on the archive, chain the
// shared CUserLogic::SerializeChain, then a `sub 4 / sub 3 / dec` running tag
// switch dispatching per-mode CArchive Read/Write (vtable +0x2c write / +0x30
// read) of the +0x34..+0x78 fields, with inline strlen/strcpy CString round-trips
// (repne scas / rep movs), a g_serialCounter bump, and two registry CMap lookups
// (0x1b8438 / 0x1b8760) re-binding the +0x40/+0x54 ids.
//
// @early-stop
// DEFERRED to the final sweep (NOT reconstructed). Two blockers: (1) the recorded
// boundary is wrong - dump_target/Ghidra report 0x31f (799 B) but the body's own
// branches target 0x99000, ~0x370 B past the entry, so the delinked target object
// is truncated and cannot match until the function length is re-derived; (2) the
// body is a ~880 B dense CArchive/CString/CMap marshaler whose stack-CString temps
// force a /GX EH-state schedule (the eh-state-numbering + outparam-zeroinit walls).
// A faithful reconstruction needs the corrected boundary first; pinned no-body so
// its RVA registers and the unit builds. See report.
RVA(0x00098c90, 0x31f)
i32 CInGameIcon::Serialize(CArchive*, i32, i32, i32) {
    return 0;
}

// ===========================================================================
// CInGameIcon::SetField54  (0x099b10)
// ===========================================================================
// When v != 0, look it up in the registry's CMap (g_gameReg->m_30->m_28, Lookup
// at +0x10) into a local, then store the located value (or 0) into +0x54.
RVA(0x00099b10, 0x36)
void CInGameIcon::SetField54(i32 v) {
    void* found = 0;
    if (v != 0) {
        found = 0;
        g_gameReg->m_30->m_28->m_10map.Lookup((void*)v, &found);
    }
    m_54 = (i32)found;
}
