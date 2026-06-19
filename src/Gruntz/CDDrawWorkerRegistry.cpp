#include "../rva.h"
// CDDrawWorkerRegistry.cpp - leaf method(s) of the tomalla-named ddrawmgr surface-family
// sub-manager CDDrawWorkerRegistry (a CDirectDrawMgr surface/page sub-manager in the
// "Harry Potter" family; see structure/managers/ddrawmgr_surface_family.h).
//
// CDDrawWorkerRegistry owns a CMapStringToOb at +0x10 (m_unknownMap) keyed by const char*
// strings. VirtualMethodUnknown54 is a keyed remove-and-destroy: Lookup the key in
// the map; if present, RemoveKey it and run the found value's scalar-deleting
// destructor (vtable +0x4, arg 1). Plain /O2 /MT leaf: NO SEH frame, NO data
// relocations - the only relocations are the reloc-masked rel32 thunk calls
// (CMapStringToOb::Lookup / RemoveKey, both out-of-line NAFXCW), and the found
// value's destructor is dispatched through its own vtable.
//
// Field names are placeholders; only the OFFSETS + emitted code bytes are load-
// bearing (campaign doctrine). The looked-up value is modeled as a polymorphic
// stub (virtuals at the right slots) ONLY so `val->ScalarDtor(1)` lowers to the
// exact `mov eax,[val]; push 1; call [eax+4]` __thiscall dispatch; its virtuals are
// never defined, so no vtable is emitted in this TU.
//
// The four factory methods construct a 0x6c-byte worker with a CByteArray member
// at +0x10, hence the TU is /GX so the inlined heap construction emits the same
// EH frame as retail.
// ---------------------------------------------------------------------------

// --- MFC placeholders (only the call symbols + the 0x10 map offset matter) -----
class CObject;
class CDDrawWorkerRegistry;

class CByteArray {
public:
    CByteArray();
    ~CByteArray();
    char m_data[0x14];
};

// POSITION is the opaque MFC iteration handle.  Native int form so the ternary
// guard initialiser passes cleanly to GetNextAssoc's POSITION & parameter; the
// reloc-masked call site is identical.
typedef int POSITION;

// CString (4-byte char* wrapper). Only the default ctor + dtor are needed;
// GetNextAssoc writes the key output into it.
class CString {
public:
    CString();
    ~CString();
    char *m_pchData;    // +0x00
};

inline void *operator new(unsigned int, void *p) { return p; }

// The looked-up value: only the scalar-deleting destructor slot (+0x04) is load-
// bearing. Declarations only - never defined, so no ??_7 is emitted here.
class SeverusValue {
public:
    virtual void Slot00();              // +0x00
    virtual int  ScalarDtor(int flag);  // +0x04  scalar-deleting destructor
};

// CMapStringToOb lives at CDDrawWorkerRegistry+0x10. Lookup/RemoveKey are out-of-line
// NAFXCW thunks (reloc-masked rel32 calls); declared with the exact MFC signatures
// so clang mangles them to the MFC-canonical names.
class CMapStringToOb {
public:
    int Lookup(const char *key, CObject *&rValue) const;
    CObject *&operator[](const char *key);
    int RemoveKey(const char *key);
    void GetNextAssoc(POSITION &rNextPosition, CString &rKey,
                      CObject *&rValue) const;
    void RemoveAll();

    // Simulated internal layout (all offsets relative to +0x10 of parent):
    void     *m_vptr;              // +0x00  CObject vptr
    unsigned  m_nHashTableSize;    // +0x04
    void    **m_pHashTable;        // +0x08
    int       m_nCount;            // +0x0c
};

class UnknownSeverusVtableView {
public:
    virtual void Slot00();
    virtual void Slot04();
    virtual void Slot08();
    virtual void Slot0C();
    virtual void Slot10();
    virtual void Slot14();
    virtual void Slot18();
    virtual void Slot1C();
    virtual void Slot20();
    virtual void Slot24();
    virtual void Slot28();
    virtual void Slot2C();
    virtual void Slot30();
    virtual void Slot34();
    virtual void Slot38();
    virtual void Slot3C();
    virtual void Slot40();
    virtual void Slot44();
    virtual void Slot48();
    virtual void Slot4C();
    virtual void Slot50();
    virtual void Slot54();
    virtual void Slot58();
};

