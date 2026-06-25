#include <rva.h>
// CDDrawSubMgrAni.cpp - the 'ANI' (animation) keyed catalog sub-manager of the
// CDirectDrawMgr surface/page-manager family (the "Harry Potter" group; see
// src/Stub/types/ddrawmgr_surface_family.h). Sibling of CDDrawSubMgrLeaf (string
// catalog) and CDDrawSubMgrLeafScan (sound cache): a CObject/Lucius-derived
// string-keyed catalog owning a CMapStringToOb at +0x10 whose values are the
// 0x28-byte ClassUnknown_38 animation elements (primary vftable @0x5efba8).
//
// Layout (offsets/sizes load-bearing; field NAMES are placeholders):
//   +0x00  vptr (CObject-derived)
//   +0x04  m_04  (i32, status word, -1 when inactive)
//   +0x08  m_08  (i32)
//   +0x0c  m_0c  (the owning CDirectDrawMgr / HarryPotter manager; its +0x28 slot
//                 is forwarded to the element's Configure)
//   +0x10  m_10  (CMapStringToOb, 0x1c bytes; keyed by const char* name)
//
// The two methods here are the directory/factory group:
//   - CreateAniEntry (0x1528d0): the 0x28-byte element factory. Allocates the
//     element, CObject-constructs it (base dtor vtable @0x5e8cb4 then the element
//     primary vtable @0x5efba8), zeroes its fields, then runs the element's
//     Configure (0x1655c0, ClassUnknown_38) keyed by the CSymTab entry, forwarding
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

// The element's two construction vtables (reloc-masked DIR32 data): the CObject
// base dtor vtable stamped before the base ctor, then the element primary vtable.
DATA(0x001e8cb4)
extern void* g_remusBaseDtorVtbl; // 0x5e8cb4 - the CObject base dtor vtable
DATA(0x001efba8)
extern void* g_aniElemVtbl; // 0x5efba8 - the 0x28-byte element primary vftable

// The element's CObject grand-base (+0x00..+0x1b): vptr at +0, a header word at
// +4, and the CObject base subobject at +8 whose (potentially throwing) init is
// 0x1b55e9 (a __thiscall on element+0x8). Its ctor stamps the base-dtor vtable
// then runs that init; being polymorphic, the derived ctor re-stamps +0 with the
// primary vtable. A real ctor here drives the factory's `new CAniElementObj`
// ctor-in-flight /GX frame (the half-constructed-element cleanup).
struct CAniElementBase {
    CAniElementBase() {
        *(void**)this = &g_remusBaseDtorVtbl;
        ((CAniElementBase*)((char*)this + 0x8))->InitBase_1b55e9();
    }
    void InitBase_1b55e9(); // 0x1b55e9

    void* m_vptr;             // +0x00
    i32 m_04;                 // +0x04 = 0 (set by the derived ctor)
    char m_pad08[0x1c - 0x8]; // +0x08..+0x1b  CObject base subobject (0x14 bytes)
};

// The 0x28-byte animation element (ClassUnknown_38; primary vftable @0x5efba8).
// Its ctor stamps the element primary vtable over the base-dtor vtable the base
// ctor left, then zeroes +0x04 and +0x1c. Configure (0x1655c0) reads its arg2's
// tag via RemusParseSource and, on a match, links a record; the failure path
// dispatches the scalar-deleting dtor (vtable slot+4). The vtable contents are not
// modeled here, so the manual stamp is the transitional workaround; a declared-only
// ~ keeps the new-expression's cleanup edge live.
struct CAniElementObj : public CAniElementBase {
    CAniElementObj() {
        *(void**)this = &g_aniElemVtbl;
        m_04 = 0;
        m_1c = 0;
    }
    ~CAniElementObj(); // declared-only -> the new's failure cleanup edge

    i32 Configure_1655c0(void* sub, void* entry, i32 flag); // 0x1655c0 __thiscall

    i32 m_1c; // +0x1c = 0
}; // size = 0x28

// A polymorphic VIEW of the element used only for the failure-path scalar-deleting
// dtor dispatch (vtbl slot+4): casting el to this lowers `el->ScalarDtor(1)` to the
// retail `mov eax,[el]; mov ecx,el; call [eax+4]` __thiscall form. Declared-only
// (never defined), so no ??_7 is emitted - the element keeps its manual stamp. Same
// idiom as the sibling TUs' CCatalogNode / LeafScanValue.
class CAniElemView {
public:
    virtual void Slot00();            // +0x00
    virtual i32 ScalarDtor(i32 flag); // +0x04  scalar-deleting destructor
};

// The CSymTab entry's type-tag reader (RemusParseSource @0x139800, __thiscall on
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
    i32 ScanTree_152ad0(CSymTab* tree, const char* prefix, const char* suffix);

    i32 m_04;            // +0x04  status word
    i32 m_08;            // +0x08
    void* m_0c;          // +0x0c  owning CDirectDrawMgr / HarryPotter manager
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
// 96.87% - the /GX ctor-in-flight frame + the whole body (incl. the new-merge null
// shape and the failure-path scalar-deleting dtor dispatch) are byte-identical to
// retail up to the `ret`; the only residue is the appended exception-cleanup unwind
// funclet (retail section-splits it out of the delinked range, MSVC5 tacks it after
// the body). docs/patterns/new-throwing-ctor-unwind-funclet-appended.md.
RVA(0x001528d0, 0xdd)
CAniElementObj* CDDrawSubMgrAni::CreateAniEntry_1528d0(const char* key, void* entry) {
    CAniElementObj* el = new CAniElementObj;
    if (el == 0) {
        return 0;
    }
    if (el->Configure_1655c0(AniMgrSubObject(m_0c), entry, 0) == 0) {
        // Foreign-vtable scalar-deleting dtor dispatch (mov eax,[el]; call [eax+4]).
        ((CAniElemView*)el)->ScalarDtor(1);
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
