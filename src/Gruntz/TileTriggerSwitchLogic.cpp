#include <rva.h>
// TileTriggerSwitchLogic.cpp - Gruntz CTileTriggerSwitchLogic (C:\Proj\Gruntz).
// CTileTriggerSwitchLogic is a tile-trigger "switch" class (vtable 0x5eae8c,
// stamped by the ctor) that owns a CObList of sibling CTileTriggerSwitchLogic
// children (head at +0x04; nodes next@+0x00, data@+0x08).  The ctor zeroes the
// 24-dword m_block at +0x2c and clears m_20; the Find/Remove accessors walk the
// child list; ValidateByType/ApplyByType dispatch on a passed type tag; the
// Transfer/Serialize methods stream the object's fields through a CSerialArchive.
//
// NOTE on the trace cluster: the dynamic this-tracer lumped ~28 RVAs under this
// class.  Only the subset that matches this layout (vtable 0x5eae8c, +0x04 child
// list, +0x2c block, stream-serialize) is reconstructed here.  The remaining
// ~20 (the 3-CObList container at +0x1c/+0x38/+0x54 - dtor 0xc8640, 116e60,
// 116f20, 1170b0, 117150, 117200, ...; and the tile-grid command class with the
// type tag at +0x04 and coords at +0x08/+0x0c - 112590, 112970, 112b70, ...)
// are a DIFFERENT shape and stay stubbed in src/Stub/Discovered.cpp.
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + the emitted
// code bytes are load-bearing (campaign doctrine).
#include <Gruntz/TileTriggerSwitchLogic.h>

#include <Mfc.h>                  // PtInRect (via <windows.h>) for BuildRockBreakInGameText
#include <Gruntz/SpriteFactory.h> // the ONE CSpriteFactory (CreateSprite @0x1597b0)
#include <Gruntz/UserLogic.h>     // CGameObject (the created Particlez/InGameText sprites)
#include <Gruntz/Viewport.h>      // CViewport (the level plane: cells + row-base table)
#include <Gruntz/SoundCue.h>      // the ONE sound-cue registry (CSndHost/CSndFinder/
                                  // CSndEmitter/CSoundCueMgr) - folds the former Rb* views

// CTileTriggerSwitchLogic is now a REAL polymorphic class (4 virtuals in the
// header): cl emits the ??_7 vftable + the implicit ctor vptr-stamp - the manual
// `*(void**)this = &g_...Vtbl` struct stamp is gone. The target-side vtable name
// ??_7CTileTriggerSwitchLogic@@6B@ is derived AUTOMATICALLY by the build: the EXE
// has no debug symbols, so labels.py applies config/vtable_names.csv (generated
// from RTTI by gruntz.analysis.vtable_scan) whenever the base obj emits the
// ??_7. No source annotation is needed - naming is a byproduct of the inventory.

// The Rez heap free (RVA 0x1b9b82, _RezFree); reloc-masked rel32 callee used by
// the inlined child-delete in RemoveByKeys.
extern "C" void RezFree(void* p);

// Per-type validators used by ValidateByType (reloc-masked rel32 callees; both
// callee-cleanup taking the object pointer).
i32 __stdcall TileSwitchCheckType4(void* obj);
i32 __stdcall TileSwitchCheckType7(void* obj);

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::CTileTriggerSwitchLogic()
// Constructor: stamps the vtable, zeroes the 24-dword m_block at +0x2c
// (rep stosd), then clears m_20 (+0x20).
// ---------------------------------------------------------------------------
RVA(0x00110430, 0x1c)
CTileTriggerSwitchLogic::CTileTriggerSwitchLogic() {
    // vptr stamp is now IMPLICIT (real polymorphic class) - cl prepends
    // `mov [this], offset ??_7CTileTriggerSwitchLogic@@6B@`, exactly the retail
    // ctor's first instruction, replacing the manual struct stamp.
    for (i32 i = 0; i < 24; i++) {
        m_block[i] = 0;
    }
    m_20 = 0;
}

