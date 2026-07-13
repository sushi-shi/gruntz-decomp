// DDrawBltErrThunk.cpp - a DDrawMgr-family display helper (C:\Proj\incs\ddrawmgr.h)
// re-homed from src/Stub/DirectDrawMgr.cpp (the stub mangling mislabeled it
// CDirectDrawMgr; by its disasm it is a different game-display class).
//
// It Blt's two RECTs through a held polymorphic surface object (m_0): the COM-ABI
// virtual at vtable slot 11 (offset 0x2c) is invoked __stdcall (the object is the
// hidden first stack arg) with the two embedded RECTs (&m_8, &m_184), and on a
// nonzero HRESULT logs it through DirSurfLog("c:\proj\incs\ddrawmgr.h", 0x135, hr).
// Returns the HRESULT. Only the +0x00/+0x08/+0x184 offsets + the slot-0x2c COM
// call shape are load-bearing; names are placeholders.
#include <rva.h>

#include <Mfc.h>   // afx-first umbrella (DDrawPtrCollections.h needs MFC CPtrList) + windows.h
#include <ddraw.h> // real IDirectDraw2 (GetCaps @slot 11) + LPDDCAPS
#include <DDrawMgr/DirectDrawMgr.h> // CDirectDrawMgr::GetErrorString (the DDraw error reporter)
#include <DDrawMgr/DDrawPtrCollections.h> // the canonical owner (ex the DDrawBltHost view)
#include <Ints.h>

// [SETTLED (Fable lane, 2026-07-13): the "DDrawBltHost/DDrawBltSurface" views WERE
// CDDrawPtrCollections + its held IDirectDraw2 (+0x00 m_surf0): the "2-RECT Blt at
// slot 11" is IDirectDraw2::GetCaps (slot 11, +0x2c, __stdcall, TWO LPDDCAPS args)
// and the "+0x08 dest RECT" is the 0x17c-byte driver DDCAPS block (+0x184 = HEL).
// The 12-slot placeholder interface is dissolved onto the real COM vtable.]

// Query the device caps into the embedded driver/HEL blocks; log a nonzero HRESULT
// through the DDraw error reporter CDirectDrawMgr::GetErrorString (0x141400).
RVA(0x0008dd80, 0x31)
i32 CDDrawPtrCollections::GetCapsChecked() {
    i32 hr = m_surf0->GetCaps((LPDDCAPS)m_driverCaps, (LPDDCAPS)m_helCaps);
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString((char*)"c:\\proj\\incs\\ddrawmgr.h", 0x135, hr);
    }
    return hr;
}

// --- vtable catalog ---
