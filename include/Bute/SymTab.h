#ifndef SRC_BUTE_SYMTAB_H
#define SRC_BUTE_SYMTAB_H

#include <Ints.h>

#include <Bute/Hash.h> // CHashElement (the embeddable hash-node prefix records carry)

void* operator new(u32 size);
#include <Rez/RezAlloc.h> // RezAlloc/RezFree (the global allocator pair)

struct CSymTabNode : public CHashElement {
    // Slot 0 (0x13c3b0): m_owner->HashStr(m_record's key), on the CHashB child-scope
    // instantiation. Defined out-of-line in Hash.cpp (retail emits it beside that
    // instantiation's HashStr/Walk - the interleave proves the TU).
    virtual u32 Hash() OVERRIDE;
    // Stamp the vptr + zero m_record - the exact two stores CSymTab's ctor (0x139de0)
    // opens with (the ex model spelled them `m_node20(&CSymTab_node_vftable), m_34(0)`
    // by hand). The enclosing ctor then re-points m_record at `this`, which is why
    // retail keeps the "dead" zero store: cl does not DSE it across the inlined
    // member ctor. Identical to CParseSlotHashNode/CSymRecNode's inline ctors.
    CSymTabNode() {
        m_record = 0;
    }
};
SIZE(CSymTabNode, 0x18); // no new fields over CHashElement
VTBL(CSymTabNode, 0x001ef748);

u32 __stdcall PackTag(const char* s);         // 0x13b910
void __stdcall UnpackTag(u32 tag, char* dst); // 0x13b970

class CSymParser;

class CSymTab; // fwd (CSymRec keeps the owning-scope back-ptr at +0x2c)

struct CSymRecNode : public CHashElement {
    // Slot 0 override: the int-key bucket hash (sub_13c340; declared-only -> the
    // emitted ??_7CSymRecNode slot reloc-masks against the retail 0x1ef744 datum).
    virtual u32 Hash() OVERRIDE;
    CSymRecNode() {
        m_record = 0;
    }
};
SIZE(CSymRecNode, 0x18); // no new fields over CHashElement
VTBL(CSymRecNode, 0x001ef744);

class CSymRec {
public:
    // The two leaf-record ctors (selected by m_owner->m_6c). Both stamp the +0x04
    // node prefix (CSymRecNode's inlined ctor), build the two hash-table members
    // (destructible -> the /GX member-construction frame), then wire key/back-ptr.
    CSymRec(i32 key, CSymTab* owner, i32 c, i32 d); // 0x139bf0 (m_6c != 0)
    CSymRec(i32 key, CSymTab* owner, i32 c);        // 0x139c80 (m_6c == 0)
    // ~CSymRec (0x139cf0, the "Clear" teardown): drain m_keyTable (iff the owning
    // parser's m_6c is set) and m_valTable (re-filing each payload to the parser's
    // free pool); the two members then auto-destruct (RemoveAll) in reverse order.
    ~CSymRec();
    void* operator new(u32 n) {
        return RezAlloc(n);
    }
    void operator delete(void* p) {
        RezFree(p);
    }

    i32 m_key;             // +0x00  int key (m_symbols hashes on this)
    CSymRecNode m_symNode; // +0x04  hash-node prefix spliced into m_symbols (vtbl 0x1ef744)
    CHash m_keyTable;      // +0x1c  second live container (drained iff parser m_6c)
    CHash m_valTable;      // +0x24  the record's value sub-table (Walk 0x13c270)
    CSymTab* m_scope;      // +0x2c  the owning scope (back-ptr)
};
SIZE(CSymRec, 0x30); // leaf-record allocation size (operator new -> RezAlloc)

struct CParseSource; // <Gruntz/ParseSource.h>

class CSymTab {
public:
    // ctor (0x139de0): stamp the +0x20 hash-node vtable + the +0x34 self-ptr, build
    // the two embedded hash tables (m_subTabs(subN), m_symbols(symN) - the /GX
    // member-construction frame), copy the scope name, then store the rest. The
    // +0x20/+0x34 stamps go in the init list so they precede the member ctors.
    CSymTab(
        CSymParser* owner,
        CSymTab* parent,
        const char* name,
        i32 dataOff,
        i32 dataSize,
        i32 seed,
        i32 subN,
        i32 symN
    );

