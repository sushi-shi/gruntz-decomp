// SymParser.cpp - CSymParser, the ButeMgr parser/owner that builds + owns the
// CSymTab scope tree (the object CSymTab::m_owner @+0x18 points back to). The five
// methods (trace group "ClassUnknown_11") all operate on one object shape that
// owns a heap root CSymTab at +0x44, an intrusive polymorphic-object list at +0x10
// (its own abstract sub-object vtable 0x5ef760) and an engine hash table at +0x80.
// See include/Bute/SymParser.h for the layout + the call-graph evidence.
#include <rva.h>

#include <Bute/SymParser.h>

// The +0x10 CObjList sub-object's two vtables (embedded-member stamps kept as raw
// field writes). CSymParser's own primary vtable is now ??_7CSymParser@@6B@
// (0x5ef750, cl-emitted; see SymParser.h). Reloc-masked DATA() externs.
DATA(0x005ef760)
void* CObjList_purecall_vftbl;
DATA(0x005ef75c)
void* CObjList_ctor_vftbl;

// 0x13ab00: the 3-arg buffer constructor. Construct the sub-object members (the +0x10
// object list, the +0x80 hash table, the +0x88 node list) + stamp the primary vtable,
// build-then-discard a default CSymParser temp, then drive the buffer through
// ParseBuffer. The destructible members + the temp force the /GX EH frame. __thiscall,
// ret 0xc; returns `this`. (The default ctor 0x13aa10 the temp uses lives in another,
// unmatched TU - a reloc-masked call.)
// @early-stop
// ~74.8% (real polymorphic now, ALL-VTABLES phase): the primary vptr is auto-
// stamped by cl @+0 at ctor entry (was the hand-rolled mid-body m_vtbl store); the
// /GX frame, the member-init store sequence (m_list, the +0x80 hash Init(1), the
// +0x88 node-list), the discarded default-temp ctor/dtor pair and the ParseBuffer
// 3-arg tail are byte-faithful. Residual is the /GX trylevel state-NUMBERING wall
// (docs/patterns/eh-state-numbering-base.md) + the vptr-first schedule. Final sweep.
RVA(0x0013ab00, 0xac)
CSymParser::CSymParser(void* buf, i32 a2, i32 a3) {
    // cl auto-stamps ??_7CSymParser @+0 at ctor entry (== the old m_vtbl stamp).
    m_list.m_head = 0;
    m_list.m_tail = 0;
    m_list.m_vtbl = &CObjList_ctor_vftbl;
    m_hash.Init(1);
    m_nodes.m_head = 0;
    m_nodes.m_tail = 0;
    {
        CSymParser tmp;
    }
    ParseBuffer(buf, a2, a3);
}

// ~CSymParser (0x13abc0): the /GX scalar destructor. Re-stamp the primary vtable,
// run Clear(0) if armed (m_parseArmed), drain the +0x10 object list, free the heap root
// CSymTab + the owned buffers, drain the +0x88 node list, then RemoveAll the +0x80
// hash member (the trylevel-0 /GX member-teardown) and re-stamp the +0x10 list
// sub-object vtable. The +0x80 CHashBase auto-destructs after the body.
// 100% (ALL-VTABLES phase): making CSymParser real-polymorphic lets cl auto-stamp
// the vptr @+0 at dtor entry (into EH-state 0, early) - which is exactly the retail
// schedule, closing the old eh-dtor-vptr-stamp-vs-trylevel-order wall that capped
// this at ~98% while the vptr was a hand-rolled store.
RVA(0x0013abc0, 0x13f)
CSymParser::~CSymParser() {
    // cl auto-stamps ??_7CSymParser @+0 at dtor entry (polymorphic class).
    if (m_parseArmed) {
        Clear(0);
    }
    CObjNode* p;
    for (p = m_list.m_head; p != 0; p = m_list.m_head) {
        m_list.Remove(p);
        m_list.m_count--;
        if (p) {
            p->Delete(1);
        }
    }
    CSymTab* root = m_root;
    if (root) {
        root->~CSymTab();
        RezFree(root);
        m_root = 0;
    }
    if (m_cachedSourceBuffer) {
        RezFree(m_cachedSourceBuffer);
        m_cachedSourceBuffer = 0;
    }
    if (m_delims) {
        RezFree(m_delims);
        m_delims = 0;
    }
    CSlotNode* node = (CSlotNode*)m_nodes.m_head;
    m_parseArmed = 0;
    m_activeNode = 0;
    m_30 = 0;
    m_34 = 0;
    m_38 = 0;
    m_3c = 0;
    m_40 = 1;
    m_root = 0;
    m_48 = 0;
    m_4c = 0;
    m_50 = 1;
    m_54 = 0;
    m_longestScopeNameLen = 0;
    m_longestLeafNameLen = 0;
    m_60 = 0;
    m_08 = 1;
    m_cachedSourceBuffer = 0;
    if (node) {
        do {
            RezFree(node->m_buffer);
            m_nodes.Unlink(node);
            RezFree(node);
            node = (CSlotNode*)m_nodes.m_head;
        } while (node);
    }
    // m_hash (RemoveAll) then m_list (vptr restore to 0x5ef760) auto-destruct here,
    // in reverse declaration order, under the /GX member-teardown trylevels.
}

