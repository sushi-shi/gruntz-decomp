// m2_ArrayE40.cpp - an MFC CArray<TYPE,ARG>::SetSize instantiation for a 40-byte
// element type (C:\Proj\Gruntz). The element is POD (ConstructElements = zero-fill,
// DestructElements = no-op), so the grow/shrink/realloc paths inline memset/memcpy
// (rep stos / rep movs) around the engine operator new (0x1b9b46) / operator delete
// (0x1b9b82). Self-contained; owner class is unidentified (best-guess naming).
#include <Ints.h>
#include <rva.h>
#include <string.h>

#include <Rez/RezMgr.h> // RezAlloc (_RezAlloc 0x1b9b46) / RezFree (_RezFree 0x1b9b82)

struct E40 {
    char m_b[40];
};

// CArray layout: a CObject-style 4-byte head (+0x00, untouched by SetSize), then
// m_pData/m_nSize/m_nMaxSize/m_nGrowBy.
struct CArrayE40 {
    void* m_head;   // +0x00
    E40* m_pData;   // +0x04
    i32 m_nSize;    // +0x08
    i32 m_nMaxSize; // +0x0c
    i32 m_nGrowBy;  // +0x10
    void SetSize(i32 nNewSize, i32 nGrowBy);
};

// CArrayE40::SetSize: the canonical MFC CArray::SetSize over a 40-byte
// POD element.
//
// @early-stop
// zero-register-pinning wall (docs/patterns/zero-register-pinning.md, topic:wall
// topic:regalloc): ~96%, every operation/offset/immediate/branch + the store
// ordering (m_nSize=m_nMaxSize=0) is byte-faithful; the sole residual is the long-
// lived "0"/null register choice - retail pins it in esi (then reloads esi as the
// memcpy src), cl pins it in edi, which cascades into the edx/ecx scratch picks in
// the realloc lea chains. Not source-steerable. Deferred to the final sweep.
RVA(0x0017f390, 0x164)
void CArrayE40::SetSize(i32 nNewSize, i32 nGrowBy) {
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
        m_pData = (E40*)RezAlloc(nNewSize * sizeof(E40));
        memset(m_pData, 0, nNewSize * sizeof(E40));
        m_nSize = m_nMaxSize = nNewSize;
    } else if (nNewSize <= m_nMaxSize) {
        if (nNewSize > m_nSize) {
            memset(&m_pData[m_nSize], 0, (nNewSize - m_nSize) * sizeof(E40));
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
        E40* pNewData = (E40*)RezAlloc(nNewMax * sizeof(E40));
        memcpy(pNewData, m_pData, m_nSize * sizeof(E40));
        memset(&pNewData[m_nSize], 0, (nNewSize - m_nSize) * sizeof(E40));
        RezFree(m_pData);
        m_pData = pNewData;
        m_nSize = nNewSize;
        m_nMaxSize = nNewMax;
    }
}

SIZE_UNKNOWN(CArrayE40);
SIZE_UNKNOWN(E40);
