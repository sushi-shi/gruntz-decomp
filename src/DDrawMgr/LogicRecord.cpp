#include <DDrawMgr/AnimWorkerObj.h>
#include <DDrawMgr/DDrawSurfaceMgr.h> // m_0c (the owner/world root)
#include <DDrawMgr/DDrawChildGroup.h> // m_childGroup->m_map48 (the id->object resolver)
#include <Gruntz/UserLogic.h> // CUserLogic (m_logic SerializeMove dispatch) + CGameObject (m_170)
#include <rva.h>
#include <Mfc.h>        // CMapPtrToPtr::Lookup (0x1b8760)
#include <Io/FileMem.h> // CFileMemBase complete type (the CSerialArchive Read/Write dispatch)

extern "C" void Engine_Delete(void* p);

extern void* operator new(u32 size);

// ---------------------------------------------------------------------------
// Dispatch (0x164830, __thiscall). Run one of the record's six actions selected
// by `mode` (3..8): refresh the cached value (m_174 from m_170->m_188), Load,
// the alternate Save-path (0x164d80), or re-resolve m_170 via the level grid.
// Then, when a sub-record is present, forward to its per-frame step
// (m_logic->SerializeMove, CUserBase slot 1); a falsey result short-circuits.
// @early-stop
// regalloc wall (docs/patterns/zero-register-pinning.md): body is structurally
// byte-exact, but retail keeps BOTH a (edi) and mode (ebp) callee-saved across
// the switch while cl pins a in ebx and SPILLS mode to the stack (re-read as
// [esp+0x1c] at the Step push); the residual is that register coin-flip plus the
// reloc-masked jump-table base + Save/Resolve extern call names. No source lever
// (local-pin of mode, arg reorder) flips the allocation. ~79%; defer to the
// final sweep.
RVA(0x00164830, 0xd3)
i32 AnimWorkerObj::Dispatch(i32 a, i32 mode, void* c, void* d) {
    if (a == 0) {
        return 0;
    }
    switch (mode) {
        case 3:
            m_targetId = 0;
            if (m_target) {
                m_targetId = m_target->m_188;
            }
            break;
        case 4:
            // the serialize walk (ForEachSerialize_15b020, WRITES the stream)
            if (Save(reinterpret_cast<CSerialArchive*>(a)) == 0) {
                return 0;
            }
            break;
        case 7:
            // the deserialize walk (Deserialize_15b0e0, READS the stream)
            if (Load(reinterpret_cast<CSerialArchive*>(a)) == 0) {
                return 0;
            }
            break;
        case 8:
            if (m_targetId) {
                void* out = 0;
                CMapPtrToPtr* res = &m_0c->m_childGroup->m_map48;
                m_target = res->Lookup(reinterpret_cast<void*>(m_targetId), out) ? static_cast<CGameObject*>(out) : static_cast<CGameObject*>(0);
            }
            break;
        default: // 5, 6
            break;
    }
    if (m_logic) {
        if (m_logic->SerializeMove(reinterpret_cast<CGruntArchive*>(a), mode, reinterpret_cast<i32>(c), reinterpret_cast<i32>(d)) == 0) {
            return 0;
        }
    }
    return 1;
}

RVA(0x00164920, 0x35)
i32 AnimWorkerObj::CacheTargetId(void* a) {
    if (a == 0) {
        return 0;
    }
    m_targetId = 0;
    if (m_target) {
        m_targetId = m_target->m_188;
    }
    return 1;
}

