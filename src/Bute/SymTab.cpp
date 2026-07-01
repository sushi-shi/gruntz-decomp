// SymTab.cpp - CSymTab, the Remus/ButeMgr hierarchical symbol table (the tree of
// named scopes the .bute parser builds). Recovered from the this/ecx trace group
// "ClassUnknown_12" (4 methods at 0x139ee0/0x13a230/0x13bae0/0x13be40), all on a
// single object shape with two embedded engine hash tables at +0x38/+0x40 and the
// owning parser at +0x18. See include/Bute/SymTab.h for the full layout.
#include <rva.h>

#include <Bute/SymParser.h> // full CSymParser (m_owner layout) + CSymTab via SymParser.h

// The child-scope hash-node vtable (the key-hash interface a scope exposes to its
// parent's m_subTabs). Manual-stamp model -> reloc-masked DATA() extern.
DATA(0x005ef748)
void* CSymTab_node_vftable;

// A leaf record's parse stream: EndParse releases its inline buffer (0x1399d0,
// CRemusReadStream); reloc-masked __thiscall, modeled with no body here.
class CRemusReadStream {
public:
    i32 EndParse(); // 0x1399d0
};

// The tokenizer's "is this character part of a token" predicate, inlined at each
// scan site. When the parser supplies a delimiter set, a token char is one NOT in
// it (strchr == 0); otherwise the default identifier classes apply: printable
// punctuation/space .. '.', digits, upper, lower. The `== 0` coercion emits the
// neg/sbb/inc int->bool normalize that retail uses (docs/patterns/
// int-to-bool-normalize.md).
static i32 IsTokenChar(const char* delims, char ch) {
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

// The 11-arg leaf-record builder's record shape (the parse slot the owner pops). The
// leaf builder fills it from a parse-stream record: the strdup'd name at +0x00, the
// owning scope at +0x10, the back-pointer to the symbol record at +0x04, three parse
// values (f1/f2/f3) at +0x14/+0x08/+0x0c, the source stream at +0x34, a self-pointer
// at +0x30, and two zeroed counters (+0x18/+0x38). ApplyRange reads +0x0c (added into
// m_10) and +0x14 (the min/max accumulator key) back out. Field names are placeholders.
struct CSymLeafBuilder {
    void Build(
        void* owner,
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
    );                  // 0x139710
    char* m_name;       // +0x00  strdup(name) (or null)
    void* m_record;     // +0x04  rec
    void* m_08;         // +0x08  f2
    i32 m_0c;           // +0x0c  f3   (read by ApplyRange)
    void* m_ownerScope; // +0x10  owning scope
    i32 m_14;           // +0x14  f1   (read by ApplyRange)
    i32 m_18;           // +0x18  = 0
    char m_pad1c[0x30 - 0x1c];
    void* m_self;         // +0x30  self
    void* m_sourceStream; // +0x34  source stream
    i32 m_38;             // +0x38  = 0
};

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
    void* owner,
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
    m_self = this;
}

// ctor (0x139de0): stamp the +0x20 hash-node vtable + a zeroed +0x34 (both in the
// init list so they precede the member ctors), build the two embedded hash tables
// (m_subTabs(subN) then m_symbols(symN) - the /GX member-construction trylevels go
// -1 -> 0 -> 1), copy `name` through the throwing ::operator new (so the state-1
// transition before it falls out), then store the remaining fields and re-point m_34
// at this. Returns this.
// @early-stop
// reloc-name plateau: every CODE byte matches retail. The residual is purely
// differently-named reloc operands (the two CHashTable::Init = MallocCtor_184960
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
    : m_node20((void*)&CSymTab_node_vftable), m_34(0), m_subTabs(subN), m_symbols(symN) {
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
    m_34 = (void*)this;
}

