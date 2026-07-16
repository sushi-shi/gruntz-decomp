// SerialObjRef.cpp - the shared serialized-object-reference Chain (0x8c00).
// See CSerialObjRef.h for the layout + the registry/archive chain notes. Every
// CUserLogic Serialize override runs this on `&this->m_34`; modeled here once as
// the real method so the reloc-masked Chain calls in the caller TUs pair to it by
// address.
//
// __thiscall, ret 0x10 (4 stack args). Frameless in retail despite the write-path
// CString temp: the temp is RVO'd straight from KeyOfValue and its only live-range
// use (the inline strcpy = rep movs) cannot throw, so /GX elides the EH frame
// (docs/patterns/scope-cstring-temp-to-elide-eh-frame.md). The 0x80-byte key buffer
// + strlen/strcpy are inline CRT idioms (docs/patterns/inline-mem-ops-rep-movs.md).
#include <Gruntz/SerialObjRef.h>
#include <Io/FileMem.h> // the serialize stream (CSerialArchive == the real CFileMemBase)

#include <rva.h>

#include <string.h> // strlen / strcpy (inlined to repne scasb / rep movs)

// ---------------------------------------------------------------------------
// 0x8c00: serialize one referenced object + its key name.
// @early-stop
// 88.0% - logic byte-faithful: the read/write virtual dispatch (Read @ +0x2c /
// Write @ +0x30), the strlen-key gate, the CMapStringToOb::Lookup + KeyOfValue
// registry chain (both real symbols paired), the inline strlen/strcpy/memset, and
// the frameless /GX-elided CString temp all match. The residual is one stack-slot-
// coloring difference (docs/patterns/stack-slot-coalesce-frame-4b.md): retail gives
// the read-path Lookup `out` and the write-path CString temp DISTINCT slots
// (sub esp,0x88: out@0x10, CString@0x14, buf@0x18) while cl COALESCES the two
// mutually-exclusive locals into one slot (sub esp,0x84) - shifting every [esp+M]
// by 4 and spilling the KeyOfValue RVO result instead of using eax. Not steerable
// from source under /O2 (function-scope `val` and inner-scope reshapes both
// regressed). Logic complete; deferred to the final sweep.
RVA(0x00008c00, 0x152)
i32 CSerialObjRef::Chain(CSerialArchive* arc, i32 mode, i32 unused, CGameObject* obj) {
    char name[0x80];

    if (arc == 0) {
        return 0;
    }
    if (mode == 7) {
        // READ: pull the key name + the 0x10 blob, then resolve the key.
        arc->Read(name, 0x80);
        arc->Read(m_blob, 0x10);
        m_00 = obj;
        m_04 = obj;
        m_08 = obj->m_7c;
        if (strlen(name) == 0) {
            m_value = 0;
            return 1;
        }
        void* val = 0; // CMapStringToPtr::Lookup (0x1b8438) takes a void&
        m_08->m_0c->m_leaf->m_10.Lookup(name, val);
        m_value = (CObject*)val; // the map stores void*; KeyOfValue_152d30 takes CObject*
        return 1;
    }
    if (mode == 4) {
        // WRITE: re-derive the value's name into the key buffer, then write both.
        for (i32 i = 0; i < 0x20; i++) {
            ((i32*)name)[i] = 0;
        }
        if (m_value != 0) {
            CString nm = m_08->m_0c->m_leaf->KeyOfValue_152d30(m_value);
            strcpy(name, (const char*)nm);
        }
        arc->Write(name, 0x80);
        arc->Write(m_blob, 0x10);
    }
    return 1;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CDDrawSubMgrLeaf);
