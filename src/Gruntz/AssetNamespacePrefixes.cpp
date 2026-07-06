// AssetNamespacePrefixes.cpp - BuildAssetNamespacePrefixes (0xdca70), the
// "GRUNTZ_<name>" per-object asset-namespace loader, a __thiscall(this;
// const CString& name, i32 mode, i32 lightGate, i32 finishGate) (ret 0x10). It is
// the per-object sibling of CAssetLoader::LoadGameAssetNamespaces (m5_
// GameAssetNamespaces.cpp): it lazily scans this object's GRUNTZ_ image/sound/aniz
// trees into the three worker registries (this->m_c->m_10/m_28/m_2c), resolving the
// "IMAGEZ_"/"SOUNDZ_"/"ANIZ_"+name trees off this->m_30 (the ButeMgr parser).
//
// Item-7 verify (vs GameAssetNamespaces.cpp / CAssetLoader): sibling loader but NOT
// the same class - a different `this` layout (CNamespaceLoader here: m_c@+0xc,
// m_30@+0x30, no dense low block; CAssetLoader there: m_mgr@+0x4/m_symParser@+0x8/
// m_workerHolder@+0xc/m_areaArg@+0x1c/version-string@+0x4c/...). Per-object vs
// per-area loaders, distinct classes -> stay separate TUs (no merge).
//
//   - mode != 0 (full): if the GRUNTZ_ image key is absent, optionally run the
//     lighting/preview draw (lightGate), then g_resourceInstallActive-gate a
//     ResolvePath("IMAGEZ_"+name) + the registry's vtable LoadTree (+0x48), then run
//     the finish hook (finishGate); the sound/aniz trees ScanTree when absent.
//   - mode == 0 (rescan): if the GRUNTZ_ key is PRESENT, re-load it directly
//     (LoadTreeDirect/ScanTreeDirect) with no namespace resolve.
//
// The ~15 "GRUNTZ_"+name CString temps give it the /GX exception frame -> `eh` unit.
// CARCASS doctrine: every callee is a reloc-masked external; the worker-registry /
// leaf-scan / ani classes mirror GameAssetNamespaces.cpp; strings are $SG
// literals reloc-masked against the matched symbols.
#include <Bute/SymTab.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Mfc.h> // MFC CString (+, LoadString, ctor/dtor)
#include <Gruntz/GameRegistry.h>
#include <DDrawMgr/DDrawAssetRegistryViews.h> // shared CDDrawWorkerRegistry/LeafScan/Ani namespace views
#include <Win32.h>                            // RECT

#include <rva.h>

DATA(0x002bf37c)
extern i32 g_resourceInstallActive; // 0x6bf37c

// The ButeMgr symbol tree (this->m_30): ResolvePath a dotted namespace to its node.

// CDDrawWorkerRegistry / CDDrawSubMgrLeafScan / CDDrawSubMgrAni: shared views from
// <DDrawMgr/DDrawAssetRegistryViews.h> (mirror the per-area loader GameAssetNamespaces.cpp).

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
DATA(0x0024556c)
extern CGameRegistry* g_gameReg; // *0x64556c

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
    CSymTab* m_30; // +0x30
};

// @source: decomp-xref
// @early-stop
// /GX shared-return block-layout wall (~90.5%): the logic, both mode branches, the
// three GRUNTZ_ namespace registrations, the lighting/preview draw, g_resourceInstallActive
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
            ((CGruntSpawnConfig*)g_gameReg->m_cueSink)->DtorBody();
            ((CTriggerMgr*)g_gameReg->m_cmdGrid)->DestroyAllAnims();
            if (lightGate != 0) {
                CString cs;
                cs.LoadString(0x819b);
                RECT r = *(RECT*)g_gameReg->m_world->m_24->m_barRect;
                RECT r2;
                CopyRect(&r2, &r);
                DrawPreview((GRAssetMgr*)g_gameReg->m_world, &cs, &r2, 0x82, 1, 0xff, 0xff, 0, 1);
            }
            g_resourceInstallActive = 1;
            void* tree = m_30->ResolvePath("IMAGEZ_" + name);
            if (tree == 0) {
                result = 0;
                goto done;
            }
            m_c->m_10->LoadTree(tree, "GRUNTZ_" + name, "_");
            g_resourceInstallActive = 0;
            if (finishGate != 0) {
                FinishAssetLoad();
            }
        }
        if (m_c->m_28->HasKeyEqual_1583c0("GRUNTZ_" + name) == 0) {
            void* tree = m_30->ResolvePath("SOUNDZ_" + name);
            if (tree != 0) {
                m_c->m_28->ScanTree_157ee0((DirNode*)tree, "GRUNTZ_" + name, "_");
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
    if (m_c->m_28->HasKeyEqual_1583c0("GRUNTZ_" + name) != 0) {
        m_c->m_28->RemoveKeysEqual_157c70("GRUNTZ_" + name, "_");
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
// CDDrawSubMgrLeafScan SIZE_UNKNOWN now comes from its canonical DDrawSubMgrLeafScan.h.
SIZE_UNKNOWN(CNamespaceLoader);
SIZE_UNKNOWN(CSymTree);
SIZE_UNKNOWN(GRAssetMgr);
SIZE_UNKNOWN(CGameRegistry);