class SeverusWorker {
public:
    virtual void Slot00();              // +0x00
    virtual int  ScalarDtor(int flag);  // +0x04
    virtual void Slot08();              // +0x08
    virtual void Slot0C();              // +0x0c
    virtual void Slot10();              // +0x10
    virtual void Slot14();              // +0x14
    virtual void Slot18();              // +0x18
    virtual void Slot1C();              // +0x1c
    virtual void Slot20();              // +0x20
    virtual int  Vfunc24(const char *key);                  // +0x24
    virtual void Slot28();                                  // +0x28
    virtual int  Vfunc2C(int a1, int a2, int a4, int a5);   // +0x2c
    virtual int  Vfunc30(int a1, int a2, int a4, int a5);   // +0x30
    virtual int  Vfunc34(int a1, int a3, int a4);           // +0x34
    virtual int  Vfunc38(int a1, int a3, int a4);           // +0x38
};

struct SeverusWorkerObj : public SeverusWorker {
    int        m_04;                    // +0x04  parent+0x1c
    int        m_08;                    // +0x08  0
    int        m_0c;                    // +0x0c  parent+0x0c
    CByteArray m_10;                    // +0x10
    char       m_pad24[0x64 - 0x24];
    int        m_64;                    // +0x64  0x1869f
    int        m_68;                    // +0x68  0
};                                      // 0x6c

DATA(0x1efc30)
extern void *g_severusWorkerBaseVtbl;
DATA(0x1efbe8)
extern void *g_severusWorkerVtbl;

DATA(0x2bf318)
extern int g_severusScratch[25];
DATA(0x2bf37c)
extern int g_severusCounterA;
DATA(0x2bf380)
extern int g_severusCounterB;

// ---------------------------------------------------------------------------
// CDDrawWorkerRegistry - only the load-bearing offset is modeled: the CMapStringToOb at
// +0x10. The matched method occupies a lower vtable slot (slot number not load-
// bearing, only body), placed last.
// ---------------------------------------------------------------------------
class CDDrawWorkerRegistry {
public:
    int  VirtualMethodUnknown14();
    int  VirtualMethodUnknown18();
    void VirtualMethodUnknown1C();
    int  VirtualMethodUnknown20();
    int  VirtualMethodUnknown24(int a1, int a2, const char *key, int a4, int a5);
    int  VirtualMethodUnknown28(int a1, int a2, SeverusWorker *worker, int a4, int a5);
    int  VirtualMethodUnknown2C(int a1, int a2, SeverusWorker *worker, int a4, int a5);
    int  VirtualMethodUnknown30(int a1, int a2, const char *key, int a4, int a5);
    int  VirtualMethodUnknown34(int a1, SeverusWorker *worker, int a3, int a4);
    int  VirtualMethodUnknown38(int a1, const char *key, int a3, int a4);
    int  VirtualMethodUnknown3C(int a1, SeverusWorker *worker, int a3, int a4);
    int  VirtualMethodUnknown40(int a1, const char *key, int a3, int a4);
    void VirtualMethodUnknown50(SeverusWorkerObj *worker);
    void VirtualMethodUnknown54(const char *key);
    void VirtualMethodUnknown58();
    void MapTeardown_1552b0();
    int  StringCopy_155810(const char *src);

    void          *m_vptr;                  // +0x00
    int            m_04;                     // +0x04  initialized to -1 when inactive
    char           m_pad08[0x0c - 0x08];    // +0x08..0x0b
    int            m_0c;                     // +0x0c  parent/root handle
    CMapStringToOb m_10;                     // +0x10  m_unknownMap

    // Engine-label backlog stubs.
    void Stub_154f80();
    void Stub_155160();
    void Stub_156df0();
    void Stub_156e80();
};

static inline int SeverusReadField1c(const CDDrawWorkerRegistry *p)
{
    return *(const int *)((const char *)p + 0x1c);
}

static inline void StampSeverusBaseVtbl(SeverusWorkerObj *w) { *(void **)w = &g_severusWorkerBaseVtbl; }
static inline void StampSeverusVtbl(SeverusWorkerObj *w) { *(void **)w = &g_severusWorkerVtbl; }

static inline SeverusWorkerObj *MakeSeverusWorker(const CDDrawWorkerRegistry *parent)
{
    SeverusWorkerObj *raw = (SeverusWorkerObj *)operator new(sizeof(SeverusWorkerObj));
    SeverusWorkerObj *w;
    if (raw != 0) {
        int field1c = SeverusReadField1c(parent);
        int harryPotter = parent->m_0c;
        StampSeverusBaseVtbl(raw);
        raw->m_04 = field1c;
        raw->m_08 = 0;
        raw->m_0c = harryPotter;
        new (&raw->m_10) CByteArray;
        StampSeverusVtbl(raw);
        raw->m_64 = 99999;
        raw->m_68 = 0;
        w = raw;
    } else {
        w = 0;
    }
    return w;
}

