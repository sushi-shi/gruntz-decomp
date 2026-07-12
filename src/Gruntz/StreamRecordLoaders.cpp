// StreamRecordLoaders.cpp - the CEventLoadRec main cluster (0x09c650) + the spatially
// adjacent 0x09cab0 orphan out-param wrapper. The former conflated peers - the trigger
// (0x009bb0), grunt-state (0x0ea990) and projectile (0x0e0d40) record loaders - were
// carved into their own TUs (TriggerLoadRec/GruntStateRec/ProjLoadRec.cpp; operation
// REHOME package D8), each an independent obj at its own far-away .text block.
//
// Both records here stream through the shared WAP32 CSerialArchive stream-reader (a
// virtual Read at vtable +0x2c), with string-valued fields read into a scratch buffer
// and interned against the game registry's name->object map (g_gameReg->m_world +0x10,
// CDDrawWorkerRegistry's CMapStringToOb::Lookup). A global sequence counter
// (g_serialCounter) ticks once per string read. Each Load is a __thiscall taking the
// reader (ret 4); names are placeholders, only the field offsets + code bytes are
// load-bearing. The shared registry views (CRegSub30 / CRegTypeTable) live in
// <Gruntz/SerialRecView.h>.
#include <rva.h>
#include <Gruntz/MgrSettings.h>   // CDDrawWorkerRegistry (the name map at g_gameReg->m_world +0x10)
#include <Gruntz/GameRegistry.h>  // CGameRegistry (g_gameReg->m_world)
#include <Gruntz/SerialArchive.h> // CSerialArchive (reader; Read @ vtable +0x2c)
#include <Gruntz/SerialRecView.h> // CRegSub30 / CRegTypeTable (shared registry views)
#include <string.h>               // inline strlen (repne scasb) over the scratch buffer

// The game registry singleton (0x64556c). The delinker's canonical symbol is the
// extern "C" _g_mgrSettings (the cplay unit owns it); reloc-masked DIR32.
DATA(0x0024556c)
extern "C" CGameRegistry* g_gameReg;

// The serialize sequence counter (0x629ad0, ?g_serialCounter@@3HA): bumped once
// per string field read.
DATA(0x00229ad0)
extern i32 g_serialCounter;

// ===========================================================================
// CEventLoadRec::Load (0x09c650) - the HandleEvent-path serialized record. Same
// reader+registry idiom as CTriggerLoadRec but a different field layout: two raw
// dwords, one plain name ref (m_8), one raw dword, five bounds-checked type-table
// index refs (m_10..m_20), then two trailing raw dwords (m_48/m_4c). __thiscall,
// ret 4. Note: unlike CTriggerLoadRec it does NOT null-check g_gameReg, only
// the m_30 sub-registry.
// ===========================================================================
struct CEventLoadRec {
    i32 Load(CSerialArchive* s);

    i32 m_0, m_4; // +0x00,+0x04  raw
    void* m_8;    // +0x08  name ref
    i32 m_c;      // +0x0c  raw
    void* m_10;   // +0x10  indexed type ref
    void* m_14;   // +0x14  indexed type ref
    void* m_18;   // +0x18  indexed type ref
    void* m_1c;   // +0x1c  indexed type ref
    void* m_20;   // +0x20  indexed type ref
    char m_pad24[0x48 - 0x24];
    i32 m_48, m_4c; // +0x48,+0x4c  raw
};

