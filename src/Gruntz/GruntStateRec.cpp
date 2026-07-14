// GruntStateRec.cpp - CGruntStateRec::Load (0x0ea990), carved out of the conflated
// StreamRecordLoaders.cpp (operation REHOME, package D8). The dual-mode grunt-state
// record loader; its retail .text sits at 0x0ea990 - far from the CEventLoadRec main
// block (0x09c650) - a separate obj. Interleaver home (RVA-neighbour caller unit):
// src/Gruntz/StatusBarTabBuilders.cpp; homing there is deferred (cross-TU class decl).
//
// Field names are placeholders; only the field offsets + code bytes are load-bearing.
#include <rva.h>
#include <Io/FileMem.h>           // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Gruntz/MgrSettings.h>   // CDDrawWorkerRegistry (name map + AnyValueMatches_155630)
#include <Gruntz/GameRegistry.h>  // CGameRegistry (g_gameReg->m_world)
#include <Gruntz/SerialArchive.h> // CSerialArchive (reader/writer; Read @ +0x2c / Write @ +0x30)
#include <Gruntz/SerialRecView.h> // CRegSub30 / CRegTypeTable (shared registry views)
#include <string.h>               // inline strlen / strcpy / memset over the scratch buffer

// The game registry singleton (0x64556c). Reloc-masked DIR32 (cplay owns the def).
extern "C" CGameRegistry* g_gameReg;

// The serialize sequence counter (0x629ad0, ?g_serialCounter@@3HA): bumped once per
// string field read.
extern i32 g_serialCounter;

// ===========================================================================
// CGruntStateRec::Load (0x0ea990) - the dual-mode grunt-state record loader. A
// __thiscall taking (reader, mode, a2, a3), ret 0x10. Bails (0) when the reader
// or the registry sub-object (g_gameReg->m_world) is absent. Mode 7 loads the
// fields as registry refs (the CEventLoadRec indexed-type-ref / name-ref idiom,
// the reader's Read at vtable +0x2c); mode 4 stores them as nested sub-records (the
// CArchiveLoadRec FillDefault + sub-reader idiom, Write at vtable +0x30). Either
// way it tail-chains the base loader (0x1848) and normalises its result to 0/1.
// The reader is the shared WAP32 CSerialArchive: Read at vtable +0x2c (mode 7 =
// load) and Write at +0x30 (mode 4 = store) - both slots off the one type.
// ===========================================================================
struct CGruntStateRec {
    i32 Load(CSerialArchive* s, i32 mode, i32 a2, i32 a3);
    i32 ChainLoad(CSerialArchive* s, i32 mode, i32 a2, i32 a3); // 0x1848 (base chain)

    char m_pad00[0x30];
    void* m_30; // +0x30  ref / sub-record
    void* m_34; // +0x34
    i32 m_38;   // +0x38
    void* m_3c; // +0x3c
    void* m_40; // +0x40
    i32 m_44;   // +0x44
    void* m_48; // +0x48
    void* m_4c; // +0x4c
    i32 m_50;   // +0x50
    void* m_54; // +0x54
    void* m_58; // +0x58
    i32 m_5c;   // +0x5c
    i32 m_60;   // +0x60
    i32 m_64;   // +0x64
    void* m_68; // +0x68
    void* m_6c; // +0x6c
    i32 m_70;   // +0x70
    void* m_74; // +0x74
};

