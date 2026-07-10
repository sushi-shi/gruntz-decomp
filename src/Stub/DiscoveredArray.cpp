// DiscoveredArray.cpp - drained.
//
// Its sole function, CDwArray::SetSize (0x150040), was the out-of-line MFC
// CArray::SetSize for CShadeTableCache's DWORD table array. It has been re-homed to
// its real owner TU src/DDrawMgr/ShadeTableCache.cpp and DISSOLVED onto the canonical
// CShadeTableArray (as CShadeTableArray::SetSizeGrow, the declared-only 0x150040 slot;
// same 0x14 layout, the i32*/CShadeTable** element retype is matching-neutral). The
// placeholder CDwArray view is gone.
#include <rva.h>