// ~CSymTab (0x139ee0): tear down the scope tree. Walk the leaf-symbol table
// (m_symbols, +0x40) clearing+freeing each record, then the child-scope table
// (m_subTabs, +0x38) recursing ~CSymTab on each, then free the owned buffers and
// null the fields. The two CHashTable members auto-destruct after the body, in
// reverse declaration order (m_symbols then m_subTabs) at descending trylevels --
// the /GX member-teardown frame (docs/patterns/eh-dtor-model-members-as-
// destructible.md).
RVA(0x00139ee0, 0x11e)
CSymTab::~CSymTab() {
    CHashTableEntry* cur;
    for (cur = m_symbols.First(); cur != 0;) {
        CHashTableEntry* next = m_symbols.Next(cur);
        m_symbols.Remove(cur);
        CSymRec* rec = (CSymRec*)cur->m_payload;
        if (rec) {
            rec->Clear();
            RezFree(rec);
        }
        cur = next;
    }
    for (cur = m_subTabs.First(); cur != 0;) {
        CHashTableEntry* next = m_subTabs.Next(cur);
        m_subTabs.Remove(cur);
        CSymTab* sub = (CSymTab*)cur->m_payload;
        if (sub) {
            sub->~CSymTab();
            RezFree(sub);
        }
        cur = next;
    }
    if (m_name) {
        RezFree(m_name);
    }
    if (m_buf48) {
        RezFree(m_buf48);
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
    m_34 = 0;
    // m_symbols, m_subTabs destruct here (reverse decl order, /GX trylevels).
}

// Insert (0x13a000): the ResolveQualified tail. Look the record up for `arg` in
// this scope's leaf table (m_symbols, +0x40); if present, walk its embedded
// sub-table (record+0x24) for `key`, forwarding m_owner->m_68 == 0 as the flag.
// The `m_68 == 0` is the int->bool sete. __thiscall, callee-clean of both args.
RVA(0x0013a000, 0x37)
i32 CSymTab::Insert(const char* key, void* arg) {
    void* rec = m_symbols.Find((const char*)arg);
    if (!rec) {
        return (i32)rec;
    }
    return (i32)((CHashTable*)((char*)rec + 0x24))->Walk(key, m_owner->m_68 == 0);
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
        arg = m_owner->ResolveName(tmp);
    } else {
        arg = 0;
    }
    return (void*)Insert(fname, arg);
}

