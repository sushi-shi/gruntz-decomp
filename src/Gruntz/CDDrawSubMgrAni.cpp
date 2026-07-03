#include <rva.h>
// CDDrawSubMgrAni.cpp - the 'ANI' (animation) keyed catalog sub-manager of the
// CDirectDrawMgr surface/page-manager family (the "DDraw surface manager" group; see
// docs/ddraw-family-names.md). Sibling of CDDrawSubMgrLeaf (string
// catalog) and CDDrawSubMgrLeafScan (sound cache): a CObject/CDDrawSubMgr-derived
// string-keyed catalog owning a CMapStringToOb at +0x10 whose values are the
// 0x28-byte tomalla-38 animation elements (primary vftable @0x5efba8).
//
// Layout (offsets/sizes load-bearing; field NAMES are placeholders):
//   +0x00  vptr (CObject-derived)
//   +0x04  m_04  (i32, status word, -1 when inactive)
//   +0x08  m_08  (i32)
//   +0x0c  m_0c  (the owning CDirectDrawMgr / CDDrawSurfaceMgr manager; its +0x28 slot
//                 is forwarded to the element's Configure)
//   +0x10  m_10  (CMapStringToOb, 0x1c bytes; keyed by const char* name)
//
// The two methods here are the directory/factory group:
//   - CreateAniEntry (0x1528d0): the 0x28-byte element factory. Allocates the
//     element, CObject-constructs it (base dtor vtable @0x5e8cb4 then the element
//     primary vtable @0x5efba8), zeroes its fields, then runs the element's
//     Configure (0x1655c0, tomalla-38) keyed by the CSymTab entry, forwarding
//     the owning manager's +0x28 sub-manager. On success links it into the map
//     under `key`; on failure destroys it via its scalar dtor. /GX EH frame.
//   - ScanTree (0x152ad0): the recursive CSymTab directory walker. Allocates a
//     0x100-byte path buffer, recurses every child scope building the joined path
//     ("%s%s%s" of prefix/suffix/name when prefix is non-empty, else a plain strcpy
//     of the name), then for each 'ANI'-tagged leaf entry not already cached builds
//     its path and creates the element, counting successes.
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + emitted code
// bytes are load-bearing (campaign doctrine).
// ---------------------------------------------------------------------------

#include <Mfc.h>         // real MFC CObject / CMapStringToOb / POSITION
#include <Bute/SymTab.h> // CSymTab - the directory/scope tree the walker iterates
#include <string.h>      // strcpy / sprintf inline CRT (rep movs / repnz scas)

// The %s%s%s path-join format the walker sprintf's through (reloc-masked DIR32).
DATA(0x0061ab18)
extern const char g_fmtPathJoin[];

// The path buffer is freed at the walker tail (_RezFree @0x1b9b82, __cdecl).
extern "C" void RezFree(void* p);
// Global operator new (engine NAFXCW _RezAlloc @0x1b9b46); external/no-body.
void* operator new(u32 n);
// sprintf the path join goes through (0x11f890; varargs __cdecl, reloc-masked).
extern "C" i32 sprintf(char* buf, const char* fmt, ...);

// The +0x08 "CObject subobject" embedded in the element (0x14 bytes): a real
// member whose (potentially throwing) ctor is 0x1b55e9 (a __thiscall on element+0x8,
// the NAFXCW CObArray default ctor). Modeling it as a member auto-runs its ctor
// during CAniElementObj construction (after the CObject base, before the body) -
// reproducing retail's base-stamp / CObject-construct / primary-stamp / field-zero
// order and replacing the former (CAniElementBase*)(this+8) cross-cast hack.
struct CAniElemSub {
    CAniElemSub();       // 0x1b55e9 (reloc-masked rel32 callee)
    void* m_vptr;        // +0x00
    char _pad[0x14 - 4]; // +0x04..+0x13
};

