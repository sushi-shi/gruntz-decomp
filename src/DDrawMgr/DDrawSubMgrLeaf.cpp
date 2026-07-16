// DDrawSubMgrLeaf.cpp - the 0x152640-0x152e83 original TU (wave4-L dossier #15,
// block S2): ONE first-link obj holding the CDDrawSubMgrLeaf string-catalog meat
// - both the plain catalog ops AND the 'ANI' factory/walker method set (the ex
// "CDDrawSubMgrAni" twin class, merged; the old A-B-A "weave" was one class's
// member defs all along) - closed by ??1CAniElement (the element class the ani
// factory news). The leaf's IsReady/dtor quartet + ClearContext/ClearMap live in
// the G obj (src/DDrawMgr/DDrawSubMgr.cpp); CAniElement's meat in the T obj.
//
// original TU: filename unknown (@identity-TODO - no __FILE__ anchor; the
// DDrawMgr keyed-catalog module).
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + emitted code
// bytes are load-bearing (campaign doctrine).
#include <rva.h>
#include <Gruntz/ParseSource.h> // CParseSource (GetEntryTag) - keep BEFORE any
// header that fwd-declares `class CParseSource` (MSVC5 default-access quirk)
#include <Mfc.h>                          // real MFC CObject / CMapStringToPtr / CString / POSITION
#include <DDrawMgr/DDrawSubMgrLeaf.h>     // CDDrawSubMgrLeaf + CCatalogNode (hoisted)
#include <DDrawMgr/DDrawSubMgrLeafScan.h> // THE canonical CDDrawSubMgrLeafScan (sibling class)
#include <DDrawMgr/DDrawSurfaceMgr.h> // the +0x0c owner (m_soundRegistry = the ANI Configure ctx)
#include <Gruntz/AniElement.h>        // canonical CAniElement (the 0x28 'ANI' element)
#include <Bute/SymTab.h>              // CSymTab - the directory/scope tree the walker iterates
#include <stdio.h>                    // sprintf (the %s%s%s path-join, 0x11f890)
#include <string.h>                   // strcpy inline CRT (rep movs / repnz scas)

// The %s%s%s path-join format the walker sprintf's through (reloc-masked DIR32).
// @data-symbol, not DATA: clang mangles the const-char[] extern with a `Q` storage
// class while cl 5.0 emits `P` (?g_fmtPathJoin@@3PBDB), so a DATA() label's clang
// mangledName never matches the cl reloc (was also a VA-typo: 0x61ab18 -> 0x21ab18).
// @data-symbol: ?g_fmtPathJoin@@3PBDB 0x0021ab18
extern const char g_fmtPathJoin[];

// Global operator new (engine NAFXCW _RezAlloc @0x1b9b46); external/no-body.
void* operator new(u32 n);
// operator delete (0x1b9b82, ??3@YAXPAX@Z): the engine Rez heap free IS the global
// operator delete (FID-verified library label) - the scalar-deleting dtor AND the
// explicit path-buffer free at the walker tail both call it.
void operator delete(void*);

// The 0x28-byte animation element is the ONE canonical CAniElement
// (<Gruntz/AniElement.h>, primary vftable ??_7CAniElement @0x5efba8, 5 slots;
// slot 1 = the cl-auto ??_G scalar-deleting dtor @0x152e10). Its inline ctor
// reproduces retail's ctor-in-flight EH shape at the factory new-sites (the
// +0x08 m_records CObArray member's potentially-throwing NAFXCW ctor @0x1b55e9).
// (The former ctor-shape view pair CAniElemSub + CAniElementObj is dissolved
// onto the canonical - one class, one def, one vtable.)

// (The ex `CDDrawSubMgrAni` twin class is MERGED onto CDDrawSubMgrLeaf - one
// receiver, one TU, one layout; proof in <DDrawMgr/DDrawSubMgrLeaf.h>. The ANI
// factory reads the owner's +0x28 m_soundRegistry as the element Configure ctx - a
// real typed hop now, not the ex raw-offset AniMgrSubObject helper.)

// ---------------------------------------------------------------------------
// Look up `key` in the map; return the found value (or null), ignoring the bool.
// COMDAT-at-usage exile kept at the 0x6bxxx obj (file-head position).
RVA(0x0006b2a0, 0x23)
CObject* CDDrawSubMgrLeaf::LookupValue_06b2a0(const char* key) {
    void* val = 0;
    m_10.Lookup(key, val); // CMapStringToPtr::Lookup @0x1b8438 (void*& out-param)
    return (CObject*)val;
}