// @early-stop
// outparam-zeroinit-scheduling wall (same as CTriggerLoadRec): logic + offsets
// byte-exact, residual is only the 6 `out = 0` store positions (retail sinks, cl
// hoists). The idx-in-callee-saved-reg regalloc is steered by the `i32 i = idx;`
// copy. ~92%.
RVA(0x0009c650, 0x372)
i32 CEventLoadRec::Load(CSerialArchive* s) {
    if (s == 0) {
        return 0;
    }
    CRegSub30* reg = (CRegSub30*)g_gameReg->m_world;
    if (reg == 0) {
        return 0;
    }

    char buf[0x80];
    void* out;
    i32 idx;

    s->Read(&m_0, 4);
    s->Read(&m_4, 4);

    g_serialCounter++;
    s->Read(buf, 0x80);
    if (strlen(buf) != 0) {
        out = 0;
        reg->m_10->m_10.Lookup(buf, (CObject*&)out);
        m_8 = out;
    } else {
        m_8 = 0;
    }

    s->Read(&m_c, 4);

    g_serialCounter++;
    s->Read(buf, 0x80);
    s->Read(&idx, 4);
    if (strlen(buf) != 0) {
        i32 i = idx;
        out = 0;
        reg->m_10->m_10.Lookup(buf, (CObject*&)out);
        CRegTypeTable* tt = (CRegTypeTable*)out;
        void* r;
        if (tt != 0 && i >= tt->m_lowerBound && i <= tt->m_upperBound) {
            r = tt->m_elems[i];
        } else {
            r = 0;
        }
        m_10 = r;
    } else {
        m_10 = 0;
    }

    g_serialCounter++;
    s->Read(buf, 0x80);
    s->Read(&idx, 4);
    if (strlen(buf) != 0) {
        i32 i = idx;
        out = 0;
        reg->m_10->m_10.Lookup(buf, (CObject*&)out);
        CRegTypeTable* tt = (CRegTypeTable*)out;
        void* r;
        if (tt != 0 && i >= tt->m_lowerBound && i <= tt->m_upperBound) {
            r = tt->m_elems[i];
        } else {
            r = 0;
        }
        m_14 = r;
    } else {
        m_14 = 0;
    }

    g_serialCounter++;
    s->Read(buf, 0x80);
    s->Read(&idx, 4);
    if (strlen(buf) != 0) {
        i32 i = idx;
        out = 0;
        reg->m_10->m_10.Lookup(buf, (CObject*&)out);
        CRegTypeTable* tt = (CRegTypeTable*)out;
        void* r;
        if (tt != 0 && i >= tt->m_lowerBound && i <= tt->m_upperBound) {
            r = tt->m_elems[i];
        } else {
            r = 0;
        }
        m_18 = r;
    } else {
        m_18 = 0;
    }

    g_serialCounter++;
    s->Read(buf, 0x80);
    s->Read(&idx, 4);
    if (strlen(buf) != 0) {
        i32 i = idx;
        out = 0;
        reg->m_10->m_10.Lookup(buf, (CObject*&)out);
        CRegTypeTable* tt = (CRegTypeTable*)out;
        void* r;
        if (tt != 0 && i >= tt->m_lowerBound && i <= tt->m_upperBound) {
            r = tt->m_elems[i];
        } else {
            r = 0;
        }
        m_1c = r;
    } else {
        m_1c = 0;
    }

    g_serialCounter++;
    s->Read(buf, 0x80);
    s->Read(&idx, 4);
    if (strlen(buf) != 0) {
        i32 i = idx;
        out = 0;
        reg->m_10->m_10.Lookup(buf, (CObject*&)out);
        CRegTypeTable* tt = (CRegTypeTable*)out;
        void* r;
        if (tt != 0 && i >= tt->m_lowerBound && i <= tt->m_upperBound) {
            r = tt->m_elems[i];
        } else {
            r = 0;
        }
        m_20 = r;
    } else {
        m_20 = 0;
    }

    s->Read(&m_48, 4);
    s->Read(&m_4c, 4);

    return 1;
}
SIZE_UNKNOWN(CEventLoadRec);

// ---------------------------------------------------------------------------
// 0x09cab0 (spatially re-homed from src/Stub/BoundaryLowerMethods.cpp; adjacent to
// CEventLoadRec at 0x09c650). Out-param wrapper: call the +0x10 sub's Lookup
// (0x1b8008 == CMapStringToPtr::Lookup) with a zeroed local and return the filled
// local. @orphan (registry class unrecovered).
struct CSub9cab0 {
    i32 Lookup(const char* key, void*& out); // 0x1b8008 (CMapStringToPtr::Lookup)
};
struct C9cab0 {
    char pad0[0x10];
    CSub9cab0 m_10; // +0x10
    i32 LookupPtr(i32 arg);
};
RVA(0x0009cab0, 0x23)
i32 C9cab0::LookupPtr(i32 arg) {
    i32 local = 0;
    m_10.Lookup((const char*)arg, (void*&)local);
    return local;
}
SIZE_UNKNOWN(CSub9cab0);
SIZE_UNKNOWN(C9cab0);