// ReleaseParseBuffers (0x13a190): drop this scope's cached parse state. If the owned
// +0x48 buffer is live, free it; otherwise walk the leaf-symbol table ending each
// record's parse stream (EndParse over the NextSym2/NextSym3 sub-chain). When
// `recurse` is set, descend into every child scope (m_subTabs). Returns 1.
RVA(0x0013a190, 0x94)
i32 CSymTab::ReleaseParseBuffers(i32 recurse) {
    if (m_buf48 != 0) {
        RezFree(m_buf48);
        m_buf48 = 0;
    } else {
        void* rec = FirstSym();
        while (rec) {
            void* sub = NextSym2(rec);
            while (sub) {
                ((CRemusReadStream*)sub)->EndParse();
                sub = NextSym3(sub);
            }
            rec = NextSym(rec);
        }
    }
    if (recurse) {
        RezNode* e = ((RezColl*)&m_subTabs)->First();
        while (e) {
            ((CSymTab*)e->m_14)->ReleaseParseBuffers(1);
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
// collection (FirstSub/FirstSym via RezColl::First on m_subTabs/+0x38 or
// m_symbols/+0x40) or from the next-link the engine embeds inside a previously-
// returned record (NextSub/NextSym* via RezNode::Next on the node at rec+OFF),
// then returns that node's payload ([node+0x14]). At the end the node is null and
// the function returns it unchanged (the null falls through, no `xor eax,eax`).
// The Next* offsets (+0x20/+0x04/+0x24/+0x1c) locate the intrusive list node the
// engine embeds inside each heterogeneous record.

// FirstSub (0x13a260): first child-scope record (m_subTabs, +0x38).
RVA(0x0013a260, 0x11)
void* CSymTab::FirstSub() {
    RezNode* n = ((RezColl*)&m_subTabs)->First();
    if (!n) {
        return n;
    }
    return n->m_14;
}

// NextSub (0x13a280): next child-scope record after `rec` (node @ rec+0x20).
RVA(0x0013a280, 0x19)
void* CSymTab::NextSub(void* rec) {
    RezNode* n = ((RezNode*)((char*)rec + 0x20))->Next();
    if (!n) {
        return n;
    }
    return n->m_14;
}

// FirstSym (0x13a2b0): first leaf-symbol record (m_symbols, +0x40).
RVA(0x0013a2b0, 0x11)
void* CSymTab::FirstSym() {
    RezNode* n = ((RezColl*)&m_symbols)->First();
    if (!n) {
        return n;
    }
    return n->m_14;
}

// NextSym (0x13a2d0): next leaf-symbol record after `rec` (node @ rec+0x04).
RVA(0x0013a2d0, 0x19)
void* CSymTab::NextSym(void* rec) {
    RezNode* n = ((RezNode*)((char*)rec + 0x4))->Next();
    if (!n) {
        return n;
    }
    return n->m_14;
}

// NextSym2 (0x13a2f0): next record after `rec` (node @ rec+0x24).
RVA(0x0013a2f0, 0x19)
void* CSymTab::NextSym2(void* rec) {
    RezNode* n = ((RezNode*)((char*)rec + 0x24))->Next();
    if (!n) {
        return n;
    }
    return n->m_14;
}

// NextSym3 (0x13a310): next record after `rec` (node @ rec+0x1c).
RVA(0x0013a310, 0x19)
void* CSymTab::NextSym3(void* rec) {
    RezNode* n = ((RezNode*)((char*)rec + 0x1c))->Next();
    if (!n) {
        return n;
    }
    return n->m_14;
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
        (void*)MakeSymSeed(),
        owner->m_subTabBucketCount,
        owner->m_symbolBucketCount
    );
    if (!child) {
        return 0;
    }
    m_subTabs.Insert((char*)child + 0x20);
    if (m_owner->m_longestScopeNameLen <= (i32)strlen(name)) {
        m_owner->m_longestScopeNameLen = strlen(name) + 1;
    }
    return child;
}

// Method4b0 (0x13a4b0): pop a fresh parse-slot record out of the owner's pool, fill it
// from this leaf record (the 11-arg CSymLeafBuilder::Build with the MakeSymSeed leftover-
// args trick: f2 = the seed, str2/f3/f1 = 0, f6/arr/stream the carried leftover slots),
// splice the built slot's +0x1c hash node into rec's (+0x24) sub-table, then bump the
// parser's longest-leaf-name counter (m_owner->m_longestLeafNameLen). Returns the popped slot. Non-EH
// (no destructible local) so no /GX frame despite the eh-profile unit. __thiscall, ret 0x10.
// 0x13ba70 is reached here as a __thiscall on the owning parser (Method4b0 reloads
// ecx=m_owner before the call), unlike the free-call ctor sites; same physical seed
// builder, just dispatched with a `this`. RVA-keyed pairing absorbs the mangling.
struct CSymSeedOwner {
    i32 MakeSeed(); // 0x13ba70 (__thiscall on m_owner)
};

RVA(0x0013a4b0, 0x75)
i32 CSymTab::Method4b0(void* a0, void* a1, void* a2, void* a3) {
    CSymLeafBuilder* slot = (CSymLeafBuilder*)m_owner->PopParseSlot();
    if (slot == 0) {
        return (i32)slot;
    }
    slot->Build(
        this,
        (const char*)a1,
        a0,
        a2,
        0,
        0,
        0,
        (void*)((CSymSeedOwner*)m_owner)->MakeSeed(),
        0,
        0,
        a3
    );
    ((CHashTable*)((char*)a2 + 0x24))->Insert((char*)slot + 0x1c);
    u32 len = strlen((char*)a1);
    if ((u32)m_owner->m_longestLeafNameLen <= len) {
        m_owner->m_longestLeafNameLen = len + 1;
    }
    return (i32)slot;
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
        RezNode* e = ((RezColl*)&m_subTabs)->First();
        while (e) {
            ((CSymTab*)e->m_14)->m_04 = 0;
            e = e->Next();
        }
        if (ApplyRange(a0, a1, a2, a3) == 0) {
            return 0;
        }
        e = ((RezColl*)&m_subTabs)->First();
        while (e) {
            CSymTab* sub = (CSymTab*)e->m_14;
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

// The parse stream ApplyRange reads each record block out of: stream->Read(pos, 0,
// len, buf) (vtable slot 2 = +0x8) fills `len` bytes and returns the count read.
// Reloc-masked virtual; modeled polymorphically so `mov eax,[a0]; call [eax+8]` falls
// out with no cast.
struct CSymRangeStream {
    virtual void s0();
    virtual void s1();
    virtual i32 Read(i32 pos, i32 zero, i32 len, void* buf); // slot 2 (+0x8)
};

// CSymLeafBuilder (the 11-arg leaf-record builder, 0x139710) is defined at the top
// of this TU in retail-RVA order; ApplyRange below uses it.

// The owner's parse-slot pool (CSymParser::PopParseSlot @0x13c0c0). Reloc-masked.
struct CSymSlotPool {
    void* PopParseSlot(); // 0x13c0c0
};

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
    m_0c = (void*)-1;
    i32 maxVal = 0;
    char* buf = (char*)::operator new((u32)a2);
    if (!buf) {
        return 0;
    }
    CSymRangeStream* stream = (CSymRangeStream*)a0;
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
                m_subTabs.Insert((char*)node + 0x20);
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
            void* rec = (void*)FindOrAddSym((i32)f5);
            i32 skip = 0;
            void* found = ((CHashTable*)((char*)rec + 0x24))->Walk(name1, 1);
            if (found) {
                if (a3 != 0) {
                    Method530(rec, found);
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
                arr = ::operator new((u32)((i32)f6 * 4));
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
                CSymLeafBuilder* slot = (CSymLeafBuilder*)((CSymSlotPool*)m_owner)->PopParseSlot();
                slot->Build(this, name1, f4, rec, str2, f3, f1, f2, f6, arr, (void*)a0);
                ((CHashTable*)((char*)rec + 0x24))->Insert((char*)slot + 0x1c);
                m_10 = (void*)((i32)m_10 + slot->m_0c);
                if ((u32)slot->m_14 < (u32)(i32)m_0c) {
                    m_0c = (void*)slot->m_14;
                }
                if ((u32)slot->m_14 > (u32)maxVal) {
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
void* CSymTab::FindOrAddSym(i32 key) {
    void* found = m_symbols.Find((const char*)key);
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
        m_symbols.Insert((char*)rec + 4);
    }
    return rec;
}

// @early-stop
// recursive path tokenizer; the inlined IsTokenChar (3x) + the working-pointer arg
// reuse + the FindSub recursion schedule against a documented regalloc/scheduling
// wall. Logic complete; byte-match parked for the final sweep.
RVA(0x0013bae0, 0x1b9)
void* CSymTab::ResolvePath(const char* path) {
    char buf[0x30];
    const char* p = path;
    if ((i32)strlen(path) > 1) {
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
// last-delimiter split + scope resolve; inlined IsTokenChar (2x) + the rep-movs token
// copy + the Find tail. The read counterpart of ResolveQualified (below) -- same
// regalloc/scheduling wall (inlined tokenizer + working-pointer reuse). Logic complete.
RVA(0x0013bca0, 0x19c)
void* CSymTab::FindQualified(const char* name) {
    char qual[0x100];
    char key[0x24];
    const char* p = name;
    i32 len = (i32)strlen(name);
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
    strncpy(qual, tail, strlen(tail) + 1);
    if (i <= 1) {
        return Find(qual);
    }
    strncpy(key, p, (u32)i);
    key[i] = 0;
    CSymTab* scope = (CSymTab*)ResolvePath(key);
    if (!scope) {
        return 0;
    }
    return scope->Find(qual);
}

// @early-stop
// last-delimiter split + scope resolve; inlined IsTokenChar + the rep-movs token
// copy + the SymTab_InsertResolved tail. Logic complete; byte-match parked.
RVA(0x0013be40, 0x1ac)
i32 CSymTab::ResolveQualified(const char* name, void* arg) {
    char qual[0x100];
    char key[0x24];
    const char* p = name;
    i32 len = (i32)strlen(name);
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
    strncpy(qual, tail, strlen(tail) + 1);
    if (i <= 0) {
        return Insert(qual, arg);
    }
    strncpy(key, p, (u32)i);
    key[i] = 0;
    CSymTab* scope = (CSymTab*)ResolvePath(key);
    if (!scope) {
        return 0;
    }
    return scope->Insert(qual, arg);
}

// CSymParser (also in SymTab.h) is annotated in SymParser.cpp.
SIZE_UNKNOWN(RezColl);
SIZE_UNKNOWN(RezNode);
SIZE_UNKNOWN(CHashTable);
SIZE_UNKNOWN(CHashTableEntry);
SIZE_UNKNOWN(CSymRec);
SIZE_UNKNOWN(CSymTab);
SIZE_UNKNOWN(CSymLeafBuilder);
SIZE_UNKNOWN(CSymSeedOwner);
SIZE_UNKNOWN(CSymRangeStream); // declared-but-undefined virtual slots; no vtable here (no VTBL)
SIZE_UNKNOWN(CSymSlotPool);
