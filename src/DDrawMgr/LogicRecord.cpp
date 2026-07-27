#include <DDrawMgr/LogicRecord.h> // own extern surface
#include <DDrawMgr/AnimWorkerObj.h>
#include <DDrawMgr/DDrawSurfaceMgr.h> // m_0c (the owner/world root)
#include <DDrawMgr/DDrawChildGroup.h> // m_childGroup->m_map48 (the id->object resolver)
#include <Gruntz/UserLogic.h> // CUserLogic (m_logic SerializeMove dispatch) + CGameObject (m_170)
#include <rva.h>
#include <Mfc.h>        // CMapPtrToPtr::Lookup (0x1b8760)
#include <Io/FileMem.h> // CFileMemBase complete type (the CFileMemBase Read/Write dispatch)

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
i32 AnimWorkerObj::Dispatch(CFileMemBase* a, i32 mode, void* c, void* d) {
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
            // the serialize walk (ForEachSerialize, WRITES the stream)
            if (Save(a) == 0) {
                return 0;
            }
            break;
        case 7:
            // the deserialize walk (Deserialize, READS the stream)
            if (Load(a) == 0) {
                return 0;
            }
            break;
        case 8:
            if (m_targetId) {
                void* out = 0;
                CMapPtrToPtr* res = &m_0c->m_childGroup->m_map48;
                m_target = res->Lookup(reinterpret_cast<void*>(m_targetId), out)
                               ? static_cast<CGameObject*>(out)
                               : static_cast<CGameObject*>(0);
            }
            break;
        default: // 5, 6
            break;
    }
    if (m_logic) {
        if (m_logic->SerializeMove(a, mode, reinterpret_cast<i32>(c), static_cast<CGameObject*>(d))
            == 0) {
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
i32 AnimWorkerObj::Save(CFileMemBase* ar) {
    if (ar == 0) {
        return 0;
    }
    ar->Write(&m_1c, 4);
    ar->Write(&m_20, 4);
    ar->Write(&m_24, 4);
    ar->Write(&m_28, 4);
    ar->Write(&m_2c, 4);
    ar->Write(&m_30, 4);
    ar->Write(&m_34, 4);
    ar->Write(&m_38, 4);
    ar->Write(&m_pad3c, 4);
    ar->Write(&m_40, 4);
    ar->Write(&m_44, 4);
    ar->Write(&m_48, 4);
    ar->Write(&m_scrollTargetX, 4);
    ar->Write(&m_scrollTargetY, 4);
    ar->Write(&m_pad54, 4);
    ar->Write(&m_58, 4);
    ar->Write(&m_5c, 4);
    ar->Write(&m_60, 4);
    ar->Write(&m_64, 4);
    ar->Write(&m_68, 4);
    ar->Write(&m_6c, 4);
    ar->Write(&m_70, 4);
    ar->Write(&m_74, 4);
    ar->Write(&m_78, 4);
    ar->Write(&m_7c, 4);
    ar->Write(&m_80, 4);
    ar->Write(&m_84, 4);
    ar->Write(&m_88, 4);
    ar->Write(&m_8c, 4);
    ar->Write(&m_90, 4);
    ar->Write(&m_94, 4);
    ar->Write(&m_98, 4);
    ar->Write(&m_9c, 4);
    ar->Write(&m_a0, 4);
    ar->Write(&m_a4, 4);
    ar->Write(&m_a8, 4);
    ar->Write(&m_ac, 4);
    ar->Write(&m_b0, 4);
    ar->Write(&m_b4, 4);
    ar->Write(&m_b8, 4);
    ar->Write(&m_bc, 4);
    ar->Write(&m_padc0, 4);
    ar->Write(&m_c4, 4);
    ar->Write(&m_c8, 4);
    ar->Write(&m_cc, 4);
    ar->Write(&m_d0, 16);
    ar->Write(&m_e0, 16);
    ar->Write(&m_switchRectA, 16);
    ar->Write(&m_switchRectB, 16);
    ar->Write(&m_pad110, 16);
    ar->Write(&m_120, 16);
    ar->Write(&m_130, 4);
    ar->Write(&m_pad134, 4);
    ar->Write(&m_138, 4);
    ar->Write(&m_13c, 4);
    ar->Write(&m_140, 4);
    ar->Write(&m_144, 4);
    ar->Write(&m_148, 4);
    ar->Write(&m_14c, 4);
    ar->Write(&m_150, 4);
    ar->Write(&m_154, 4);
    ar->Write(&m_158, 4);
    ar->Write(&m_15c, 4);
    ar->Write(&m_160, 4);
    ar->Write(&m_164, 4);
    ar->Write(&m_targetId, 4);
    ar->Write(&m_payloadSize, 4);
    void* payload = m_payload;
    if (payload && m_payloadSize > 0) {
        ar->Write(payload, m_payloadSize);
    }
    return 1;
}

RVA(0x00164d80, 0x421)
i32 AnimWorkerObj::Load(CFileMemBase* ar) {
    if (ar == 0) {
        return 0;
    }
    ar->Read(&m_1c, 4);
    ar->Read(&m_20, 4);
    ar->Read(&m_24, 4);
    ar->Read(&m_28, 4);
    ar->Read(&m_2c, 4);
    ar->Read(&m_30, 4);
    ar->Read(&m_34, 4);
    ar->Read(&m_38, 4);
    ar->Read(&m_pad3c, 4);
    ar->Read(&m_40, 4);
    ar->Read(&m_44, 4);
    ar->Read(&m_48, 4);
    ar->Read(&m_scrollTargetX, 4);
    ar->Read(&m_scrollTargetY, 4);
    ar->Read(&m_pad54, 4);
    ar->Read(&m_58, 4);
    ar->Read(&m_5c, 4);
    ar->Read(&m_60, 4);
    ar->Read(&m_64, 4);
    ar->Read(&m_68, 4);
    ar->Read(&m_6c, 4);
    ar->Read(&m_70, 4);
    ar->Read(&m_74, 4);
    ar->Read(&m_78, 4);
    ar->Read(&m_7c, 4);
    ar->Read(&m_80, 4);
    ar->Read(&m_84, 4);
    ar->Read(&m_88, 4);
    ar->Read(&m_8c, 4);
    ar->Read(&m_90, 4);
    ar->Read(&m_94, 4);
    ar->Read(&m_98, 4);
    ar->Read(&m_9c, 4);
    ar->Read(&m_a0, 4);
    ar->Read(&m_a4, 4);
    ar->Read(&m_a8, 4);
    ar->Read(&m_ac, 4);
    ar->Read(&m_b0, 4);
    ar->Read(&m_b4, 4);
    ar->Read(&m_b8, 4);
    ar->Read(&m_bc, 4);
    ar->Read(&m_padc0, 4);
    ar->Read(&m_c4, 4);
    ar->Read(&m_c8, 4);
    ar->Read(&m_cc, 4);
    ar->Read(&m_d0, 16);
    ar->Read(&m_e0, 16);
    ar->Read(&m_switchRectA, 16);
    ar->Read(&m_switchRectB, 16);
    ar->Read(&m_pad110, 16);
    ar->Read(&m_120, 16);
    ar->Read(&m_130, 4);
    ar->Read(&m_pad134, 4);
    ar->Read(&m_138, 4);
    ar->Read(&m_13c, 4);
    ar->Read(&m_140, 4);
    ar->Read(&m_144, 4);
    ar->Read(&m_148, 4);
    ar->Read(&m_14c, 4);
    ar->Read(&m_150, 4);
    ar->Read(&m_154, 4);
    ar->Read(&m_158, 4);
    ar->Read(&m_15c, 4);
    ar->Read(&m_160, 4);
    ar->Read(&m_164, 4);
    ar->Read(&m_targetId, 4);
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
