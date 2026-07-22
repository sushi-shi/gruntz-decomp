#ifndef GRUNTZ_GRUNTZ_CFADERMGR_H
#define GRUNTZ_GRUNTZ_CFADERMGR_H

#include <Ints.h>
#include <rva.h>
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
    CFaderMgr();                                           // 0x17d8f0
    ~CFaderMgr();                                          // 0x17d910
    i32 SetConfig(i32 a, i32 b, i32 c);                    // 0x17d980
    void FreeAll();                                        // 0x17d9a0
    CFader* Add(i32 nFaderType, class CFxModeDesc* pInit); // 0x17d9c0 (pInit = the
                                                           // per-type transition descriptor)
    RVA(0x0017e160, 0x8)
    i32 Flush() {
        return (reinterpret_cast<CFaderMgr*>(&m_sharedSet2cArg))->Flush();
    }
    void Remove(CFader* pFader); // 0x17e170
    void DeleteAll();            // 0x17e1d0

    i32 m_timerArgA;      // +0x00
    i32 m_timerArgB;      // +0x04
    i32 m_active;         // +0x08
    i32 m_0c;             // +0x0c
    CFaderArray m_arr;    // +0x10 element array subobject
    i32 m_sharedSet2cArg; // +0x24
};
SIZE_UNKNOWN();

#endif // GRUNTZ_GRUNTZ_CFADERMGR_H
