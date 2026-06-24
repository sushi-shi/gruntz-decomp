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

// The global bute store the icon Setup queries (g_buteTree.Find). Owned by
// another TU; declared extern so `ecx=&g_buteTree; call Find` reloc-masks.
extern CButeTree g_buteTree;

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
