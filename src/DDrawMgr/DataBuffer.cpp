// DataBuffer.cpp - the small RezAlloc-backed serialized buffer holder
// (tracer placeholder tomalla-4). Four self-contained methods; the only
// externals are the Rez heap allocator/freer (RezAlloc 0x1b9b46 / RezFree
// 0x1b9b82), modeled no-body so their `call rel32` displacements are
// reloc-masked. Methods in ascending retail-RVA order.
#include <Mfc.h> // CFile / CMemFile / CString (the readers slurp through MFC files)
#include <DDrawMgr/ShadeTableCache.h> // the canonical CShadeTable (retail name; ex CDataBuffer view)
#include <Rez/RezAlloc.h>             // RezAlloc/RezFree

#include <rva.h>

// Rez heap allocator/freer (external, reloc-masked).

// ctor: zero the valid flag, size and blob (id is left alone).
RVA(0x00150180, 0xd)
CShadeTable::CShadeTable() {
    m_alloc = 0;
    m_size = 0;
    m_data = 0;
}

// Reset (0x150190): free the blob if loaded (tail-call to Free). Out-of-line
// (retail emits it standalone; the inline member folded away and never emitted).
RVA(0x00150190, 0xb)
void CShadeTable::Reset() {
    if (m_alloc != 0) {
        Free();
    }
}

// Set: (re)allocate a `size`-byte blob; record id on success.
RVA(0x001501a0, 0x44)
i32 CShadeTable::Set(u32 size, i32 id) {
    if (m_data) {
        RezFree(m_data);
    }
    m_size = size;
    m_data = static_cast<u8*>(RezAlloc(size));
    if (!m_data) {
        return 0;
    }
    m_alloc = 1;
    m_key = id;
    return 1;
}

// ReadFrom: pull a 4-byte element count out of `file` (CFile::Read,
// vtable slot +0x3c), (re)allocate the blob via Set(count, id), then read the blob
// bytes through the same CFile::Read. Returns Set's result; on success re-stamps the
// valid flag + id and returns 1. __thiscall, ret 8.
RVA(0x001501f0, 0x54)
i32 CShadeTable::ReadFrom(CFile* file, i32 id) {
    file->Read(&m_size, 4);
    if (Set(m_size, id) == 0) {
        return 0;
    }
    file->Read(m_data, m_size);
    m_alloc = 1;
    m_key = id;
    return 1;
}

// LoadFromFile: open `path` read-only-shared through a local CFile, slurp
// it into the buffer via ReadFrom, and close. The CString temp (from the implicit
// LPCTSTR open) and the local CFile both destruct on every path -> /GX EH frame.
// ret 8.
//
// @early-stop
// /GX EH-frame wall (~72%): logic + the CFile ctor/Open/ReadFrom/~CFile call chain +
// the CFileException CString temp are faithful; residue is the EH-state numbering +
// the local CFile's stack-slot scheduling inside the 0x10-byte frame. Deferred to the
// final sweep.
RVA(0x00150250, 0xd1)
i32 CShadeTable::LoadFromFile(const char* path, i32 id) {
    CFile file;
    if (!file.Open(path, CFile::modeRead | CFile::shareDenyWrite, 0)) {
        return 0;
    }
    return ReadFrom(&file, id);
}

// LoadFromMem: wrap `buf`/`len` in a local CMemFile (0x400-byte grow
// chunk), Attach the caller's buffer, slurp it via ReadFrom, then ~CMemFile detaches
// it. /GX EH frame, ret 0xc.
//
// @early-stop
// /GX EH-frame wall (~83%): logic + the CMemFile ctor/Attach/ReadFrom/~CMemFile chain
// are faithful; residue is the local CMemFile's stack-slot offset (retail lands it at
// [esp+0x10], our cl at [esp+0x8]) + the EH-state numbering. Deferred to the final sweep.
RVA(0x00150330, 0x87)
i32 CShadeTable::LoadFromMem(void* buf, u32 len, i32 id) {
    CMemFile file(0x400);
    file.Attach(static_cast<BYTE*>(buf), len);
    return ReadFrom(&file, id);
}

// Free: if valid, free + clear the blob and size, then clear valid.
RVA(0x001503c0, 0x2e)
void CShadeTable::Free() {
    if (m_alloc != 0) {
        if (m_data) {
            RezFree(m_data);
            m_data = 0;
        }
        m_size = 0;
    }
    m_alloc = 0;
}

// SaveToFile: create `path` (modeCreate|modeWrite), write the 4-byte
// element count then the blob, close. The by-value CString arg (destructed at exit,
// outermost cleanup) + the local CFile -> /GX EH frame; on an Open failure the CFile
// is destructed and 0 returned. ret 4 (the CString is one dword).
RVA(0x001503f0, 0xdc)
i32 CShadeTable::SaveToFile(CString path) {
    CFile file;
    if (!file.Open(path, CFile::modeCreate | CFile::modeWrite, 0)) {
        return 0;
    }
    file.Write(&m_size, 4);
    file.Write(m_data, m_size);
    file.Close();
    return 1;
}
