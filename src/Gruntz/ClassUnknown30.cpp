// ClassUnknown30.cpp - ClassUnknown_30::GetStr (0x09a260, __thiscall) returns the
// leading CString member by value (NRVO into the hidden return slot): a single
// CString copy-ctor (0x1b9ba3) from m_str into *retptr. The MFC copy-ctor is
// external (reloc-masked).
#include <rva.h>
#include <Mfc.h>

class ClassUnknown_30 {
public:
    CString m_str; // offset 0x0
    CString GetStr_09a260();
};

// @early-stop
// 66.7%: returns the leading CString member by value (single copy-ctor 0x1b9ba3
// into the NRVO return slot) - logic correct. Retail reserves+zeroes one dead
// 4-byte stack local (push ecx; mov [esp+8],0; pop ecx) that no return-by-value
// spelling reproduces under MSVC5 /O2 (DCE'd here); unreproduced codegen artifact.
RVA(0x0009a260, 0x1d)
CString ClassUnknown_30::GetStr_09a260() {
    return m_str;
}
