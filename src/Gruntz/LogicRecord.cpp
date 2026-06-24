// LogicRecord.cpp - CLogicRecord: the CObject-derived ~0x17c-byte serialized
// "logic record" the per-grunt logic owner (AddLogicHit/Attack/Bump at
// 0x150f50/0x151030/0x151110) new's and stows at owner+0x80. The record holds a
// flat block of replayable state read/written through a stream object
// (Write@slot 0x2c, Read@slot 0x30). Non-RTTI vtable (0x5efb80) + a base vtable
// (0x5e8cb4) re-stamped by the dtor; both modeled as external reloc-masked
// DATA() symbols while the class's virtuals are not all matched.
//
// Matched methods (RVA order):
//   ~CLogicRecord  0x151da0  /GX dtor (vptr re-stamp + sub-record teardown)
//   Init           0x151e20  bind primary data + zero the record
//   Consume        0x15b340  subtract from the remaining-count at +0x20
//   Dispatch       0x164830  switch(mode) over the record's actions
//   Load           0x164960  read the flat block back from a stream
//
// NOTE: 0x495750/0x495890/0x4aa6e0 (the three identical message handlers that
// switch on owner->m_7c->m_1c and `new` a sub-record) were trace-clustered here
// but are __cdecl free functions that never touch ecx - NOT members. Left
// stubbed in src/Stub/Discovered.cpp; see the report.
#include <Gruntz/LogicRecord.h>
#include <rva.h>

// The most-derived record vftable (0x5efb80), stamped at dtor entry, and the
// base-subobject vftable (0x5e8cb4) re-stamped at dtor exit - transitional
// reloc-masked DIR32 stores while the class stays non-polymorphic (its virtuals
// live in other TUs, so letting the compiler emit a vtable would diverge).
DATA(0x001efb80)
extern void* const g_LogicRecordVtbl[];
DATA(0x001e8cb4)
extern void* const g_LogicRecordBaseVtbl[];

// Engine operator delete (0x1b9b82, __cdecl) - reloc-masked rel32.
extern "C" void Engine_Delete(void* p);

// ---------------------------------------------------------------------------
// ~CLogicRecord (0x151da0, __thiscall, /GX). Stamp the derived vptr, free the
// owned heap block (m_14), tear down the polymorphic sub-record (m_18) via its
// virtual slot-0 destructor, zero the live fields, then restamp the base vptr.
// @early-stop
// eh-dtor-needs-base-subobject wall (docs/patterns/eh-dtor-needs-base-subobject.md):
// the body (derived vptr stamp, m_14 free, m_18->vtbl[0](1), field zeroing, base
// vptr restamp) is byte-exact, but the retail /GX frame (push -1 / fs:0 / trylevel)
// comes from a non-trivial CObject base subobject the manual-vptr non-polymorphic
// model can't emit. Defer to the final sweep once the base + full vtable are modeled.
RVA(0x00151da0, 0x80)
CLogicRecord::~CLogicRecord() {
    m_vptr = (void*)g_LogicRecordVtbl;
    m_10 = 0;
    if (m_14) {
        Engine_Delete(m_14);
        m_14 = 0;
        m_178 = 0;
    }
    if (m_18) {
        m_18->Destroy(1);
        m_18 = 0;
    }
    m_170 = 0;
    m_08 = 0;
    m_0c = 0;
    m_04 = -1;
    m_vptr = (void*)g_LogicRecordBaseVtbl;
}

// ---------------------------------------------------------------------------
// Init (0x151e20, __thiscall). Bind the primary data pointer (m_10) and the
// frame stamp (m_08), zeroing the working fields. Returns 0 if pData is null.
RVA(0x00151e20, 0x46)
i32 CLogicRecord::Init(void* pData, i32 frame) {
    if (pData == 0) {
        return 0;
    }
    m_10 = pData;
    m_08 = frame;
    m_14 = 0;
    m_18 = 0;
    m_serial[(0x20 - 0x20) / 4] = 0; // m_20
    m_serial[(0x24 - 0x20) / 4] = 0; // m_24
    m_serial[(0x2c - 0x20) / 4] = 0; // m_2c
    m_serial[(0x34 - 0x20) / 4] = 0; // m_34
    m_serial[(0x30 - 0x20) / 4] = 0; // m_30
    m_serial[(0x38 - 0x20) / 4] = 0; // m_38
    m_168 = 0;
    m_16c = 0;
    m_serial[(0x28 - 0x20) / 4] = 0; // m_28
    return 1;
}

// ---------------------------------------------------------------------------
// Consume (0x15b340, __thiscall). Draw `amount` from the remaining-count at
// m_20: returns 1 with the balance debited while it covers the request, else
// clamps to 0 and returns 0. No-op (returns into eax undefined per retail) when
// the count is already 0.
RVA(0x0015b340, 0x2b)
i32 CLogicRecord::Consume(i32 amount) {
    i32 remaining = m_serial[(0x20 - 0x20) / 4]; // m_20
    if (remaining == 0) {
        return remaining; // eax already holds 0
    }
    if ((u32)amount >= (u32)remaining) {
        m_serial[(0x20 - 0x20) / 4] = 0;
        return 0;
    }
    m_serial[(0x20 - 0x20) / 4] = remaining - amount;
    return 1;
}