RVA(0x00164960, 0x41a)
i32 AnimWorkerObj::Save(CSerialArchive* ar) {
    if (ar == 0) {
        return 0;
    }
    char* p = reinterpret_cast<char*>(this);
    ar->Write(p + 0x1c, 4);
    ar->Write(p + 0x20, 4);
    ar->Write(p + 0x24, 4);
    ar->Write(p + 0x28, 4);
    ar->Write(p + 0x2c, 4);
    ar->Write(p + 0x30, 4);
    ar->Write(p + 0x34, 4);
    ar->Write(p + 0x38, 4);
    ar->Write(p + 0x3c, 4);
    ar->Write(p + 0x40, 4);
    ar->Write(p + 0x44, 4);
    ar->Write(p + 0x48, 4);
    ar->Write(p + 0x4c, 4);
    ar->Write(p + 0x50, 4);
    ar->Write(p + 0x54, 4);
    ar->Write(p + 0x58, 4);
    ar->Write(p + 0x5c, 4);
    ar->Write(p + 0x60, 4);
    ar->Write(p + 0x64, 4);
    ar->Write(p + 0x68, 4);
    ar->Write(p + 0x6c, 4);
    ar->Write(p + 0x70, 4);
    ar->Write(p + 0x74, 4);
    ar->Write(p + 0x78, 4);
    ar->Write(p + 0x7c, 4);
    ar->Write(p + 0x80, 4);
    ar->Write(p + 0x84, 4);
    ar->Write(p + 0x88, 4);
    ar->Write(p + 0x8c, 4);
    ar->Write(p + 0x90, 4);
    ar->Write(p + 0x94, 4);
    ar->Write(p + 0x98, 4);
    ar->Write(p + 0x9c, 4);
    ar->Write(p + 0xa0, 4);
    ar->Write(p + 0xa4, 4);
    ar->Write(p + 0xa8, 4);
    ar->Write(p + 0xac, 4);
    ar->Write(p + 0xb0, 4);
    ar->Write(p + 0xb4, 4);
    ar->Write(p + 0xb8, 4);
    ar->Write(p + 0xbc, 4);
    ar->Write(p + 0xc0, 4);
    ar->Write(p + 0xc4, 4);
    ar->Write(p + 0xc8, 4);
    ar->Write(p + 0xcc, 4);
    ar->Write(p + 0xd0, 16);
    ar->Write(p + 0xe0, 16);
    ar->Write(p + 0xf0, 16);
    ar->Write(p + 0x100, 16);
    ar->Write(p + 0x110, 16);
    ar->Write(p + 0x120, 16);
    ar->Write(p + 0x130, 4);
    ar->Write(p + 0x134, 4);
    ar->Write(p + 0x138, 4);
    ar->Write(p + 0x13c, 4);
    ar->Write(p + 0x140, 4);
    ar->Write(p + 0x144, 4);
    ar->Write(p + 0x148, 4);
    ar->Write(p + 0x14c, 4);
    ar->Write(p + 0x150, 4);
    ar->Write(p + 0x154, 4);
    ar->Write(p + 0x158, 4);
    ar->Write(p + 0x15c, 4);
    ar->Write(p + 0x160, 4);
    ar->Write(p + 0x164, 4);
    ar->Write(p + 0x174, 4);
    ar->Write(&m_payloadSize, 4);
    void* payload = m_payload;
    if (payload && m_payloadSize > 0) {
        ar->Write(payload, m_payloadSize);
    }
    return 1;
}

RVA(0x00164d80, 0x421)
i32 AnimWorkerObj::Load(CSerialArchive* ar) {
    if (ar == 0) {
        return 0;
    }
    char* p = reinterpret_cast<char*>(this);
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
    ar->Read(&m_payloadSize, 4);
    if (m_payloadSize > 0) {
        m_payload = static_cast<u8*>(::operator new(m_payloadSize));
        ar->Read(m_payload, m_payloadSize);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// ResolveTarget (0x1651b0, __thiscall). The standalone Dispatch-case-8 hook:
// when a cached reference id (m_174) is present, re-resolve the target pointer
// (m_170) by looking the id up in the level grid's CMapPtrToPtr resolver.
// Returns 0 on a null argument, 1 otherwise.
// @early-stop
// 99.33% - regalloc coin-flip wall (docs/patterns/zero-register-pinning.md):
// body byte-identical (the out=0 store schedule, push order, and if/else branch
// polarity all match after computing `res` before `out`), but retail pins m_0c in
// edx and &out in ecx (`mov edx,[esi+0xc]; lea ecx,[esp+8]`) while cl pins m_0c in
// ecx and &out in edx - a 3-instruction operand-register choice the permuter can't
// flip. Not source-steerable.
RVA(0x001651b0, 0x5d)
i32 AnimWorkerObj::ResolveTarget(void* a) {
    if (a == 0) {
        return 0;
    }
    if (m_targetId) {
        CMapPtrToPtr* res = &m_0c->m_childGroup->m_map48;
        void* out = 0;
        if (!res->Lookup(reinterpret_cast<void*>(m_targetId), out)) {
            m_target = 0;
        } else {
            m_target = static_cast<CGameObject*>(out);
        }
    }
    return 1;
}
