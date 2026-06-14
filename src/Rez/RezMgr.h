// RezMgr.h - the Monolith "RezMgr" archive container classes: CRezItmBase (the
// shared resource-tree node base), CRezItm (a leaf resource / file node) and
// CRezDir (a subdirectory node). These build the in-memory directory tree that
// the engine's RezSync/CRezDir loader walks over a Gruntz.REZ / GRUNTZ.VRZ
// archive. The container is the Monolith "RezMgr Version 1" format.
//
// Field names are placeholders (m_<hexoffset>); ONLY the OFFSETS + code bytes
// are load-bearing (campaign doctrine). The layouts below are CONFIRMED from the
// two ctors (@0x13c540 / @0x13c940), the shared base ctor (@0x13c4e0), the
// `operator new` sizes (0x24 leaf / 0x38 dir), and the field stores in
// FindEntry/Load/OpenSub.
//
// ---------------------------------------------------------------------------
// CRezItmBase (16 bytes) - the shared base of every directory-tree node. Both
// ctors call the same base ctor @0x13c4e0 (stores base vtbl @0x5ef768 @+0 and
// the parent pointer @+0xc).
//   +0x00  m_vtbl   : vtable pointer (base @0x5ef768; the derived ctor
//                     overwrites it - two-phase construction).
//   +0x04  m_4      : (not written by these ctors)
//   +0x08  m_8      : (not written by these ctors; OpenSub zeroes its owner's +8)
//   +0x0c  m_parent : owning/parent pointer (the one base-ctor arg).
//
// ---------------------------------------------------------------------------
// CRezItm : CRezItmBase (0x24 = 36 bytes) - a leaf resource (file) node.
// derived vtbl @0x5ef788.
//   +0x10  m_10  : 0      +0x14  m_14 : 0      +0x20  m_20 : -1
//   (+0x18/+0x1c set by the virtual load, not the ctor)
//
// ---------------------------------------------------------------------------
// CRezDir : CRezItmBase (>= 0x38; ctor builds 0x38 = 56 bytes) - a subdirectory
// node. derived vtbl @0x5ef7a8. The ctor inits an embedded child collection
// sub-object (two vtables @0x5ef7c8 at +0x10 and +0x1c) and bookkeeping; the
// higher fields (+0x40..+0x64) are written by Load/OpenSub at runtime.
//   +0x10  m_vtblA : 0x5ef7c8 (collection vtable #1)
//   +0x14  m_14    : 0        (collection head)
//   +0x18  m_18    : 0        (collection tail)
//   +0x1c  m_vtblB : 0x5ef7c8 (collection vtable #2)
//   +0x20  m_20 :0  +0x24 m_24:0  +0x28 m_28:0  +0x34 m_34:0
//   +0x2c  m_2c    : ctor arg2 (the owning RezMgr back-pointer)
//   +0x30  m_30    : 1        ("valid"/initialized flag)
// Runtime fields (NOT ctor-initialized; pinned from Load/OpenSub):
//   +0x38  m_coll2 : a second/child collection base (Load iterates &this+0x38)
//   +0x40  m_loaded: OpenSub's load gate (return 0 if zero)
//   +0x44  m_44    : passed to the recursive walker
//   +0x48  m_loaded2: Load's already-loaded gate (return 1 if nonzero)
//   +0x54  m_w     +0x58 m_h  +0x5c m_x  +0x60 m_y : running max dims (OpenSub)
//   +0x64  m_name  : cached lookup-name buffer (operator new'd / freed)
#ifndef SRC_REZ_REZMGR_H
#define SRC_REZ_REZMGR_H

// ---------------------------------------------------------------------------
// External engine helpers, modeled with NO body so their `call rel32`
// displacements are reloc-masked in objdiff (the "external no-body callee"
// idiom). Calling-convention/arg-shape pinned from the disasm.
// ---------------------------------------------------------------------------

