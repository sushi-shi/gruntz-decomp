// DiscoveredArray.cpp - a trace-discovered MFC CArray<TYPE,ARG>::SetSize
// instantiation re-homed from src/Stub/Discovered.cpp (ClassUnknown_1 @0x150040).
// The element is a 4-byte POD (DWORD), so the grow/shrink/realloc paths inline
// memset/memcpy (rep stosd/movsd) around the engine operator new (0x1b9b46) /
// operator delete (0x1b9b82). Self-contained; owner class is unidentified.
#include <Ints.h>
#include <rva.h>
#include <string.h>

// Engine operator new / operator delete (cdecl; reloc-masked rel32).
extern "C" void* RezAlloc(u32 n); // 0x1b9b46
extern "C" void RezFree(void* p); // 0x1b9b82

// CArray layout: a CObject-style 4-byte head (+0x00, untouched by SetSize), then
// m_pData/m_nSize/m_nMaxSize/m_nGrowBy.
struct CDwArray {
    void* m_head;   // +0x00
    i32* m_pData;   // +0x04
    i32 m_nSize;    // +0x08
    i32 m_nMaxSize; // +0x0c
    i32 m_nGrowBy;  // +0x10
    void SetSize(i32 nNewSize, i32 nGrowBy);
};

// CDwArray::SetSize: the canonical MFC CArray::SetSize over a 4-byte POD
// element (twin of CArrayE40::SetSize @0x17f390 / m2_ArrayE40.cpp).
//
// @early-stop
// inline-memset-codegen + zero-register-pinning wall (docs/patterns/zero-register-
// pinning.md, topic:wall topic:regalloc): ~86%. Every op/offset/immediate/branch +
// the fresh-alloc and realloc memcpy/memset (byte-count rep stosd+stosb) are
// byte-faithful; the residual is (1) the WITHIN-CAPACITY zero-fill, where retail
// emits a pure element-count `rep stosd` (count = nNewSize-m_nSize) but cl emits the
// generic byte-count split (shl/shr + `rep stosb` remainder) for the same
// `memset(&m_pData[n], 0, k*sizeof(i32))`, and (2) the long-lived 0/null register
// pin cascade the 40-byte twin (CArrayE40, 96%) also hits. Not source-steerable
// (the memset element-vs-byte choice is an MSVC5 inline-expansion decision). Parked.
RVA(0x00150040, 0x136)
void CDwArray::SetSize(i32 nNewSize, i32 nGrowBy) {
    if (nGrowBy != -1) {
        m_nGrowBy = nGrowBy;
    }
    if (nNewSize == 0) {
        if (m_pData != 0) {
            RezFree(m_pData);
            m_pData = 0;
        }
        m_nSize = m_nMaxSize = 0;
    } else if (m_pData == 0) {
        m_pData = (i32*)RezAlloc(nNewSize * sizeof(i32));
        memset(m_pData, 0, nNewSize * sizeof(i32));
        m_nSize = m_nMaxSize = nNewSize;
    } else if (nNewSize <= m_nMaxSize) {
        if (nNewSize > m_nSize) {
            memset(&m_pData[m_nSize], 0, (nNewSize - m_nSize) * sizeof(i32));
        }
        m_nSize = nNewSize;
    } else {
        i32 grow = m_nGrowBy;
        if (grow == 0) {
            grow = m_nSize / 8;
            if (grow < 4) {
                grow = 4;
            } else if (grow > 1024) {
                grow = 1024;
            }
        }
        i32 nNewMax;
        if (nNewSize < m_nMaxSize + grow) {
            nNewMax = m_nMaxSize + grow;
        } else {
            nNewMax = nNewSize;
        }
        i32* pNewData = (i32*)RezAlloc(nNewMax * sizeof(i32));
        memcpy(pNewData, m_pData, m_nSize * sizeof(i32));
        memset(&pNewData[m_nSize], 0, (nNewSize - m_nSize) * sizeof(i32));
        RezFree(m_pData);
        m_pData = pNewData;
        m_nSize = nNewSize;
        m_nMaxSize = nNewMax;
    }
}

SIZE_UNKNOWN(CDwArray);

