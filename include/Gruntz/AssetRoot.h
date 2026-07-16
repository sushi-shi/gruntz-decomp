// AssetRoot.h - the global game asset-root path CString (0x64e25c). Set by
// CGruntzMgr::SetAssetRoot (0x92060) and consumed by the title/splash sequence.
// The .bss datum is constructed/destructed by explicit-ctor/dtor thunks; its single
// DATA binding lives in src/Net/NetMgrMisc.cpp. Declared here so consumers reference
// it from one owner header instead of per-TU externs (each of which carried its own
// redundant DATA pin at the same RVA).
#ifndef GRUNTZ_ASSETROOT_H
#define GRUNTZ_ASSETROOT_H

#include <Gruntz/String.h> // MFC CString

extern CString g_assetRoot; // 0x64e25c

#endif // GRUNTZ_ASSETROOT_H
