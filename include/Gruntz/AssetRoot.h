#ifndef GRUNTZ_ASSETROOT_H
#define GRUNTZ_ASSETROOT_H

template<class Tag> struct CStringStaticPool {
    static CString s_value;
};

struct CAssetRootTag;
typedef CStringStaticPool<CAssetRootTag> CAssetRootStorage;

#endif
