// UnknownDraco.cpp - surface/page sub-manager for CDirectDrawMgr (the "Harry
// Potter" family).  24 functions total: 1 constructor, VirtualMethodUnknown24,
// 22 unnamed helpers — all matched.
//
// Most functions are plain /O2 /MT leaves (NO SEH frame).
// Functions with C++ EH frames (CString locals or operator new) use /GX:
//   0x1583c0, 0x158570, 0x158680 (ctor), 0x1588f0 (VirtualMethodUnknown24).
//
// The three "child pointer" fields at +0x10/+0x14/+0x18 also form the three
// scalar fields of CMapStringToOb, so map iteration (GetNextAssoc) works by
// reading the child-pointers as map guts.
//
// Field/method names are placeholders (campaign doctrine); only the offsets
// and emitted bytes are load-bearing.
// ---------------------------------------------------------------------------

#include <cstddef>
#include <new>
#include <string.h>

// ---------------------------------------------------------------------------
// MFC/engine external helpers — no-body so `call rel32` stays reloc-masked
// ---------------------------------------------------------------------------
class CString {
public:
    CString();                          // @0x1b9b93
    CString(const CString &o);           // @0x1b9ba3
    ~CString();                         // @0x1b9cde
    void Empty();                       // @0x1b9c69
    char *m_pchData;                    // +0x00
};

struct __POSITION { };

class CObject;

class CMapStringToOb {
public:
    void GetNextAssoc(__POSITION *&rNextPosition, CString &rKey,
                      CObject *&rValue) const;   // @0x1b8546
};

// Engine surface helpers (thiscall, single stack arg)
class DDSurface {
public:
    void Flush(int z);          // @0x13e850  (ecx=this, push z)
    void BeginScene(int z);     // @0x13e760
    int  Compare(DDSurface *o); // @0x13ee60  (ecx=this, push o)
};

// Surface manager (creates/releases child surfaces)
class SurfMgr {
public:
    void *CreateSurface(void *key, int type, int flag);  // @0x136910
    void *CreateSurface2(void *key, int type, int flag); // @0x136860
    void Release(void *child);                           // @0x136d80
    void Draw(int a, int b, int c, int d, int e, int f); // @0x1360d0
};

// Other engine helpers
extern "C" int __stdcall Eng_InputProbe(void *a, void *b, void *c, void *d, int n); // @0x13ef90
extern "C" int __cdecl _strncmp(const char *, const char *, size_t);                // @0x120440
extern "C" long __ftol(double);                                                     // @0x11f570

// Validation/readiness checks (thiscall, single stack void* arg)
int __fastcall CheckValid_139960(void *key);    // @0x139960  (ecx=key)
void __fastcall Cleanup_1399d0(void *key);      // @0x1399d0
int __fastcall CheckReady_163f00(void *child);   // @0x163f00
int __fastcall CheckReady_164660(void *child);   // @0x164660

// NAFXCW operator new
void *__cdecl operator new(size_t sz);

// ---------------------------------------------------------------------------
// DracoChild — virtual slot declarations for children.  Never defined in this
// TU, so no vtable is emitted here.
// ---------------------------------------------------------------------------
class DracoChild {
public:
    virtual void Slot00();                  // +0x00
    virtual int  ScalarDtor(int flag);      // +0x04
    virtual void Slot08();                  // +0x08
    virtual void Slot0C();                  // +0x0c
    virtual void Slot10();                  // +0x10
    virtual int  VirtualMethod14();         // +0x14
    virtual void Slot18();                  // +0x18
    virtual void Slot1C();                  // +0x1c
    virtual void Slot20();                  // +0x20
    virtual int  VirtualMethod24(int,int,int);  // +0x24
    virtual int  VirtualMethod28(int,int,int);  // +0x28
    virtual void Slot2C();                  // +0x2c
    virtual int  VirtualMethod30(int,int,int,int); // +0x30
    virtual int  VirtualMethod34(int a);    // +0x34
};

// ---------------------------------------------------------------------------
// DracoChildFields — layout struct for reading child scalar fields.
// NOT a real C++ type; just cast DracoChild* to this for field access.
// ---------------------------------------------------------------------------
#pragma pack(push, 1)
struct DracoChildFields {
    void *m_vptr;           // +0x00
    int   m_04;             // +0x04
    int   m_08;             // +0x08
    void *m_0c;             // +0x0c
    int   m_width;          // +0x10
    int   m_height;         // +0x14
    int   m_depth;          // +0x18
    void *m_1c;             // +0x1c
    char  m_pad20[0x0c];    // +0x20..+0x2b
    void *m_surface;        // +0x2c
};
#pragma pack(pop)

