// DataBuffer.cpp - the small RezAlloc-backed serialized buffer holder
// (tracer placeholder ClassUnknown_4). Four self-contained methods; the only
// externals are the Rez heap allocator/freer (RezAlloc 0x1b9b46 / RezFree
// 0x1b9b82), modeled no-body so their `call rel32` displacements are
// reloc-masked. Methods in ascending retail-RVA order.
#include <Gruntz/DataBuffer.h>

#include <rva.h>

// Rez heap allocator/freer (external, reloc-masked).
extern "C" void* RezAlloc(u32 size); // 0x1b9b46
extern "C" void RezFree(void* p);    // 0x1b9b82

// ctor: zero the valid flag, size and blob (id is left alone).
RVA(0x00150180, 0xd)
CDataBuffer::CDataBuffer() {
    m_loaded = 0;
    m_size = 0;
    m_data = 0;
}

// Reset: free the blob only if currently valid (tail-jmp to Free).
RVA(0x00150190, 0xb)
void CDataBuffer::Reset() {
    if (m_loaded != 0) {
        Free();
    }
}

// Set: (re)allocate a `size`-byte blob; record id on success.
RVA(0x001501a0, 0x44)
i32 CDataBuffer::Set(u32 size, i32 id) {
    if (m_data) {
        RezFree(m_data);
    }
    m_size = size;
    m_data = RezAlloc(size);
    if (!m_data) {
        return 0;
    }
    m_loaded = 1;
    m_id = id;
    return 1;
}

// ReadFrom: pull a 4-byte element count out of `file` (CFile::Read,
// vtable slot +0x3c), (re)allocate the blob via Set(count, id), then read the blob
// bytes through the same CFile::Read. Returns Set's result; on success re-stamps the
// valid flag + id and returns 1. __thiscall, ret 8.
RVA(0x001501f0, 0x54)
i32 CDataBuffer::ReadFrom(CFile* file, i32 id) {
    file->Read(&m_size, 4);
    if (Set(m_size, id) == 0) {
        return 0;
    }
    file->Read(m_data, m_size);
    m_loaded = 1;
    m_id = id;
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
i32 CDataBuffer::LoadFromFile(const char* path, i32 id) {
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
i32 CDataBuffer::LoadFromMem(void* buf, u32 len, i32 id) {
    CMemFile file(0x400);
    file.Attach((BYTE*)buf, len);
    return ReadFrom(&file, id);
}

// Free: if valid, free + clear the blob and size, then clear valid.
RVA(0x001503c0, 0x2e)
void CDataBuffer::Free() {
    if (m_loaded != 0) {
        if (m_data) {
            RezFree(m_data);
            m_data = 0;
        }
        m_size = 0;
    }
    m_loaded = 0;
}

// SaveToFile: create `path` (modeCreate|modeWrite), write the 4-byte
// element count then the blob, close. The by-value CString arg (destructed at exit,
// outermost cleanup) + the local CFile -> /GX EH frame; on an Open failure the CFile
// is destructed and 0 returned. ret 4 (the CString is one dword).
RVA(0x001503f0, 0xdc)
i32 CDataBuffer::SaveToFile(CString path) {
    CFile file;
    if (!file.Open(path, CFile::modeCreate | CFile::modeWrite, 0)) {
        return 0;
    }
    file.Write(&m_size, 4);
    file.Write(m_data, m_size);
    file.Close();
    return 1;
}
