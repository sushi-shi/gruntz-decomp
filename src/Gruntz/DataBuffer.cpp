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

// 0x150180 - ctor: zero the valid flag, size and blob (id is left alone).
RVA(0x00150180, 0xd)
CDataBuffer::CDataBuffer() {
    m_00 = 0;
    m_04 = 0;
    m_08 = 0;
}

// 0x150190 - Reset: free the blob only if currently valid (tail-jmp to Free).
RVA(0x00150190, 0xb)
void CDataBuffer::Reset() {
    if (m_00 != 0) {
        Free();
    }
}

// 0x1501a0 - Set: (re)allocate a `size`-byte blob; record id on success.
RVA(0x001501a0, 0x44)
i32 CDataBuffer::Set(u32 size, i32 id) {
    if (m_08) {
        RezFree(m_08);
    }
    m_04 = size;
    m_08 = RezAlloc(size);
    if (!m_08) {
        return 0;
    }
    m_00 = 1;
    m_0c = id;
    return 1;
}

// 0x1503c0 - Free: if valid, free + clear the blob and size, then clear valid.
RVA(0x001503c0, 0x2e)
void CDataBuffer::Free() {
    if (m_00 != 0) {
        if (m_08) {
            RezFree(m_08);
            m_08 = 0;
        }
        m_04 = 0;
    }
    m_00 = 0;
}