// The 4 virtuals are DECLARED-ONLY: their real bodies live in unmatched engine
// TUs, so we don't define them here. cl still emits the ??_7 vftable (the ctor
// references it) with the 4 slots as external refs - enough for the ctor's
// implicit vptr-stamp to be byte-exact. The vtable's slot CONTENTS are verified
// only once each virtual is reconstructed (then named via @rva-symbol).

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::FindIndexByKey
// Linear scan of the 24-dword m_block; returns 1 on a hit, 0 otherwise.
// ---------------------------------------------------------------------------
RVA(0x00110820, 0x23)
i32 CTileTriggerSwitchLogic::FindIndexByKey(i32 key) {
    // Scans the 24-dword array that begins at +0x3c (== &m_block[4]).
    for (i32 i = 0; i < 24; i++) {
        if (m_block[i + 4] == key) {
            return 1;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::VerifyBlockLinksB
// The FindChild(key, 3) variant of VerifyBlockLinks (byte-identical structure,
// different diagnostic codes 0x44d/0x44e and the slow-lookup kind arg 3 vs 8):
// gate on m_linkGate, find the owner's child that claims this object (FindIndexByKey),
// ack 0x44d on miss; then validate each nonzero block key resolves (FindChild
// (key, 3)) to a gated child, acking 0x44e on a lookup miss.
// ---------------------------------------------------------------------------
// @early-stop
// this-spill frame wall (~86%, same as VerifyBlockLinks 0x112c70): body
// byte-identical; retail reserves a `push ecx` stack local for `this` + reloads it
// to seed the `child` loop cursor, the recompile seeds it from a register. Dead seed
// value, non-steerable frame choice. See docs/patterns/this-spilled-to-local-for-loop-seed.md
RVA(0x00111f40, 0xc4)
i32 CTileTriggerSwitchLogic::VerifyBlockLinksB() {
    if (m_linkGate == 0) {
        return 0;
    }
    ListNode* node = m_owner->m_20;
    i32 found = 0;
    CTileTriggerSwitchLogic* child = this;
    while (node != 0) {
        if (found != 0) {
            break;
        }
        ListNode* cur = node;
        node = node->m_next;
        child = cur->m_data;
        if (child != 0 && child->FindIndexByKey(m_key1) != 0) {
            found = 1;
        }
    }
    if (found == 0) {
        g_gameReg->Ack(0x80de, 0x44d);
        return 0;
    }
    i32* p = &child->m_block[4]; // child+0x3c
    for (i32 i = 0; i < 24; i++) {
        i32 key = *p;
        if (key == 0) {
            return 1;
        }
        CTileTriggerSwitchLogic* c = m_owner->FindChild(key, 3);
        if (c == 0) {
            g_gameReg->Ack(0x80dd, 0x44e);
            return 0;
        }
        if (c->m_linkGate == 0) {
            return 0;
        }
        p++;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::VerifyBlockLinks
// Linkage validator: if this->m_linkGate is clear, succeed (return 0 short-circuit on
// the null gate).  Otherwise walk the owner's child list (head @ owner->m_20),
// asking each child's FindIndexByKey(this->m_key1) until one claims this object.
// If none does, ack diagnostic 0x452 and fail.  Then, for the claiming child,
// scan its 24-dword key block (child->m_block[4..27]): an empty slot succeeds;
// each nonzero key must resolve via owner->FindChild(key, 8) to a child whose
// m_linkGate is set (else fail, acking 0x453 when the lookup itself misses).
// Returns 1 on the early empty-slot success, 0 otherwise.
// ---------------------------------------------------------------------------
// @early-stop
// this-spill frame wall (~86%): body byte-identical; retail reserves a `push ecx`
// stack local for `this` + reloads it (`mov edi,[esp+0x10]`) to seed the `child`
// loop cursor, the recompile seeds it from a register (`mov edi,ebp`) with no slot.
// Dead seed value, non-steerable frame choice. See
// docs/patterns/this-spilled-to-local-for-loop-seed.md
RVA(0x00112c70, 0xc4)
i32 CTileTriggerSwitchLogic::VerifyBlockLinks() {
    if (m_linkGate == 0) {
        return 0;
    }
    ListNode* node = m_owner->m_20;
    i32 found = 0;
    CTileTriggerSwitchLogic* child = this;
    while (node != 0) {
        if (found != 0) {
            break;
        }
        ListNode* cur = node;
        node = node->m_next;
        child = cur->m_data;
        if (child != 0 && child->FindIndexByKey(m_key1) != 0) {
            found = 1;
        }
    }
    if (found == 0) {
        g_gameReg->Ack(0x80de, 0x452);
        return 0;
    }
    i32* p = &child->m_block[4]; // child+0x3c
    for (i32 i = 0; i < 24; i++) {
        i32 key = *p;
        if (key == 0) {
            return 1;
        }
        CTileTriggerSwitchLogic* c = m_owner->FindChild(key, 8);
        if (c == 0) {
            g_gameReg->Ack(0x80dd, 0x453);
            return 0;
        }
        if (c->m_linkGate == 0) {
            return 0;
        }
        p++;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::LoadState - 0x1139a0. Network deserialize: gate on a
// non-null stream + the registry active-game flag, then read the eight scalar
// fields (skipping m_owner at +0x24) and the 24-dword m_block via the stream's
// read slot (+0x2c). Returns 1 (or 0 when gated off).
// ---------------------------------------------------------------------------
RVA(0x001139a0, 0xb4)
i32 CTileTriggerSwitchLogic::LoadState(CSerialArchive* s) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    s->Read(&m_08, 4);
    s->Read(&m_key0c, 4);
    s->Read(&m_key1, 4);
    s->Read(&m_linkGate, 4);
    s->Read(&m_18, 4);
    s->Read(&m_1c, 4);
    s->Read(&m_20, 4);
    s->Read(&m_28, 4);
    i32* p = m_block;
    for (i32 i = 0; i < 24; i++) {
        s->Read(p, 4);
        p++;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::ValidateByType
// Returns 0 if obj is null; for type 4 / 7 defers to the matching validator
// (returns 0 on its failure); any other type passes (returns 1).
// ---------------------------------------------------------------------------
// @early-stop
// regalloc wall (~93%): switch-case ordering + validator calls match; retail
// keeps arg1 in eax (returns it as the null-zero, push eax for validators) vs
// our ecx + explicit xor in the null block. Entry-block register only.
RVA(0x00113a90, 0x3b)
i32 CTileTriggerSwitchLogic::ValidateByType(void* obj, i32 type, i32 a3, i32 a4) {
    if (obj == 0) {
        return 0;
    }
    switch (type) {
        case 4:
            if (TileSwitchCheckType4(obj) == 0) {
                return 0;
            }
            break;
        case 7:
            if (TileSwitchCheckType7(obj) == 0) {
                return 0;
            }
            break;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::ApplyByType
// Returns 0 if obj is null or the base apply fails; for type 4 / 7 defers to the
// matching per-type apply (returns 0 on its failure); otherwise returns 1.
// ---------------------------------------------------------------------------
RVA(0x00113d40, 0x6f)
i32 CTileTriggerSwitchLogic::ApplyByType(void* obj, i32 type, i32 a3, i32 a4) {
    if (obj == 0) {
        return 0;
    }
    if (ApplyBase(obj, type, a3, a4) == 0) {
        return 0;
    }
    switch (type) {
        case 4:
            if (ApplyType4(obj) == 0) {
                return 0;
            }
            break;
        case 7:
            if (ApplyType7(obj) == 0) {
                return 0;
            }
            break;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::SerializeMatrix
// Streams two header dwords (+0xc0, +0xc4) then a 3x3 dword matrix (+0x9c..) via
// the stream's Transfer.  Returns 0 if the stream or the active game-manager
// (g_gameReg+0x30) is null, else 1.
// ---------------------------------------------------------------------------
// @early-stop
// regalloc wall (~95%): whole body byte-identical; retail pins this->esi /
// stream->edi (pushes all 4 callee regs, then loads args) vs our this->edi /
// stream->esi (arg load interleaved with the pushes). Reg-pair swap only.
RVA(0x00113dd0, 0x7b)
i32 CTileTriggerSwitchLogic::SerializeMatrix(CSerialArchive* s) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    s->Write(&m_block[37], 4); // +0xc0
    s->Write(&m_block[38], 4); // +0xc4
    i32* p = &m_block[28];     // +0x9c
    for (i32 r = 0; r < 3; r++) {
        for (i32 c = 0; c < 3; c++) {
            s->Write(p, 4);
            p++;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::GetFlag74
// Try-acquire on the +0x74 flag (== m_block[18]): returns 0 if already set,
// otherwise sets it to 1 and returns 1.
// ---------------------------------------------------------------------------
RVA(0x00115f00, 0x13)
i32 CTileTriggerSwitchLogic::GetFlag74() {
    if (m_block[18] != 0) {
        return 0;
    }
    m_block[18] = 1;
    return 1;
}

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::RemoveByKeys
// Walks the child list (head @ +0x04, an MFC CObList: node next@0, prev@4, data@8);
// on the first child whose +0x04 == k2 and +0x10 == k1, deletes the child (inlined
// dtor vptr-restamp + RezFree), unlinks the node via CObList::RemoveAt, returns 1.
// Returns 0 if no match. The loop is the inlined CObList::GetNext idiom: a saved
// position (cur, esi) + the GetNext local (pn, ecx) are two distinct node copies;
// pn advances node then derefs data, so retail materializes the extra `mov ecx,eax`
// copy and defers the data load past the advance.
// ---------------------------------------------------------------------------
RVA(0x00116320, 0x66)
i32 CTileTriggerSwitchLogic::RemoveByKeys(i32 k1, i32 k2) {
    ListNode* node = (ListNode*)m_04;
    while (node) {
        ListNode* cur = node; // savePos (esi)
        ListNode* pn = node;  // GetNext local (ecx)
        node = node->m_next;
        CTileTriggerSwitchLogic* data = pn->m_data;
        if (data->m_04 == k2 && data->m_key1 == k1) {
            if (data) {
                // The inlined `delete data`: the dtor restamps the vptr
                // (`mov [data],offset ??_7`) + clears m_20, then RezFree frees it.
                data->~CTileTriggerSwitchLogic();
                RezFree(data);
            }
            ListRemoveAt(cur);
            return 1;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::FindChild
// Walks the child list (head @ +0x04; node next@+0x00, data@+0x08); returns the
// first child whose +0x10 == k1 and (k2==0 || +0x04 == k2), else NULL.
// ---------------------------------------------------------------------------
RVA(0x00116ee0, 0x2f)
CTileTriggerSwitchLogic* CTileTriggerSwitchLogic::FindChild(i32 k1, i32 k2) {
    ListNode* node = (ListNode*)m_04;
    while (node) {
        ListNode* cur = node;
        node = node->m_next;
        CTileTriggerSwitchLogic* data = cur->m_data;
        if (data->m_key1 == k1) {
            if (k2 == 0 || data->m_04 == k2) {
                return data;
            }
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::FindByField0C
// Walks the list at +0x58; returns the first child whose +0x0c == key, else NULL.
// ---------------------------------------------------------------------------
RVA(0x001171d0, 0x20)
CTileTriggerSwitchLogic* CTileTriggerSwitchLogic::FindByField0C(i32 key) {
    ListNode* node = (ListNode*)m_block[11];
    while (node) {
        ListNode* cur = node;
        node = node->m_next;
        CTileTriggerSwitchLogic* data = cur->m_data;
        if (data->m_key0c == key) {
            return data;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::TransferFlag74
// If the stream is null returns 0; if the active game-manager (g_gameReg+0x30)
// is null returns 0; otherwise transfers the 4-byte +0x74 flag through the
// stream's Transfer (vtable slot 12) and returns 1.
// ---------------------------------------------------------------------------
RVA(0x00117e20, 0x36)
i32 CTileTriggerSwitchLogic::TransferFlag74(CSerialArchive* s) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    s->Write(&m_block[18], 4);
    return 1;
}

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::LoadFlag74
// The read counterpart of TransferFlag74: same null/game-manager gates, but
// reads the 4-byte +0x74 flag (== m_block[18]) through the stream's read slot
// (vtable +0x2c) instead of the write slot (+0x30). Returns 1 on success.
// ---------------------------------------------------------------------------
RVA(0x00117e70, 0x36)
i32 CTileTriggerSwitchLogic::LoadFlag74(CSerialArchive* s) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    s->Read(&m_block[18], 4);
    return 1;
}

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::ScanNeighborhood
// Scans the 3x3 cell neighborhood centered on (x, y): for px in [x-1, x+2) and
// py in [y-1, y+2), probes cell (py + (px << 8)) with kind 0x16; returns the
// first nonzero probe result, else 0.
// ---------------------------------------------------------------------------
// @early-stop
// regalloc wall (~76%): logic exact, inner loop + ProbeCell call/regs match.
// Retail reserves an 8B frame (sub esp,8), keeps the inner bound py_end in a
// callee-saved REGISTER (ebp, hoisted once) and spills px/base to stack, whereas
// MSVC5 keeps px/base in registers and reloads py_end from the frame each inner
// step. The strength-reduced form (base += 0x100 accumulator) reproduces retail's
// outer tail but flips the same 2 callee-saved registers the other way (72%), so
// the plain double-for is the closest. Which of {px,base,py_end} wins the two
// callee-saved regs is a non-steerable regalloc pick. Final-sweep.
RVA(0x00117ec0, 0x7f)
i32 CTileTriggerSwitchLogic::ScanNeighborhood(i32 x, i32 y) {
    for (i32 px = x - 1; px < x + 2; px++) {
        i32 base = px << 8;
        for (i32 py = y - 1; py < y + 2; py++) {
            i32 r = ProbeCell(py + base, 0x16);
            if (r != 0) {
                return r;
            }
        }
    }
    return 0;
}

// Engine-label backlog stubs (moved from src/Stub/CTileTriggerSwitchLogic.cpp).

// @confidence: high
// @source: rtti-vptr
// @stub
RVA(0x00115f60, 0x2de)
void CTileTriggerSwitchLogic::CTileTriggerSwitchLogic_115f60() {}

// ---------------------------------------------------------------------------
// BuildRockBreakInGameText - the rock-break tile-effect loader (RVA 0x1122a0).
//
// MISLABELED: the delinker filed this __thiscall(void) under CTileTriggerSwitchLogic
// (Ghidra RTTI-vptr guess), but xref shows its real callers are CTerrainTileLoader::
// Load (0x75e90) and CRockBreakMgr::BuildRockBreakParticles (0x7b440), and `this` is
// a tile-command object whose +0x8/+0xc are the tile (x, y) and +0x9c a 9-cell value
// block. The RVA is locked to this class name (moving it craters the delinker pack),
// so it is reconstructed here by raw this+offset - only the offsets/bytes are load-
// bearing. It: (1) gates on whether the tile center sits inside the view rect;
// (2) walks the 3x3 neighborhood writing each saved cell value back into the level
// plane + notifying the tile grid (and, when in-rect, spawning a Particlez/
// LEVEL_ROCKBREAK sprite per cell); (3) fires the command-grid effect at the tile
// center; (4) when +0xc4 is set spawns an InGameText sprite carrying it; (5) if the
// tile is on-screen with no active override, plays the LEVEL_ROCKBREAK cue (rate-
// limited by the kill-cue clock).
// ---------------------------------------------------------------------------

// Kill-cue clock + sound flags (named so the DIR32 datum reloc-masks).
extern "C" i32 g_killCueClock; // _g_killCueClock @0x6bf3c0
extern i32 g_sndEnabled;       // ?g_sndEnabled@@3HA @0x61ab20
extern i32 g_sndCueTag;        // ?g_sndCueTag@@3HA  @0x61ab24

// The sound-cue registry (g->m_world->m_28) + its Lookup result (the CSndEmitter cue
// record whose m_14 last-play / m_18 cooldown rate-limit the CSoundCueMgr it plays) are
// the canonical CSndHost/CSndFinder/CSndEmitter/CSoundCueMgr from <Gruntz/SoundCue.h>
// (included above); the former per-TU RbSoundReg/RbLookupTable/RbCueRec/RbCueSound views
// are dissolved onto them (same offsets + RVAs, xref-confirmed: Lookup 0x1b8438,
// ConfigureItem 0x1360d0).

// The world command-grid effect sink (g_gameReg->m_cmdGrid, reg+0x68). This is the ONE
// genuinely-unrecovered +0x68 slot GameRegistry.h documents as a red flag: every TU
// downcasts m_cmdGrid to a DIFFERENT concrete type (CSbIconSet/CTeleIconTable/CTriggerSink/
// CGruntRec**/...), so a single real class is not recovered. Kept as a method-only view
// (identity-recovery TODO), consistent with all other m_cmdGrid consumers.
struct RbCmdGrid {
    void Fire(i32 key, i32 x, i32 y, i32 slot, i32 a, i32 b); // 0x152d
};
SIZE_UNKNOWN(RbCmdGrid);

// `this` stays in esi; tile (x, y) are re-read from +0x8/+0xc at each use (retail
// caches neither, so caching them here would spill the frame from 0x14 to 0x38).
#define TX (*(i32*)(self + 0x8))
#define TY (*(i32*)(self + 0xc))

// @early-stop
// loop-body regalloc wall (~69%): complete + correct reconstruction - the frame
// (0x14, 5 locals), the PtInRect gate, and ALL of steps 3-5 (the m_cmdGrid Fire, the
// InGameText sprite + m_124 stamp, the view-rect bounds cascade, the sound-registry
// Lookup + kill-cue-clock cooldown Play) are byte-exact. The residual is confined to
// the 3x3 loop body: retail spills BOTH counters i (edi) and j (ebx) to the frame and
// reloads them, chaining the plane through a 2nd register so g_gameReg stays live in
// edi for the +0x70 tileGrid read; MSVC5 here keeps i in ebx and re-loads g_gameReg,
// which also duplicates the inner-tail (an extra jmp + a 2nd copy). Source nudges
// (read-order swap, an explicit g_gameReg loop local, px/py caching) all leave the
// i->edi / j->ebx spill-and-reload assignment unmoved - a non-steerable regalloc pick
// inside the hottest block. Deferred to the final sweep.
RVA(0x001122a0, 0x241)
void CTileTriggerSwitchLogic::BuildRockBreakInGameText() {
    char* self = (char*)this;
    CSpriteFactoryHolder* gameMgr = g_gameReg->m_world; // cached only for the loop sprite

    // (1) in-rect gate: is the tile center inside the view rect (+0x13c)?
    i32 inRect = 0;
    POINT pt;
    pt.y = (TY << 5) + 0x10;
    pt.x = (TX << 5) + 0x10;
    if (PtInRect((const RECT*)&g_gameReg->m_viewOriginL, pt)) {
        inRect = 1;
    }

    // (2) 3x3 neighborhood: write each saved cell value into the level plane + notify
    // the tile grid; when in-rect, spawn a Particlez/LEVEL_ROCKBREAK sprite per cell.
    i32* cursor = (i32*)(self + 0x9c);
    for (i32 j = 0; j <= 2; j++) {
        for (i32 i = 0; i <= 2; i++) {
            i32 value = *cursor;
            i32 px = i + TX - 1;
            i32 py = j + TY - 1;
            CViewport* plane = (CViewport*)g_gameReg->m_world->m_24->m_5c;
            plane->m_cells[plane->m_rowBase[py] + px] = value;
            g_gameReg->m_tileGrid->Notify(px, py, value);
            if (inRect) {
                CGameObject* spr = gameMgr->m_8->CreateSprite(
                    0,
                    ((i + TX) << 5) - 0x10,
                    ((j + TY) << 5) - 0x10,
                    0xcf84f,
                    "Particlez",
                    0x40003
                );
                if (spr != 0) {
                    spr->ApplyName("LEVEL_ROCKBREAK");
                    spr->ApplyLookupGeometry("LEVEL_ROCKBREAK", 0);
                }
            }
            cursor++;
        }
    }

    // (3) fire the command-grid effect at the tile center (cx/cy reused by step 4).
    i32 cx = (TX << 5) + 0x10;
    i32 cy = (TY << 5) + 0x10;
    ((RbCmdGrid*)g_gameReg->m_cmdGrid)
        ->Fire(*(i32*)(self + 0xc0), cx, cy, *(i32*)(self + 0x30), 1, 0);

    // (4) when +0xc4 is set, spawn an InGameText sprite carrying it.
    if (*(i32*)(self + 0xc4) != 0) {
        CGameObject* txt =
            g_gameReg->m_world->m_8->CreateSprite(0, cx, cy, 0x17318, "InGameText", 0x40003);
        if (txt == 0) {
            return;
        }
        txt->m_124 = *(i32*)(self + 0xc4);
    }

    // (5) on-screen + no active override -> play the LEVEL_ROCKBREAK cue.
    if ((TX << 5) + 0x10 >= g_gameReg->m_viewOriginR || (TX << 5) + 0x10 < g_gameReg->m_viewOriginL
        || (TY << 5) + 0x10 >= g_gameReg->m_viewOriginB
        || (TY << 5) + 0x10 < g_gameReg->m_viewOriginT) {
        return;
    }
    CSndHost* sreg = (CSndHost*)gameMgr->m_28; // m_28 kept void* on the 60-TU holder (see
                                               // GameRegistry.h); CSndHost is the real class
    if (sreg->m_30 != 0) {
        return;
    }
    CSndEmitter* out = 0;
    sreg->m_10.Lookup("LEVEL_ROCKBREAK", &out);
    if (out == 0) {
        return;
    }
    if (g_sndEnabled == 0) {
        return;
    }
    i32 kc = g_killCueClock;
    if ((u32)(kc - out->m_14) < (u32)out->m_18) {
        return;
    }
    out->m_14 = kc;
    out->m_10->ConfigureItem(g_sndCueTag, 0, 0, 0);
}

#undef TX
#undef TY
