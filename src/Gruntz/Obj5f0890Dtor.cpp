// Obj5f0890Dtor.cpp - 0x181720, CFader1816c0::~CFader1816c0: the virtual destructor
// of the screen-fader subtype whose vftable is at 0x5f0890 (== ??_7CFader1816c0@@6B@,
// confirmed by the entry vptr-stamp reloc). Stamp-first vptr, then free the six owned
// heap buffers (m_478, m_44, m_48, m_4c, m_488, m_48c) with operator delete, then chain
// to the CFader base destructor at 0x17e4a0. The CFader base subobject's non-trivial
// virtual dtor gives the /GX unwind frame (state 0 -> -1 around the base-dtor call). The
// implicit vptr stamp reloc-masks against the retail 0x5f0890 vtable; operator delete and
// the base dtor are reloc-masked externs.
//
// (Was a standalone placeholder CObj5f0890 : CObj5f0890Base; folded onto the real
// CFader1816c0 - identical field layout, and CObj5f0890Base was CFader (both base dtor
// 0x17e4a0). The dtor now carries the correct ??_7CFader1816c0 name.)
#include <Gruntz/CFaderSubtypes.h> // the real CFader1816c0 (: public CFader)
#include <rva.h>

void __cdecl operator delete(void* p); // ??3@YAXPAX@Z (0x1b9b82)

// ---------------------------------------------------------------------------
RVA(0x00181720, 0xb3)
CFader1816c0::~CFader1816c0() {
    if (m_478) {
        operator delete((void*)m_478);
    }
    if (m_44) {
        operator delete((void*)m_44);
    }
    if (m_48) {
        operator delete((void*)m_48);
    }
    if (m_4c) {
        operator delete((void*)m_4c);
    }
    if (m_488) {
        operator delete((void*)m_488);
    }
    if (m_48c) {
        operator delete((void*)m_48c);
    }
}