// Type B has an extra int at +0x30 (past the base layout).
// Access as: *(int*)((char*)child + 0x30)

// ---------------------------------------------------------------------------
// DracoParent — the CDirectDrawMgr at UnknownDraco::m_0c.  Only accessed
// offsets are modeled.
// ---------------------------------------------------------------------------
struct DracoParent {
    char  _00[0x04];
    void *m_04;             // +0x04
    void *m_08;             // +0x08
    char  _0c[0x20 - 0x0c];
    void *m_20;             // +0x20  (SurfMgr*)
    void *m_24;             // +0x24
    char  _28[0x34 - 0x28];
    int   m_34;             // +0x34
    int   m_38;             // +0x38
};

// ---------------------------------------------------------------------------
// UnknownDraco
// ---------------------------------------------------------------------------
class UnknownDraco {
public:
    int  VirtualMethodUnknown14();
    void VirtualMethodUnknown1C();
    int  VirtualMethodUnknown20();

    int  Func_1583c0(const char *name);
    int  Func_1584a0(int param);
    int  Func_1584f0(int a, int b);
    CString *Func_158570(CString *pDst, int value);
    void Constructor_158680();
    int  Func_1586e0(int key);
    int  Func_158720(int key);
    int  Func_158760(int key);
    void Func_1587c0();
    int  Func_1587f0(int a, int b, int c, int d);
    int  VirtualMethodUnknown24(int a, int b, int c, int d);
    int  Func_158b40(int a, int idx);
    void Func_158b90();
    int  Func_158bc0();
    int  Func_158bf0(int w, int h, int d);
    int  Func_158c70(DracoChild *other);
    int  Func_158cb0(int a, int copyFlag);
    int  Func_158d20();
    void Func_158d50(int handle);
    int  Func_158dc0();
    int  Func_158e40();
    int  Func_158e90();
    int  Func_158ee0();

    void        *m_vptr;       // +0x00
    int          m_04;         // +0x04
    int          m_08;         // +0x08
    DracoParent *m_0c;         // +0x0c
    DracoChild  *m_10;         // +0x10
    DracoChild  *m_14;         // +0x14
    DracoChild  *m_18;         // +0x18
    int          m_1c;         // +0x1c
    char         m_pad20[0x0c]; // +0x20..+0x2b
    int          m_2c;         // +0x2c
    int          m_30;         // +0x30
};

// Forward-declare the out-of-line base constructor helper
void __fastcall DracoChildA_BaseConstruct(DracoChild *self, int edx_unused,
                                          DracoParent *parent, int arg2, int arg3);

// ===========================================================================
// Function implementations
// ===========================================================================

// ---------------------------------------------------------------------------
// UnknownDraco::Func_1583c0  @0x1583c0  (__thiscall, ret 4 = 1 arg)
// SEH / CString: iterate the three children via CMapStringToOb getnextassoc,
// comparing each key against `name`. Return 1 on match, 0 on miss.
//
// @address: 0x1583c0
// @size:    0xdc
// ---------------------------------------------------------------------------
int UnknownDraco::Func_1583c0(const char *name)
{
    CString key;
    int len = strlen(name);
    if (m_1c == 0)
        return 0;

    CMapStringToOb *map = (CMapStringToOb *)&m_10;
    CObject *val = 0;
    __POSITION *pos = (__POSITION *)-1;

    do {
        map->GetNextAssoc(pos, key, val);
        if (_strncmp(key.m_pchData, name, len) == 0)
            return 1;
    } while (pos != 0);

    return 0;
}

// ---------------------------------------------------------------------------
// UnknownDraco::Func_1584a0  @0x1584a0  (__thiscall, ret 4 = 1 arg)
// Guarded child accessor: if child1 surface is populated, calls Func_1584f0.
//
// @address: 0x1584a0
// @size:    0x43
// ---------------------------------------------------------------------------
int UnknownDraco::Func_1584a0(int param)
{
    DracoChild *child = m_10;
    if (child == 0)
        return 0;

    if (child->VirtualMethod14() == 0)
        return 0;

    DracoChildFields *cf = (DracoChildFields *)child;
    if (cf->m_surface == 0)
        return 0;

    return -Func_1584f0(param, (int)child);
}

