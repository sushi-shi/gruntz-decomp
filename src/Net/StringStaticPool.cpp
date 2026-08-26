#include <rva.h>

#include <Net/StringStaticPool.h>

#include <Mfc.h>

RVA_DYNINIT(0x000f9820, 0xa, s_value)
RVA_DYNINIT(0x000f9840, 0xa, s_value)
RVA_DYNINIT(0x000f9860, 0xe, s_value)
RVA_DYNINIT(0x000f9880, 0x1f, s_value)
template<> DATA(0x0024f1b4)
CString CStringStaticPool<CAssetRootTag>::s_value;
