// GruntNames.cpp - the named-grunt string table builder (free fn).
// BuildNamedGruntTable @0x0c16b0 (61 B): constructs four global AfxString objects
// with the named grunt leader strings: "Beefy", "REDITZ", "Serra", "Jebediah".
// These globals are at 0x64bdb0/0x64bdb4/0x64bdb8/0x64bdbc (each 4 B).
// Plain /O2 /MT (no /GX): no stack C++ object / EH frame.

#include "../Wap32/Wap32.h"

// The MFC CString (AfxString) - a single char* pointer. The const-char* ctor
// at @0x1b9d4c is used; the NAFXCW CString::operator=(const char*) @0x1b9d4c
// is at the same address - reloc-masked either way.
class AfxString {
public:
    AfxString(const char *src);          // @0x1b9d4c (or operator=)
private:
    char *m_pchData;
};

// The four global CString objects (each 4 B packed at 4 B stride, VA 0x64bdb0).
// @data: 0x24bdb0
extern AfxString g_namedGrunt_00;
// @data: 0x24bdb4
extern AfxString g_namedGrunt_01;
// @data: 0x24bdb8
extern AfxString g_namedGrunt_02;
// @data: 0x24bdbc
extern AfxString g_namedGrunt_03;

// ---------------------------------------------------------------------------
// BuildNamedGruntTable  @ RVA 0x0c16b0  (61 B, __cdecl void)
//
// One-shot builder that assigns the four named-grunt strings to the global
// CString table. Each assignment is:
//   push str_literal
//   mov ecx, &g_namedGrunt_N
//   call AfxString::operator=(const char*)  (or ctor, same addr @0x1b9d4c)
//
// @address: 0x0c16b0
// @size:    0x3d
// ---------------------------------------------------------------------------
void BuildNamedGruntTable()
{
    g_namedGrunt_00 = "Beefy";
    g_namedGrunt_01 = "REDITZ";
    g_namedGrunt_02 = "Serra";
    g_namedGrunt_03 = "Jebediah";
}