// The two reader constructors ParseBuffer builds (text @0x13c940 ctor(this, m_2c),
// binary @0x13c540 ctor(this)). Both __thiscall on a freshly allocated reader block.
// Reloc-masked externs.
struct CTextReaderInit {
    char m_storage[0x38];
    CTextReaderInit(CSymParser* p, i32 a); // 0x13c940
};
struct CBinReaderInit {
    char m_storage[0x24];
    CBinReaderInit(CSymParser* p); // 0x13c540
};

// CRT strdup-style helpers + the inline strlen/strcpy the buffer-recache emits.
extern "C" u32 strlen(const char* s);
extern "C" char* strcpy(char* d, const char* s);

// @early-stop
// 0x3b8 (952 B) /GX text/binary loader. The body reproduces the buffer recache
// (free+strdup m_cachedSourceBuffer), the Classify dispatch, both reader builds (operator-new +
// ctor-throw cleanup states 0..4), the reader->Read / ReadRaw virtual calls, the root
// `new CSymTab` (MakeSymSeed leftover-args trick), and the binary-header field copies +
// magic validation. The plateau is the documented /GX trylevel state-machine + the
// heavy regalloc across the four allocation sites, plus the differently-named
// reader-ctor / 0x139de0 / 0x13b300 reloc operands (scoring artifact). Logic complete.
RVA(0x0013ad00, 0x3b8)
i32 CSymParser::ParseBuffer(void* buf, i32 a, i32 b) {
    m_40 = a;
    if (a == 0) {
        return 0;
    }
    if (m_cachedSourceBuffer) {
        ::operator delete(m_cachedSourceBuffer);
    }
    char* src = (char*)::operator new(strlen((char*)buf) + 1);
    m_cachedSourceBuffer = src;
    strcpy(src, (char*)buf);
    i32 tag = Classify((char*)buf);
    if (tag != 0) {
        // text / structured stream
        if (m_40 == 0) {
            return 0;
        }
        CObjNode* reader = (CObjNode*)new CTextReaderInit(this, m_2c);
        if (reader == 0) {
            ::operator delete(m_cachedSourceBuffer);
            m_cachedSourceBuffer = 0;
            return 0;
        }
        m_activeNode = reader;
        m_list.Link(reader);
        m_list.m_count++;
        if (reader->Read(buf, a, b) == 0) {
            return 0;
        }
        m_parseArmed = (void*)1;
        CSymTab* node = new CSymTab(
            this,
            0,
            g_emptyString,
            0,
            0,
            (void*)MakeSymSeed(),
            m_subTabBucketCount,
            m_symbolBucketCount
        );
        m_root = node;
        ParseRecords(reader, node, (char*)m_cachedSourceBuffer, 0);
        return 1;
    }
    // binary stream
    CObjNode* reader = (CObjNode*)new CBinReaderInit(this);
    if (reader == 0) {
        ::operator delete(m_cachedSourceBuffer);
        m_cachedSourceBuffer = 0;
        return 0;
    }
    m_activeNode = reader;
    m_list.Link(reader);
    m_list.m_count++;
    if (reader->Read(buf, a, b) == 0) {
        return 0;
    }
    m_parseArmed = (void*)1;
    if (b != 0) {
        m_3c = 0xa8;
        m_4c = 1;
        CSymTab* node = new CSymTab(
            this,
            0,
            g_emptyString,
            0,
            0,
            (void*)MakeSymSeed(),
            m_subTabBucketCount,
            m_symbolBucketCount
        );
        m_root = node;
        return 1;
    }
    // b == 0: read the 0xa8-byte binary header, copy its packed fields, validate magic.
    char hdr[0xa8];
    reader->ReadRaw(0, 0, 0xa8, hdr);
    m_50 = *(i32*)(hdr + 0x7f);
    m_30 = *(i32*)(hdr + 0x83);
    m_34 = *(i32*)(hdr + 0x87);
    m_38 = *(i32*)(hdr + 0x8b);
    m_3c = *(i32*)(hdr + 0x8f);
    m_48 = *(i32*)(hdr + 0x93);
    m_54 = *(i32*)(hdr + 0x97);
    m_longestScopeNameLen = *(i32*)(hdr + 0x9b);
    m_longestLeafNameLen = *(i32*)(hdr + 0x9f);
    m_60 = *(i32*)(hdr + 0xa3);
    m_08 = *(i32*)(hdr + 0xa7) & 0xff;
    if (hdr[0] != 0x0d || hdr[0x3f] != 0x0a || hdr[0x7e] != 0x1a || b != 1) {
        return 0;
    }
    CSymTab* node = new CSymTab(
        this,
        0,
        g_emptyString,
        (void*)m_30,
        (void*)m_34,
        (void*)m_38,
        m_subTabBucketCount,
        m_symbolBucketCount
    );
    m_root = node;
    node->ApplyRecursive((i32)reader, m_30, m_34, 0);
    return 1;
}