// The 0x28-byte animation element (tomalla-38; primary vftable @0x5efba8), a real MFC
// ::CObject leaf (5 slots = CObject's 0/2/3/4 thunks + slot 1 = the cl-auto ??_G
// scalar-deleting dtor @0x152e10; no new virtuals). cl auto-emits ??_7CAniElementObj
// (masks 0x5efba8) + stamps the CObject grand-base (masks 0x5e8cb4) then the primary
// vptr. Configure (0x1655c0) reads its arg2's tag via CParseSource and, on a match,
// links a record; the failure path deletes it via the virtual scalar-deleting dtor.
// (vtable_hierarchy: derive CObject; the former fabricated CAniElementBase is deleted.)
struct CAniElementObj : public CObject {
    CAniElementObj() {
        m_04 = 0;
        m_1c = 0;
    }
    virtual ~CAniElementObj() OVERRIDE; // [1] 0x152e10, declared-only (cl-auto ??_G)

    i32 Configure_1655c0(void* sub, void* entry, i32 flag);  // 0x1655c0 __thiscall
    i32 Configure2_165620(void* sub, void* entry, i32 flag); // 0x165620 variant

    i32 m_04;         // +0x04 = 0
    CAniElemSub m_08; // +0x08..+0x1b  embedded CObject subobject
    i32 m_1c;         // +0x1c = 0
}; // size = 0x28

// The CSymTab entry's type-tag reader (CParseSource @0x139800, __thiscall on
// the entry node; the 'ANI'==0x414e49 gate). Modeled as a layout-compatible view
// so `mov ecx,entry; call 0x139800` falls out; external/no-body.
class CSymTabTag {
public:
    i32 ParseTag_139800(); // 0x139800
};

// ---------------------------------------------------------------------------
// The ANI catalog sub-manager. Map at +0x10; the owning manager at +0x0c.
// ---------------------------------------------------------------------------
class CDDrawSubMgrAni : public CObject {
public:
    CAniElementObj* CreateAniEntry_1528d0(const char* key, void* entry);
    CAniElementObj* CreateAniEntry2_1529b0(const char* key, void* entry);
    i32 ScanTree_152ad0(CSymTab* tree, const char* prefix, const char* suffix);

    i32 m_04;            // +0x04  status word
    i32 m_08;            // +0x08
    void* m_0c;          // +0x0c  owning CDirectDrawMgr / CDDrawSurfaceMgr manager
    CMapStringToOb m_10; // +0x10  keyed animation catalog
};

// Read the owning manager's +0x28 sub-manager slot (forwarded to Configure).
static inline void* AniMgrSubObject(void* mgr) {
    return *(void**)((char*)mgr + 0x28);
}

// ---------------------------------------------------------------------------
// 0x1528d0: the 0x28-byte element factory. Allocate the element; on success stamp
// the base-dtor vtable, CObject-construct it, stamp the primary vtable, zero +0x4
// and +0x1c; then run its Configure keyed by the entry, forwarding the owning
// manager's +0x28 sub-manager. On Configure failure destroy via the scalar dtor
// and return 0; on success link into the map under `key`. /GX EH frame.
// 2 stack args (ret 8). Returns the element (or 0).
// @early-stop
// ~99.99% - modeling the +0x08 subobject as a real CObject member (CAniElemSub, ctor
// 0x1b55e9) + deriving CAniElementObj from real ::CObject reproduces the ctor-in-flight
// frame exactly (up from 96.87% under the old cross-cast hierarchy). The whole body
// (new-merge null shape + failure-path scalar-deleting dtor dispatch) is byte-identical
// to retail; the sole residue is the appended exception-cleanup unwind funclet (retail
// section-splits it out of the delinked range, MSVC5 tacks it after the body).
// docs/patterns/new-throwing-ctor-unwind-funclet-appended.md.
RVA(0x001528d0, 0xdd)
CAniElementObj* CDDrawSubMgrAni::CreateAniEntry_1528d0(const char* key, void* entry) {
    CAniElementObj* el = new CAniElementObj;
    if (el == 0) {
        return 0;
    }
    if (el->Configure_1655c0(AniMgrSubObject(m_0c), entry, 0) == 0) {
        // Virtual scalar-deleting dtor dispatch (mov eax,[el]; call [eax+4]).
        delete el;
        return 0;
    }
    m_10[key] = (CObject*)el;
    return el;
}