// @early-stop
// scratch-slot scheduling tail (same family as CTriggerLoadRec/CEventLoadRec/
// CArchiveLoadRec): the dual-mode switch, every Read field/size, the indexed-ref
// bounds checks, the name-ref Lookups, the FillDefault sub-records, the inline
// strlen/strcpy, the g_serialCounter bumps and the tail-chain + 0/1 normalise are
// byte-faithful; residual is the MSVC5 scratch-buffer slot assignment + the
// outparam zero-init store positions. Not source-steerable.
RVA(0x000ea990, 0xa72)
i32 CGruntStateRec::Load(CSerialArchive* s, i32 mode, i32 a2, i32 a3) {
    if (s == 0) {
        return 0;
    }
    CRegSub30* reg = (CRegSub30*)g_gameReg->m_world;
    if (reg == 0) {
        return 0;
    }

    char buf[0x80];
    CObject* out;
    i32 idx;
    i32 v;

    switch (mode) {
        case 4:
            // --- mode 4 (store): nested sub-records via FillDefault + Write @ +0x30 ---
#define GS_SUBREC(field)                                                                           \
    g_serialCounter++;                                                                             \
    memset(buf, 0, sizeof(buf));                                                                   \
    v = 0;                                                                                         \
    if (field != 0) {                                                                              \
        reg->m_10->AnyValueMatches_155630((i32)field, (i32)buf, (i32) & v);                        \
    }                                                                                              \
    s->Write(buf, 0x80);                                                                           \
    s->Write(&v, 4)

            GS_SUBREC(m_30);
            GS_SUBREC(m_34);
            s->Write(&m_38, 4);
            GS_SUBREC(m_3c);
            GS_SUBREC(m_40);
            s->Write(&m_44, 4);
            GS_SUBREC(m_48);
            GS_SUBREC(m_4c);
            s->Write(&m_50, 4);
            GS_SUBREC(m_54);
            GS_SUBREC(m_58);
            s->Write(&m_5c, 4);
            GS_SUBREC(m_6c);
            s->Write(&m_70, 4);
            s->Write(&m_60, 4);
            s->Write(&m_64, 4);
#undef GS_SUBREC

            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            if (m_74 != 0) {
                strcpy(buf, (char*)m_74 + 0x24);
            }
            s->Write(buf, 0x80);

            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            if (m_68 != 0) {
                strcpy(buf, (char*)m_68 + 0x24);
            }
            s->Write(buf, 0x80);
            break;

        case 7:
            // --- mode 7: registry refs via the +0x2c reader ---
#define GS_IDXREF(field)                                                                           \
    g_serialCounter++;                                                                             \
    s->Read(buf, 0x80);                                                                            \
    s->Read(&idx, 4);                                                                              \
    if (strlen(buf) != 0) {                                                                        \
        i32 i = idx;                                                                               \
        out = 0;                                                                                   \
        reg->m_10->m_10map.Lookup(buf, out);                                                         \
        CRegTypeTable* tt = (CRegTypeTable*)out;                                                   \
        void* r;                                                                                   \
        if (tt != 0 && i >= tt->m_lowerBound && i <= tt->m_upperBound) {                           \
            r = tt->m_elems[i];                                                                    \
        } else {                                                                                   \
            r = 0;                                                                                 \
        }                                                                                          \
        field = r;                                                                                 \
    } else {                                                                                       \
        field = 0;                                                                                 \
    }
#define GS_NAMEREF(field)                                                                          \
    g_serialCounter++;                                                                             \
    s->Read(buf, 0x80);                                                                            \
    if (strlen(buf) != 0) {                                                                        \
        out = 0;                                                                                   \
        reg->m_10->m_10map.Lookup(buf, out);                                                         \
        field = out;                                                                               \
    } else {                                                                                       \
        field = 0;                                                                                 \
    }

            GS_IDXREF(m_30);
            GS_IDXREF(m_34);
            s->Read(&m_38, 4);
            GS_IDXREF(m_40);
            s->Read(&m_44, 4);
            GS_IDXREF(m_48);
            GS_IDXREF(m_4c);
            s->Read(&m_50, 4);
            GS_IDXREF(m_54);
            GS_IDXREF(m_58);
            s->Read(&m_5c, 4);
            GS_IDXREF(m_6c);
            s->Read(&m_70, 4);
            s->Read(&m_60, 4);
            s->Read(&m_64, 4);
            GS_NAMEREF(m_74);
            GS_NAMEREF(m_68);
#undef GS_IDXREF
#undef GS_NAMEREF
            break;
    }

    return ChainLoad(s, mode, a2, a3) != 0 ? 1 : 0;
}
SIZE_UNKNOWN(CGruntStateRec);
