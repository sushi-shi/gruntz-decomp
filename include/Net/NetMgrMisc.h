// NetMgrMisc.h - the NetMgrMisc.cpp TU's exported globals/functions.
#ifndef GRUNTZ_NET_NETMGRMISC_H
#define GRUNTZ_NET_NETMGRMISC_H

class CString;

// @identity-TODO
// Retail's four compiler-private static-object helpers prove that the asset-root
// CString is an explicit specialization of a template static data member. The
// stripped image does not preserve the original template, tag, or member names.
template<class Tag> struct CStringStaticPool {
    static CString s_value;
};

struct CAssetRootTag;
typedef CStringStaticPool<CAssetRootTag> CAssetRootStorage;

#endif // GRUNTZ_NET_NETMGRMISC_H