// Raw heap alloc/free the container links in (0x1b9b46 alloc(size) returns a
// pointer; 0x1b9b82 free(ptr)). __cdecl, args on the stack.
extern "C" void *RezAlloc(unsigned int size);   // 0x1b9b46
extern "C" void  RezFree(void *p);               // 0x1b9b82

// The directory-entry "stat" reader: fills a 0x24-byte WIN32-find-style record
// for `name`, returns 0 on success (FindFirstFileA + GetDriveTypeA + file-time
// conversions live here @0x18c780). The attribute dword lives at byte +6 of the
// record; bit 0x4000 marks a directory entry.
struct RezFindRec { char raw[0x24]; };
extern "C" int RezStatEntry(const char *name, RezFindRec *rec);   // 0x18c780

class CRezDir;

// The list/collection iteration helpers the directory tree uses (engine fns,
// external no-body, __thiscall - the collection/node arrives in ecx, no stack
// args). Modeled as member functions (First on the collection, Next on a node)
// so the `lea ecx,[..]; call` / `mov ecx,..; call` shapes fall out, reloc-masked.
//   0x184ae0  RezColl::First()  -> first child node
//   0x1848b0  RezNode::Next()   -> next sibling node
struct RezNode;

// 0x184e00 - the engine assert/trace sink: prints/logs the message string.
extern "C" void RezAssertFail(const char *msg);     // 0x184e00

// ---------------------------------------------------------------------------
// CRezItmBase - the shared node base (vtable @0x5ef768, parent ptr @+0xc).
// Polymorphic so the vptr lands at +0x00 and the two-phase vtable stores fall
// out; the ctor takes the parent pointer (stored @+0xc).
// ---------------------------------------------------------------------------
class CRezItmBase {
public:
    CRezItmBase(void *parent);
    virtual ~CRezItmBase() {}

    void  *m_4;        // +0x04
    void  *m_8;        // +0x08
    void  *m_parent;   // +0x0c
};

// ---------------------------------------------------------------------------
// CRezItm (0x24 = 36 bytes) - a leaf resource/file node.
// ---------------------------------------------------------------------------
class CRezItm : public CRezItmBase {
public:
    CRezItm(void *parent);
    virtual ~CRezItm() {}

    int   m_10;   // +0x10  (= 0)
    int   m_14;   // +0x14  (= 0)
    int   m_18;   // +0x18  (set by load)
    int   m_1c;   // +0x1c  (set by load)
    int   m_20;   // +0x20  (= -1)
};

// ---------------------------------------------------------------------------
// CRezDir (ctor builds 0x38 = 56 bytes; runtime fields extend to +0x68) - a
// subdirectory node + the directory the loader walks (Load/OpenSub/FindEntry).
// ---------------------------------------------------------------------------
class CRezDir : public CRezItmBase {
public:
    CRezDir(void *parent, void *rezMgr);
    virtual ~CRezDir() {}

    int   FindEntry(char *name);        // 0x13c080
    // OpenSub (@0x13b0c0, 568 B) is NOT matched in this TU - see RezMgr.cpp note.

    // --- ctor-initialized embedded child collection (+0x10..+0x34) ---
    void *m_vtblA;   // +0x10  (= 0x5ef7c8)
    int   m_14;      // +0x14  (= 0, collection head)
    int   m_18;      // +0x18  (= 0, collection tail)
    void *m_vtblB;   // +0x1c  (= 0x5ef7c8)
    int   m_20;      // +0x20  (= 0)
    int   m_24;      // +0x24  (= 0)
    int   m_28;      // +0x28  (= 0)
    void *m_2c;      // +0x2c  (= ctor arg2)
    int   m_30;      // +0x30  (= 1)
    int   m_34;      // +0x34  (= 0)
    // --- runtime-only fields (NOT set by the ctor) ---
    int   m_38;      // +0x38  (second collection base; Load walks &this+0x38)
    int   m_3c;      // +0x3c
    int   m_loaded;  // +0x40  (OpenSub gate)
    int   m_44;      // +0x44
    int   m_loaded2; // +0x48  (Load gate)
    int   m_4c;      // +0x4c
    int   m_50;      // +0x50
    int   m_54;      // +0x54  (max width)
    int   m_58;      // +0x58  (max height)
    int   m_5c;      // +0x5c  (max x)
    int   m_60;      // +0x60  (max y)
    void *m_name;    // +0x64  (cached lookup-name buffer)
};

