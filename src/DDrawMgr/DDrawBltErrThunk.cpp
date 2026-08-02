#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawPtrCollections.h>
#include <DDrawMgr/DirectDrawMgr.h>
#include <Ints.h>

#include <ddraw.h>

RVA(0x0008dd80, 0x31)
i32 CDDrawPtrCollections::GetCapsChecked() {
    i32 hr = m_device->GetCaps(&m_driverCaps, &m_helCaps);
    if (hr != 0) {
        CDDrawPtrCollections::GetErrorString(
            const_cast<char*>("c:\\proj\\incs\\ddrawmgr.h"),
            0x135,
            hr
        );
    }
    return hr;
}
