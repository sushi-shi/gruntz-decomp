#ifndef GRUNTZ_GRUNTZ_CFADERMGR_H
#define GRUNTZ_GRUNTZ_CFADERMGR_H

#include <Ints.h>
#include <rva.h>
#include <Mfc.h>          // CString (the Trace sink takes one by value) - afx-first
#include <Wap32/Object.h> // CObject - the shared CObject-like grand-base
#include <Gruntz/Fader.h> // the real polymorphic CFader element base (virtual ~CFader)

struct CFaderArray : public CObject {
    virtual ~CFaderArray() OVERRIDE;               // slot 1 (retail dtor 0x17e430)
    virtual void Serialize(CArchive& ar) OVERRIDE; // slot 2 (0x17e2a0, declared-only)

    CFader** m_pData; // +0x04 (manager +0x14)
    i32 m_nSize;      // +0x08 (manager +0x18)
    i32 m_nMaxSize;   // +0x0c (manager +0x1c)
    i32 m_nGrowBy;    // +0x10 (manager +0x20)

    CFaderArray();
};
SIZE_UNKNOWN();

inline CFaderArray::CFaderArray() {
    m_pData = 0;
    m_nGrowBy = 0;
    m_nMaxSize = 0;
    m_nSize = 0;
}

inline CFaderArray::~CFaderArray() {
    if (m_pData) {
        ::operator delete(m_pData);
    }
}

class CFaderMgr {
public:
    CFaderMgr();  // 0x17d8f0
    ~CFaderMgr(); // 0x17d910
    i32 SetConfig(
        class CDDSurface* src,
        class CDDSurface* dst,
        class CDDrawPtrCollections* pool
    );                                                     // 0x17d980
    void FreeAll();                                        // 0x17d9a0
    CFader* Add(i32 nFaderType, class CFxModeDesc* pInit); // 0x17d9c0 (pInit = the
    // per-type transition descriptor)
    void Remove(CFader* pFader); // 0x17e170
    void DeleteAll();            // 0x17e1d0
    // 0x17e230 - the release-build TRACE sink: an empty __thiscall member whose only
    // emitted code is the by-value CString argument's destructor. It is a MEMBER, not a
    // free function: every Add call site sets `mov ecx,<this>` right before the call.
    void Trace(CString s);

    class CDDSurface* m_timerArgA; // +0x00  default source surface handed to each fader
    class CDDSurface* m_timerArgB; // +0x04  default dest surface
    i32 m_active;                  // +0x08
    i32 m_0c;                      // +0x0c
    CFaderArray m_arr;             // +0x10 element array subobject
    // +0x24  the DirectDraw manager handed to every fader's Set2c (retail binds
    // 0 - see CFader::m_ptrColl). Ex the i32 "m_sharedSet2cArg".
    class CDDrawPtrCollections* m_sharedPtrColl;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_GRUNTZ_CFADERMGR_H