// ParseRecords' CRT directory-walk family (_findfirst/_findnext/_findclose @0x11f900/
// 0x11fa30/0x11fb50), _strlwr (0x18d330), _splitpath (0x18c530), atoi (0x11ff10) and the
// per-file leaf builders (0x13b970 / 0x13cac0). All reloc-masked. The "." / ".." / "\\"
// / "*.*" comparands are pooled .rodata literals.
struct SymFindData {
    i32 m_attrib; // +0x00
    char m_pad04[0x1c - 0x04];
    i32 m_size; // +0x1c
    char m_name[0x108];
};
extern "C" i32 SymFindFirst(const char* spec, SymFindData* fd);                       // 0x11f900
extern "C" i32 SymFindNext(i32 h, SymFindData* fd);                                   // 0x11fa30
extern "C" i32 SymFindClose(i32 h);                                                   // 0x11fb50
extern "C" char* _strlwr(char* s);                                                    // 0x18d330
extern "C" i32 atoi(const char* s);                                                   // 0x11ff10
extern "C" void _splitpath(const char* p, char* drv, char* dir, char* fn, char* ext); // 0x18c530
void SymBuildLeaf(CSymParser* p, void* recArg, void* extKey);                         // 0x13b970
void SymBindRecord(void* rec, char* name, i32 h);                                     // 0x13cac0
extern const char g_sepSlash[]; // 0x60cff0  "\\"
extern const char g_wildcard[]; // 0x61a0a0  "*.*"
extern const char g_dotDot[];   // 0x5ee8ec  ".."
extern const char g_dot[];      // 0x60cf90  "."

// @early-stop
// 0x545 (1349 B) /GX recursive directory loader: enumerates `path` with
// _findfirst/_findnext, skips "." / "..", recurses into subdirectories (CreateSub +
// self-call) and folds files into the scope (FindOrAddSym / Insert / the leaf record
// build). This is a structural reconstruction of the control flow + every scope/parser
// call; the >1KB /GX stack frame + alloca probe, the many inline strlen/strcpy/strcmp
// idioms, and the differently-named _findfirst/CSymTab reloc operands are the documented
// walls. Logic shape complete; byte-match parked for the final sweep.
RVA(0x0013b300, 0x545)
i32 CSymParser::ParseRecords(void* reader, CSymTab* node, char* path, i32 flag) {
    char pattern[0x500];
    strcpy(pattern, path);
    if (pattern[strlen(pattern) - 1] != '\\') {
        strcpy(pattern + strlen(pattern), g_sepSlash);
    }
    char full[0x600];
    strcpy(full, pattern);
    strcpy(full + strlen(full), g_wildcard);
    SymFindData fd;
    i32 h = SymFindFirst(full, &fd);
    if (h < 0) {
        return 1;
    }
    do {
        if (strcmp(fd.m_name, g_dotDot) == 0 || strcmp(fd.m_name, g_dot) == 0) {
            continue;
        }
        if ((fd.m_attrib & 0x10) == 0x10) {
            // subdirectory: build a child scope and recurse into it
            char childpath[0x600];
            strcpy(childpath, pattern);
            strcpy(childpath + strlen(childpath), fd.m_name);
            _strlwr(childpath);
            void* child = node->FindSub(fd.m_name);
            if (child == 0) {
                child = node->CreateSub(fd.m_name);
                if (child == 0) {
                    continue;
                }
            }
            ParseRecords(reader, (CSymTab*)child, childpath, flag);
            continue;
        }
        // a file: split off its extension, resolve the leaf record
        char fname[0x108];
        char ext[0x108];
        _splitpath(fd.m_name, 0, 0, fname, ext);
        _strlwr(ext);
        i32 nleft = (i32)strlen(fname);
        i32 i = 0;
        while (i < nleft && fname[i] >= '0' && fname[i] <= '9') {
            i++;
        }
        i32 key = (i >= nleft) ? atoi(fname) : (i32)m_nextGeneratedFileKey++;
        void* extKey = 0;
        if (ext[0] != 0) {
            _strlwr(ext);
            extKey = ResolveName(ext);
        }
        SymBuildLeaf(this, &fd, extKey);
        void* rec = node->FindOrAddSym(key);
        if (node->Insert(fname, extKey) == 0) {
            node->Method4b0((void*)(u32)fd.m_size, rec, full, 0);
        } else if (flag != 0) {
            node->Method530(rec, extKey);
            node->Method4b0((void*)(u32)fd.m_size, rec, full, 0);
        }
        void* node2 = node->FindOrAddSym(key);
        if (node2) {
            SymBindRecord(node2, full, h);
        }
    } while (SymFindNext(h, &fd) != 0);
    SymFindClose(h);
    return 1;
}

