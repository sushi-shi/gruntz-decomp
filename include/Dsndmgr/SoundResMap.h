#ifndef GRUNTZ_SOUNDRESMAP_H
#define GRUNTZ_SOUNDRESMAP_H

#include <Ints.h>
#include <rva.h>

#include <Mfc.h>          // CMapStringToPtr / CString / POSITION + <windows.h>
#include <Wap32/Object.h> // CObject - the MFC-free WAP grand-base (namespace-qualified)

SIZE_UNKNOWN(CSoundRes);
class CSoundRes : public CObject {
public:
    virtual ~CSoundRes() OVERRIDE; // slot 1 (scalar-deleting dtor)
};

SIZE_UNKNOWN(CSoundResMap);
class CSoundResMap {
public:
    void RemoveByValue(CSoundRes* p); // 0x157b00

    char m_00[0x10];       // +0x00 (preceding state, not touched here)
    CMapStringToPtr m_map; // +0x10
};

#endif // GRUNTZ_SOUNDRESMAP_H