static inline SeverusWorkerObj *FindOrCreateWorker(CDDrawWorkerRegistry *parent, const char *key)
{
    CObject *found = 0;
    parent->m_10.Lookup(key, found);
    if (found == 0) {
        SeverusWorkerObj *worker = MakeSeverusWorker(parent);
    if (worker->Vfunc24(key) == 0) {
        if (worker != 0)
            worker->ScalarDtor(1);
        return 0;
    }
        parent->m_10[key] = (CObject *)worker;
        found = (CObject *)worker;
    }
    return (SeverusWorkerObj *)found;
}

// ---------------------------------------------------------------------------
// Clears the 25-dword scratch table and seeds the first entry to 100.
RVA(0x154aa0, 0x20)
int CDDrawWorkerRegistry::VirtualMethodUnknown18()
{
    for (int i = 0; i < 25; ++i)
        g_severusScratch[i] = 0;
    g_severusScratch[0] = 100;
    return 1;
}

// ---------------------------------------------------------------------------
// Runs the +0x58 hook, then clears the two counters.
RVA(0x154ac0, 0x12)
void CDDrawWorkerRegistry::VirtualMethodUnknown1C()
{
    ((UnknownSeverusVtableView *)this)->Slot58();
    g_severusCounterA = 0;
    g_severusCounterB = 0;
}

// ---------------------------------------------------------------------------
// Finds or creates the keyed worker, then forwards to worker slot +0x38.
RVA(0x154ae0, 0xfc)
int CDDrawWorkerRegistry::VirtualMethodUnknown38(int a1, const char *key, int a3, int a4)
{
    SeverusWorkerObj *worker = FindOrCreateWorker(this, key);
    if (worker == 0)
        return 0;
    return worker->Vfunc38(a1, a3, a4);
}

// ---------------------------------------------------------------------------
// Finds or creates the keyed worker, then forwards to worker slot +0x34.
RVA(0x154be0, 0xfc)
int CDDrawWorkerRegistry::VirtualMethodUnknown40(int a1, const char *key, int a3, int a4)
{
    SeverusWorkerObj *worker = FindOrCreateWorker(this, key);
    if (worker == 0)
        return 0;
    return worker->Vfunc34(a1, a3, a4);
}

// ---------------------------------------------------------------------------
// Finds or creates the keyed worker, then forwards to worker slot +0x30.
RVA(0x154ce0, 0x101)
int CDDrawWorkerRegistry::VirtualMethodUnknown30(int a1, int a2, const char *key, int a4, int a5)
{
    SeverusWorkerObj *worker = FindOrCreateWorker(this, key);
    if (worker == 0)
        return 0;
    return worker->Vfunc30(a1, a2, a4, a5);
}

// ---------------------------------------------------------------------------
// Finds or creates the keyed worker, then forwards to worker slot +0x2c.
RVA(0x154df0, 0x101)
int CDDrawWorkerRegistry::VirtualMethodUnknown24(int a1, int a2, const char *key, int a4, int a5)
{
    SeverusWorkerObj *worker = FindOrCreateWorker(this, key);
    if (worker == 0)
        return 0;
    return worker->Vfunc2C(a1, a2, a4, a5);
}

// ---------------------------------------------------------------------------
// Thin forwarder to worker slot +0x34.
RVA(0x154f00, 0x1b)
int CDDrawWorkerRegistry::VirtualMethodUnknown3C(int a1, SeverusWorker *worker, int a3, int a4)
{
    return worker->Vfunc34(a1, a3, a4);
}

// ---------------------------------------------------------------------------
// Thin forwarder to worker slot +0x38.
RVA(0x154f20, 0x1b)
int CDDrawWorkerRegistry::VirtualMethodUnknown34(int a1, SeverusWorker *worker, int a3, int a4)
{
    return worker->Vfunc38(a1, a3, a4);
}

// ---------------------------------------------------------------------------
// Thin forwarder to worker slot +0x30.
RVA(0x154f40, 0x20)
int CDDrawWorkerRegistry::VirtualMethodUnknown2C(int a1, int a2, SeverusWorker *worker, int a4, int a5)
{
    return worker->Vfunc30(a1, a2, a4, a5);
}

// ---------------------------------------------------------------------------
// Thin forwarder to worker slot +0x2c.
RVA(0x154f60, 0x20)
int CDDrawWorkerRegistry::VirtualMethodUnknown28(int a1, int a2, SeverusWorker *worker, int a4, int a5)
{
    return worker->Vfunc2C(a1, a2, a4, a5);
}

// ---------------------------------------------------------------------------
// Removes a non-null worker from the map by its key at +0x24, then destroys it.
RVA(0x155280, 0x22)
void CDDrawWorkerRegistry::VirtualMethodUnknown50(SeverusWorkerObj *worker)
{
    if (worker != 0) {
        m_10.RemoveKey((const char *)((char *)worker + 0x24));
        worker->ScalarDtor(1);
    }
}

