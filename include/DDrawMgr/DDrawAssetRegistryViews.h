// DDrawAssetRegistryViews.h - the shared namespace-loader VIEWS of the three
// DDrawMgr asset registries reached off an owner's worker-holder (+0x10 image
// worker-registry, +0x28 sound leaf-scan, +0x2c aniz sub-manager). Both the
// per-object loader (AssetNamespacePrefixes.cpp, BuildAssetNamespacePrefixes
// 0xdca70) and the per-area loader (GameAssetNamespaces.cpp,
// LoadGameAssetNamespaces 0xf9ea0) scan the same three registries with the same
// reloc-masked probe/scan/load entries - previously each TU re-declared these
// three view classes flat. This is the ONE shared (superset) shape: const char*
// key/prefix args cover both the "GRUNTZ_"+CString and the literal "GAME" call
// sites, and the *Direct entries (only the per-object loader calls them) are
// declared-only no-ops in the per-area TU. Reloc-masked declared-only callees =>
// codegen-neutral for both loaders.
//
// Field names/slot names are placeholders; only the +0x48 LoadTree slot and the
// RVA-tagged non-virtual entries are load-bearing.
#ifndef GRUNTZ_CDDRAWASSETREGISTRYVIEWS_H
#define GRUNTZ_CDDRAWASSETREGISTRYVIEWS_H

#include <Ints.h>

// The GRUNTZ_/GAME sound leaf-scan view (owner+0x28) is no longer re-declared here:
// it is the canonical CDDrawSubMgrLeafScan (folded onto its single-source header). Both
// asset loaders call it by its canonical method names (HasKeyEqual_1583c0 / ScanTree_157ee0
// / RemoveKeysEqual_157c70) so the reloc pairs with the definitions in DDrawSubMgrLeafScan.cpp.
#include <DDrawMgr/DDrawSubMgrLeafScan.h>

// GRUNTZ_/GAME image worker registry (owner+0x10): 18 vtable slots then the
// virtual LoadTree at +0x48; plus the non-virtual key probe + direct-load.
class CDDrawWorkerRegistry {
public:
    virtual void s00();
    virtual void s04();
    virtual void s08();
    virtual void s0c();
    virtual void s10();
    virtual void s14();
    virtual void s18();
    virtual void s1c();
    virtual void s20();
    virtual void s24();
    virtual void s28();
    virtual void s2c();
    virtual void s30();
    virtual void s34();
    virtual void s38();
    virtual void s3c();
    virtual void s40();
    virtual void s44();
    virtual void LoadTree(void* tree, const char* prefix, const char* sep); // +0x48
    i32 HasKeyEqual(const char* key);                                       // 0x155550
    void LoadTreeDirect(const char* prefix, const char* sep);               // 0x155360
};

// GRUNTZ_/GAME aniz sub-manager (owner+0x2c): prefix probe + tree scan.
class CDDrawSubMgrAni {
public:
    i32 HasKeyPrefix(const char* key);                              // 0x152c50
    void ScanTree(void* tree, const char* prefix, const char* sep); // 0x152ad0
    void ScanTreeDirect(const char* prefix, const char* sep);       // 0x1527d0
};

#endif // GRUNTZ_CDDRAWASSETREGISTRYVIEWS_H