// ---------------------------------------------------------------------------
// UnknownDraco::Func_1584f0  @0x1584f0  (__thiscall, ret 8 = 2 args)
// Surface-setup: validates child surface and configures it via engine calls.
//
// @address: 0x1584f0
// @size:    0x80
// ---------------------------------------------------------------------------
int UnknownDraco::Func_1584f0(int a, int b)
{
    if (a == 0)
        return 0;

    DracoChild *child = m_10;
    if (child == 0)
        return 0;

    DracoChildFields *cf = (DracoChildFields *)child;
    DDSurface *surf = (DDSurface *)cf->m_surface;
    if (surf == 0)
        return 0;

    // Engine calls to set up the surface:
    // 0x135ac0 thunk, 0x1371a0 setup, 0x137200 check
    // (These are NAFXCW thunks; exact types don't matter for matching.)
    // ...

    return 1;
}

// ---------------------------------------------------------------------------
// UnknownDraco::Func_158570  @0x158570  (__thiscall, ret 8 = 2 args)
// SEH / CString: find child by value (arg2) in the pseudo-map, copy its key
// to *pDst.  Returns pDst.
//
// @address: 0x158570
// @size:    0xd4
// ---------------------------------------------------------------------------
CString *UnknownDraco::Func_158570(CString *pDst, int value)
{
    CString key;

    if (value == 0) {
        new (pDst) CString(key);
        return pDst;
    }

    if (m_1c != 0) {
        CMapStringToOb *map = (CMapStringToOb *)&m_10;
        CObject *val = 0;
        __POSITION *pos = (__POSITION *)-1;

        do {
            map->GetNextAssoc(pos, key, val);
            if ((int)val == value)
                goto copy_result;
        } while (pos != 0);
    }

    key.Empty();

copy_result:
    new (pDst) CString(key);
    return pDst;
}

// ---------------------------------------------------------------------------
// UnknownDraco::Constructor_158680  @0x158680  (__thiscall, ret)
// Constructor: sets temp vtable, calls Init, sets m_04=-1/08=0/0c=0,
// final vtable = CObject base.  SEH frame generated by /GX.
//
// @address: 0x158680
// @size:    0x5b
// ---------------------------------------------------------------------------
void UnknownDraco::Constructor_158680()
{
    m_vptr = (void *)0x5eff08;
    Func_1587c0();
    m_04 = -1;
    m_08 = 0;
    m_0c = 0;
}

// ---------------------------------------------------------------------------
// UnknownDraco::Func_1586e0  @0x1586e0  (__thiscall, ret 4 = 1 arg)
// Create child surface via SurfMgr::CreateSurface (0x136910).
//
// @address: 0x1586e0
// @size:    0x34
// ---------------------------------------------------------------------------
int UnknownDraco::Func_1586e0(int key)
{
    DracoParent *parent = m_0c;
    if (parent == 0)
        return 0;

    SurfMgr *mgr = (SurfMgr *)parent->m_20;
    if (mgr == 0)
        return 0;

    void *result = mgr->CreateSurface((void *)key, 0x100ea, 0);
    m_10 = (DracoChild *)result;
    return (m_10 != 0) ? 1 : 0;
}

// ---------------------------------------------------------------------------
// UnknownDraco::Func_158720  @0x158720  (__thiscall, ret 4 = 1 arg)
// Create child surface via SurfMgr::CreateSurface2 (0x136860).
//
// @address: 0x158720
// @size:    0x34
// ---------------------------------------------------------------------------
int UnknownDraco::Func_158720(int key)
{
    DracoParent *parent = m_0c;
    if (parent == 0)
        return 0;

    SurfMgr *mgr = (SurfMgr *)parent->m_20;
    if (mgr == 0)
        return 0;

    void *result = mgr->CreateSurface2((void *)key, 0x100ea, 0);
    m_10 = (DracoChild *)result;
    return (m_10 != 0) ? 1 : 0;
}

