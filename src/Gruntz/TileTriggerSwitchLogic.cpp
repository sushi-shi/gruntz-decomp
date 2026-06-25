#include <rva.h>
// TileTriggerSwitchLogic.cpp - Gruntz CTileTriggerSwitchLogic (C:\Proj\Gruntz).
// CTileTriggerSwitchLogic is a tile-trigger "switch" class (vtable 0x5eae8c,
// stamped by the ctor) that owns a CObList of sibling CTileTriggerSwitchLogic
// children (head at +0x04; nodes next@+0x00, data@+0x08).  The ctor zeroes the
// 24-dword m_block at +0x2c and clears m_20; the Find/Remove accessors walk the
// child list; ValidateByType/ApplyByType dispatch on a passed type tag; the
// Transfer/Serialize methods stream the object's fields through a CSerialStream.
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

// CTileTriggerSwitchLogic's own vftable, referenced as a DIR32 datum and stamped
// into the object by the ctor (transitional manual vptr; the class's virtuals
// aren't all modeled yet so we can't let cl emit a matching vtable).
DATA(0x005eae8c)
extern void* g_tileTriggerSwitchLogicVtbl;

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
    *(void**)this = &g_tileTriggerSwitchLogicVtbl;
    for (i32 i = 0; i < 24; i++) {
        m_block[i] = 0;
    }
    m_20 = 0;
}

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
// gate on m_14, find the owner's child that claims this object (FindIndexByKey),
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
    if (m_14 == 0) {
        return 0;
    }
    ListNode* node = (ListNode*)m_owner->m_20;
    i32 found = 0;
    CTileTriggerSwitchLogic* child = this;
    while (node != 0) {
        if (found != 0) {
            break;
        }
        ListNode* cur = node;
        node = node->m_next;
        child = cur->m_data;
        if (child != 0 && child->FindIndexByKey(m_10) != 0) {
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
        if (c->m_14 == 0) {
            return 0;
        }
        p++;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::VerifyBlockLinks
// Linkage validator: if this->m_14 is clear, succeed (return 0 short-circuit on
// the null gate).  Otherwise walk the owner's child list (head @ owner->m_20),
// asking each child's FindIndexByKey(this->m_10) until one claims this object.
// If none does, ack diagnostic 0x452 and fail.  Then, for the claiming child,
// scan its 24-dword key block (child->m_block[4..27]): an empty slot succeeds;
// each nonzero key must resolve via owner->FindChild(key, 8) to a child whose
// m_14 gate is set (else fail, acking 0x453 when the lookup itself misses).
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
    if (m_14 == 0) {
        return 0;
    }
    ListNode* node = (ListNode*)m_owner->m_20;
    i32 found = 0;
    CTileTriggerSwitchLogic* child = this;
    while (node != 0) {
        if (found != 0) {
            break;
        }
        ListNode* cur = node;
        node = node->m_next;
        child = cur->m_data;
        if (child != 0 && child->FindIndexByKey(m_10) != 0) {
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
        if (c->m_14 == 0) {
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
i32 CTileTriggerSwitchLogic::LoadState(CSerialStream* s) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_30 == 0) {
        return 0;
    }
    s->TransferIn(&m_08, 4);
    s->TransferIn(&m_0c, 4);
    s->TransferIn(&m_10, 4);
    s->TransferIn(&m_14, 4);
    s->TransferIn(&m_18, 4);
    s->TransferIn(&m_1c, 4);
    s->TransferIn(&m_20, 4);
    s->TransferIn(&m_28, 4);
    i32* p = m_block;
    for (i32 i = 0; i < 24; i++) {
        s->TransferIn(p, 4);
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
i32 CTileTriggerSwitchLogic::SerializeMatrix(CSerialStream* s) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_30 == 0) {
        return 0;
    }
    s->Transfer(&m_block[37], 4); // +0xc0
    s->Transfer(&m_block[38], 4); // +0xc4
    i32* p = &m_block[28];        // +0x9c
    for (i32 r = 0; r < 3; r++) {
        for (i32 c = 0; c < 3; c++) {
            s->Transfer(p, 4);
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
// Walks the child list (head @ +0x04); on the first child whose +0x04 == k2 and
// +0x10 == k1, deletes the child (inlined ~ + RezFree), unlinks the node via
// CObList::RemoveAt, and returns 1.  Returns 0 if no match.
// ---------------------------------------------------------------------------
// @early-stop
// regalloc wall (~96%): logic + key/reg mapping exact; retail materializes the
// node in a 2nd reg (ecx) to deref data while keeping cur in esi for RemoveAt,
// vs our single live-node deref. Loop-body copy scheduling only; final-sweep.
RVA(0x00116320, 0x66)
i32 CTileTriggerSwitchLogic::RemoveByKeys(i32 k1, i32 k2) {
    ListNode* node = (ListNode*)m_04;
    while (node) {
        ListNode* cur = node;
        CTileTriggerSwitchLogic* data = node->m_data;
        node = node->m_next;
        if (data->m_04 == k2 && data->m_10 == k1) {
            if (data) {
                *(void**)data = &g_tileTriggerSwitchLogicVtbl;
                data->m_20 = 0;
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
        if (data->m_10 == k1) {
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
        if (data->m_0c == key) {
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
i32 CTileTriggerSwitchLogic::TransferFlag74(CSerialStream* s) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_30 == 0) {
        return 0;
    }
    s->Transfer(&m_block[18], 4);
    return 1;
}

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::LoadFlag74
// The read counterpart of TransferFlag74: same null/game-manager gates, but
// reads the 4-byte +0x74 flag (== m_block[18]) through the stream's read slot
// (vtable +0x2c) instead of the write slot (+0x30). Returns 1 on success.
// ---------------------------------------------------------------------------
RVA(0x00117e70, 0x36)
i32 CTileTriggerSwitchLogic::LoadFlag74(CSerialStream* s) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_30 == 0) {
        return 0;
    }
    s->TransferIn(&m_block[18], 4);
    return 1;
}

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::ScanNeighborhood
// Scans the 3x3 cell neighborhood centered on (x, y): for px in [x-1, x+2) and
// py in [y-1, y+2), probes cell (py + (px << 8)) with kind 0x16; returns the
// first nonzero probe result, else 0.
// ---------------------------------------------------------------------------
// @early-stop
// spill-scheduling wall (~76%): logic exact, inner loop + ProbeCell call/regs
// match; retail reserves 8B of frame (sub esp,8) and hoists the inner (y-1,y+2)
// bounds out of the outer loop, vs our tighter frame. Final-sweep.
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

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x001122a0, 0x241)
void CTileTriggerSwitchLogic::BuildRockBreakInGameText() {}
