// ArraySerialize.cpp - CMovieScratch::Serialize (0x39fa0), the movie/creditz playlist
// array's MFC CArray<CMovieClip*>::Serialize (vtable slot 2 of ??_7?$CArray@... @0x1e971c,
// modeled as CMovieScratch). __thiscall, dispatched only through the MFC CArchive Serialize
// mechanism (reached via the uncalled ILT thunk 0x1e56 = CMovieScratch vtable slot 2), so it
// has no rel32 caller. Kept in its own TU (its retail COMDAT sits far from the movie cluster).
//
// The MFC CArray<T>::Serialize shape with SetSize inlined: when storing, write the element
// count then the raw block; when loading, read the count, (re)size the buffer (alloc /
// grow-with-copy / shrink-in-place per the MFC grow heuristic m_nSize/8 clamped to [4,0x400]),
// then read the raw block. The formerly-local ArcSer/TArray views are dissolved onto the
// canonical MfcArchive (the reloc-masked CArchive accessor) and CMovieScratch (the real
// array class in <Io/MoviePlayer.h>). Field names are placeholders; offsets + code bytes
// are the load-bearing fact.
#include <Ints.h>
#include <rva.h>
#include <string.h>          // memcpy/memset -> rep movs/stos at /O2 /Oi
#include <Wap32/MfcArchive.h> // MfcArchive - the reloc-masked CArchive transfer accessor
#include <Io/MoviePlayer.h>   // CMovieScratch (CArray<CMovieClip*>) + CArchive

// @early-stop
// regalloc/scheduling wall (~73%): the SetSize-inlined load/grow/shrink logic is
// byte-faithful, but MSVC pins this->ebx / arg->esi (mine lands ebp/edi) and
// interleaves the prologue push/mov differently; not source-steerable.
RVA(0x00039fa0, 0x188)
void CMovieScratch::Serialize(CArchive& ar) {
    MfcArchive* a = (MfcArchive*)&ar;
    if ((a->m_nMode & 1) == 0) {
        i32 n = a->ReadCount();
        if (n == 0) {
            if (m_pData != 0) {
                ::operator delete(m_pData);
                m_pData = 0;
            }
            m_nMaxSize = 0;
            m_nSize = 0;
        } else if (m_pData == 0) {
            m_pData = (CMovieClip**)::operator new(n * 4);
            memset(m_pData, 0, n * 4);
            m_nMaxSize = n;
            m_nSize = n;
        } else if (n <= m_nMaxSize) {
            if (n > m_nSize) {
                memset(m_pData + m_nSize, 0, (n - m_nSize) * 4);
            }
            m_nSize = n;
        } else {
            i32 grow = m_nGrowBy;
            if (grow == 0) {
                grow = m_nSize / 8;
                if (grow < 4) {
                    grow = 4;
                } else if (grow > 0x400) {
                    grow = 0x400;
                }
            }
            i32 newcap = m_nMaxSize + grow;
            if (n >= newcap) {
                newcap = n;
            }
            CMovieClip** nd = (CMovieClip**)::operator new(newcap * 4);
            memcpy(nd, m_pData, m_nSize * 4);
            memset(nd + m_nSize, 0, (n - m_nSize) * 4);
            ::operator delete(m_pData);
            m_pData = nd;
            m_nSize = n;
            m_nMaxSize = newcap;
        }
    } else {
        a->WriteCount(m_nSize);
    }

    if (a->m_nMode & 1) {
        a->WriteData(m_pData, m_nSize * 4);
    } else {
        a->ReadData(m_pData, m_nSize * 4);
    }
}