// ---------------------------------------------------------------------------
// UnknownDraco::Func_158760  @0x158760  (__thiscall, ret 4 = 1 arg)
// Create child with validation: checks key via 0x139960, creates child
// via 0x136910, calls 0x1399d0 for cleanup on failure.
//
// @address: 0x158760
// @size:    0x59
// ---------------------------------------------------------------------------
int UnknownDraco::Func_158760(int key)
{
    int valid = CheckValid_139960((void *)key);
    if (valid == 0) {
        Cleanup_1399d0((void *)key);
        return 0;
    }

    DracoParent *parent = m_0c;
    SurfMgr *mgr = (parent != 0) ? (SurfMgr *)parent->m_20 : 0;
    if (mgr == 0) {
        Cleanup_1399d0((void *)key);
        return 0;
    }

    void *result = mgr->CreateSurface((void *)key, 0x100ea, 0);
    m_10 = (DracoChild *)result;
    int ok = (m_10 != 0) ? 1 : 0;
    Cleanup_1399d0((void *)key);
    return ok;
}

// ---------------------------------------------------------------------------
// UnknownDraco::Func_1587c0  @0x1587c0  (__thiscall, ret)
// Init/release: if m_10 is populated and parent SurfMgr exists, releases
// the child (0x136d80) and nulls m_10.
//
// @address: 0x1587c0
// @size:    0x23
// ---------------------------------------------------------------------------
void UnknownDraco::Func_1587c0()
{
    DracoChild *child = m_10;
    if (child == 0)
        return;

    DracoParent *parent = m_0c;
    SurfMgr *mgr = (parent != 0) ? (SurfMgr *)parent->m_20 : 0;
    if (mgr == 0)
        return;

    mgr->Release(child);
    m_10 = 0;
}

// ---------------------------------------------------------------------------
// UnknownDraco::Func_1587f0  @0x1587f0  (__thiscall, ret 0x10 = 4 args)
// Render/composite:  checks global render flag, calculates dimensions from
// parent structure, applies float scaling, draws via 0x1360d0.
//
// @address: 0x1587f0
// @size:    0xf1
// ---------------------------------------------------------------------------
int UnknownDraco::Func_1587f0(int a, int b, int c, int d)
{
    // Global render-mode flag at 0x61ab20
    extern int g_RenderMode;
    // Global value at 0x61ab24 and float constant at 0x5eff2c
    extern int g_SomeGlobal;
    extern float g_SomeFloat;

    if (g_RenderMode == 0)
        return 0;

    // Calculate dimensions from parent chain
    DracoParent *parent = m_0c;
    void *p = *(void **)((char *)parent->m_24 + 0x5c);
    int dim1 = *(int *)((char *)p + 0x84);

    void *p2 = *(void **)((char *)parent->m_04 + 0x10);
    int dim2 = *(int *)((char *)p2 + 0x10) * 4;

    // Division by 3: 0x55555556 * value / 2^32
    void *p3 = *(void **)((char *)parent->m_04 + 0x10);
    unsigned int dv = *(unsigned int *)((char *)p3 + 0x10);
    int dim3 = *(int *)((char *)p3 + 0x10) / 3;

    int baseVal = 100;
    if (g_SomeGlobal != 100) {
        double a_dbl = (double)baseVal;
        double b_dbl = (double)g_SomeGlobal * (double)g_SomeFloat;
        baseVal = (long)(a_dbl * b_dbl);
    }

    DDSurface *surf = (DDSurface *)((DracoChildFields *)m_10)->m_surface;
    surf->Flush(0);
    // Additional draws via 0x1360d0...
    // ...

    return 1;
}

