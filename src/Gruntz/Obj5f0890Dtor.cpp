// Obj5f0890Dtor.cpp - 0x181720, the virtual destructor of the class whose vtable
// lives at 0x5f0890 (ClassWithUnknownVTable73). Stamp-first vptr, then free the six
// owned heap buffers (m_478, m_44, m_48, m_4c, m_488, m_48c) with operator delete,
// then chain to the base destructor at 0x17e4a0. The base subobject's non-trivial
// dtor gives the /GX unwind frame (state 0 -> -1 around the base-dtor call). The
// implicit vptr stamp reloc-masks against the retail 0x5f0890 vtable; operator
// delete and the base dtor are reloc-masked externs.
#include <Ints.h>
#include <rva.h>

void __cdecl operator delete(void* p); // ??3@YAXPAX@Z (0x1b9b82)

// The base class: a non-trivial virtual dtor (0x17e4a0) so the derived dtor chains
// to it and carries the unwind state.
class CObj5f0890Base {
public:
    virtual ~CObj5f0890Base(); // 0x17e4a0 (external, reloc-masked)
};

class CObj5f0890 : public CObj5f0890Base {
public:
    virtual ~CObj5f0890(); // 0x181720
    char m_pad04[0x44 - 0x04];
    void* m_44; // +0x44
    void* m_48; // +0x48
    void* m_4c; // +0x4c
    char m_pad50[0x478 - 0x50];
    void* m_478; // +0x478
    char m_pad47c[0x488 - 0x47c];
    void* m_488; // +0x488
    void* m_48c; // +0x48c
};

// ---------------------------------------------------------------------------
RVA(0x00181720, 0xb3)
CObj5f0890::~CObj5f0890() {
    if (m_478) {
        operator delete(m_478);
    }
    if (m_44) {
        operator delete(m_44);
    }
    if (m_48) {
        operator delete(m_48);
    }
    if (m_4c) {
        operator delete(m_4c);
    }
    if (m_488) {
        operator delete(m_488);
    }
    if (m_48c) {
        operator delete(m_48c);
    }
}
