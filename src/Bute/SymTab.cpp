// SymTab.cpp - the ButeMgr symbol-table ORIGINAL TU (interval
// dossier #14A): one obj spanning retail 0x1396f0-0x13c22a. The former
// cremusreadstream (ParseSource.cpp), symrec (SymRec.cpp) and symparser
// (SymParser.cpp) units were text-A-B-A-woven slices of this file and are folded
// in, in retail-RVA order: CParseSource (the positioned byte-reader / parse
// slot), CSymLeafBuilder, CSymRec (the leaf record), CSymTab (the scope tree)
// and CSymParser (the parser/owner) - plus two stray fns that carry Rez names
// but whose text AND private .data cells sit inside this obj's band
// (Load@CRezDirNode 0x13a0f0, FindEntry@CRezDir 0x13c080; @identity-TODO).
#include <Mfc.h> // afx-first (RezMgr.h below pulls MFC/Win32 for the two Rez strays)
#include <rva.h>
#include <io.h>     // _finddata_t / _findfirst / _findnext / _findclose (ParseRecords)
#include <stdlib.h> // _splitpath (0x18c530) / atoi (0x11ff10)
#include <string.h> // strchr/strncpy/inline strcpy+strlen/_strupr (0x18d330)
#include <time.h>   // time (0x120210) - the MakeSymSeed clock seed

#include <Bute/SymParser.h>     // full CSymParser + CSymTab/CSymRec via SymTab.h
#include <Gruntz/ParseSource.h> // canonical CParseSource (the 0x3c parse-slot record)
#include <Rez/RezMgr.h>         // CRezDirNode/CRezDir/RezSrc (the two stray fns)

// Placement new (construct-in-place; no allocation) for the embedded hash-node ctor.
inline void* operator new(u32, void* p) {
    return p;
}

// A leaf record's parse stream is the canonical CParseSource (included above);
// EndParse (0x1399d0) releases its inline buffer, reloc-masked __thiscall.

// The tokenizer's "is this character part of a token" predicate, inlined at each
// scan site. When the parser supplies a delimiter set, a token char is one NOT in
// it (strchr == 0); otherwise the default identifier classes apply: printable
// punctuation/space .. '.', digits, upper, lower. The `== 0` coercion emits the
// neg/sbb/inc int->bool normalize that retail uses (docs/patterns/
// int-to-bool-normalize.md).
static __inline i32 IsTokenChar(const char* delims, char ch) {
    if (delims) {
        return strchr(delims, ch) == 0;
    }
    if (ch >= ' ' && ch <= '.') {
        return 1;
    }
    if (ch >= '0' && ch <= '9') {
        return 1;
    }
    if (ch >= 'A' && ch <= 'Z') {
        return 1;
    }
    if (ch >= 'a' && ch <= 'z') {
        return 1;
    }
    return 0;
}

// The +0x80 hash member's construction (0x184960) is CHashBase::Construct - a real
// method on the parser's
// CParserHash/CHashBase member, reached cast-free via m_hash.Construct(1).

// ===========================================================================
// 0x1396f0 - Init: initialize a fresh parse
// slot - placement-construct the embedded hash-node (stamping its vptr @0x5ef740), null
// the name/mapped/reader bookkeeping, self-link +0x30. Returns this. __thiscall.
// @early-stop
// placement-new-guard wall (topic:eh topic:wall, ~56.7%): the node's ctor now supplies
// the vptr stamp + the m_record(+0x30) zero in retail's exact order (the ex `volatile`
// pin is gone - see ParseSource.h), but cl's placement-new emits the documented null
// guard `lea ecx,[this+0x1c]; cmp ecx,0; je` and then addresses the node off ecx, where
// retail has NO guard and addresses everything off `this` (eax). Retail therefore did
// not construct this node with placement new; the no-guard spelling is unrecovered
// (an explicit ctor call is not legal C++). Logic + layout complete; ??_7 named via VTBL.
// ===========================================================================
RVA(0x001396f0, 0x1a)
CParseSource* CParseSource::Init() {
    new (&m_node1c) CParseSlotHashNode; // stamps the vptr + zeroes m_record (+0x30)
    m_reader = 0;
    m_mapped = 0;
    m_name = 0;
    m_node1c.m_record = this; // the element's record IS this source (key = m_name @+0)
    return this;
}

// The 11-arg leaf-record builder's record shape (the parse slot the owner pops). The
// leaf builder fills it from a parse-stream record: the strdup'd name at +0x00, the
// owning scope at +0x10, the back-pointer to the symbol record at +0x04, three parse
// values (f1/f2/f3) at +0x14/+0x08/+0x0c, the source stream at +0x34, a self-pointer
// at +0x30, and two zeroed counters (+0x18/+0x38). ApplyRange reads +0x0c (added into
// m_10) and +0x14 (the min/max accumulator key) back out. Field names are placeholders.
// CSymLeafBuilder (the leaf-record parse slot) now lives in <Bute/SymTab.h> (included
// above) - a real class belongs in the module header. Method bodies stay here.

// CSymLeafBuilder::Build (0x139710): populate a freshly-popped leaf-record slot from a
// parse-stream record. The name is duplicated through the throwing ::operator new
// (0x1b9b46) when present, else stored as-is (null). __thiscall, callee-cleans its 11
// stack args (ret 0x2c). f4/str2/f6/arr are forwarded by the caller but consumed by a
// later stage, not stored here.
// @early-stop
// reloc-name plateau: all 141 instructions are byte-identical to retail (verified
// llvm-objdump base vs target). The 96.30% residual is purely the one ::operator new
// `call rel32` pairing base's ??2@YAPAXI@Z against the delinker's name for 0x1b9b46
// (RezAlloc) - the same documented scoring artifact the CSymTab ctor below carries.
RVA(0x00139710, 0x8d)
void CSymLeafBuilder::Build(
    CSymTab* owner,
    const char* name,
    void* f4,
    void* rec,
    void* str2,
    void* f3,
    void* f1,
    void* f2,
    void* f6,
    void* arr,
    void* stream
) {
    m_sourceStream = stream;
    m_ownerScope = owner;
    if (name == 0) {
        m_name = (char*)name;
    } else {
        m_name = (char*)::operator new(strlen(name) + 1);
        if (m_name) {
            strcpy(m_name, name);
        }
    }
    m_record = rec;
    m_0c = (i32)f3;
    m_14 = (i32)f1;
    m_08 = f2;
    m_38 = 0;
    m_18 = 0;
    m_node.m_record = this;
}

// ---------------------------------------------------------------------------
// CSymLeafBuilder::Teardown (0x1397a0, RVA-adjacent to CSymLeafBuilder::Build): the
// leaf-VALUE-record teardown. Free the strdup'd name (m_name); then free the owned
// value buffer (m_38) UNLESS the owning scope's shared buffer is live (m_ownerScope &&
// m_ownerScope->m_buf48 != 0); then clear nine fields. __thiscall, void (the two
// `if (m_38) free` arms tail-merge). Xref: called by ~CSymRec (0x139cf0) over each
// m_valTable payload and by CSymTab::AddNodeSubEntry (0x13a530) when a stale value key
// is re-scanned - proving the record is the leaf VALUE record CSymLeafBuilder::Build
// fills.
RVA(0x001397a0, 0x57)
void CSymLeafBuilder::Teardown() {
    if (m_name) {
        ::operator delete(m_name);
    }
    if (m_ownerScope != 0) {
        if (m_ownerScope->m_buf48 == 0) {
            if (m_38) {
                ::operator delete(m_38);
            }
        }
    } else {
        if (m_38) {
            ::operator delete(m_38);
        }
    }
    m_name = 0;
    m_record = 0;
    m_08 = 0;
    m_0c = 0;
    m_38 = 0;
    m_ownerScope = 0;
    m_14 = 0;
    m_18 = 0;
    m_node.m_record = 0;
}

// ===========================================================================
// 0x139800 - GetEntryTag: return the first dword of the keyed-store entry (the
// packed 4-char format tag). Out-of-line (retail keeps a real body here that
// every consumer TU calls, never inlines).
// ===========================================================================
RVA(0x00139800, 0x6)
i32 CParseSource::GetEntryTag() {
    return *(i32*)m_entry;
}

// ===========================================================================
// Homed from src/Stub/GapFunctions.cpp (matcher-5): both physically sit inside
// CParseSource's .text block (RVA-first home) but belong to a DISTINCT, unresolved
// scope-chain class (holds a CSymTab* at +0x10; NOT CParseSource, whose +0x10 is
// the mapped source). The gap tool had over-spanned 0x139810 by 6 bytes into the
// 0x139950 getter (its true size is 0x140, not 0x146) - now split.
// ===========================================================================
// @identity-TODO (matcher-5): 0x139810 (__thiscall(dst, size)) builds the `\`-joined
// qualified path of the scope at this->+0x10 into dst by walking the CSymTab scope chain up
// via parent(+0x1c): scratch=RezAlloc(size); dst=""; for(node=this->m_10; node; node=node->
// +0x1c){ strcpy(scratch,dst); dst = node->+0x1c ? g_sepSlash : ""; strcat(dst,node->m_name);
// strcat(dst,scratch);} ::operator delete(scratch); a lone root returns strcpy(dst, g_sepSlash "\").
// The WALKED NODE is the real CSymTab (its ctor @0x139de0 proves m_name@+0, parent@+0x1c);
// g_sepSlash @0x60cff0, g_emptyString @0x6293f4. But `this`'s OWN class - the holder of a
// current-scope CSymTab* at +0x10 - is a ZERO-REF ORPHAN (no rel32/vtable/data-ref caller
// anywhere in the image; verified by a full-binary VA byte-scan), so its identity is
// unrecoverable and NOT CParseSource/CSymTab (their +0x10 roles are disproven). Homed as a
// stub rather than fabricate a per-TU view of an un-xref-able receiver (no-fake-view rule).
RVA(0x00139810, 0x140)
i32 Gap_139810(void) {
    return 0;
}