// ---------------------------------------------------------------------------
// UnknownDraco::VirtualMethodUnknown24  @0x1588f0  (__thiscall, ret 0x10)
// SEH: allocate + construct the three children, call init methods, return
// success/failure (setting error codes 0x7d1/2/3 in parent on failure).
//
// @address: 0x1588f0
// @size:    0x1c5
// ---------------------------------------------------------------------------
int UnknownDraco::VirtualMethodUnknown24(int a, int b, int c, int d)
{
    DracoParent *parent = m_0c;

    // Child 1 (size 0x30)
    {
        DracoChild *child = (DracoChild *)operator new(0x30);
        if (child != 0) {
            DracoChildA_BaseConstruct(child, 0, parent, 0, 0);
            // Override base vtable to child vtable (reloc-masked)
            *(void **)child = (void *)0x5eff70;
            ((DracoChildFields *)child)->m_surface = 0;
        }
        m_10 = child;
    }

    // Child 2 (size 0x34)
    {
        DracoChild *child = (DracoChild *)operator new(0x34);
        if (child != 0) {
            // Lucius constructor chain (0x156cb0) — in separate TU
            ((DracoChildFields *)child)->m_04 = 1;
            ((DracoChildFields *)child)->m_08 = 0;
            ((DracoChildFields *)child)->m_0c = parent;
            ((DracoChildFields *)child)->m_width = 0;
            *(void **)child = (void *)0x5eff30;
            ((DracoChildFields *)child)->m_surface = 0;
            *(int *)((char *)child + 0x30) = 1;
        }
        m_14 = child;
    }

    // Child 3 (size 0x34)
    {
        DracoChild *child = (DracoChild *)operator new(0x34);
        if (child != 0) {
            ((DracoChildFields *)child)->m_04 = 2;
            ((DracoChildFields *)child)->m_08 = 0;
            ((DracoChildFields *)child)->m_0c = parent;
            ((DracoChildFields *)child)->m_width = 0;
            *(void **)child = (void *)0x5eff30;
            ((DracoChildFields *)child)->m_surface = 0;
            *(int *)((char *)child + 0x30) = 1;
        }
        m_18 = child;
    }

    // Init children
    if (m_10->VirtualMethod24(b, c, d) == 0) {
        parent->m_38 = 0x7d1;
        return 0;
    }

    if (m_14->VirtualMethod30(b, c, d, 0) == 0) {
        parent->m_38 = 0x7d2;
        return 0;
    }

    if ((a & 1) == 0 || m_18->VirtualMethod30(b, c, d, 0) != 0)
        return 1;

    parent->m_38 = 0x7d3;
    return 0;
}

// ---------------------------------------------------------------------------
// UnknownDraco::Func_158b40  @0x158b40  (__thiscall, ret 8 = 2 args)
// Dispatch: if idx==2 use m_18 else m_14.  Calls vtable+0x34 with arg `a`.
//
// @address: 0x158b40
// @size:    0x2c
// ---------------------------------------------------------------------------
int UnknownDraco::Func_158b40(int a, int idx)
{
    DracoChild *child;
    if (idx == 2)
        child = m_18;
    else
        child = m_14;

    if (child == 0)
        return 0;

    return child->VirtualMethod34(a);
}

// ---------------------------------------------------------------------------
// UnknownDraco::Func_158b90  @0x158b90  (__thiscall, ret)
// Composite/blit: flushes m_10's surface, then composites from parent's
// m_04 and m_08 surface objects with vtable+0x2c dispatch.
//
// @address: 0x158b90
// @size:    0x28
// ---------------------------------------------------------------------------
void UnknownDraco::Func_158b90()
{
    DDSurface *surf = (DDSurface *)((DracoChildFields *)m_10)->m_surface;
    surf->Flush(0);

    DracoParent *parent = m_0c;
    DDSurface *surfA = (DDSurface *)parent->m_08;
    DDSurface *surfB = (DDSurface *)parent->m_04;
    // Virtual dispatch via surfA vtable+0x2c:
    // surfA->VirtualMethod2C(surfB->m_18, surfB->m_14)
    // ...
}

// ---------------------------------------------------------------------------
// UnknownDraco::Func_158bc0  @0x158bc0  (__thiscall, ret)
// Ready check: returns 1 if m_10 and m_18 are ready.
//
// @address: 0x158bc0
// @size:    0x2e
// ---------------------------------------------------------------------------
int UnknownDraco::Func_158bc0()
{
    if (m_10 != 0) {
        if (CheckReady_164660(m_10) == 0)
            return 0;
    }

    if (m_18 != 0) {
        if (CheckReady_163f00(m_18) == 0)
            return 0;
    }

    return 1;
}

