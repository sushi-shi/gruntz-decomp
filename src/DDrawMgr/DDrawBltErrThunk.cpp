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

#include <Ints.h>

// The held surface/display object: COM-ABI, so its virtuals are __stdcall with the
// interface as the hidden first arg. Slot 11 (offset 0x2c) is the 2-RECT Blt.
struct DDrawBltSurface {
    virtual i32 __stdcall s00();
    virtual i32 __stdcall s01();
    virtual i32 __stdcall s02();
    virtual i32 __stdcall s03();
    virtual i32 __stdcall s04();
    virtual i32 __stdcall s05();
    virtual i32 __stdcall s06();
    virtual i32 __stdcall s07();
    virtual i32 __stdcall s08();
    virtual i32 __stdcall s09();
    virtual i32 __stdcall s10();
    virtual i32 __stdcall Blt(void* dstRect, void* srcRect); // slot 11 (+0x2c)
};

// The display helper that owns the surface at +0 and the two RECTs at +8 / +0x184.
struct DDrawBltHost {
    i32 BltChecked();
    DDrawBltSurface* m_0;    // +0x00  held surface object
    char m_pad04[0x8 - 0x4]; //
    char m_8[0x184 - 0x8];   // +0x08  destination RECT (passed as &m_8)
    char m_184[16];          // +0x184 source RECT (passed as &m_184)
};

// The DDraw error sink (src/DDrawMgr/DirectDrawMgr.cpp @0x141400): __cdecl
// (file, line, hr); call displacement reloc-masked.
extern void __cdecl DirSurfLog(const char* file, i32 line, i32 hr);

// Blt the two RECTs through the held surface; log a nonzero HRESULT.
RVA(0x0008dd80, 0x31)
i32 DDrawBltHost::BltChecked() {
    i32 hr = m_0->Blt(&m_8, &m_184);
    if (hr != 0) {
        DirSurfLog("c:\\proj\\incs\\ddrawmgr.h", 0x135, hr);
    }
    return hr;
}

SIZE_UNKNOWN(DDrawBltSurface);
SIZE_UNKNOWN(DDrawBltHost);

// --- vtable catalog ---