// @identity-TODO (matcher-5): 0x139950 == the same orphan holder's `return this->m_10->m_name`
// (mov eax,[ecx+0x10]; mov eax,[eax]; ret) - the current scope's (CSymTab) name. Same
// unrecoverable receiver as 0x139810; homed as a stub, no fabricated view.
RVA(0x00139950, 0x6)
i32 Gap_139950(void) {
    return 0;
}

// ===========================================================================
// 0x139960 - BeginParse: resolve the live source pointer for a parse pass. If
// the mapped source is active, return its mapped address adjusted for the stream
// base. Otherwise lazily allocate the +0x38 inline buffer and fill it with one
// virtual Read of the whole limit; on a short read, free + return 0.
// ===========================================================================
RVA(0x00139960, 0x6b)
i32 CParseSource::BeginParse() {
    if (m_mapped->m_mapping != 0) {
        return m_base - m_mapped->m_baseOffset + m_mapped->m_mapping;
    }
    if (m_buffer != 0) {
        return m_buffer;
    }
    if (m_length == 0) {
        return 0;
    }
    m_buffer = (i32)RezAlloc(m_length);
    if (m_buffer == 0) {
        return 0;
    }
    if (m_reader->Read(m_base, 0, m_length, (void*)m_buffer) != static_cast<i32>(m_length)) {
        ::operator delete((void*)m_buffer);
        m_buffer = 0;
    }
    return m_buffer;
}

// ===========================================================================
// 0x1399d0 - EndParse: release the inline parse buffer; returns 1.
// ===========================================================================
RVA(0x001399d0, 0x21)
i32 CParseSource::EndParse() {
    if (m_buffer != 0) {
        ::operator delete((void*)m_buffer);
        m_buffer = 0;
    }
    return 1;
}

// ===========================================================================
// 0x139a40 - ReadAt(dst, pos, len): copy `len` bytes from absolute position `pos`
// in whichever backing store is live (mapped source / inline buffer / virtual
// reader), WITHOUT seeking the cursor or clamping to the limit. The two buffer
// paths return 1; the virtual-reader path returns whether the full `len` was read.
// ===========================================================================
RVA(0x00139a40, 0x95)
i32 CParseSource::ReadAt(void* dst, i32 pos, u32 len) {
    ParseMappedSource* sd = m_mapped;
    if (sd->m_mapping != 0) {
        memcpy(dst, (const void*)(m_base - sd->m_baseOffset + pos + sd->m_mapping), len);
        return 1;
    }
    if (m_buffer != 0) {
        memcpy(dst, (const void*)(m_buffer + pos), len);
        return 1;
    }
    return m_reader->Read(m_base, pos, len, dst) == static_cast<i32>(len);
}

// SetPos (0x139ae0): seek the read cursor, report success. Out-of-line (retail
// keeps it a real 15-byte function; an inline body folds into Read and never emits).
RVA(0x00139ae0, 0xf)
i32 CParseSource::SetPos(i32 pos) {
    m_cursor = pos;
    return 1;
}