// Clear (0x13b850): drop the active node (m_activeNode) + the +0x10 object list,
// free the heap root CSymTab + the cached source buffer, then null m_parseArmed. The arg is unused;
// the return is the active node's slot[5] (Detach) result, left in eax.
// @early-stop
// regalloc wall: retail pins `this`->edi + the walked node->esi; recompile swaps
// them (this->esi, node->edi) - same instruction stream, opposite callee-saved
// assignment. Body byte-exact modulo the register-naming; logic complete. ~91%,
// parked for the final sweep.
RVA(0x0013b850, 0xa8)
void* CSymParser::Clear(i32 final) {
    (void) final;
    void* r = m_activeNode->Detach();
    m_list.Remove(m_activeNode);
    m_list.m_count--;
    if (m_activeNode) {
        m_activeNode->Delete(1);
    }
    m_activeNode = 0;
    CObjNode* p;
    for (p = m_list.m_head; p != 0; p = m_list.m_head) {
        p->Detach();
        m_list.Remove(p);
        m_list.m_count--;
        if (p) {
            p->Delete(1);
        }
    }
    if (m_root) {
        m_root->~CSymTab();
        RezFree(m_root);
        m_root = 0;
    }
    if (m_cachedSourceBuffer) {
        RezFree(m_cachedSourceBuffer);
        m_cachedSourceBuffer = 0;
    }
    m_parseArmed = 0;
    return r;
}

// ResolveQualified (0x13bff0): forward (name, arg) into GetRoot()'s CSymTab.
RVA(0x0013bff0, 0x19)
i32 CSymParser::ResolveQualified(const char* name, void* arg) {
    return GetRoot()->ResolveQualified(name, arg);
}

// ResolvePath (0x13c030): forward `path` into GetRoot()'s CSymTab.
RVA(0x0013c030, 0x14)
void* CSymParser::ResolvePath(const char* path) {
    return GetRoot()->ResolvePath(path);
}

// ReParse (0x13c050): if the parser is armed (m_parseArmed non-null), drop the current
// parse state (Clear(0)) and re-parse the cached +0x64 source buffer; return that
// parse's result. Not armed -> return 0. (ParseBuffer @0x13ad00 is the big buffer
// parser, modeled as a reloc-masked extern.)
RVA(0x0013c050, 0x28)
i32 CSymParser::ReParse() {
    if (m_parseArmed == 0) {
        return 0;
    }
    Clear(0);
    return ParseBuffer(m_cachedSourceBuffer, 1, 0);
}

// AddNode (0x13c210): splice a record's intrusive node (rec+0x1c) into the +0x80
// hash table, when rec is non-null.
RVA(0x0013c210, 0x1a)
void CSymParser::AddNode(void* rec) {
    if (rec) {
        m_hash.Insert((CHashInsertNode*)((char*)rec + 0x1c));
    }
}

// A parse-slot record (0x3c bytes): a CParseSource whose hash-node prefix is at
// +0x1c (stamped by Init) and whose self-ptr lives at +0x30. Init (0x1396f0) stamps
// the node vtable + nulls the body; reloc-masked __thiscall, no body here.
struct CParseSlot {
    char m_pad00[0x1c];
    void* m_node1c; // +0x1c  hash-node vtable prefix (the insert handle)
    char m_pad20[0x30 - 0x20];
    void* m_self; // +0x30
    char m_pad34[0x3c - 0x34];
    void Init(); // 0x1396f0
};

