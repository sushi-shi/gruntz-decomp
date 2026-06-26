// CMgrSettings.cpp - CMgrSettings::Serialize (0x109e00). See CMgrSettings.h. The
// scalar block (3 ints + 5 doubles) streams directly; the trailing object
// reference round-trips through the game registry (READ: name+index -> Lookup ->
// indexed record element; WRITE: m_38 -> AnyValueMatches -> name+index). The
// inline strlen/memset are CRT idioms (repne scas / rep stos); all engine callees
// are reloc-masked.
#include <Gruntz/CMgrSettings.h>

#include <string.h> // strlen / memset (inlined to repne scasb / rep stos)

// @early-stop
// 89.1% - logic byte-faithful: the mode-4/7 dispatch, the eight scalar Read/Write
// virtual calls, g_serialCount bump, inline strlen/memset, the Lookup + indexed
// record resolve, and the AnyValueMatches reverse-probe all match. Residual is one
// regalloc choice in the lookup range-check: retail keeps `index` in callee-saved
// esi across the Lookup and materializes the out-init 0 transiently, while cl pins
// the constant 0 in esi and spills `index` (docs/patterns/zero-register-pinning.md
// + pin-local-for-callee-saved-reg). Not source-steerable (& and || forms both
// normalize to the same fail-first regalloc). Logic complete; final-sweep deferred.
RVA(0x00109e00, 0x245)
i32 CMgrSettings::Serialize(CMgrArchive* arc, i32 mode, i32 a3, i32 a4) {
    if (arc == 0) {
        return 0;
    }
    CMgrActiveHolder* lvl = g_gameReg->m_30;
    if (lvl == 0) {
        return 0;
    }
    if (mode != 4) {
        if (mode == 7) {
            // READ the scalar block, then resolve the object reference.
            arc->Read(&m_00, 4);
            arc->Read(&m_04, 4);
            arc->Read(&m_08, 4);
            arc->Read(&m_10, 8);
            arc->Read(&m_18, 8);
            arc->Read(&m_20, 8);
            arc->Read(&m_28, 8);
            arc->Read(&m_30, 8);
            g_serialCount++;

            char name[0x80];
            i32 index;
            arc->Read(name, 0x80);
            arc->Read(&index, 4);
            if (strlen(name) == 0) {
                m_38 = 0;
                return 1;
            }
            CObject* out = 0;
            lvl->m_10->m_10.Lookup(name, out);
            CMgrLookupRec* rec = (CMgrLookupRec*)out;
            if (rec == 0 || index < rec->m_64 || index > rec->m_68) {
                m_38 = 0;
            } else {
                m_38 = rec->m_14[index];
            }
            return 1;
        }
    } else {
        // WRITE the scalar block, then the resolved object's name + index.
        arc->Write(&m_00, 4);
        arc->Write(&m_04, 4);
        arc->Write(&m_08, 4);
        arc->Write(&m_10, 8);
        arc->Write(&m_18, 8);
        arc->Write(&m_20, 8);
        arc->Write(&m_28, 8);
        arc->Write(&m_30, 8);
        g_serialCount++;

        void* obj = m_38;
        char name[0x80];
        i32 index = 0;
        memset(name, 0, 0x80);
        if (obj != 0) {
            lvl->m_10->AnyValueMatches_155630((i32)obj, (i32)name, (i32)&index);
        }
        arc->Write(name, 0x80);
        arc->Write(&index, 4);
    }
    return 1;
}
