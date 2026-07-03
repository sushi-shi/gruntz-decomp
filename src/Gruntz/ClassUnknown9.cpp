// ClassUnknown9.cpp - 0x11c630 is the ClassUnknown_9 constructor taking a CString
// by value (+ a pointer): the CString
// member m_str is default-constructed (member-init, 0x1b9b93), assigned from the
// by-value arg (operator=, 0x1b9e25), then m_4/m_8 are set; the by-value CString
// parameter is destroyed by the callee at return (0x1b9cde) -> /GX EH frame.
// MFC CString ctor/operator=/dtor are external (reloc-masked); flags=eh.
#include <rva.h>
#include <Mfc.h>

SIZE_UNKNOWN(ClassUnknown_9);
class ClassUnknown_9 {
public:
    CString m_str; // 0x0
    int m_4;       // 0x4
    void* m_8;     // 0x8
    ClassUnknown_9(CString name, void* p);
};

RVA(0x0011c630, 0x6e)
ClassUnknown_9::ClassUnknown_9(CString name, void* p) {
    m_str = name;
    m_4 = 0;
    m_8 = p;
}
