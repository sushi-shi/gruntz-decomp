#ifndef GRUNTZ_GRUNTZ_CFADERMGR_H
#define GRUNTZ_GRUNTZ_CFADERMGR_H

#include <rva.h>

#include <Mfc.h>

#include <Gruntz/Fader.h>
#include <Ints.h>
#include <Wap32/Object.h>

#include <afxtempl.h>

GZ_ENUM_FORWARD(FaderKind);

typedef CArray<CFader*, CFader*> CFaderArray;

class CFaderMgr {
public:
    CFaderMgr();
    ~CFaderMgr();
    i32 SetDefaults(
        class CDDSurface* primary,
        class CDDSurface* secondary,
        class CDDrawDeviceManager* manager
    );
    void FreeAll();
    CFader* Add(FaderKind nFaderType, class CFaderConfig* pInit);

    void Remove(CFader* pFader);
    void DeleteAll();

    void SetTraceEnabled(b32 enabled);
    void Trace(CString s);

    class CDDSurface* m_primarySurface;
    class CDDSurface* m_secondarySurface;
    b32 m_active;
    b32 m_traceEnabled;
    CFaderArray m_arr;

    class CDDrawDeviceManager* m_deviceManager;
};

#endif // GRUNTZ_GRUNTZ_CFADERMGR_H