// ---------------------------------------------------------------------------
// 0x1529b0: the 0x28-byte element factory variant. Byte-for-byte twin of
// CreateAniEntry_1528d0 except the element configure goes through the second
// Configure (0x165620) instead of 0x1655c0: allocate + CObject-construct the
// element, run Configure2 keyed by the entry (forwarding the owning manager's
// +0x28 sub-manager); on failure scalar-delete + return 0, on success link into
// the map under `key`. /GX EH frame. 2 stack args (ret 8). Returns the element.
// @early-stop
// ~99.99% - twin of CreateAniEntry_1528d0's wall (improved by the real CObject-member
// modeling of CAniElementObj): the /GX ctor-in-flight frame + the whole body are
// byte-identical up to the `ret`; the only residue is the appended exception-cleanup
// unwind funclet (retail section-splits it out of the delinked range).
// docs/patterns/new-throwing-ctor-unwind-funclet-appended.md.
RVA(0x001529b0, 0xdd)
CAniElementObj* CDDrawSubMgrAni::CreateAniEntry2_1529b0(const char* key, void* entry) {
    CAniElementObj* el = new CAniElementObj;
    if (el == 0) {
        return 0;
    }
    if (el->Configure2_165620(AniMgrSubObject(m_0c), entry, 0) == 0) {
        // Virtual scalar-deleting dtor dispatch (mov eax,[el]; call [eax+4]).
        delete el;
        return 0;
    }
    m_10[key] = (CObject*)el;
    return el;
}

// ---------------------------------------------------------------------------
// 0x152ad0: recursive CSymTab directory walker. Allocate a 0x100-byte path buffer
// (return 0 on failure), then for each child scope build the joined path and
// recurse, summing the count; then for each leaf entry, for each 'ANI'-tagged
// record not yet cached, build its path and create the element, counting
// successes. Frees the buffer and returns the count. 3 stack args (ret 0xc).
RVA(0x00152ad0, 0x17f)
i32 CDDrawSubMgrAni::ScanTree_152ad0(CSymTab* tree, const char* prefix, const char* suffix) {
    i32 count = 0;
    char* buf = (char*)operator new(0x100);
    if (buf == 0) {
        return 0;
    }
    buf[0] = 0;
    CSymTab* node = (CSymTab*)tree->FirstSub();
    while (node != 0) {
        if (prefix != 0 && *prefix != 0) {
            sprintf(buf, g_fmtPathJoin, prefix, suffix, node->m_name);
        } else {
            strcpy(buf, node->m_name);
        }
        count += ScanTree_152ad0(node, buf, suffix);
        node = (CSymTab*)tree->NextSub(node);
    }
    void* grp = tree->FirstSym();
    if (grp != 0) {
        do {
            CSymTab* fn = (CSymTab*)tree->NextSym2(grp);
            while (fn != 0) {
                if (((CSymTabTag*)fn)->ParseTag_139800() == 0x414e49) {
                    if (prefix != 0 && *prefix != 0) {
                        sprintf(buf, g_fmtPathJoin, prefix, suffix, fn->m_name);
                    } else {
                        strcpy(buf, fn->m_name);
                    }
                    if (CreateAniEntry_1528d0(buf, fn) != 0) {
                        ++count;
                    }
                }
                fn = (CSymTab*)tree->NextSym3(fn);
            }
            grp = tree->NextSym(grp);
        } while (grp != 0);
    }
    RezFree(buf);
    return count;
}

SIZE_UNKNOWN(CAniElemSub);
SIZE(CAniElementObj, 0x28);
SIZE_UNKNOWN(CSymTabTag);
VTBL(CAniElementObj, 0x001efba8); // ??_7CAniElementObj (was g_aniElemVtbl, 5 slots)