// PopParseSlot (0x13c0c0): see SymParser.h. The /GX frame guards the freshly Rez-
// alloc'd slot block while its elements are being initialized + registered.
// @early-stop
// EH-state + regalloc wall (~77%): logic complete. The node/array allocations land in
// a swapped callee-saved register (ebx vs ebp), the operator-new trylevel transitions
// + the slot-block down-counter init loop idiom diverge, and the hash-method reloc
// operands are differently named. Banked for the final sweep.
RVA(0x0013c0c0, 0x14b)
void* CSymParser::PopParseSlot() {
    CHashEntry* e = m_hash.First();
    void* rec = e ? e->m_record : 0;
    if (rec == 0) {
        CSlotNode* node = (CSlotNode*)RezAlloc(0xc);
        if (node == 0) {
            return 0;
        }
        i32 n = m_parseSlotBlockCount;
        CParseSlot* arr = (CParseSlot*)RezAlloc(n * 0x3c);
        if (arr) {
            CParseSlot* p = arr;
            i32 i = n;
            i--;
            if (i >= 0) {
                i++;
                do {
                    p->Init();
                    p++;
                    i--;
                } while (i);
            }
        }
        node->m_buffer = arr;
        if (arr == 0) {
            RezFree(node);
            return 0;
        }
        for (i32 j = 0; (u32)j < (u32)m_parseSlotBlockCount; j++) {
            CParseSlot* el = (CParseSlot*)((char*)node->m_buffer + j * 0x3c);
            el->m_self = el;
            m_hash.Insert((CHashInsertNode*)((char*)el + 0x1c));
        }
        m_nodes.Link(node);
        e = m_hash.First();
        rec = e->m_record;
    }
    if (rec) {
        m_hash.Remove((CHashEntry*)((char*)rec + 0x1c));
    }
    return rec;
}

// CObjList::Remove (0x1852e0): unlink `node` from the intrusive {head@+4,tail@+8}
// chain (m_list at CSymParser+0x10). The node's links are m_next@+4 / m_prev@+8; a
// null prev/next means `node` was the head/tail. __thiscall on the list head,
// callee-cleanup of the single arg.
RVA(0x001852e0, 0x35)
void CObjList::Remove(CObjNode* node) {
    if (node->m_prev) {
        node->m_prev->m_next = node->m_next;
    } else {
        m_head = node->m_next;
    }
    if (node->m_next) {
        node->m_next->m_prev = node->m_prev;
    } else {
        m_tail = node->m_prev;
    }
}

// ---------------------------------------------------------------------------
// CSymParseConfig::Construct (0x13aa10) - a symbol-parser config/state object,
// re-homed from src/Stub/MallocConstructors. Stamps vtable 0x5ef750, builds a
// CSymList (0x184960) at +0x80, and seeds the parse config defaults (+0x24=1,
// +0x28=0x77359400, +0x2c=3, +0x40=1, +0x08=1, +0x70/+0x74=0x13, +0x78=5,
// +0x7c=9, +0x90=0x64). xref (gruntz.analysis.xref): built by CSymParser::CSymParser
// (0x13ab00), RezSync::Init (0x83450) and CGruntzMgr::LoadWorldMode (0x91a40). Exact
// class name unresolved (non-RTTI vtable 0x5ef750); modeled as a plain shell.
// Reconstruction deferred.
struct CSymParseConfig {
    CSymParseConfig* Construct(); // 0x13aa10
};
// @confidence: high
// @source: xref
// @stub
RVA(0x0013aa10, 0xdc)
CSymParseConfig* CSymParseConfig::Construct() {
    return this;
}

// --- class-metadata sweep (Bute module): SymParser.h + .cpp-local SIZE at this .cpp
// EOF (all SIZE_UNKNOWN). CSymTab (also in SymParser.h) is annotated in SymTab.cpp.
SIZE_UNKNOWN(CSymParseConfig); // 0x13aa10 parser config/state (name TBD)
SIZE_UNKNOWN(CObjNode); // declared-but-undefined virtual slots; no vtable emitted here (no VTBL)
SIZE_UNKNOWN(CObjList);
SIZE_UNKNOWN(CSlotNode);
SIZE_UNKNOWN(CParserHash);
SIZE_UNKNOWN(CSymParser);
SIZE_UNKNOWN(CTextReaderInit);
SIZE_UNKNOWN(CBinReaderInit);
SIZE_UNKNOWN(SymFindData);
SIZE_UNKNOWN(CParseSlot);