// ===========================================================================
// 0x139af0 - Read(dst, len, seekPos): optionally seek (seekPos != -1), clamp the
// request to the remaining bytes, then copy from the live backing store. Returns
// the byte count, or 0 on empty/short read. The unsigned length math yields the
// `jbe` limit/empty tests.
// ===========================================================================
// @early-stop
// regalloc/scheduling wall (docs/patterns/pin-local-for-callee-saved-reg.md +
// reread-member-view-pointer.md): logic + control-flow byte-exact (same 3-way
// dispatch, shared return-0 epilogue, both inline rep movsd/movsb memcpys, the
// vtable call). Residue is allocator/selection only: retail holds the mapped-
// source ptr in edx and folds `sub esi,[edx+0xc]` (1 instr) where cl loads
// sd->m_baseOffset to a reg first, plus a je-vs-jbe 1-byte branch encoding on the empty
// check. Not source-steerable. ~89%; SetPos is 100%.
RVA(0x00139af0, 0xcc)
i32 CParseSource::Read(void* dst, u32 len, i32 seekPos) {
    if (seekPos != -1) {
        SetPos(seekPos);
    }

    u32 pos = static_cast<u32>(m_cursor);
    u32 want = len;
    if (pos + want > m_length) {
        want = m_length - pos;
    }
    if (want != 0) {
        ParseMappedSource* sd = m_mapped;
        if (sd->m_mapping) {
            const char* base = (const char*)(m_base - sd->m_baseOffset + sd->m_mapping + pos);
            memcpy(dst, base, want);
            m_cursor += want;
            return want;
        }
        if (m_buffer) {
            const char* base = (const char*)(m_buffer + pos);
            memcpy(dst, base, want);
            m_cursor += want;
            return want;
        }
        if (m_reader->Read(m_base, pos, want, dst) == static_cast<i32>(want)) {
            m_cursor += want;
            return want;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CSymRec - the two leaf-record ctors + the /GX teardown (0x139bf0/0x139c80/
// 0x139cf0), unified onto the canonical <Bute/SymTab.h> CSymRec. Both ctors stamp the +0x04 node prefix
// (CSymRecNode's inlined ctor: vptr 0x1ef744 + zeroed payload), build the two
// hash-table members (reloc-masked container ctors 0x184950/0x184960; the
// destructible members force the /GX EH frame), then wire key/back-ptr/scope.
// Xref: both ctors are called only by CSymTab::FindOrAddSym (0x13a940) and the
// dtor only by ~CSymTab (0x139ee0) - the same original TU.
// @early-stop
// ~99.1% register-assignment tail (this 4-arg ctor): body byte-faithful (the
// IDENTICAL body gives the 3-arg overload 100%). The extra `d` param's stack
// shift only flips cl's key<->owner GPR choice on the two trailing arg reloads
// + one store slot. Pure allocator coin-flip; source-order tuned to the best of 3.
RVA(0x00139bf0, 0x71)
CSymRec::CSymRec(i32 key, CSymTab* owner, i32 c, i32 d) : m_keyTable(c), m_valTable(d) {
    m_scope = owner;
    m_key = key;
    m_symNode.m_record = this;
}

// The 3-arg overload: m_keyTable uses the default container ctor (0x184950),
// m_valTable the sized one (0x184960).
RVA(0x00139c80, 0x6c)
CSymRec::CSymRec(i32 key, CSymTab* owner, i32 c) : m_keyTable(), m_valTable(c) {
    m_key = key;
    m_symNode.m_record = this;
    m_scope = owner;
}

// ~CSymRec (0x139cf0): drain m_keyTable when the owning parser's m_6c flag is
// set, then drain m_valTable - re-filing each node's payload (tearing it down
// via CSymLeafBuilder::Teardown @0x1397a0 above) back to the parser's free pool -
// then zero the key + the node payload. The two CHash members auto-
// destruct after the body (reverse declaration order, /GX trylevels).
RVA(0x00139cf0, 0xd7)
CSymRec::~CSymRec() {
    if (m_scope->m_owner->m_6c != 0) {
        CHashElement* n = m_keyTable.First();
        while (n) {
            CHashElement* cur = n;
            n = cur->Next();
            m_keyTable.Remove(cur);
        }
    }
    CHashElement* n = m_valTable.First();
    while (n) {
        CHashElement* cur = n;
        n = cur->Next();
        m_valTable.Remove(cur);
        ((CSymLeafBuilder*)cur->m_record)->Teardown();
        m_scope->m_owner->AddNode(cur->m_record);
    }
    m_key = 0;
    m_symNode.m_record = 0;
}

// ctor (0x139de0): stamp the +0x20 hash-node vtable + a zeroed +0x34 (both in the
// init list so they precede the member ctors), build the two embedded hash tables
// (m_subTabs(subN) then m_symbols(symN) - the /GX member-construction trylevels go
// -1 -> 0 -> 1), copy `name` through the throwing ::operator new (so the state-1
// transition before it falls out), then store the remaining fields and re-point m_34
// at this. Returns this.
// @early-stop
// reloc-name plateau: every CODE byte matches retail. The residual is purely
// differently-named reloc operands (the two CHashBase::Construct = 0x184960
// ctor calls, ::operator new vs the delinked _RezAlloc, and the +0x20 vtable named
// by the delinker after its containing PTR_LAB) - the documented scoring artifact,
// not a logic gap. Confirm with llvm-objdump -dr base vs target.
RVA(0x00139de0, 0xd4)
CSymTab::CSymTab(
    CSymParser* owner,
    void* p1,
    const char* name,
    void* p3,
    void* p4,
    void* p5,
    i32 subN,
    i32 symN
)
    : m_subTabs(subN), m_symbols(symN) {
    m_name = (char*)::operator new(strlen(name) + 1);
    if (m_name) {
        strcpy(m_name, name);
    }
    m_14 = p5;
    m_08 = p4;
    m_04 = p3;
    m_owner = owner;
    m_10 = 0;
    m_0c = 0;
    m_buf48 = 0;
    m_1c = p1;
    m_node20.m_record = this;
}

// ~CSymTab (0x139ee0): tear down the scope tree. Walk the leaf-symbol table
// (m_symbols, +0x40) clearing+freeing each record, then the child-scope table
// (m_subTabs, +0x38) recursing ~CSymTab on each, then free the owned buffers and
// null the fields. The two hash-table members (CHashB then CHash) auto-destruct after the body, in
// reverse declaration order (m_symbols then m_subTabs) at descending trylevels --
// the /GX member-teardown frame (docs/patterns/eh-dtor-model-members-as-
// destructible.md).
// @early-stop
// this-register + /GX-frame wall (88.7%): logic + both member-teardown walks are
// byte-faithful; retail pins this->ebp where cl uses ebx, and the /GX scopetable push
// immediate (0xb vs 0x0) is reloc-masked. A callee-saved coin-flip. Final sweep.
RVA(0x00139ee0, 0x11e)
CSymTab::~CSymTab() {
    CHashElement* cur;
    for (cur = m_symbols.First(); cur != 0;) {
        CHashElement* next = cur->Next();
        m_symbols.Remove(cur);
        CSymRec* rec = (CSymRec*)cur->m_record;
        delete rec; // ~CSymRec non-virtual; CSymRec::operator delete inlines to RezFree (0x1b9b82)
        cur = next;
    }
    for (cur = m_subTabs.First(); cur != 0;) {
        CHashElement* next = cur->Next();
        m_subTabs.Remove(cur);
        CSymTab* sub = (CSymTab*)cur->m_record;
        delete sub; // ~CSymTab non-virtual; CSymTab::operator delete inlines to RezFree
        cur = next;
    }
    if (m_name) {
        ::operator delete(m_name);
    }
    if (m_buf48) {
        ::operator delete(m_buf48);
    }
    m_name = 0;
    m_14 = 0;
    m_08 = 0;
    m_04 = 0;
    m_10 = 0;
    m_0c = 0;
    m_buf48 = 0;
    m_owner = 0;
    m_1c = 0;
    m_node20.m_record = 0;
    // m_symbols, m_subTabs destruct here (reverse decl order, /GX trylevels).
}

// Insert (0x13a000): the ResolveQualified tail. Look the record up for `arg` in
// this scope's leaf table (m_symbols, +0x40); if present, walk its embedded
// sub-table (record+0x24) for `key`, forwarding m_owner->m_68 == 0 as the flag.
// The `m_68 == 0` is the int->bool sete. __thiscall, callee-clean of both args.
RVA(0x0013a000, 0x37)
i32 CSymTab::Insert(const char* key, void* arg) {
    CSymRec* rec = (CSymRec*)m_symbols.FindInt((u32)arg);
    if (!rec) {
        return (i32)rec;
    }
    return (i32)rec->m_valTable.Walk(key, m_owner->m_68 == 0);
}

// Find (0x13a040): split `path` into its components, derive the leaf record's value
// from the (upcased, dot-stripped) extension via the parser's name->key map, and
// insert/resolve it under the file-name key. Returns the leaf record (Insert tail).
RVA(0x0013a040, 0xa2)
void* CSymTab::Find(const char* path) {
    char dir[260];
    char fname[260];
    char ext[260];
    char drive[4];
    char tmp[8];
    _splitpath(path, drive, dir, fname, ext);
    void* arg;
    if (strlen(ext) != 0) {
        strcpy(tmp, ext + 1);
        _strupr(tmp);
        arg = (void*)PackTag(tmp);
    } else {
        arg = 0;
    }
    return (void*)Insert(fname, arg);
}

// ---------------------------------------------------------------------------
// CRezDirNode::Load(childFlag)
// Recursive directory parse / load. If already loaded (m_buf != 0) return 1.
// Validate the source (m_src->m_8 nonzero, m_src->m_1c <= 1) else assert "File
// is not sorted!". If m_size > 0, allocate the payload buffer and virtually read
// it from the source stream at (m_off, 0, m_size, buf). When childFlag is set,
// iterate the child collection (First/Next) and recurse Load(1) into each
// child's sub-dir node (node->m_14). Returns 1.
SYMBOL(?Load@CRezDirNode@@QAEHH@Z)
RVA(0x0013a0f0, 0x99)
i32 CRezDirNode::Load(i32 childFlag) {
    if (m_buf != 0) {
        return 1;
    }

    RezSrc* src = m_src;
    if (src->m_8 == 0 || static_cast<u32>(src->m_1c) > 1) {
        RezAssertFail("CRezDir::Load Failed! (File is not sorted!)");
        return 0;
    }

    if (m_size > 0) {
        m_buf = RezAlloc(m_size);
        if (m_buf != 0) {
            m_src->m_stream->ReadAt(m_off, 0, m_size, m_buf);
        }
    }

    if (childFlag != 0) {
        for (CHashElement* n = m_kids.First(); n != 0; n = n->Next()) {
            // CHashElement::m_record is the shared hash-node's generic (void*) payload
            // slot - it holds a CSymTab*/CSymRec* in Bute and a CRezDirNode* here;
            // typed to the concrete element type at this use site.
            ((CRezDirNode*)n->m_record)->Load(1);
        }
    }
    return 1;
}

// ReleaseParseBuffers (0x13a190): drop this scope's cached parse state. If the owned
// +0x48 buffer is live, free it; otherwise walk the leaf-symbol table ending each
// record's parse stream (EndParse over the NextSym2/NextSym3 sub-chain). When
// `recurse` is set, descend into every child scope (m_subTabs). Returns 1.
RVA(0x0013a190, 0x94)
i32 CSymTab::ReleaseParseBuffers(i32 recurse) {
    if (m_buf48 != 0) {
        ::operator delete(m_buf48);
        m_buf48 = 0;
    } else {
        void* rec = FirstSym();
        while (rec) {
            void* sub = NextSym2(rec);
            while (sub) {
                ((CParseSource*)sub)->EndParse();
                sub = NextSym3(sub);
            }
            rec = NextSym(rec);
        }
    }
    if (recurse) {
        CHashElement* e = m_subTabs.First();
        while (e) {
            ((CSymTab*)e->m_record)->ReleaseParseBuffers(1);
            e = e->Next();
        }
    }
    return 1;
}

// FindSub (0x13a230): look up `name` in the child-scope table (m_subTabs, +0x38),
// forwarding `m_owner->m_68 == 0` as the walk's flag.
RVA(0x0013a230, 0x29)
void* CSymTab::FindSub(const char* name) {
    if (!name) {
        return (void*)name;
    }
    return m_subTabs.Walk(name, m_owner->m_68 == 0);
}

// Iteration accessors (0x13a260..0x13a326). Each fetches a node from the embedded
// collection (FirstSub/FirstSym via CHashBase::First on m_subTabs/+0x38 or
// m_symbols/+0x40) or from the next-link the engine embeds inside a previously-
// returned record (NextSub/NextSym* via CHashElement::Next on the node at rec+OFF),
// then returns that node's payload ([node+0x14]). At the end the node is null and
// the function returns it unchanged (the null falls through, no `xor eax,eax`).
// The Next* offsets (+0x20/+0x04/+0x24/+0x1c) locate the intrusive list node the
// engine embeds inside each heterogeneous record.

// FirstSub (0x13a260): first child-scope record (m_subTabs, +0x38).
RVA(0x0013a260, 0x11)
void* CSymTab::FirstSub() {
    CHashElement* n = m_subTabs.First();
    if (!n) {
        return n;
    }
    return n->m_record;
}

// NextSub (0x13a280): next child-scope record after `rec` (node @ rec+0x20).
RVA(0x0013a280, 0x19)
void* CSymTab::NextSub(void* rec) {
    CHashElement* n = ((CHashElement*)((char*)rec + 0x20))->Next();
    if (!n) {
        return n;
    }
    return n->m_record;
}

// FirstSym (0x13a2b0): first leaf-symbol record (m_symbols, +0x40).
RVA(0x0013a2b0, 0x11)
void* CSymTab::FirstSym() {
    CHashElement* n = m_symbols.First();
    if (!n) {
        return n;
    }
    return n->m_record;
}

// NextSym (0x13a2d0): next leaf-symbol record after `rec` (node @ rec+0x04).
RVA(0x0013a2d0, 0x19)
void* CSymTab::NextSym(void* rec) {
    CHashElement* n = ((CHashElement*)((char*)rec + 0x4))->Next();
    if (!n) {
        return n;
    }
    return n->m_record;
}

// NextSym2 (0x13a2f0): FIRST record of the value sub-collection at rec+0x24 (a
// CSymRec's m_valTable is a whole CHashBase table, not an embedded node), so retail
// calls CHashBase::First (0x184ae0), not CHashElement::Next. reloc_fidelity fix R8.
RVA(0x0013a2f0, 0x19)
void* CSymTab::NextSym2(void* rec) {
    CHashElement* n = ((CHashBase*)((char*)rec + 0x24))->First();
    if (!n) {
        return n;
    }
    return n->m_record;
}

// NextSym3 (0x13a310): next record after `rec` (node @ rec+0x1c).
RVA(0x0013a310, 0x19)
void* CSymTab::NextSym3(void* rec) {
    CHashElement* n = ((CHashElement*)((char*)rec + 0x1c))->Next();
    if (!n) {
        return n;
    }
    return n->m_record;
}

// CreateSub (0x13a330): add a child scope named `name`. If it already exists (the
// m_subTabs walk hits), return 0; otherwise `new CSymTab` (Rez heap, ctor-throw
// cleanup) a child inheriting the owner and parented at this, splice it into
// m_subTabs via its +0x20 hash node, and grow the parser's longest-name counter.
// @early-stop
// EH/regalloc wall (~80%): logic complete (incl. the MakeSymSeed leftover-args ctor
// trick). The /GX ctor-throw cleanup state around `new CSymTab` plus the callee-saved
// register assignment diverge from retail; banked for the final sweep.
RVA(0x0013a330, 0xce)
CSymTab* CSymTab::CreateSub(const char* name) {
    CSymParser* owner = m_owner;
    if (m_subTabs.Walk(name, owner->m_68 == 0) != 0) {
        return 0;
    }
    CSymTab* child = new CSymTab(
        owner,
        this,
        name,
        0,
        0,
        (void*)owner->MakeSeed(),
        owner->m_subTabBucketCount,
        owner->m_symbolBucketCount
    );
    if (!child) {
        return 0;
    }
    m_subTabs.Insert(&child->m_node20);
    if (m_owner->m_longestScopeNameLen <= static_cast<i32>(strlen(name))) {
        m_owner->m_longestScopeNameLen = strlen(name) + 1;
    }
    return child;
}

// AddNamedValue (0x13a400): find/create the int-keyed record for `key`, and if `name`
// is not already present in that record's value sub-table (+0x24), pop a parse-slot,
// build a leaf record into it (the same 11-arg MakeSeed leftover-args trick as
// AddNodeEntry: str2/f3/f1 = 0, f2 = the seed, f6/arr = 0, stream = m_owner's active
// node), splice it in and bump the parser's longest-leaf-name counter. Returns the
// slot (0 when the name already existed / the pop failed). __thiscall, ret 0xc.
// @early-stop
// regalloc wall (~88%): every instruction is structurally byte-exact (verified via
// --diff); the residual is the MSVC5 callee-save coin-flip - retail keeps `rec` in ebp
// and re-`lea`s &rec->m_valTable + puts m_owner in edx, while cl keeps &m_valTable in
// ebp + m_owner in ecx, cascading the reg names through the body. The permuter finds no
// operand-order fix (not source-steerable); same wall its siblings CreateSub/
// ApplyRecursive @early-stop on. Banked for the final sweep.
RVA(0x0013a400, 0xa9)
i32 CSymTab::AddNamedValue(void* a1, void* name, i32 key) {
    CSymRec* rec = FindOrAddSym(key);
    if (rec->m_valTable.Walk((const char*)name, m_owner->m_68 == 0) != 0) {
        return 0;
    }
    CSymLeafBuilder* slot = m_owner->PopParseSlot();
    slot->Build(
        this,
        (const char*)name,
        &rec->m_valTable,
        rec,
        0,
        0,
        0,
        (void*)m_owner->MakeSeed(),
        0,
        0,
        m_owner->m_activeNode
    );
    if (slot == 0) {
        return 0;
    }
    rec->m_valTable.Insert(&slot->m_node);
    u32 len = strlen((char*)name);
    if (static_cast<u32>(m_owner->m_longestLeafNameLen) <= len) {
        m_owner->m_longestLeafNameLen = len + 1;
    }
    return (i32)slot;
}

// AddNodeEntry (0x13a4b0): pop a fresh parse-slot record out of the owner's pool, fill it
// from this leaf record (the 11-arg CSymLeafBuilder::Build with the MakeSymSeed leftover-
// args trick: f2 = the seed, str2/f3/f1 = 0, f6/arr/stream the carried leftover slots),
// splice the built slot's +0x1c hash node into rec's (+0x24) sub-table, then bump the
// parser's longest-leaf-name counter (m_owner->m_longestLeafNameLen). Returns the popped slot. Non-EH
// (no destructible local) so no /GX frame despite the eh-profile unit. __thiscall, ret 0x10.
// 0x13ba70 is reached here as a __thiscall on the owning parser (AddNodeEntry reloads
// ecx=m_owner before the call), unlike the free-call ctor sites; same physical seed
// builder, just dispatched with a `this`. RVA-keyed pairing absorbs the mangling.
RVA(0x0013a4b0, 0x75)
i32 CSymTab::AddNodeEntry(void* a0, void* a1, void* a2, void* a3) {
    CSymLeafBuilder* slot = m_owner->PopParseSlot();
    if (slot == 0) {
        return (i32)slot;
    }
    slot->Build(this, (const char*)a1, a0, a2, 0, 0, 0, (void*)m_owner->MakeSeed(), 0, 0, a3);
    ((CSymRec*)a2)->m_valTable.Insert(&slot->m_node);
    u32 len = strlen((char*)a1);
    if (static_cast<u32>(m_owner->m_longestLeafNameLen) <= len) {
        m_owner->m_longestLeafNameLen = len + 1;
    }
    return (i32)slot;
}

// The removed value-entry's teardown (0x1397a0 = CSymLeafBuilder::Teardown, defined in
// full above in this TU): `found` is the leaf VALUE record CSymLeafBuilder::Build fills.

// CSymTab::AddNodeSubEntry (0x13a530): the
// leaf-merge helper ApplyRange calls when a value key already exists in a leaf record's
// +0x24 value sub-table. Subtract the stale entry's span from this scope's running total
// (m_10), unlink it from the parent record's value sub-table, tear it down, and return it
// to the parser's free list (clearing the parser's re-scan flag m_08). __thiscall(rec,
// found), ret 8; matches the SymTab.h declaration (both params void*, as ApplyRange passes).
RVA(0x0013a530, 0x47)
i32 CSymTab::AddNodeSubEntry(void* rec, void* found) {
    m_10 -= *(i32*)((char*)found + 0xc);
    ((CSymRec*)rec)->m_valTable.Remove((CHashElement*)((char*)found + 0x1c));
    ((CSymLeafBuilder*)found)->Teardown();
    m_owner->AddNode(found);
    m_owner->m_08 = 0;
    return 1;
}

// ApplyRecursive (0x13a580): a2 == 0 is a no-op returning 1. Otherwise null each
// child scope's m_04, run the big range pass (ApplyRange, 0x13a640) over this scope,
// then recurse into every child whose m_04 the pass set, ANDing the results.
// @early-stop
// regalloc wall (~70%): logic complete. Retail pins a2 in ebx and the shared 0
// constant in ebp; the recompile swaps them (a2->ebp, 0->ebx), which cascades through
// every null check + the recursion arg setup. Banked for the final sweep.
RVA(0x0013a580, 0xb2)
i32 CSymTab::ApplyRecursive(i32 a0, i32 a1, i32 a2, i32 a3) {
    i32 ok = 1;
    if (a2 != 0) {
        CHashElement* e = m_subTabs.First();
        while (e) {
            ((CSymTab*)e->m_record)->m_04 = 0;
            e = e->Next();
        }
        if (ApplyRange(a0, a1, a2, a3) == 0) {
            return 0;
        }
        e = m_subTabs.First();
        while (e) {
            CSymTab* sub = (CSymTab*)e->m_record;
            if (sub->m_04 != 0) {
                if (sub->ApplyRecursive(a0, (i32)sub->m_04, (i32)sub->m_08, a3) == 0) {
                    ok = 0;
                }
            }
            e = e->Next();
        }
    }
    return ok;
}

// The parse stream ApplyRange reads each record block out of is the archive reader
// node itself - a CRezDir/CRezItm (both `: CRezItmBase`, <Rez/RezMgr.h>): ApplyRecursive
// forwards the reader (`(i32)node`/`(i32)reader`, LoadEntry/ParseBuffer) as `a0`, and
// stream->Read(pos, 0, len, buf) is CRezItmBase::Read (slot 2 = +0x8). Reloc-masked
// virtual; the `mov eax,[a0]; call [eax+8]` dispatch falls out of the base with no local
// view.

// CSymLeafBuilder (the 11-arg leaf-record builder, 0x139710) is defined at the top
// of this TU in retail-RVA order; ApplyRange below uses it.

// PopParseSlot is CSymParser::PopParseSlot (0x13c0c0); ApplyRange/AddNodeEntry call it
// directly on m_owner (SymParser.h is in scope), so no receiver-view struct is needed.

// @early-stop
// >512 B (0x2f7) /GX leaf-builder loop: the body reproduces both record arms (sub-scope
// merge into m_subTabs incl. the `new CSymTab` ctor-throw cleanup, and the leaf arm's
// FindOrAddSym + +0x24 Walk + the 11-arg builder + the dword-array copy). The plateau is
// the documented heavy-regalloc + /GX trylevel wall plus the tail max-accumulator (a dead
// store retail keeps but cl DCE's) and the differently-named Walk/builder reloc operands.
// The 0x139710 builder's callee-cleanup (ret 0x2c) is inferred from the absence of an
// `add esp,0x2c` after the call; the arg order is the reversed push sequence at 0x13a893.
RVA(0x0013a640, 0x2f7)
i32 CSymTab::ApplyRange(i32 a0, i32 a1, i32 a2, i32 a3) {
    m_10 = 0;
    m_0c = -1;
    i32 maxVal = 0;
    char* buf = (char*)::operator new(static_cast<u32>(a2));
    if (!buf) {
        return 0;
    }
    CRezItmBase* stream = (CRezItmBase*)a0;
    if (stream->Read(a1, 0, a2, buf) != a2) {
        ::operator delete(buf);
        return 0;
    }
    char* p = buf;
    char* end = buf + a2;
    while (p < end) {
        if (*(i32*)p == 1) {
            // sub-scope record: { tag, fA, fB, fC, name\0 }
            void* fA = *(void**)(p + 4);
            p += 8;
            void* fB = *(void**)p;
            void* fC = *(void**)(p + 4);
            p += 8;
            char* name = p;
            p += strlen(name) + 1;
            void* existing = m_subTabs.Walk(name, m_owner->m_68 == 0);
            if (existing == 0) {
                CSymParser* o = m_owner;
                CSymTab* node = new CSymTab(
                    o,
                    this,
                    name,
                    fA,
                    fB,
                    fC,
                    o->m_subTabBucketCount,
                    o->m_symbolBucketCount
                );
                m_subTabs.Insert(&node->m_node20);
            } else {
                ((CSymTab*)existing)->m_04 = fA;
                ((CSymTab*)existing)->m_08 = fB;
                ((CSymTab*)existing)->m_14 = fC;
            }
        } else {
            // leaf record: { tag, f1, f3, f2, f4, f5(key), f6, name\0, str2\0, dwords[f6] }
            void* f1 = *(void**)(p + 4);
            p += 8;
            void* f3 = *(void**)p;
            void* f2 = *(void**)(p + 4);
            p += 8;
            void* f4 = *(void**)p;
            void* f5 = *(void**)(p + 4);
            p += 8;
            void* f6 = *(void**)p;
            p += 4;
            char* name1 = p;
            p += strlen(name1) + 1;
            CSymRec* rec = FindOrAddSym((i32)f5);
            i32 skip = 0;
            void* found = rec->m_valTable.Walk(name1, 1);
            if (found) {
                if (a3 != 0) {
                    AddNodeSubEntry(rec, found);
                } else {
                    skip = 1;
                }
            }
            char* str2 = p;
            if (*str2 == 0) {
                str2 = 0;
            }
            p += strlen(p) + 1;
            void* arr;
            if (f6 != 0) {
                arr = ::operator new(static_cast<u32>(((i32)f6 * 4)));
                for (i32 i = (i32)f6; i != 0; i--) {
                    *(void**)arr = *(void**)p;
                    arr = (char*)arr + 4;
                    p += 4;
                }
                arr = (char*)arr - (i32)f6 * 4;
            } else {
                arr = 0;
            }
            if (!skip) {
                CSymLeafBuilder* slot = m_owner->PopParseSlot();
                slot->Build(this, name1, f4, rec, str2, f3, f1, f2, f6, arr, (void*)a0);
                rec->m_valTable.Insert(&slot->m_node);
                m_10 = m_10 + slot->m_0c;
                if (static_cast<u32>(slot->m_14) < static_cast<u32>(m_0c)) {
                    m_0c = slot->m_14;
                }
                if (static_cast<u32>(slot->m_14) > static_cast<u32>(maxVal)) {
                    maxVal = slot->m_14;
                }
            }
            if (arr) {
                ::operator delete(arr);
            }
        }
    }
    ::operator delete(buf);
    return 1;
}

// FindOrAddSym (0x13a940): look the int key up in m_symbols; if absent, `new CSymRec`
// (Rez heap, ctor-throw cleanup) the right leaf-record flavor (4-arg when the parser's
// m_6c is set, else 3-arg) and splice it into m_symbols via its +0x04 hash node.
// @early-stop
// regalloc wall (~86%): logic complete. Retail dedicates a 4th callee-saved register
// (ebp via FPO) to hold &m_symbols live across the alloc + ctor calls; the recompile
// uses only three and recomputes it, swapping key/&table register roles throughout.
// Banked for the final sweep.
RVA(0x0013a940, 0xc2)
CSymRec* CSymTab::FindOrAddSym(i32 key) {
    CSymRec* found = (CSymRec*)m_symbols.FindInt(static_cast<u32>(key));
    if (found) {
        return found;
    }
    CSymRec* rec;
    if (m_owner->m_6c != 0) {
        rec = new CSymRec(key, this, m_owner->m_74, m_owner->m_70);
    } else {
        rec = new CSymRec(key, this, m_owner->m_70);
    }
    if (rec) {
        m_symbols.Insert(&rec->m_symNode);
    }
    return rec;
}

// ---------------------------------------------------------------------------
// CSymParser::CSymParser() (0x13aa10) - the DEFAULT ctor (Ghidra-mislabeled
// CSymParseConfig::Construct; xref proves it: the 3-arg buf-ctor 0x13ab00 builds
// its discarded `CSymParser tmp;` through it, and RezSync::Init (0x83450) +
// CGruntzMgr::LoadWorldMode (0x91a40) new one). cl auto-stamps ??_7CSymParser @+0;
// the m_list member ctor auto-stamps ??_7CObjList @+0x10; m_hash.Construct(1) builds the
// +0x80 list. Seeds the parse-config defaults; leaves m_34/m_38 untouched. The
// destructible m_hash/m_list members force the /GX EH frame.
// @early-stop
// vptr-first-schedule wall (~72.6%, the SAME plateau as the sibling buf-ctor
// 0x13ab00): every field store + the member Init call is byte-faithful. Residual:
// making CSymParser real-polymorphic makes cl auto-stamp ??_7CSymParser @+0 at ctor
// ENTRY, where retail defers it to AFTER the m_hash member ctor (late) - and that
// pressure shift makes cl cache the constant 1 in ebx (extra push/pop ebx + reg
// stores) where retail re-materializes the `$1` immediates. Plus the /GX scopetable
// state-numbering (push 0xb vs relative 0). Non-source-steerable; see
// docs/patterns/eh-state-numbering-base.md + the buf-ctor's note.
RVA(0x0013aa10, 0xdc)
CSymParser::CSymParser() {
    m_list.m_head = 0;
    m_list.m_tail = 0;
    m_hash.Construct(1);
    m_nodes.m_head = 0;
    m_nodes.m_tail = 0;
    m_parseArmed = 0;
    m_activeNode = 0;
    m_list.m_count = 0;
    m_30 = 0;
    m_3c = 0;
    m_root = 0;
    m_48 = 0;
    m_4c = 0;
    m_50 = 0;
    m_54 = 0;
    m_longestScopeNameLen = 0;
    m_longestLeafNameLen = 0;
    m_60 = 0;
    m_cachedSourceBuffer = 0;
    m_delims = 0;
    m_68 = 0;
    m_6c = 0;
    m_70 = 0x13;
    m_74 = 0x13;
    m_24 = 1;
    m_nextGeneratedFileKey = 0x77359400;
    m_40 = 1;
    m_08 = 1;
    m_2c = 3;
    m_subTabBucketCount = 5;
    m_symbolBucketCount = 9;
    m_parseSlotBlockCount = 0x64;
}

// ---------------------------------------------------------------------------
// 0x13aaf0 - ??1CParserObjList, the standalone out-of-line COMDAT copy of the parser
// list's inline dtor (<Bute/SymParser.h>): 7 bytes, `mov [ecx],??_7CObjListBase; ret`
// (the own-vtable re-stamp is dead-store-eliminated into the inlined ~CObjListBase base
// stamp). It exists because CSymParser's /GX dtor unwind funclets (0x1e0b30/0x1e0b50/
// 0x1e0b90 - xref-verified, its ONLY referents) take the member dtor's address.
// Sibling of ??1CRezList @0x13ca30 (RezFile.cpp). An inline dtor can't hang an RVA(), so
// it is pinned by mangled name.
// @rva-symbol: ??1CParserObjList@@QAE@XZ 0x0013aaf0 0x7

// 0x13ab00: the 3-arg buffer constructor. Construct the sub-object members (the +0x10
// object list, the +0x80 hash table, the +0x88 node list) + stamp the primary vtable,
// build-then-discard a default CSymParser temp, then drive the buffer through
// ParseBuffer. The destructible members + the temp force the /GX EH frame. __thiscall,
// ret 0xc; returns `this`. (The default ctor 0x13aa10 the temp uses lives in another,
// unmatched TU - a reloc-masked call.)
// @early-stop
// ~74.8% (real polymorphic now, ALL-VTABLES phase): the primary vptr is auto-
// stamped by cl @+0 at ctor entry; the
// /GX frame, the member-init store sequence (m_list, the +0x80 hash Init(1), the
// +0x88 node-list), the discarded default-temp ctor/dtor pair and the ParseBuffer
// 3-arg tail are byte-faithful. Residual is the /GX trylevel state-NUMBERING wall
// (docs/patterns/eh-state-numbering-base.md) + the vptr-first schedule. Final sweep.
RVA(0x0013ab00, 0xac)
CSymParser::CSymParser(void* buf, i32 a2, i32 a3) {
    // cl auto-stamps ??_7CSymParser @+0 at ctor entry, and the m_list member ctor
    // auto-stamps ??_7CObjList @+0x10.
    m_list.m_head = 0;
    m_list.m_tail = 0;
    m_hash.Construct(1);
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
// 99.99% (ALL-VTABLES phase): making CSymParser real-polymorphic lets cl auto-stamp
// the vptr @+0 at dtor entry (into EH-state 0, early) - which is exactly the retail
// schedule. Code bytes are byte-identical. (The old 0.009% reloc-NAME residual is
// GONE: m_nodes is the shared DSoundList itself now - the ex-CHashSlotList twin was
// a fake view - so the +0x88 node-list Unlink emits ?Unlink@DSoundList@@, exactly
// the name 0x1391e0 is blessed under.)
RVA(0x0013abc0, 0x13f)
CSymParser::~CSymParser() {
    // cl auto-stamps ??_7CSymParser @+0 at dtor entry (polymorphic class).
    if (m_parseArmed) {
        Clear(0);
    }
    CRezItmBase* p;
    for (p = (CRezItmBase*)m_list.m_head; p != 0; p = (CRezItmBase*)m_list.m_head) {
        m_list.Remove((CObjNode*)p);
        m_list.m_count--;
        delete p; // the slot-1 scalar-deleting dtor (delete emits the same null test)
    }
    CSymTab* root = m_root;
    if (root) {
        delete root; // ~CSymTab non-virtual; CSymTab::operator delete inlines to RezFree
        m_root = 0;
    }
    if (m_cachedSourceBuffer) {
        ::operator delete(m_cachedSourceBuffer);
        m_cachedSourceBuffer = 0;
    }
    if (m_delims) {
        ::operator delete(m_delims);
        m_delims = 0;
    }
    // chain head points at &node->m_link (offset 0 in CSlotNode) - direct reinterpret
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
            ::operator delete(node->m_buffer);
            m_nodes.Unlink(&node->m_link);
            ::operator delete(node);
            node = (CSlotNode*)m_nodes.m_head;
        } while (node);
    }
    // m_hash (RemoveAll) then m_list (vptr restore to 0x5ef760) auto-destruct here,
    // in reverse declaration order, under the /GX member-teardown trylevels.
}

// The two concrete reader nodes ParseBuffer builds are the REAL Rez archive
// classes (<Rez/RezMgr.h>, included above): the text reader is CRezDir (ctor
// 0x13c940 (this, m_2c) stamps ??_7CRezDir @0x5ef7a8, 0x38 B) and the binary
// reader CRezItm (ctor 0x13c540 (this) stamps ??_7CRezItm @0x5ef788, 0x24 B) -
// both `: CRezItmBase`, so `new T(...)` upcasts plainly (base @ offset 0) and the
// list-Link / Open / Read dispatches fall out of the inherited base.

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
        CRezItmBase* reader = new CRezDir(this, m_2c);
        if (reader == 0) {
            ::operator delete(m_cachedSourceBuffer);
            m_cachedSourceBuffer = 0;
            return 0;
        }
        m_activeNode = reader;
        m_list.Link(reader);
        m_list.m_count++;
        if (reader->Open((char*)buf, a, b) == 0) {
            return 0;
        }
        m_parseArmed = 1;
        CSymTab* node = new CSymTab(
            this,
            0,
            g_emptyString,
            0,
            0,
            (void*)this->MakeSeed(),
            m_subTabBucketCount,
            m_symbolBucketCount
        );
        m_root = node;
        ParseRecords(reader, node, m_cachedSourceBuffer, 0);
        return 1;
    }
    // binary stream
    CRezItmBase* reader = new CRezItm(this);
    if (reader == 0) {
        ::operator delete(m_cachedSourceBuffer);
        m_cachedSourceBuffer = 0;
        return 0;
    }
    m_activeNode = reader;
    m_list.Link(reader);
    m_list.m_count++;
    if (reader->Open((char*)buf, a, b) == 0) {
        return 0;
    }
    m_parseArmed = 1;
    if (b != 0) {
        m_3c = 0xa8;
        m_4c = 1;
        CSymTab* node = new CSymTab(
            this,
            0,
            g_emptyString,
            0,
            0,
            (void*)this->MakeSeed(),
            m_subTabBucketCount,
            m_symbolBucketCount
        );
        m_root = node;
        return 1;
    }
    // b == 0: read the 0xa8-byte binary header, copy its packed fields, validate magic.
    char hdr[0xa8];
    reader->Read(0, 0, 0xa8, hdr); // [2] (the view's "ReadRaw")
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

// ---------------------------------------------------------------------------
// The two archive-tree node classes LoadEntry builds are the REAL Rez archive
// readers (<Rez/RezMgr.h>, included above), exactly as ParseBuffer above builds
// them: the directory reader is CRezDir (ctor 0x13c940 (this, maxOpen), size 0x38)
// and the file reader CRezItm (ctor 0x13c540 (this), size 0x24), both `: CRezItmBase`.
// Read is CRezItmBase slot 2 (+0x08), Open slot 4 (+0x10); the `new X(...)` upcasts
// to CRezItmBase* plainly (base @ offset 0) and every dispatch falls out of the
// inherited base with no cast.

// ---------------------------------------------------------------------------
// LoadEntry (0x13b0c0) - mount one archive entry `name` into the scope tree.
// Gate on m_40; cache `name` into m_cachedSourceBuffer; Classify() it. A
// directory (Classify != 0) builds a CRezDir node, links it into m_list, opens
// it and recurses ParseRecords into it. A file builds a CRezItm node, opens it,
// Reads its 0xa8-byte header to fold the running max dims (+0x54..+0x60) and
// runs the root scope's ApplyRecursive over it. The two `new X(...)` sites carry
// the MSVC5 nothrow-new null check + /GX ctor-throw cleanup (trylevel 0 for the
// dir new, 1 for the file new). Called by RezSync::Init to mount GRUNTZ.VRZ/.ZZZ/
// .XXX. Ghidra-mislabeled CRezDir::Stub_13b0c0.
// @early-stop
// 98.3% - STRUCTURALLY byte-exact (verified llvm-objdump -dr base vs target): every
// opcode/ModRM, both nothrow-new null checks AND all three /GX ctor-throw state
// writes (mov [esp+ehstate], 0 / 1 / -1) match retail exactly. The sole residual is
// the MSVC5 scratch-register coin-flip on ~5 load-then-push arg sites (the two m_64
// free cleanups, the ParseRecords/ApplyRecursive/Read arg loads): retail rotates
// eax<-ecx<-edx where cl picks ecx<-edx<-eax for the same temps - identical
// instruction stream, opposite scratch assignment, not source-steerable
// (docs/patterns regalloc coin-flip). Plus the reloc-masked node-ctor / operator-new
// / ParseRecords / ApplyRecursive operands. Parked for the final sweep.
RVA(0x0013b0c0, 0x238)
i32 CSymParser::LoadEntry(char* name, i32 flag) {
    if (m_40 == 0) {
        return 0;
    }
    m_08 = 0;
    if (m_cachedSourceBuffer) {
        ::operator delete(m_cachedSourceBuffer);
    }
    char* buf = (char*)::operator new(strlen(name) + 1);
    m_cachedSourceBuffer = buf;
    strcpy(buf, name);

    if (Classify(name)) {
        CRezItmBase* node = new CRezDir(this, m_2c);
        if (node == 0) {
            ::operator delete(m_cachedSourceBuffer);
            m_cachedSourceBuffer = 0;
            return 0;
        }
        m_list.Link(node);
        m_list.m_count++;
        if (node->Open(name, 1, 0) == 0) {
            return 0;
        }
        m_parseArmed = 1;
        ParseRecords(node, m_root, m_cachedSourceBuffer, flag);
        return 1;
    }

    CRezItmBase* node = new CRezItm(this);
    if (node == 0) {
        ::operator delete(m_cachedSourceBuffer);
        m_cachedSourceBuffer = 0;
        return 0;
    }
    m_list.Link(node);
    m_list.m_count++;
    if (node->Open(name, 1, 0) == 0) {
        return 0;
    }

    char hdr[0xa8];
    node->Read(0, 0, 0xa8, hdr);
    u32 v;
    v = *(u32*)(hdr + 0x97);
    if (v > static_cast<u32>(m_54)) {
        m_54 = v;
    }
    v = *(u32*)(hdr + 0x9b);
    if (v > static_cast<u32>(m_longestScopeNameLen)) {
        m_longestScopeNameLen = v;
    }
    v = *(u32*)(hdr + 0x9f);
    if (v > static_cast<u32>(m_longestLeafNameLen)) {
        m_longestLeafNameLen = v;
    }
    v = *(u32*)(hdr + 0xa3);
    if (v > static_cast<u32>(m_60)) {
        m_60 = v;
    }
    m_root->ApplyRecursive((i32)node, *(i32*)(hdr + 0x83), *(i32*)(hdr + 0x87), flag);
    return 1;
}

// ParseRecords' CRT directory walk is the real _findfirst/_findnext/_findclose
// (0x11f900/0x11fa30/0x11fb50, FID-carved) over <io.h>'s _finddata_t; _strlwr
// (0x18d330), _splitpath (0x18c530), atoi (0x11ff10) and the per-file leaf builders
// (0x13b970 / 0x13cac0) round it out. All reloc-masked. The former hand-rolled
// SymFindData view mis-placed size/name at +0x1c/+0x20 (SIZE 0x128); the ParseRecords
// disasm proves the STANDARD _finddata_t (0x118 B): retail reads fd.attrib at fd+0x00
// (`and eax,0x10 / cmp al,0x10`, _A_SUBDIR) and fd.name at fd+0x14 (fd base at [esp+0x30],
// name at [esp+0x44]). time_t/_fsize_t are 4-byte here, so size@+0x10, name@+0x14 - the
// same layout CustomLevelList uses. The "." / ".." / "\\" / "*.*" comparands are pooled
// .rodata literals.
void SymBuildLeaf(CSymParser* p, void* recArg, void* extKey); // 0x13b970
void SymBindRecord(void* rec, char* name, i32 h);             // 0x13cac0
// The path separator this walker appends (0x60cff0 "\\"). Owner-TU definition:
// this TU is its dominant referencing unit (4 sites; heapdiag/portalpath/
// customworlddialog share it). Length NULL-TERMINATOR-PROVEN ("\\" + NUL = 2 B),
// not a gap guess. extern "C" avoids the const-array mangling that drops DATA().
DATA(0x0020cff0)
const char g_sepSlash[] = "\\"; // decl in <Bute/SymTab.h>
// The find-all glob (0x61a0a0 "*.*"; DEFINED in src/Rez/RezFile.cpp - its .data
// run continues with that TU's "r+b"/"w+b" fopen-mode literals).
extern "C" const char g_wildcard[];
// The two skip-names of the directory walk below. NOTE the previous names were
// SWAPPED relative to the retail bytes: 0x5ee8ec is "." (a CRT .rdata literal in
// the CRT band near the _y* strings; extern only) and 0x60cf90 is ".." (DEFINED in
// src/Gruntz/CustomWorldDialog.cpp, whose .data literal run holds it).
extern char g_dot[];    // 0x5ee8ec  "." (def: Dsndmgr/SoundBankLoad.cpp)
extern char g_dotDot[]; // 0x60cf90  ".."

// @early-stop
// 0x545 (1349 B) /GX recursive directory loader: enumerates `path` with
// _findfirst/_findnext, skips "." / "..", recurses into subdirectories (CreateSub +
// self-call) and folds files into the scope (FindOrAddSym / Insert / the leaf record
// build). This is a structural reconstruction of the control flow + every scope/parser
// call; the >1KB /GX stack frame + alloca probe, the many inline strlen/strcpy/strcmp
// idioms, and the differently-named _findfirst/CSymTab reloc operands are the documented
// walls. Correcting the fd view to the real _finddata_t (name +0x20->+0x14) was
// match-NEUTRAL (55.42%): cl places fd at a wholly different [esp+N] than retail, so
// every fd access diverges on the frame displacement regardless of the intra-struct
// offset - the frame-layout wall dominates. Logic shape complete; parked for the final sweep.
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
    _finddata_t fd;
    i32 h = _findfirst(full, &fd);
    if (h < 0) {
        return 1;
    }
    do {
        if (strcmp(fd.name, g_dot) == 0 || strcmp(fd.name, g_dotDot) == 0) {
            continue;
        }
        if ((fd.attrib & 0x10) == 0x10) {
            // subdirectory: build a child scope and recurse into it
            char childpath[0x600];
            strcpy(childpath, pattern);
            strcpy(childpath + strlen(childpath), fd.name);
            _strlwr(childpath);
            void* child = node->FindSub(fd.name);
            if (child == 0) {
                child = node->CreateSub(fd.name);
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
        _splitpath(fd.name, 0, 0, fname, ext);
        _strlwr(ext);
        i32 nleft = static_cast<i32>(strlen(fname));
        i32 i = 0;
        while (i < nleft && fname[i] >= '0' && fname[i] <= '9') {
            i++;
        }
        i32 key = (i >= nleft) ? atoi(fname) : static_cast<i32>(m_nextGeneratedFileKey++);
        void* extKey = 0;
        if (ext[0] != 0) {
            _strlwr(ext);
            extKey = (void*)PackTag(ext);
        }
        SymBuildLeaf(this, &fd, extKey);
        void* rec = node->FindOrAddSym(key);
        if (node->Insert(fname, extKey) == 0) {
            node->AddNodeEntry((void*)static_cast<u32>(fd.size), rec, full, 0);
        } else if (flag != 0) {
            node->AddNodeSubEntry(rec, extKey);
            node->AddNodeEntry((void*)static_cast<u32>(fd.size), rec, full, 0);
        }
        void* node2 = node->FindOrAddSym(key);
        if (node2) {
            SymBindRecord(node2, full, h);
        }
    } while (_findnext(h, &fd) != 0);
    _findclose(h);
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
i32 CSymParser::Clear(i32 final) {
    (void) final;
    i32 r = m_activeNode->Close(); // [5] (the view's "Detach")
    m_list.Remove((CObjNode*)m_activeNode);
    m_list.m_count--;
    delete m_activeNode; // slot-1 scalar dtor (delete emits the same null test)
    m_activeNode = 0;
    CRezItmBase* p;
    for (p = (CRezItmBase*)m_list.m_head; p != 0; p = (CRezItmBase*)m_list.m_head) {
        p->Close();
        m_list.Remove((CObjNode*)p);
        m_list.m_count--;
        delete p;
    }
    if (m_root) {
        delete m_root; // ~CSymTab non-virtual; CSymTab::operator delete inlines to RezFree
        m_root = 0;
    }
    if (m_cachedSourceBuffer) {
        ::operator delete(m_cachedSourceBuffer);
        m_cachedSourceBuffer = 0;
    }
    m_parseArmed = 0;
    return r;
}

// ---------------------------------------------------------------------------
// The ButeMgr string<->DWORD "tag" pack/unpack free helpers (__stdcall). These are
// the real bodies the SymParser.h "ResolveName"/"SymBuildLeaf" placeholder decls were
// guessing at: PackTag is the file-extension -> int-key mapper (map an upcased name to
// its packed int key), UnpackTag its inverse. Callers: CSymTab::Find (0x13a040),
// CSymParser::ParseRecords (0x13b300). Both bodies byte-exact. (Re-homed from
// src/Stub/BoundaryUpper2.cpp; NO COMDAT folding in MSVC5 - one real home each.)
// ---------------------------------------------------------------------------

// 0x13b910 - pack up to 4 leading chars of a string into a right-justified DWORD
// (reverse byte order). __stdcall, 1 arg.
RVA(0x0013b910, 0x58)
u32 __stdcall PackTag(const char* s) {
    if (!s) {
        return 0;
    }
    u32 r = 0;
    u8* rb = (u8*)&r;
    i32 len = static_cast<i32>(strlen(s));
    if (len > 0) {
        rb[len - 1] = s[0];
    }
    if (len > 1) {
        rb[len - 2] = s[1];
    }
    if (len > 2) {
        rb[len - 3] = s[2];
    }
    if (len > 3) {
        rb[len - 4] = s[3];
    }
    return r;
}

// 0x13b970 - inverse of PackTag: unpack a DWORD tag into a string (high non-zero byte
// first), null-terminated. __stdcall, 2 args (tag, dst).
RVA(0x0013b970, 0x72)
void __stdcall UnpackTag(u32 tag, char* dst) {
    if (!dst) {
        return;
    }
    u8* tb = (u8*)&tag;
    i32 len = 0;
    if (tb[3]) {
        len = 4;
    } else if (tb[2]) {
        len = 3;
    } else if (tb[1]) {
        len = 2;
    } else if (tb[0]) {
        len = 1;
    }
    if (len > 0) {
        dst[0] = tb[len - 1];
    }
    if (len > 1) {
        dst[1] = tb[len - 2];
    }
    if (len > 2) {
        dst[2] = tb[len - 3];
    }
    if (len > 3) {
        dst[3] = tb[len - 4];
    }
    dst[len] = 0;
}

// ---------------------------------------------------------------------------
// The three CSymParser primary vtable slots (??_7CSymParser @0x1ef750). Retail's
// bodies are inert defaults - the parser's "subclass me" hooks, which every shipped
// caller leaves at the base. They land in THIS TU's 0x13b9e2..0x13ba20 gap (between
// UnpackTag and CheckNodes), which is what homes them here rather than proximity.
// Signatures are read off the bytes; see the decl comments in <Bute/SymParser.h>.
// ---------------------------------------------------------------------------

// V0 (0x13b9f0): xor eax,eax; ret 4 - one stack arg, returns 0.
RVA(0x0013b9f0, 0x5)
i32 CSymParser::V0(i32 a) {
    return 0;
}

// V1 (0x13ba00): ret 4 - one stack arg, void.
RVA(0x0013ba00, 0x3)
void CSymParser::V1(i32 a) {}

// V2 (0x13ba10): xor eax,eax; ret - no args, returns 0.
RVA(0x0013ba10, 0x3)
i32 CSymParser::V2() {
    return 0;
}

// CheckNodes (0x13ba20): walk the +0x10 object list, calling each node's slot-7
// probe; return 1 iff every node returned nonzero (accumulates, no early exit).
// __thiscall, no args. Orphan copy (fully inlined at all call sites).
RVA(0x0013ba20, 0x27)
i32 CSymParser::CheckNodes() {
    i32 ok = 1;
    for (CRezItmBase* n = (CRezItmBase*)m_list.m_head; n != 0; n = n->m_next) {
        if (n->Check() == 0) {
            ok = 0;
        }
    }
    return ok;
}

// 0x13ba70 - CSymParser::MakeSeed: the ButeMgr clock seed builder ParseBuffer/
// ParseRecords use (returns time(&t); ignores `this`). It is genuinely __thiscall
// (every caller loads ecx=parser before the call - proven by AddNodeEntry's byte-exact
// `mov ecx,m_owner; call`), so it is modeled as a real CSymParser method.
// 0x120210 == CRT time(). Byte-exact.
RVA(0x0013ba70, 0x10)
i32 CSymParser::MakeSeed() {
    time_t t;
    return static_cast<i32>(time(&t));
}

// SetDelims (0x13ba80): free the current owned delimiter buffer, then own a fresh
// strdup of `s`. __thiscall, 1 arg. Orphan copy (fully inlined at all call sites).
RVA(0x0013ba80, 0x57)
void CSymParser::SetDelims(char* s) {
    if (m_delims != 0) {
        ::operator delete(m_delims);
    }
    m_delims = (char*)::operator new(strlen(s) + 1);
    strcpy(m_delims, s);
}

// @early-stop
// ~95% (was 33% until IsTokenChar was marked __inline - /Ob1 wouldn't inline the plain
// static, so retail's 3x-inlined tokenizer showed as out-of-line calls). Sole residual:
// the token-copy loop's induction-variable representation - retail strength-reduces
// buf[n] to a running [edi+esi] base-offset pointer (edi = &buf - p) where cl keeps the
// indexed buf[n], plus the this->ebp/ebx regalloc coin-flip. Logic byte-faithful.
RVA(0x0013bae0, 0x1b9)
void* CSymTab::ResolvePath(const char* path) {
    char buf[0x30];
    const char* p = path;
    if (static_cast<i32>(strlen(path)) > 1) {
        if (!IsTokenChar(m_owner->m_delims, *p)) {
            ++p;
            path = p;
        }
    }
    i32 n = 0;
    while (IsTokenChar(m_owner->m_delims, *p)) {
        buf[n] = *p;
        ++n;
        ++p;
    }
    buf[n] = 0;
    void* sub = FindSub(buf);
    if (!sub) {
        return sub;
    }
    char c = path[n];
    if (c == 0) {
        return sub;
    }
    while (!IsTokenChar(m_owner->m_delims, c)) {
        c = path[n + 1];
        ++n;
        if (c == 0) {
            return sub;
        }
    }
    return ((CSymTab*)sub)->ResolvePath(path + n);
}

// @early-stop
// ~95% (was 51%): the two fixes were IsTokenChar __inline (2x) and the `qual` copy as
// the strcpy intrinsic strcpy(qual,tail) - retail inlines it (repnz-scas strlen +
// rep-movsd/movsb of strlen+1) where strncpy(...,strlen+1) stayed an out-of-line call.
// The `key` copy is a genuine strncpy(0x120340) call (kept). Residual is the tokenizer
// induction-variable + this-register regalloc coin-flip. Read peer of ResolveQualified.
RVA(0x0013bca0, 0x19c)
void* CSymTab::FindQualified(const char* name) {
    char qual[0x100];
    char key[0x24];
    const char* p = name;
    i32 len = static_cast<i32>(strlen(name));
    if (len > 1) {
        if (!IsTokenChar(m_owner->m_delims, *p)) {
            ++p;
            --len;
        }
    }
    i32 i = len - 1;
    while (!IsTokenChar(m_owner->m_delims, p[i])) {
        --i;
        if (i < 0) {
            break;
        }
    }
    if (i == len) {
        return 0;
    }
    const char* tail = p + i + 1;
    strcpy(qual, tail);
    if (i <= 1) {
        return Find(qual);
    }
    strncpy(key, p, static_cast<u32>(i));
    key[i] = 0;
    CSymTab* scope = (CSymTab*)ResolvePath(key);
    if (!scope) {
        return 0;
    }
    return scope->Find(qual);
}

// @early-stop
// ~92% (was 52%): same two fixes as FindQualified - IsTokenChar __inline + the `qual`
// copy as strcpy(qual,tail) (intrinsic). Residual is the this-register regalloc coin-flip
// (retail keeps `this` in esi; cl uses edx + extra stack reloads) + the tokenizer
// induction variable. The write peer (Insert tail) of FindQualified. Logic byte-faithful.
RVA(0x0013be40, 0x1ac)
i32 CSymTab::ResolveQualified(const char* name, void* arg) {
    char qual[0x100];
    char key[0x24];
    const char* p = name;
    i32 len = static_cast<i32>(strlen(name));
    if (len > 1) {
        if (!IsTokenChar(m_owner->m_delims, *p)) {
            ++p;
            --len;
        }
    }
    i32 i = len - 1;
    while (!IsTokenChar(m_owner->m_delims, p[i])) {
        --i;
        if (i < 0) {
            break;
        }
    }
    if (i == len) {
        return 0;
    }
    const char* tail = p + i + 1;
    strcpy(qual, tail);
    if (i <= 0) {
        return Insert(qual, arg);
    }
    strncpy(key, p, static_cast<u32>(i));
    key[i] = 0;
    CSymTab* scope = (CSymTab*)ResolvePath(key);
    if (!scope) {
        return 0;
    }
    return scope->Insert(qual, arg);
}

// GetRoot (0x13b900): the heap root CSymTab accessor - `mov eax,[ecx+0x44]; ret`.
RVA(0x0013b900, 0x4)
CSymTab* CSymParser::GetRoot() {
    return m_root;
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

// ---------------------------------------------------------------------------
// The CRT _stat (0x18c780, LIBCMT) that the RezStatEntry label aliased. Declared
// with a void* buffer so the RezFindRec byte-view below stays load-bearing.
extern "C" i32 _stat(const char* path, void* statbuf);

// CRezDir::FindEntry(char* name)
// Despite the tomalla "binary search" label, the bytes are a stat: build a
// 0x24-byte find-record on the stack, _stat(name, &rec); on failure
// return 0; on success return whether the entry's attribute dword (at byte +6
// of the record) has bit 0x4000 set (i.e. the entry is a directory).
// `this` is never read here.
RVA(0x0013c080, 0x3c)
i32 CRezDir::FindEntry(char* name) {
    RezFindRec rec;
    if (_stat(name, &rec) != 0) {
        return 0;
    }
    // Language-forced int-view over the fixed byte record: the entry's attribute
    // dword sits at the packed (unaligned) offset +6 of the 0x24-byte find-record;
    // bit 0x4000 marks a directory. Reading a dword from a byte buffer needs the cast.
    return (*(i32*)(rec.raw + 6) & 0x4000) == 0x4000;
}

// A parse-slot record is the 0x3c CSymLeafBuilder leaf record (m_node @+0x1c, self-ptr
// @+0x30 == m_node.m_record). A freshly-popped slot is init'd as a CParseSource parse
// stream (Init @0x1396f0 stamps its node vtable + nulls the body; reloc-masked
// __thiscall) and later repurposed by Build into a leaf value record - one 0x3c memory,
// two views.

// PopParseSlot (0x13c0c0): see SymParser.h. The /GX frame guards the freshly Rez-
// alloc'd slot block while its elements are being initialized + registered.
// @early-stop
// EH-state + regalloc wall (~77%): logic complete. The node/array allocations land in
// a swapped callee-saved register (ebx vs ebp), the operator-new trylevel transitions
// + the slot-block down-counter init loop idiom diverge, and the hash-method reloc
// operands are differently named. Banked for the final sweep.
RVA(0x0013c0c0, 0x14b)
CSymLeafBuilder* CSymParser::PopParseSlot() {
    CHashElement* e = m_hash.First();
    void* rec = e ? e->m_record : 0;
    if (rec == 0) {
        CSlotNode* node = (CSlotNode*)RezAlloc(0xc);
        if (node == 0) {
            return 0;
        }
        i32 n = m_parseSlotBlockCount;
        CSymLeafBuilder* arr = (CSymLeafBuilder*)RezAlloc(n * 0x3c);
        if (arr) {
            CSymLeafBuilder* p = arr;
            i32 i = n;
            i--;
            if (i >= 0) {
                i++;
                do {
                    ((CParseSource*)p)->Init();
                    p++;
                    i--;
                } while (i);
            }
        }
        node->m_buffer = arr;
        if (arr == 0) {
            ::operator delete(node);
            return 0;
        }
        for (i32 j = 0; static_cast<u32>(j) < static_cast<u32>(m_parseSlotBlockCount); j++) {
            CSymLeafBuilder* el = &node->m_buffer[j];
            el->m_node.m_record = el;
            m_hash.Insert(&el->m_node);
        }
        m_nodes.InsertHead(&node->m_link);
        e = m_hash.First();
        rec = e->m_record;
    }
    if (rec) {
        m_hash.Remove(&((CSymLeafBuilder*)rec)->m_node);
    }
    return (CSymLeafBuilder*)rec;
}

// AddNode (0x13c210): splice a parse-slot record's intrusive node (its m_node @0x1c)
// into the +0x80 hash table, when rec is non-null.
RVA(0x0013c210, 0x1a)
void CSymParser::AddNode(void* rec) {
    if (rec) {
        m_hash.Insert(&((CSymLeafBuilder*)rec)->m_node);
    }
}

// CObjList::Remove (0x1852e0) moved to src/Rez/RezList.cpp (its retail
// emission sits at the rezlist obj's tail in the 0x1832d0 engine-util pocket, far
// from this TU's 0x13axxx core; callers here reference it externally).

// CHashBase::Construct (0x184960) + CHash::CHash() (0x184950)
// + the 0x184900 hash reverse-iterator gap live in src/Rez/RezColl.cpp (the
// 0x1832d0-pocket rez/sym/hash utility obj is ONE original TU; this file keeps the
// 0x139xxx/0x13axxx CSymTab core).
// All SIZE()s are annotated atop their class definitions (this TU's .cpp-local
// structs above; SymTab.h for CSymRec/CSymTab; Bute/Hash.h for the canonical hash
// classes CHashBase/CHashElement/CHash/CHashB). CSymParser is annotated in SymParser.h.

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CParseSource);
SIZE_UNKNOWN(ParseMappedSource);
SIZE_UNKNOWN(ParseVReader);
