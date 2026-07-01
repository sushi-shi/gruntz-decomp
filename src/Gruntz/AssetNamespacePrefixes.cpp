// AssetNamespacePrefixes.cpp - BuildAssetNamespacePrefixes (0xdca70), the
// "GRUNTZ_<name>" per-object asset-namespace loader, a __thiscall(this;
// const CString& name, i32 mode, i32 lightGate, i32 finishGate) (ret 0x10). It is
// the per-object sibling of CAssetLoader::LoadGameAssetNamespaces (m5_
// GameAssetNamespaces.cpp): it lazily scans this object's GRUNTZ_ image/sound/aniz
// trees into the three worker registries (this->m_c->m_10/m_28/m_2c), resolving the
// "IMAGEZ_"/"SOUNDZ_"/"ANIZ_"+name trees off this->m_30 (the Remus parser).
//
//   - mode != 0 (full): if the GRUNTZ_ image key is absent, optionally run the
//     lighting/preview draw (lightGate), then g_severusCounterA-gate a
//     ResolvePath("IMAGEZ_"+name) + the registry's vtable LoadTree (+0x48), then run
//     the finish hook (finishGate); the sound/aniz trees ScanTree when absent.
//   - mode == 0 (rescan): if the GRUNTZ_ key is PRESENT, re-load it directly
//     (LoadTreeDirect/ScanTreeDirect) with no namespace resolve.
//
// The ~15 "GRUNTZ_"+name CString temps give it the /GX exception frame -> `eh` unit.
// CARCASS doctrine: every callee is a reloc-masked external; the worker-registry /
// leaf-scan / ani classes mirror m5_GameAssetNamespaces.cpp; strings are $SG
// literals reloc-masked against the matched symbols.
#include <Mfc.h>   // MFC CString (+, LoadString, ctor/dtor)
#include <Win32.h> // RECT

#include <rva.h>

DATA(0x002bf37c)
extern i32 g_severusCounterA; // 0x6bf37c

// The Remus symbol tree (this->m_30): ResolvePath a dotted namespace to its node.
struct CSymTree {
    void* ResolvePath(const char* path); // 0x13bae0
};

// GRUNTZ_ image worker registry (this->m_c->m_10): 18 vtable slots then LoadTree at
// +0x48 (virtual); plus the non-virtual key probe + direct-load.
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

class CDDrawSubMgrLeafScan {
public:
    i32 HasKeyEqual(const char* key);                         // 0x1583c0
    void ScanTree(void* tree, const char* prefix, char* sep); // 0x157ee0
    void ScanTreeDirect(const char* prefix, const char* sep); // 0x157c70
};

class CDDrawSubMgrAni {
public:
    i32 HasKeyPrefix(const char* key);                        // 0x152c50
    void ScanTree(void* tree, const char* prefix, char* sep); // 0x152ad0
    void ScanTreeDirect(const char* prefix, const char* sep); // 0x1527d0
};

struct AssetRoot { // this->m_c
    char m_pad00[0x10];
    CDDrawWorkerRegistry* m_10; // +0x10
    char m_pad14[0x28 - 0x14];
    CDDrawSubMgrLeafScan* m_28; // +0x28
    CDDrawSubMgrAni* m_2c;      // +0x2c
};

// The lighting/preview draw helpers off g_gameReg (0x64556c).
struct GRAssetMgr {
    char m_pad00[0x24];
    void* m_24; // +0x24  (rect source at +0x10)
};
struct GRLightObj {
    void Tick(); // m_60 -> 0x20a4 (thunk)
};
struct GRFxObj {
    void Update(); // m_68 -> 0x15c3 (thunk)
};
struct GameReg {
    char m_pad00[0x30];
    GRAssetMgr* m_30; // +0x30
    char m_pad34[0x60 - 0x34];
    GRLightObj* m_60; // +0x60
    char m_pad64[0x68 - 0x64];
    GRFxObj* m_68; // +0x68
};
DATA(0x0024556c)
extern GameReg* g_gameReg; // *0x64556c

// The preview draw (0x1c5d) + the finish hook (0x35e4).
extern "C" void
DrawPreview(GRAssetMgr* ctx, CString* text, RECT* rc, i32 y, i32 f, i32 b, i32 g, i32 r, i32 a9);
void FinishAssetLoad(); // 0x35e4