// ---------------------------------------------------------------------------
// CRezDirNode - the directory-tree node that Load (@0x13a0f0) is a method of.
// This is the recursively-walked dir node; its field set DIFFERS from the 0x38
// CRezDir built by the OpenSub allocator (the +0x10/+0x18/+0x1c slots that the
// 0x38 ctor uses for the embedded collection's vtables are, on THIS node, a
// data-size @+0x10 and an archive-source pointer @+0x18). Load pins:
//   +0x0c  m_off    : payload offset (passed to the source's virtual read)
//   +0x10  m_size   : payload byte size (allocates a buffer of this size)
//   +0x14  m_subdir : child sub-dir CRezDirNode* (the recursion target)
//   +0x18  m_src    : the archive-source object (its vtable is at m_src+0x20;
//                     slot +8 = ReadAt(off, 0, size, buf); +0x08 a sort/state
//                     field; +0x1c a version/flag that must be <= 1)
//   +0x38  m_kids   : the child collection (First/Next iterated here)
//   +0x48  m_buf    : the loaded payload buffer (also the "already loaded" gate)
// ---------------------------------------------------------------------------
class CRezDirNode;  // fwd (RezNode holds a CRezDirNode* sub-dir at +0x14)

// A polymorphic stream object: the recursive read does
//   ecx = src->m_stream (the object @ src+0x20)
//   edx = *(void**)ecx   (its vtable)
//   call [edx+8]         (the 3rd virtual slot) with this=ecx, args (off,0,size,buf)
// So model it as a class with the read method at vtable slot index 2.
class RezStream {
public:
    virtual void v0() = 0;
    virtual void v1() = 0;
    virtual int ReadAt(int off, int zero, unsigned size, void *buf) = 0;  // slot +0x08
};

// The archive source object that the dir node points to at +0x18. Load checks
// m_8 (nonzero) and m_1c (<=1) and reads through the stream at +0x20.
struct RezSrc {
    char       m_pad0[0x08];
    int        m_8;        // +0x08  (must be nonzero)
    char       m_padc[0x1c - 0x0c];
    int        m_1c;       // +0x1c  (must be <= 1)
    RezStream *m_stream;   // +0x20  (the polymorphic read stream)
};

// The child collection embedded at CRezDirNode+0x38 (First/Next iterated).
// First() is __thiscall (0x184ae0): returns the first child node or 0.
struct RezColl {
    RezNode *First();   // 0x184ae0
    char m_pad[0x10];
};

class CRezDirNode {
public:
    int Load(int childFlag);            // 0x13a0f0

    void   *m_0;        // +0x00 (vtable / base, not touched by Load)
    void   *m_4;        // +0x04
    void   *m_8;        // +0x08
    int     m_off;      // +0x0c  (payload offset)
    unsigned m_size;    // +0x10  (payload size)
    void   *m_subdir;   // +0x14  (unused by Load on `this`)
    RezSrc *m_src;      // +0x18  (archive source object)
    char    m_pad1c[0x38 - 0x1c];
    RezColl m_kids;     // +0x38..+0x47  (child collection, 0x10 bytes)
    void   *m_buf;      // +0x48  (payload buffer / loaded gate)
};

// A child entry node in a CRezDirNode's collection: holds the sub-dir node ptr
// at +0x14 that Load recurses into. Next() is __thiscall (0x1848b0): returns the
// next sibling node or 0.
struct RezNode {
    RezNode *Next();    // 0x1848b0
    char         m_pad0[0x14];
    CRezDirNode *m_14;  // +0x14  (sub-dir node; Load recurses on it)
};

#endif // SRC_REZ_REZMGR_H