    // ~CSymTab (0x139ee0): walk both tables freeing each entry's record + node,
    // free the owned buffers, null the fields; the two hash-table members (CHashB
    // then CHash) then auto-destruct at descending trylevels (the /GX frame).
    ~CSymTab();

    // CSymTab uses the Rez heap (nothrow): operator new = RezAlloc(0x4c), delete =
    // RezFree, so `new CSymTab(...)` drives the alloc + ctor-throw cleanup EH pattern.
    void* operator new(u32 n) {
        return RezAlloc(n);
    }
    void operator delete(void* p) {
        RezFree(p);
    }

    // CreateSub (0x13a330): if `name` is not already a child scope, heap-alloc a new
    // CSymTab child (owner inherited, parent=this), insert it into m_subTabs, and
    // bump the parser's longest-name counter. Returns the child (or 0).
    CSymTab* CreateSub(const char* name);

    // FindOrAddSym (0x13a940): find the int-keyed leaf record, or build one (the ctor
    // variant chosen by m_owner->m_6c) and insert it into m_symbols. Returns it.
    CSymRec* FindOrAddSym(i32 key);

    // AddNamedValue (0x13a400): find/create the int-keyed record (FindOrAddSym), then
    // if `name` is not already in that record's value sub-table, pop a parse-slot,
    // build a leaf record for it and splice it into the record's +0x24 sub-table,
    // bumping the parser's longest-leaf-name counter. Returns the slot (0 if the name
    // already existed or the slot pop failed). Higher-level twin of AddNodeEntry.
    i32 AddNamedValue(void* a1, void* name, i32 key); // 0x13a400

    // ApplyRecursive (0x13a580): clear each child's m_04, run the range operation
    // (0x13a640) over this scope, then recurse into children whose m_04 was set.
    // Returns 1 unless a recursion failed. (a2 == 0 is a no-op returning 1.)
    i32 ApplyRecursive(i32 a0, i32 a1, i32 a2, i32 a3);

    // The big range operation 0x13a640 invoked by ApplyRecursive: reads a block of
    // (a0-stream) records into a temp buffer and folds each into this scope's tables
    // (sub-scope records -> m_subTabs; leaf records -> the leaf builder + m_symbols).
    i32 ApplyRange(i32 a0, i32 a1, i32 a2, i32 a3); // 0x13a640

    // AddNodeEntry (0x13a4b0): pop a fresh parse-slot, build a leaf record into it from
    // (name=a1, rec=a2, f4=a0, stream=a3), splice it into rec's +0x24 sub-table, and
    // bump the parser's longest-leaf-name counter (m_owner->m_longestLeafNameLen).
    // Returns the slot.
    i32 AddNodeEntry(void* a0, void* a1, void* a2, void* a3); // 0x13a4b0

    // The leaf-merge helper ApplyRange calls when a leaf record's +0x24 sub-table walk
    // already has the key (0x13a530, __thiscall(rec, found)). Reloc-masked extern.
    i32 AddNodeSubEntry(void* rec, void* found); // 0x13a530

    // Walk m_subTabs (+0x38) for `name`, forwarding m_owner->m_68 == 0 (0x13a230).
    class CSymTab*
    Get_13b900(); // 0x13b900 (unnamed CSymTab sub-tab getter, in the 0x13axxx CSymTab region)
    void* FindSub(const char* name);

    // Resolve/insert a dotted path under this scope (0x13bae0): tokenize the first
    // segment, recurse on the remainder.
    void* ResolvePath(const char* path);

    // Resolve a fully-qualified name by its last delimiter (0x13be40): split off
    // the trailing key, resolve the leading scope, then dispatch.
    struct CParseSource* ResolveQualified(const char* name, void* arg); // returns the leaf parse record

    // Insert/resolve `key` directly into this scope's leaf table (+0x40), passing
    // m_owner->m_68 == 0 (0x13a000; the ResolveQualified tail). __thiscall extern,
    // no body -> reloc-masked. (Currently labeled ClassUnknown_14; a CSymTab method.)
    struct CParseSource* Insert(const char* key, void* arg); // returns the leaf parse record

    // Look `key` up directly in this scope's leaf table (the +0x40 path); the read
    // counterpart of Insert (0x13a040, one-arg __thiscall). Reloc-masked extern.
    void* Find(const char* key);