// The leaf vtable slots 6/7 (S2-resident tiny virtuals, out-of-line like retail).
RVA(0x00152640, 0x6)
i32 CDDrawSubMgrLeaf::Slot06_152640() {
    return 1;
}
RVA(0x00152650, 0x5)
void CDDrawSubMgrLeaf::Cleanup() {
    FreeAll_152720();
}

// ---------------------------------------------------------------------------
// 0x152660: remove ONE value by pointer identity. When `target` is non-null,
// walk the map via GetNextAssoc; on the first entry whose value pointer equals
// `target`, RemoveKey it, destroy `target` through its scalar-deleting dtor
// (vtbl +0x4 arg 1), and stop. /GX EH frame for the local CString key. 1 arg.
// @early-stop
// ~99.85% - map-scan idiom (top-tested while + real GetStartPosition kills the
// peel, pos declared before key computes it before the ctor like retail; docs/
// patterns/mfc-map-walk-while-not-guard-dowhile.md). Every instruction byte
// matches; the sole residue is a key<->pos stack-slot swap (retail key=[esp+0x20]/
// pos=[esp+0x8], ours the reverse) - the stack-slot-coalesce coin-flip, only the
// [esp+N] displacement bytes differ. docs/patterns/stack-slot-coalesce-frame-4b.md.
RVA(0x00152660, 0xb2)
void CDDrawSubMgrLeaf::RemoveValue_152660(CCatalogNode* target) {
    if (target == 0) {
        return;
    }
    POSITION pos = m_10.GetStartPosition();
    CString key;
    void* val = 0;
    while (pos != 0) {
        m_10.GetNextAssoc(pos, key, val);
        if ((void*)target == val) {
            m_10.RemoveKey(key);
            delete target;
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// Free-all: iterate every entry in m_10 via GetNextAssoc, destroying each value
// via its scalar-deleting destructor (vtbl +0x4 arg 1), then RemoveAll the map.
// Same source as the 100%-matched CDDrawWorkerRegistry::MapTeardown_1552b0.
// @early-stop
// store-scheduling coin-flip (~94.7%) - complete & correct: byte-identical to
// retail EXCEPT the `CObject* val = 0` store position (retail schedules it after
// the CString ctor; MSVC5 here emits it before the count read) + the reloc-masked
// EH-state push. The surrounding symbol set re-rolls the allocator; the identical
// sibling source matched 100% elsewhere. docs/patterns/zero-register-pinning.md.
RVA(0x00152720, 0xa2)
void CDDrawSubMgrLeaf::FreeAll_152720() {
    void* val = 0;
    POSITION pos = (POSITION)(m_10.GetCount() != 0 ? -1 : 0);
    CString key;
    if (*(volatile i32*)&pos != 0) {
        do {
            m_10.GetNextAssoc(pos, key, val);
            if (val != 0) {
                delete ((CCatalogNode*)val);
            }
        } while (pos != 0);
    }
    m_10.RemoveAll();
}

// ---------------------------------------------------------------------------
// Remove every entry whose key strncmp-equals `str` over the full length of the
// (CString-built) compare string, destroying each removed value via its scalar
// dtor; returns the count. The compare string is a CString built from `base`
// then assigned `str`.
// @early-stop
// regalloc wall (~91%) - complete & correct: logic/CFG/all calls/args/offsets
// reproduced. Residue is the val/loop-flag stack-slot swap + reloc-masked EH-state
// push, identical to the sibling CDDrawWorkerRegistry::RemoveKeysEqual_155360.
// docs/patterns/zero-register-pinning.md.
RVA(0x001527d0, 0xf8)
i32 CDDrawSubMgrLeaf::RemoveKeysEqual_1527d0(const char* base, const char* str) {
    CString match(base);
    match = str;
    i32 len = match.GetLength();
    CString key;
    void* val = 0;
    POSITION pos = m_10.GetStartPosition();
    i32 n = 0;
    while (pos != 0) {
        m_10.GetNextAssoc(pos, key, val);
        if (strncmp(key, match, len) == 0) {
            m_10.RemoveKey(key);
            if (val != 0) {
                delete ((CCatalogNode*)val);
            }
            ++n;
        }
    }
    return n;
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
// 0x1b55e9) + the canonical CAniElement : ::CObject reproduces the ctor-in-flight
// frame exactly. The whole body (new-merge null shape + failure-path scalar-deleting
// dtor dispatch) is byte-identical to retail; the sole residue is the appended
// exception-cleanup unwind funclet (retail section-splits it out of the delinked
// range). docs/patterns/new-throwing-ctor-unwind-funclet-appended.md.
RVA(0x001528d0, 0xdd)
CAniElement* CDDrawSubMgrLeaf::CreateAniEntry_1528d0(const char* key, void* entry) {
    CAniElement* el = new CAniElement;
    if (el == 0) {
        return 0;
    }
    if (el->Configure_1655c0(m_0c->m_soundRegistry, entry, 0) == 0) {
        // Virtual scalar-deleting dtor dispatch (mov eax,[el]; call [eax+4]).
        delete el;
        return 0;
    }
    m_10[key] = el;
    return el;
}

// ---------------------------------------------------------------------------
// 0x1529b0: the element factory variant - byte-for-byte twin of CreateAniEntry_1528d0
// except the element configure goes through the second Configure (0x165620).
// @early-stop
// ~99.99% - twin of CreateAniEntry_1528d0's wall; the only residue is the appended
// exception-cleanup unwind funclet (retail section-splits it out of the delinked
// range). docs/patterns/new-throwing-ctor-unwind-funclet-appended.md.
RVA(0x001529b0, 0xdd)
CAniElement* CDDrawSubMgrLeaf::CreateAniEntry2_1529b0(const char* key, void* entry) {
    CAniElement* el = new CAniElement;
    if (el == 0) {
        return 0;
    }
    if (el->LoadFile_165620(m_0c->m_soundRegistry, entry, 0) == 0) {
        // Virtual scalar-deleting dtor dispatch (mov eax,[el]; call [eax+4]).
        delete el;
        return 0;
    }
    m_10[key] = el;
    return el;
}

// ---------------------------------------------------------------------------
// 0x152ad0: recursive CSymTab directory walker. Allocate a 0x100-byte path buffer
// (return 0 on failure), then for each child scope build the joined path and
// recurse, summing the count; then for each leaf entry, for each 'ANI'-tagged
// record not yet cached, build its path and create the element, counting
// successes. Frees the buffer and returns the count. 3 stack args (ret 0xc).
RVA(0x00152ad0, 0x17f)
i32 CDDrawSubMgrLeaf::ScanTree_152ad0(CSymTab* tree, const char* prefix, const char* suffix) {
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
                if (((CParseSource*)fn)->GetEntryTag() == 0x414e49) {
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
    ::operator delete(buf);
    return count;
}

// ---------------------------------------------------------------------------
// Return 1 if any key strncmp-equals `str` over strlen(str), else 0.
RVA(0x00152c50, 0xdc)
i32 CDDrawSubMgrLeaf::HasKeyPrefix_152c50(const char* str) {
    i32 len = strlen(str);
    CString key;
    void* val = 0;
    POSITION pos = m_10.GetStartPosition();
    while (pos != 0) {
        m_10.GetNextAssoc(pos, key, val);
        if (strncmp(key, str, len) == 0) {
            return 1;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Reverse lookup: return (by value) the key of the entry whose value pointer ==
// `target`; an empty key for target==0 or no match. The map-scan idiom (top-tested
// while + real GetStartPosition, key.Empty() before the final return key) closes it.
RVA(0x00152d30, 0xd4)
CString CDDrawSubMgrLeaf::KeyOfValue_152d30(CObject* target) {
    CString key;
    if (target == 0) {
        return key;
    }
    void* val = 0;
    POSITION pos = m_10.GetStartPosition();
    while (pos != 0) {
        m_10.GetNextAssoc(pos, key, val);
        if (val == target) {
            return key;
        }
    }
    key.Empty();
    return key;
}

// ===========================================================================
// 0x152e30 - ~CAniElement: stamp own vtable, run DeleteAll (the most-derived
// teardown), then the CObArray member destructs and ~CObject folds in to
// restore the grand-base vtable. /GX frame from the destructible base+member.
// (The S2 obj's tail: the element class the ani factory above news.)
// ===========================================================================
// Real polymorphic: cl emits the implicit ??_7CAniElement own-vptr stamp in
// the ENTRY state (stamp-first, == retail), then DeleteAll, then the member
// ~CAniRecordArray (trylevel 0) and ~CObject grand-base re-stamp fold in.
// (eh-dtor-implicit-vptr-stamp-first.md.)
// The cl-auto scalar-deleting destructor (vtable slot 1; generated from the
// virtual dtor below - @rva-symbol pairs the retail copy with the base COMDAT).
// @rva-symbol: ??_GCAniElement@@UAEPAXI@Z 0x00152e10 0x1e
RVA(0x00152e30, 0x53)
CAniElement::~CAniElement() {
    DeleteAll();
    // m_records.~CAniRecordArray() (trylevel 0) + ~CObject() (grand-base restore) fold here.
}