// ---------------------------------------------------------------------------
// Constant state id.
RVA(0x156de0, 0x6)
int CDDrawWorkerRegistry::VirtualMethodUnknown20()
{
    return 0x12;
}

// ---------------------------------------------------------------------------
// Same base readiness predicate used by several Lucius-derived managers.
RVA(0x1576d0, 0x16)
int CDDrawWorkerRegistry::VirtualMethodUnknown14()
{
    if (m_0c == 0)
        goto fail;
    if (m_04 != -1)
        return 1;

fail:
    return 0;
}

// ---------------------------------------------------------------------------
// Lookup `key` in the map; if found, RemoveKey it and run the value's scalar-
// deleting destructor (vtbl +0x4, arg 1).
//
// RESIDUE (~77.5%, NOT a logic/offset/type/CFG error - register-allocation +
// store/load-scheduling entropy, matching-patterns.md / match-learnings.md
// "register-alloc coin-flip"): the logic, CFG, the val=0 init, both library calls,
// their args, and the dtor dispatch are all reproduced; only the register schedule
// the allocator picked differs. The target holds `key` in EDI and keeps `val`
// purely on the stack, reloading it (`mov ecx,[esp+8]`) AFTER RemoveKey and
// branching on Lookup's return in EAX. MSVC5 on this (isolated) TU instead caches
// `key` in EBX and `val` in EDI across the calls and branches on the cached val -
// equivalent codegen, an extra callee-saved register, a different ecx/edx pairing.
// Every source form tried (if(Lookup(...)) / if(...!=0) / captured `int found` /
// volatile val) produced the identical schedule; the surrounding symbol-set is what
// re-rolls the allocation, so no in-function lever flips it. Left as the plateau.
RVA(0x156ec0, 0x40)
void CDDrawWorkerRegistry::VirtualMethodUnknown54(const char *key)
{
    CObject *val = 0;
    if (m_10.Lookup(key, val)) {
        m_10.RemoveKey(key);
        ((SeverusValue *)val)->ScalarDtor(1);
    }
}

// ---------------------------------------------------------------------------
// Map teardown: iterate all entries in m_10 via GetNextAssoc, destroying each
// CObject* value via its scalar-deleting destructor (vtbl +0x4 arg 1), then
// RemoveAll the map. Same pattern as CDDrawWorkerMapSmall::VirtualMethodUnknown1C but
// without the final m_64 clear (that field does not exist in this class).
//
// Carries a /GX EH frame for the local CString key (destructor must fire on
// unwind through the iteration loop).
RVA(0x165210, 0xa2)
void CDDrawWorkerRegistry::VirtualMethodUnknown58()
{
    CObject *val = 0;
    int pos = (m_10.m_nCount != 0) ? -1 : 0;
    CString key;
    if (*(volatile int *)&pos != 0) {
        do {
            m_10.GetNextAssoc(pos, key, val);
            if (val != 0)
                ((SeverusValue *)val)->ScalarDtor(1);
        } while (pos != 0);
    }
    m_10.RemoveAll();
}

extern "C" char *_strncpy(char *, const char *, unsigned int);

// ---------------------------------------------------------------------------
// Map teardown leaf (SEH)
// Iterates m_10 via GetNextAssoc, destroys each CObject* value via
// scalar-deleting dtor (vtbl+0x4 arg 1), then RemoveAll.
RVA(0x1552b0, 0xa2)
void CDDrawWorkerRegistry::MapTeardown_1552b0()
{
    CObject *val = 0;
    int pos = (m_10.m_nCount != 0) ? -1 : 0;
    CString key;
    if (*(volatile int *)&pos != 0) {
        do {
            m_10.GetNextAssoc(pos, key, val);
            if (val != 0)
                ((SeverusValue *)val)->ScalarDtor(1);
        } while (pos != 0);
    }
    m_10.RemoveAll();
}

// ---------------------------------------------------------------------------
// String copy leaf
// Copies at most 0x3F bytes from src into this+0x24, null-terminates at
// this+0x63, returns 1.
RVA(0x155810, 0x23)
int CDDrawWorkerRegistry::StringCopy_155810(const char *src)
{
    _strncpy((char *)this + 0x24, src, 0x3f);
    *((char *)this + 0x63) = 0;
    return 1;
}

// -------------------------------------------------------------------------
// Engine-label backlog stubs.
// -------------------------------------------------------------------------
// @confidence: high
// @source: tomalla
// @stub
RVA(0x154f80, 0x1d5)
void CDDrawWorkerRegistry::Stub_154f80() {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x155160, 0x11e)
void CDDrawWorkerRegistry::Stub_155160() {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x156df0, 0x1e)
void CDDrawWorkerRegistry::Stub_156df0() {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x156e80, 0x38)
void CDDrawWorkerRegistry::Stub_156e80() {}