    // Find a fully-qualified name by its last delimiter (0x13bca0): split off the
    // trailing key, resolve the leading scope (ResolvePath), then Find. The read
    // counterpart of ResolveQualified.
    void* FindQualified(const char* name);

    // Drop this scope's cached parse state (0x13a190): free the owned +0x48 buffer
    // if live, else end each leaf record's parse stream; when `recurse`, descend into
    // every child scope. Returns 1.
    i32 ReleaseParseBuffers(i32 recurse);

    // Iteration accessors over the two embedded tables. First* are __thiscall on
    // the scope; Next* take a previously-returned record and advance via the node
    // the engine embeds inside it at a fixed offset (each record IS an intrusive
    // list node). All return the entry's payload ([entry+0x14]), or 0 at the end.
    void* FirstSub();          // 0x13a260  m_subTabs.First()->payload
    void* NextSub(void* rec);  // 0x13a280  (rec+0x20)->Next()->payload
    void* FirstSym();          // 0x13a2b0  m_symbols.First()->payload
    void* NextSym(void* rec);  // 0x13a2d0  (rec+0x04)->Next()->payload
    void* NextSym2(void* rec); // 0x13a2f0  (rec+0x24)->Next()->payload
    void* NextSym3(void* rec); // 0x13a310  (rec+0x1c)->Next()->payload

    char* m_name; // +0x00
    // +0x04/+0x08 are the scope's data EXTENT in the parse stream, not pointers. Proven
    // by what ApplyRange (their only consumer) does with them: ApplyRecursive forwards
    // them as `(a1, a2)`, and ApplyRange spends a1 as the read OFFSET and a2 as a byte
    // COUNT - `::operator new(a2)`, `stream->Read(a1, 0, a2, buf)`, `end = buf + a2`.
    // They arrive as two raw dwords of the sub-scope record ({tag, fA, fB, fC, name}).
    // The void* was what forced the `(i32)sub->m_04` / `(i32)sub->m_08` reads.
    i32 m_dataOff;       // +0x04  sub-scope data offset in the stream
    i32 m_dataSize;      // +0x08  sub-scope data byte count
    i32 m_baseOffset;    // +0x0c  the scope's base file offset (ApplyRange min-accumulates it,
                         //        seed -1; the parse-stream side reads it as the mapped-window base)
    i32 m_10;            // +0x10  sum-accumulator in ApplyRange
    // +0x14 the name-keyed clock seed: every construction site passes
    // `(void*)owner->MakeSeed()`, and MakeSeed (0x13ba70) returns i32 - the cast existed
    // only to squeeze an int through a void* slot. (Never read back in this TU; its
    // SOURCE is what proves the type.)
    i32 m_seed;          // +0x14  the parser's name-keyed clock seed
    CSymParser* m_owner; // +0x18
    // +0x1c the PARENT scope. All three construction sites say so: the root
    // (CSymParser::ParseBuffer) passes 0 - it has no parent - and both child sites
    // (CreateSub 0x13a330 / the ApplyRange sub-scope arm) pass `this`, a CSymTab*.
    CSymTab* m_parent;   // +0x1c
    // +0x20 the scope's OWN hash-node - the element the PARENT splices into its
    // m_subTabs (CreateSub/AddNodeEntry do `m_subTabs.Insert(&child->m_node20)`).
    // The ex model spelled this a `void* m_node20` vtable slot + a 12-byte pad +
    // `void* m_30` + `void* m_34`; that IS a CHashElement dword for dword (vptr
    // +0x20, m_link +0x24, m_owner +0x2c, m_bucket +0x30, m_record +0x34) and the
    // Insert call sites had to cast &m_node20 to CHashElement* to say so. Layout is
    // unchanged (CHashElement is 0x18: +0x20..+0x37, m_subTabs still lands at +0x38).
    CSymTabNode m_node20; // +0x20
    CHashB m_subTabs;     // +0x38  child-scope table (Walk 0x13c3f0; destructed last)
    CHash m_symbols;      // +0x40  leaf-symbol table (Walk 0x13c270; destructed first)
    char* m_mappedBuf;    // +0x48  owned mapped/shared buffer (nonzero = mapping active)
};
SIZE(CSymTab, 0x4c); // operator new -> RezAlloc(0x4c); fields through m_buf48 @0x48

extern "C" const char g_sepSlash[]; // 0x60cff0  "\"  (CSymTab directory-path builder)

#endif // SRC_BUTE_SYMTAB_H