// ---------------------------------------------------------------------------
// Dispatch (0x164830, __thiscall). Run one of the record's six actions selected
// by `mode` (3..8): refresh the cached value (m_174 from m_170->m_188), Load,
// the alternate Save-path (0x164d80), or re-resolve m_170 via the level grid.
// Then, when a sub-record is present, forward to its per-frame step
// (m_18->vtbl[1]); a falsey step result short-circuits the success return.
// @early-stop
// regalloc wall (docs/patterns/zero-register-pinning.md): body is structurally
// byte-exact, but retail keeps BOTH a (edi) and mode (ebp) callee-saved across
// the switch while cl pins a in ebx and SPILLS mode to the stack (re-read as
// [esp+0x1c] at the Step push); the residual is that register coin-flip plus the
// reloc-masked jump-table base + Save/Resolve extern call names. No source lever
// (local-pin of mode, arg reorder) flips the allocation. ~79%; defer to the
// final sweep.
RVA(0x00164830, 0xd3)
i32 CLogicRecord::Dispatch(i32 a, i32 mode, void* c, void* d) {
    if (a == 0) {
        return 0;
    }
    switch (mode) {
        case 3:
            m_174 = 0;
            if (m_170) {
                m_174 = ((LogicTarget*)m_170)->m_188;
            }
            break;
        case 4:
            if (Load((LogicArchive*)a) == 0) {
                return 0;
            }
            break;
        case 7:
            if (Save((LogicArchive*)a) == 0) {
                return 0;
            }
            break;
        case 8:
            if (m_174) {
                void* out = 0;
                LogicResolver* res = (LogicResolver*)(((LogicContext*)m_0c)->m_08 + 0x48);
                m_170 = res->Resolve((void*)m_174, &out) ? out : (void*)0;
            }
            break;
        default: // 5, 6
            break;
    }
    if (m_18) {
        if (m_18->Step(a, mode, c, d) == 0) {
            return 0;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// Load (0x164960, __thiscall). Read the flat record block back from the stream
// (Read@slot 0x30): the tag at 0x1c, the 4-byte field run 0x20..0x164 (skipping
// the runtime-only m_168/m_16c/m_170), the six 16-byte blocks 0xd0..0x120, the
// trailer 0x174/0x178, then m_178 bytes of the m_14 payload when both are set.
RVA(0x00164960, 0x41a)
i32 CLogicRecord::Load(LogicArchive* ar) {
    if (ar == 0) {
        return 0;
    }
    char* p = (char*)this;
    ar->Read(p + 0x1c, 4);
    ar->Read(p + 0x20, 4);
    ar->Read(p + 0x24, 4);
    ar->Read(p + 0x28, 4);
    ar->Read(p + 0x2c, 4);
    ar->Read(p + 0x30, 4);
    ar->Read(p + 0x34, 4);
    ar->Read(p + 0x38, 4);
    ar->Read(p + 0x3c, 4);
    ar->Read(p + 0x40, 4);
    ar->Read(p + 0x44, 4);
    ar->Read(p + 0x48, 4);
    ar->Read(p + 0x4c, 4);
    ar->Read(p + 0x50, 4);
    ar->Read(p + 0x54, 4);
    ar->Read(p + 0x58, 4);
    ar->Read(p + 0x5c, 4);
    ar->Read(p + 0x60, 4);
    ar->Read(p + 0x64, 4);
    ar->Read(p + 0x68, 4);
    ar->Read(p + 0x6c, 4);
    ar->Read(p + 0x70, 4);
    ar->Read(p + 0x74, 4);
    ar->Read(p + 0x78, 4);
    ar->Read(p + 0x7c, 4);
    ar->Read(p + 0x80, 4);
    ar->Read(p + 0x84, 4);
    ar->Read(p + 0x88, 4);
    ar->Read(p + 0x8c, 4);
    ar->Read(p + 0x90, 4);
    ar->Read(p + 0x94, 4);
    ar->Read(p + 0x98, 4);
    ar->Read(p + 0x9c, 4);
    ar->Read(p + 0xa0, 4);
    ar->Read(p + 0xa4, 4);
    ar->Read(p + 0xa8, 4);
    ar->Read(p + 0xac, 4);
    ar->Read(p + 0xb0, 4);
    ar->Read(p + 0xb4, 4);
    ar->Read(p + 0xb8, 4);
    ar->Read(p + 0xbc, 4);
    ar->Read(p + 0xc0, 4);
    ar->Read(p + 0xc4, 4);
    ar->Read(p + 0xc8, 4);
    ar->Read(p + 0xcc, 4);
    ar->Read(p + 0xd0, 16);
    ar->Read(p + 0xe0, 16);
    ar->Read(p + 0xf0, 16);
    ar->Read(p + 0x100, 16);
    ar->Read(p + 0x110, 16);
    ar->Read(p + 0x120, 16);
    ar->Read(p + 0x130, 4);
    ar->Read(p + 0x134, 4);
    ar->Read(p + 0x138, 4);
    ar->Read(p + 0x13c, 4);
    ar->Read(p + 0x140, 4);
    ar->Read(p + 0x144, 4);
    ar->Read(p + 0x148, 4);
    ar->Read(p + 0x14c, 4);
    ar->Read(p + 0x150, 4);
    ar->Read(p + 0x154, 4);
    ar->Read(p + 0x158, 4);
    ar->Read(p + 0x15c, 4);
    ar->Read(p + 0x160, 4);
    ar->Read(p + 0x164, 4);
    ar->Read(p + 0x174, 4);
    ar->Read(&m_178, 4);
    void* payload = m_14;
    if (payload && m_178 > 0) {
        ar->Read(payload, m_178);
    }
    return 1;
}
