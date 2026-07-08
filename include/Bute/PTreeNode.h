// PTreeNode.h - the shared zPTree config-tree node family (Bute .bute parser).
// zPTree is multiply-derived (zErrHandling primary @+0x00, CButeNodeEntry second
// base @+0x08); REAL POLYMORPHIC per the ALL-VTABLES mandate (cl auto-stamps both
// most-derived vptrs in the derived ctor). Concrete nodes derive zPTree and get
// their own most-derived primary (+0x00) + second-base-in-derived (+0x08) vtables.
// Ctor bodies live in src/Bute/ButeNode.cpp; only the shapes are here so sibling
// TUs (e.g. ButeSectionCtor.cpp's CBSecStream) can derive zPTree too.
//
// Field names are placeholders; only OFFSETS + code bytes are load-bearing.
#ifndef SRC_BUTE_PTREENODE_H
#define SRC_BUTE_PTREENODE_H
#include <rva.h>
#include <Ints.h>

extern void* g_buteNodeErrMsg; // DAT_006bf480 - the node's error-message global

// zErrHandling - the container-library exception base (vptr@0, msg@4), RTTI-real
// (zPTree's base-class-array names its primary base `zErrHandling`). Ctor 0x16d9c0
// (real body in GameText), __thiscall(this,msg); external no-body here so the call
// reloc-masks. REAL POLYMORPHIC: vtbl@0 is the implicit vptr (virtual dtor).
class zErrHandling {
public:
    zErrHandling(void* msg);
    virtual ~zErrHandling(); // +0x00 vptr; external no-body dtor (real body in GameText)

    void* m_msg; // +0x04
};

// The node subobject at zPTree+0x8 (a small keyed-store entry): zPTree's SECOND
// polymorphic base. Ctor 0x16df70 auto-stamps ??_7CButeNodeEntry (retail 0x5f04d8)
// @+0x00, then desc@+4, (WORD)n@+8, 0@+0xc.
class CButeNodeEntry {
public:
    CButeNodeEntry(i32 n, void* desc);
    virtual ~CButeNodeEntry(); // +0x00 vptr; external no-body scalar-deleting dtor

    void* m_desc; // +0x04  kind descriptor (opaque; a tag pointer)
    i16 m_kind;   // +0x08  (WORD)n
    char m_pada[2];
    i32 m_0c; // +0x0c  zero-init
};
SIZE(CButeNodeEntry, 0x10);       // { vptr, desc, kind, 0 }
VTBL(CButeNodeEntry, 0x001f04d8); // the entry member's own (base) vtable

// zPTree layout (two vptrs): +0x00 zErrHandling base, +0x08 CButeNodeEntry base
// (spans +0x08..+0x18), +0x18/+0x28 child links. Ctor 0x16dff0.
class zPTree : public zErrHandling, public CButeNodeEntry {
public:
    virtual ~zPTree() OVERRIDE; // slot 0 (scalar-dtor 0x004372; overrides zErrHandling)
    zPTree(void* desc, i32 n);

    i32 m_child18; // +0x18  child link (zeroed)
    char m_pad1c[0x28 - 0x1c];
    i32 m_child28; // +0x28  child link (zeroed)
};
SIZE(zPTree, 0x2c); // measured: new(0x2c) -> ctor 0x16dff0

#endif // SRC_BUTE_PTREENODE_H
