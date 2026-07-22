#include <rva.h>

#include <Mfc.h>   // afx-first umbrella (DDrawPtrCollections.h needs MFC CPtrList) + windows.h
#include <ddraw.h> // real IDirectDraw2 (GetCaps @slot 11) + LPDDCAPS
#include <DDrawMgr/DirectDrawMgr.h> // CDDrawPtrCollections::GetErrorString (the DDraw error reporter)
#include <DDrawMgr/DDrawPtrCollections.h> // the canonical owner (ex the DDrawBltHost view)
#include <Ints.h>

RVA(0x0008dd80, 0x31)
i32 CDDrawPtrCollections::GetCapsChecked() {
    i32 hr = m_device->GetCaps(reinterpret_cast<LPDDCAPS>(m_driverCaps), reinterpret_cast<LPDDCAPS>(m_helCaps));
    if (hr != 0) {
        CDDrawPtrCollections::GetErrorString(const_cast<char*>("c:\\proj\\incs\\ddrawmgr.h"), 0x135, hr);
    }
    return hr;
}
