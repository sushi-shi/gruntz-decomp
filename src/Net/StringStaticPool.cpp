#include <rva.h>

#include <Net/StringStaticPool.h>

#include <Mfc.h>

RVA_DYNINIT(0x000f96f0, 0xa, s_value)
RVA_DYNINIT(0x000f9710, 0xa, s_value)
RVA_DYNINIT(0x000f9730, 0xe, s_value)
RVA_DYNINIT(0x000f9750, 0x1f, s_value)
template<> DATA(0x0024e25c)
CString CStringStaticPool<CAssetRootTag>::s_value;