class CNamespaceLoader {
public:
    i32 BuildAssetNamespacePrefixes(const CString& name, i32 mode, i32 lightGate, i32 finishGate);

    char m_pad00[0xc];
    AssetRoot* m_c; // +0x0c
    char m_pad10[0x30 - 0x10];
    CSymTree* m_30; // +0x30
};

// @source: decomp-xref
// @early-stop
// /GX shared-return block-layout wall (~90.5%): the logic, both mode branches, the
// three GRUNTZ_ namespace registrations, the lighting/preview draw, g_severusCounterA
// gating and the vtable-LoadTree/direct-load split are all byte-identical to retail.
// The residual is where cl places the single return-cleanup epilogue: the recompile
// makes the ResolvePath-fail path the fall-through (epilogue mid-body at ~0x173) and
// reaches LoadTree by branch, while retail makes the success path the fall-through
// (epilogue at the tail); that block-ordering drift cascades stack-offset/branch bytes
// through the back half. Both `return 0`/single-`goto done` spellings compile identical
// (block-layout heuristic, not source-steerable). The rest is reloc/EH scoring artifact
// (differently-named externs, the __except handler-index push, __imp__CopyRect vs the
// 0x6c44bc pointer). Verified llvm-objdump -dr, base main body vs delinked target.
RVA(0x000dca70, 0x4a4)
i32 CNamespaceLoader::BuildAssetNamespacePrefixes(
    const CString& name,
    i32 mode,
    i32 lightGate,
    i32 finishGate
) {
    i32 result;
    if (mode != 0) {
        if (m_c->m_10->HasKeyEqual("GRUNTZ_" + name) == 0) {
            g_gameReg->m_60->Tick();
            g_gameReg->m_68->Update();
            if (lightGate != 0) {
                CString cs;
                cs.LoadString(0x819b);
                RECT r = *(RECT*)((char*)g_gameReg->m_30->m_24 + 0x10);
                RECT r2;
                CopyRect(&r2, &r);
                DrawPreview(g_gameReg->m_30, &cs, &r2, 0x82, 1, 0xff, 0xff, 0, 1);
            }
            g_severusCounterA = 1;
            void* tree = m_30->ResolvePath("IMAGEZ_" + name);
            if (tree == 0) {
                result = 0;
                goto done;
            }
            m_c->m_10->LoadTree(tree, "GRUNTZ_" + name, "_");
            g_severusCounterA = 0;
            if (finishGate != 0) {
                FinishAssetLoad();
            }
        }
        if (m_c->m_28->HasKeyEqual("GRUNTZ_" + name) == 0) {
            void* tree = m_30->ResolvePath("SOUNDZ_" + name);
            if (tree != 0) {
                m_c->m_28->ScanTree(tree, "GRUNTZ_" + name, "_");
            }
        }
        if (m_c->m_2c->HasKeyPrefix("GRUNTZ_" + name) == 0) {
            void* tree = m_30->ResolvePath("ANIZ_" + name);
            if (tree == 0) {
                result = 0;
                goto done;
            }
            m_c->m_2c->ScanTree(tree, "GRUNTZ_" + name, "_");
        }
        result = 1;
        goto done;
    }

    if (m_c->m_10->HasKeyEqual("GRUNTZ_" + name) != 0) {
        m_c->m_10->LoadTreeDirect("GRUNTZ_" + name, "_");
        if (finishGate != 0) {
            FinishAssetLoad();
        }
    }
    if (m_c->m_28->HasKeyEqual("GRUNTZ_" + name) != 0) {
        m_c->m_28->ScanTreeDirect("GRUNTZ_" + name, "_");
    }
    if (m_c->m_2c->HasKeyPrefix("GRUNTZ_" + name) != 0) {
        m_c->m_2c->ScanTreeDirect("GRUNTZ_" + name, "_");
    }
    result = 1;
done:
    return result;
}

SIZE_UNKNOWN(AssetRoot);
SIZE_UNKNOWN(CDDrawSubMgrAni);
SIZE_UNKNOWN(CDDrawSubMgrLeafScan);
SIZE_UNKNOWN(CNamespaceLoader);
SIZE_UNKNOWN(CSymTree);
SIZE_UNKNOWN(GRAssetMgr);
SIZE_UNKNOWN(GRFxObj);
SIZE_UNKNOWN(GRLightObj);
SIZE_UNKNOWN(GameReg);
