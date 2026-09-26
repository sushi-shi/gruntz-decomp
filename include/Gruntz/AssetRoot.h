#ifndef GRUNTZ_ASSETROOT_H
#define GRUNTZ_ASSETROOT_H

// @identity-TODO
// Only static-storage helpers survive; an original mangled symbol or debug record
// would be needed to recover the template, tag, and member names.
template<class Tag> struct CStringStaticPool {
    static CString s_value;
};

struct CAssetRootTag;
typedef CStringStaticPool<CAssetRootTag> CAssetRootStorage;

#endif // GRUNTZ_ASSETROOT_H
