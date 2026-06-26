// CFaderMgr.h - the Gruntz screen-fader manager (tracer placeholder
// ClassUnknown_48). A polymorphic owner of a growable CPtrArray of CFader*
// objects. CFaderMgr::Add(nFaderType, pInit) is a 7-way factory: it allocates
// one of six CFader subclasses by type code, primes it from the manager's
// shared timing fields (m_00/m_04 -> SetTimers, m_24 -> Set2c), default-inits or
// copy-inits it from pInit, validates it, and appends it to the array - tracing
// "CFaderMgr::Add (...) - ..." and deleting the fader on any failure.
//
// Field names are placeholders (m_<hexoffset>); only offsets + code bytes are
// load-bearing. The class carries two shared timer fields m_00/m_04 (+0x00/+0x04)
// and m_08 (+0x08), then the embedded polymorphic element-array subobject m_arr
// (+0x10): vtable at +0x10, m_pData at +0x14, m_nSize at +0x18, m_nMaxSize at
// +0x1c, m_nGrowBy at +0x20; followed by m_24 (+0x24). The destructor inlines the
// array subobject teardown (restore array vtable, free m_pData, restore grand-base
// vtable) under the /GX EH frame. The CFader subclass ctors/init/validate helpers,
// operator new/delete, and CString helpers are external/reloc-masked.
#ifndef GRUNTZ_GRUNTZ_CFADERMGR_H
#define GRUNTZ_GRUNTZ_CFADERMGR_H

#include <Ints.h>
#include <rva.h>

// The array element. Polymorphic: slot 0 of its vtable is the scalar-deleting
// destructor, invoked as `pFader->Delete(1)` to free a fader (__thiscall virtual
// dispatch: ecx=pFader, one stack arg). Declared with a pure virtual so the call
// lowers to a vtable[0] dispatch with no vtable emitted here. The six concrete
// subclass ctors/methods live in sibling TUs (reloc-masked).
struct CFader {
    virtual void* Delete(i32 flags) = 0; // slot 0 (vptr at +0x00)
};

// The growable element-array subobject (lives at manager +0x10). Polymorphic:
// its own vftable is at +0x00. Layout mirrors a CPtrArray: m_pData(+0x04),
// m_nSize(+0x08), m_nMaxSize(+0x0c), m_nGrowBy(+0x10). Its destructor (restore
// vtable, free m_pData, restore grand-base vtable) is inlined - as a member dtor -
// into the manager dtor, which is what gives that dtor its /GX EH frame. Its grow
// logic (SetAtGrow) is inlined by Add.
struct CFaderArray {
    void* m_vtbl;     // +0x00 (manager +0x10)
    CFader** m_pData; // +0x04 (manager +0x14)
    i32 m_nSize;      // +0x08 (manager +0x18)
    i32 m_nMaxSize;   // +0x0c (manager +0x1c)
    i32 m_nGrowBy;    // +0x10 (manager +0x20)

    ~CFaderArray();
};

// Foreign vftables stamped by the array teardown (reloc-masked DIR32 data).
DATA(0x001f0790)
extern void* g_faderArrayVtbl; // 0x5f0790 - the element-array base vtable
DATA(0x001e8cb4)
extern void* g_remusBaseDtorVtbl; // 0x5e8cb4 - the grand-base dtor vtable

inline CFaderArray::~CFaderArray() {
    m_vtbl = &g_faderArrayVtbl;
    if (m_pData) {
        operator delete(m_pData);
    }
    m_vtbl = &g_remusBaseDtorVtbl;
}

class CFaderMgr {
public:
    ~CFaderMgr();                               // 0x17d910
    i32 SetConfig(i32 a, i32 b, i32 c);         // 0x17d980
    void FreeAll();                             // 0x17d9a0
    CFader* Add(i32 nFaderType, CFader* pInit); // 0x17d9c0
    i32 Flush();                                // 0x17e160
    void Remove(CFader* pFader);                // 0x17e170
    void DeleteAll();                           // 0x17e1d0

    i32 m_00;          // +0x00 shared timer arg A
    i32 m_04;          // +0x04 shared timer arg B
    i32 m_08;          // +0x08
    i32 m_0c;          // +0x0c
    CFaderArray m_arr; // +0x10 element array subobject
    i32 m_24;          // +0x24 shared Set2c arg
};

#endif // GRUNTZ_GRUNTZ_CFADERMGR_H
