// CFaderMgr.h - the Gruntz screen-fader manager (tracer placeholder
// ClassUnknown_48). A polymorphic owner of a growable CPtrArray of CFader*
// objects. CFaderMgr::Add(nFaderType, pInit) is a 7-way factory: it allocates
// one of six CFader subclasses by type code, primes it from the manager's
// shared timing fields (m_timerArgA/m_timerArgB -> SetTimers, m_sharedSet2cArg -> Set2c), default-inits or
// copy-inits it from pInit, validates it, and appends it to the array - tracing
// "CFaderMgr::Add (...) - ..." and deleting the fader on any failure.
//
// Field names are placeholders (m_<hexoffset>); only offsets + code bytes are
// load-bearing. The class carries two shared timer fields m_timerArgA/m_timerArgB
// (+0x00/+0x04) and m_active (+0x08), then the embedded polymorphic element-array subobject m_arr
// (+0x10): vtable at +0x10, m_pData at +0x14, m_nSize at +0x18, m_nMaxSize at
// +0x1c, m_nGrowBy at +0x20; followed by m_sharedSet2cArg (+0x24). The destructor inlines the
// array subobject teardown (restore array vtable, free m_pData, restore grand-base
// vtable) under the /GX EH frame. The CFader subclass ctors/init/validate helpers,
// operator new/delete, and CString helpers are external/reloc-masked.
#ifndef GRUNTZ_GRUNTZ_CFADERMGR_H
#define GRUNTZ_GRUNTZ_CFADERMGR_H

#include <Ints.h>
#include <rva.h>
#include <Wap32/CObject.h> // Wap::CObject - the shared CObject-like grand-base

// The array element. Polymorphic: slot 0 of its vtable is the scalar-deleting
// destructor, invoked as `pFader->Delete(1)` to free a fader (__thiscall virtual
// dispatch: ecx=pFader, one stack arg). Declared with a pure virtual so the call
// lowers to a vtable[0] dispatch with no vtable emitted here. The six concrete
// subclass ctors/methods live in sibling TUs (reloc-masked).
struct CFader {
    virtual void* Delete(i32 flags) = 0; // slot 0 (vptr at +0x00)
};

// The growable element-array subobject (lives at manager +0x10). A Wap::CObject-derived
// polymorphic node: its own vftable (@0x5f0790, uncatalogued -> unpaired ??_7CFaderArray)
// overrides the grand-base dtor (slot 1, retail 0x17e430) and slot 2 (retail 0x17e2a0);
// slots 0/3/4 come from Wap::CObject via inheritance. cl stamps ??_7CFaderArray vptr-first
// in the ctor and folds the ~Wap::CObject grand-base restamp (masks 0x5e8cb4) into the
// dtor - no manual stamp. Layout mirrors a CPtrArray: m_pData(+0x04), m_nSize(+0x08),
// m_nMaxSize(+0x0c), m_nGrowBy(+0x10). Both ctor/dtor are inlined - as member subobject
// ctor/dtor - into CFaderMgr's ctor/dtor (the dtor's /GX EH frame comes from the
// member teardown). The grow logic (SetAtGrow) is inlined by Add.
struct CFaderArray : public Wap::CObject {
    virtual ~CFaderArray() OVERRIDE;      // slot 1 (retail dtor 0x17e430)
    virtual void FUN_004028ec() OVERRIDE; // slot 2 (retail 0x17e2a0)

    CFader** m_pData; // +0x04 (manager +0x14)
    i32 m_nSize;      // +0x08 (manager +0x18)
    i32 m_nMaxSize;   // +0x0c (manager +0x1c)
    i32 m_nGrowBy;    // +0x10 (manager +0x20)

    CFaderArray();
};

// cl stamps ??_7CFaderArray vptr-first, then zero the bookkeeping fields in
// declaration-store order pData/growby/maxsize/size. Inlined into CFaderMgr's ctor.
inline CFaderArray::CFaderArray() {
    m_pData = 0;
    m_nGrowBy = 0;
    m_nMaxSize = 0;
    m_nSize = 0;
}

// Free m_pData; cl folds the own vptr stamp (entry) + the ~Wap::CObject grand-base
// restamp (masks 0x5e8cb4) around it.
inline CFaderArray::~CFaderArray() {
    if (m_pData) {
        operator delete(m_pData);
    }
}

class CFaderMgr {
public:
    CFaderMgr();                                // 0x17d8f0
    ~CFaderMgr();                               // 0x17d910
    i32 SetConfig(i32 a, i32 b, i32 c);         // 0x17d980
    void FreeAll();                             // 0x17d9a0
    CFader* Add(i32 nFaderType, CFader* pInit); // 0x17d9c0
    i32 Flush();                                // 0x17e160
    void Remove(CFader* pFader);                // 0x17e170
    void DeleteAll();                           // 0x17e1d0

    i32 m_timerArgA;      // +0x00
    i32 m_timerArgB;      // +0x04
    i32 m_active;         // +0x08
    i32 m_0c;             // +0x0c
    CFaderArray m_arr;    // +0x10 element array subobject
    i32 m_sharedSet2cArg; // +0x24
};

#endif // GRUNTZ_GRUNTZ_CFADERMGR_H
