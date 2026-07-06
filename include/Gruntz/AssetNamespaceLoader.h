// AssetNamespaceLoader.h - CNamespaceLoader, the asset-namespace prefix builder
// (m_curState's +0x2c/+0x70 registrar). Promoted from AssetNamespacePrefixes.cpp so
// the reduced per-TU register-views can fold onto it.
#ifndef GRUNTZ_GRUNTZ_ASSETNAMESPACELOADER_H
#define GRUNTZ_GRUNTZ_ASSETNAMESPACELOADER_H

#include <Mfc.h>
#include <Ints.h>
#include <rva.h>

class AssetRoot;
class CSymTab;

SIZE_UNKNOWN(CNamespaceLoader);
class CNamespaceLoader {
public:
    i32 BuildAssetNamespacePrefixes(const CString& name, i32 mode, i32 lightGate, i32 finishGate);

    char m_pad00[0xc];
    AssetRoot* m_c; // +0x0c
    char m_pad10[0x30 - 0x10];
    CSymTab* m_30; // +0x30
};

#endif // GRUNTZ_GRUNTZ_ASSETNAMESPACELOADER_H
