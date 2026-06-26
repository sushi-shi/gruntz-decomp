// ArraySerialize.cpp - a typed growable array's Serialize (0x39fa0). __thiscall.
// The MFC CArray<DWORD>::Serialize shape with SetSize inlined: when the archive
// is storing, write the element count then the raw block; when loading, read the
// count, (re)size the buffer (alloc/grow-with-copy/shrink-in-place per the MFC
// grow heuristic m_size/8 clamped to [4,0x400]), then read the raw block. No
// relocs (all rel32 to the archive methods + global new/delete). Field names are
// placeholders; offsets + code bytes are the load-bearing fact.
#include <Ints.h>
#include <rva.h>
#include <string.h> // memcpy/memset -> rep movs/stos at /O2 /Oi

// The archive arg: flags @+0x14 (bit0 = IsStoring); count/raw transfer methods.
struct ArcSer {
    void WriteCount(u32 n);             // 0x1c7334  ar << (DWORD)
    i32 ReadCount();                    // 0x1c7362  ar >> (DWORD)
    void WriteData(void* p, i32 bytes); // 0x1c7168
    void ReadData(void* p, i32 bytes);  // 0x1c705a
    char p0[0x14];
    u32 m_14; // +0x14  archive flags (bit0 storing)
};

struct TArray {
    void Serialize(ArcSer* ar); // 0x39fa0
    char p0[0x4];
    void* m_data; // +0x04
    i32 m_size;   // +0x08
    i32 m_cap;    // +0x0c
    i32 m_growBy; // +0x10
};

// @early-stop
// regalloc/scheduling wall (~73%): the SetSize-inlined load/grow/shrink logic is
// byte-faithful, but MSVC pins this->ebx / arg->esi (mine lands ebp/edi) and
// interleaves the prologue push/mov differently; not source-steerable.
RVA(0x00039fa0, 0x188)
void TArray::Serialize(ArcSer* ar) {
    if ((ar->m_14 & 1) == 0) {
        i32 n = ar->ReadCount();
        if (n == 0) {
            if (m_data != 0) {
                ::operator delete(m_data);
                m_data = 0;
            }
            m_cap = 0;
            m_size = 0;
        } else if (m_data == 0) {
            m_data = ::operator new(n * 4);
            memset(m_data, 0, n * 4);
            m_cap = n;
            m_size = n;
        } else if (n <= m_cap) {
            if (n > m_size) {
                memset((i32*)m_data + m_size, 0, (n - m_size) * 4);
            }
            m_size = n;
        } else {
            i32 grow = m_growBy;
            if (grow == 0) {
                grow = m_size / 8;
                if (grow < 4)
                    grow = 4;
                else if (grow > 0x400)
                    grow = 0x400;
            }
            i32 newcap = m_cap + grow;
            if (n >= newcap)
                newcap = n;
            i32* nd = (i32*)::operator new(newcap * 4);
            memcpy(nd, m_data, m_size * 4);
            memset(nd + m_size, 0, (n - m_size) * 4);
            ::operator delete(m_data);
            m_data = nd;
            m_size = n;
            m_cap = newcap;
        }
    } else {
        ar->WriteCount(m_size);
    }

    if (ar->m_14 & 1)
        ar->WriteData(m_data, m_size * 4);
    else
        ar->ReadData(m_data, m_size * 4);
}
