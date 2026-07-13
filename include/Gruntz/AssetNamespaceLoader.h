// AssetNamespaceLoader.h - CNamespaceLoader, the asset-namespace prefix builder
// (m_curState's +0x2c/+0x70 registrar). Promoted from AssetNamespacePrefixes.cpp so
// the reduced per-TU register-views can fold onto it.
#ifndef GRUNTZ_GRUNTZ_ASSETNAMESPACELOADER_H
#define GRUNTZ_GRUNTZ_ASSETNAMESPACELOADER_H

#include <Mfc.h>
#include <Ints.h>
#include <rva.h>

struct CSpriteFactoryHolder; // +0x0c: the CState::m_c holder (GameRegistry.h) - the
                             // former `AssetRoot` view of it is dissolved
class CSymTab;

// NB CNamespaceLoader is itself a FACET of CState/CPlay (its +0x0c is CState::m_c
// and its +0x30 is CState::m_gruntzBank; every caller casts a CPlay `this` to it).
// The full fold onto CState is deferred (the bound 0xdca70 symbol + cross-TU
// callers move together); flagged 2026-07-13.
SIZE_UNKNOWN(CNamespaceLoader);
class CNamespaceLoader {
public:
    i32 BuildAssetNamespacePrefixes(const CString& name, i32 mode, i32 lightGate, i32 finishGate);

    char m_pad00[0xc];
    CSpriteFactoryHolder* m_c; // +0x0c  (== CState::m_c)
    char m_pad10[0x30 - 0x10];
    CSymTab* m_30; // +0x30  (== CState::m_gruntzBank)
};

#endif // GRUNTZ_GRUNTZ_ASSETNAMESPACELOADER_H
