// TriggerLoadRec.cpp - CTriggerLoadRec::Deserialize (0x009bb0), carved out of the
// conflated StreamRecordLoaders.cpp (operation REHOME, package D8). This is the
// serialized trigger sub-record CTriggerMgr news at +0x25c and loads from the
// stream via the shared WAP32 CSerialArchive reader (Read @ vtable +0x2c), interning
// string-valued fields against the game registry name map. Its retail .text sits at
// 0x009bb0 - far from the CEventLoadRec main block (0x09c650) - a separate obj.
// Interleaver home (RVA-neighbour caller unit): src/Gruntz/WorldSoundSet.cpp; homing
// there is deferred (cross-TU class decl).
//
// Field names are placeholders; only the field offsets + code bytes are load-bearing.
#include <rva.h>
#include <Gruntz/MgrSettings.h>   // CDDrawWorkerRegistry (name map + AnyValueMatches)
#include <Gruntz/GameRegistry.h>  // CGameRegistry (g_gameReg->m_world)
#include <Gruntz/SerialArchive.h> // CSerialArchive (reader; Read @ +0x2c)
#include <Gruntz/SerialRecView.h> // CRegSub30 / CRegTypeTable (shared registry views)
#include <string.h>               // inline strlen (repne scasb) over the scratch buffer

// The game registry singleton (0x64556c). The delinker's canonical symbol is the
// extern "C" _g_mgrSettings (the cplay unit owns it); reloc-masked DIR32.
DATA(0x0024556c)
extern "C" CGameRegistry* g_gameReg;

// The serialize sequence counter (0x629ad0, ?g_serialCounter@@3HA): bumped once per
// string field read.
DATA(0x00229ad0)
extern i32 g_serialCounter;

// ===========================================================================
// CTriggerLoadRec::Load (0x009bb0) - a serialized trigger sub-record CTriggerMgr
// news at +0x25c and loads from the stream. Reads a fixed run of raw fields, then
// three plain name->object refs (m_30/m_34/m_38) and three bounds-checked
// type-table index refs (m_10/m_1c/m_20). __thiscall, ret 4; returns 1, or 0 if
// the stream / registry is absent.
// ===========================================================================
struct CTriggerLoadRec {
    i32 Deserialize(CSerialArchive* s);

    i32 m_0, m_4, m_8, m_c; // +0x00..+0x0c  raw
    void* m_10;             // +0x10  indexed type ref
    i32 m_14, m_18;         // +0x14,+0x18  raw
    void* m_1c;             // +0x1c  indexed type ref
    void* m_20;             // +0x20  indexed type ref
    i32 m_24, m_28, m_2c;   // +0x24,+0x28,+0x2c  raw
    void* m_30;             // +0x30  name ref
    void* m_34;             // +0x34  name ref
    void* m_38;             // +0x38  name ref
    i32 m_3c;               // +0x3c  raw
};

// @early-stop
// outparam-zeroinit-scheduling wall (docs/patterns/outparam-zeroinit-scheduling.md),
// 92.2%: logic + offsets byte-exact. The indexed-block regalloc (idx pinned in a
// callee-saved reg across the Lookup call) WAS cracked here by the `i32 i = idx;`
// copy (88.9 -> 92.2). Sole residual: the 6 `out = 0` stores - retail SINKS
// `mov [&out],eax` (reusing strlen's `xor eax,eax` zero) past the arg pushes, cl
// HOISTS it after `lea &out`. Tried comma-injecting the store into the call's
// this-expression and a map-receiver temp (regressed to 89%); the store position
// is the MSVC5 scheduler coin-flip, source-invariant.
RVA(0x00009bb0, 0x367)
i32 CTriggerLoadRec::Deserialize(CSerialArchive* s) {
    if (s == 0) {
        return 0;
    }
    CGameRegistry* gr = g_gameReg;
    if (gr == 0) {
        return 0;
    }
    CRegSub30* reg = (CRegSub30*)gr->m_world;
    if (reg == 0) {
        return 0;
    }

    char buf[0x80];
    CObject* out;
    i32 idx;

    s->Read(&m_0, 8);
    s->Read(&m_8, 4);
    s->Read(&m_c, 4);
    s->Read(&m_3c, 4);
    s->Read(&m_2c, 4);
    s->Read(&m_14, 8);
    s->Read(&m_24, 8);

    g_serialCounter++;
    s->Read(buf, 0x80);
    if (strlen(buf) != 0) {
        out = 0;
        reg->m_10->m_10.Lookup(buf, out);
        m_30 = out;
    } else {
        m_30 = 0;
    }

    g_serialCounter++;
    s->Read(buf, 0x80);
    if (strlen(buf) != 0) {
        out = 0;
        reg->m_10->m_10.Lookup(buf, out);
        m_34 = out;
    } else {
        m_34 = 0;
    }

    g_serialCounter++;
    s->Read(buf, 0x80);
    if (strlen(buf) != 0) {
        out = 0;
        reg->m_10->m_10.Lookup(buf, out);
        m_38 = out;
    } else {
        m_38 = 0;
    }

    g_serialCounter++;
    s->Read(buf, 0x80);
    s->Read(&idx, 4);
    if (strlen(buf) != 0) {
        i32 i = idx;
        out = 0;
        reg->m_10->m_10.Lookup(buf, out);
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
        reg->m_10->m_10.Lookup(buf, out);
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
        reg->m_10->m_10.Lookup(buf, out);
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

    return 1;
}
SIZE_UNKNOWN(CTriggerLoadRec);