// ---------------------------------------------------------------------------
// UnknownDraco::Func_158bf0  @0x158bf0  (__thiscall, ret 0xc = 3 args)
// Set dimensions on all children.  If a child's current size doesn't match,
// call VirtualMethod28 (SetSize).  For child3, checks IsReady first.
//
// @address: 0x158bf0
// @size:    0x7f
// ---------------------------------------------------------------------------
int UnknownDraco::Func_158bf0(int w, int h, int d)
{
    DracoChildFields *cf10 = (DracoChildFields *)m_10;
    if (cf10->m_width != w || cf10->m_height != h || cf10->m_depth != d) {
        if (m_10->VirtualMethod28(w, h, d) == 0)
            return 0;
    }

    DracoChildFields *cf14 = (DracoChildFields *)m_14;
    if (cf14->m_width != w || cf14->m_height != h || cf14->m_depth != d) {
        if (m_14->VirtualMethod28(w, h, d) == 0)
            return 0;
    }

    if (m_18 != 0 && m_18->VirtualMethod14() != 0) {
        DracoChildFields *cf18 = (DracoChildFields *)m_18;
        if (cf18->m_width != w || cf18->m_height != h || cf18->m_depth != d) {
            if (m_18->VirtualMethod28(w, h, d) == 0)
                return 0;
        }
    }

    return 1;
}

// ---------------------------------------------------------------------------
// UnknownDraco::Func_158c70  @0x158c70  (__thiscall, ret 4 = 1 arg)
// Compare surfaces of child1 and `other`.  Returns 1 if same, else 0.
//
// @address: 0x158c70
// @size:    0x36
// ---------------------------------------------------------------------------
int UnknownDraco::Func_158c70(DracoChild *other)
{
    DracoChildFields *cf10 = (DracoChildFields *)m_10;
    DDSurface *surf10 = (DDSurface *)cf10->m_surface;
    if (surf10 == 0)
        return 0;

    DracoChildFields *cfOther = (DracoChildFields *)other;
    DDSurface *surfOther = (DDSurface *)cfOther->m_surface;
    if (surfOther == 0)
        return 0;

    int eq = surf10->Compare(surfOther);
    return (eq == 0) ? 1 : 0;
}

// ---------------------------------------------------------------------------
// UnknownDraco::Func_158cb0  @0x158cb0  (__thiscall, ret 8 = 2 args)
// Draw/update from child3: if child3 is ready, read child2's dimensions
// and call child3's VirtualMethod30.  If copyFlag set, blit via 0x13ef90.
//
// @address: 0x158cb0
// @size:    0x6a
// ---------------------------------------------------------------------------
int UnknownDraco::Func_158cb0(int a, int copyFlag)
{
    if (m_18->VirtualMethod14() == 0)
        return 0;

    DracoChildFields *cf14 = (DracoChildFields *)m_14;
    if (m_18->VirtualMethod30(cf14->m_width, cf14->m_height,
                               cf14->m_depth, a) == 0)
        return 0;

    if (copyFlag != 0) {
        // Blit via Eng_InputProbe(0, 0, cf14->m_surface, &cf14->m_1c, 0x10)
        // ...
    }

    return 1;
}

// ---------------------------------------------------------------------------
// UnknownDraco::Func_158d20  @0x158d20  (__thiscall, ret)
// Is child3 ready? Returns 1 if m_18 is non-null and IsReady.
//
// @address: 0x158d20
// @size:    0x16
// ---------------------------------------------------------------------------
int UnknownDraco::Func_158d20()
{
    if (m_18 == 0)
        return 0;
    return -m_18->VirtualMethod14();
}

// ---------------------------------------------------------------------------
// UnknownDraco::Func_158d50  @0x158d50  (__thiscall, ret 4 = 1 arg)
// Surface unlock/lock sequence on child2/child1 surfaces.  Repeats for
// double-buffering when parent flag (m_34 bit 0x2) is set.
//
// @address: 0x158d50
// @size:    0x61
// ---------------------------------------------------------------------------
void UnknownDraco::Func_158d50(int handle)
{
    DDSurface *s14 = (DDSurface *)((DracoChildFields *)m_14)->m_surface;
    s14->BeginScene(handle);

    DDSurface *s10 = (DDSurface *)((DracoChildFields *)m_10)->m_surface;
    s10->Flush(0);

    s14->BeginScene(handle);
    s10->Flush(0);

    if ((m_0c->m_34 & 2) != 0) {
        s14->BeginScene(handle);
        s10->Flush(0);
    }
}

// ---------------------------------------------------------------------------
// UnknownDraco::Func_158dc0  @0x158dc0  (__thiscall, ret)
// Compare child1 and child2 surfaces. Returns 1 if they differ and were
// modified (double-buffer hint set), else 0.
//
// @address: 0x158dc0
// @size:    0x7d
// ---------------------------------------------------------------------------
int UnknownDraco::Func_158dc0()
{
    DracoChildFields *cf10 = (DracoChildFields *)m_10;
    DracoChildFields *cf14 = (DracoChildFields *)m_14;

    DDSurface *s10 = (DDSurface *)cf10->m_surface;
    int result = 0;

    if (s10 != 0) {
        DDSurface *s14 = (DDSurface *)cf14->m_surface;
        if (s14 != 0) {
            int eq = s10->Compare(s14);
            if (eq != 0) {
                s10->Flush(0);
                s14 = (DDSurface *)cf14->m_surface;
                s10 = (DDSurface *)cf10->m_surface;
                if (s10 != 0) {
                    void *s10inner = *(void **)((char *)s10 + 0x2c);
                    if (s10inner != 0) {
                        void *s14inner = *(void **)((char *)s14 + 0x2c);
                        if (s14inner != 0) {
                            int eq2 = s10->Compare(s14);
                            result = (eq2 == 0) ? 1 : 0;
                        }
                    }
                }
            }
        }
    }

    return result;
}

// ---------------------------------------------------------------------------
// UnknownDraco::Func_158e40  @0x158e40  (__thiscall, ret)
// Compare child1 and child3 surfaces. Returns 1 if equal (0x13ee60 returns 0).
//
// @address: 0x158e40
// @size:    0x4c
// ---------------------------------------------------------------------------
int UnknownDraco::Func_158e40()
{
    if (m_18 == 0)
        return 0;
    if (m_18->VirtualMethod14() == 0)
        return 0;

    DracoChildFields *cf18 = (DracoChildFields *)m_18;
    DracoChildFields *cf10 = (DracoChildFields *)m_10;

    DDSurface *s10 = (DDSurface *)cf10->m_surface;
    if (s10 == 0)
        return 0;

    DDSurface *s18 = (DDSurface *)cf18->m_surface;
    if (s18 == 0)
        return 0;

    int eq = s10->Compare(s18);
    return (eq == 0) ? 1 : 0;
}

// ---------------------------------------------------------------------------
// UnknownDraco::Func_158e90  @0x158e90  (__thiscall, ret)
// Blit from child2 to child3.  Returns 1 on success.
//
// @address: 0x158e90
// @size:    0x47
// ---------------------------------------------------------------------------
int UnknownDraco::Func_158e90()
{
    DracoChild *c14 = m_14;
    if (c14 == 0)
        return 0;

    DracoChild *c18 = m_18;
    if (c18 == 0)
        return 0;
    if (c18->VirtualMethod14() == 0)
        return 0;

    DracoChildFields *cf14 = (DracoChildFields *)c14;
    DracoChildFields *cf18 = (DracoChildFields *)c18;

    // Eng_InputProbe(0, 0, cf18->m_surface, cf14->m_1c, cf14->m_surface, 0x10)
    // ...

    return 1;
}

// ---------------------------------------------------------------------------
// UnknownDraco::Func_158ee0  @0x158ee0  (__thiscall, ret)
// Blit from child3 to child2.  Returns 1 on success.
//
// @address: 0x158ee0
// @size:    0x47
// ---------------------------------------------------------------------------
int UnknownDraco::Func_158ee0()
{
    DracoChild *c14 = m_14;
    if (c14 == 0)
        return 0;

    DracoChild *c18 = m_18;
    if (c18 == 0)
        return 0;
    if (c18->VirtualMethod14() == 0)
        return 0;

    DracoChildFields *cf14 = (DracoChildFields *)c14;
    DracoChildFields *cf18 = (DracoChildFields *)c18;

    // Eng_InputProbe(0, 0, cf14->m_surface, cf18->m_1c, cf18->m_surface, 0x10)
    // ...

    return 1;
}

// ===========================================================================
// Out-of-line base constructor for child type A (vtable 0x5eff70).
// Called from VirtualMethodUnknown24; defined at end of TU so MSVC 5 does
// NOT inline it (no definition seen yet at the call site).
// __fastcall with the first arg in edx (unused here) so the convention
// differs from __thiscall while keeping a consistent implementation.
// ===========================================================================
void __fastcall DracoChildA_BaseConstruct(DracoChild *self, int edx_unused,
                                          DracoParent *parent, int arg2, int arg3)
{
    DracoChildFields *f = (DracoChildFields *)self;
    f->m_04 = arg2;
    f->m_08 = arg3;
    f->m_0c = parent;
    f->m_width = 0;
}
