// StreamRecordLoaders.cpp - the WAP "binary record load" idiom cluster. Several
// unrelated game classes deserialize a fixed field sequence from a stream-reader
// object (a virtual Read at vtable +0x2c), with string-valued fields read into a
// scratch buffer and interned against the game registry's name->object map
// (g_gameReg->m_world->m_10 + 0x10, CMapStringToPtr::Lookup @0x1b8008). A global
// sequence counter (g_serialCounter) ticks once per string read. Each Load is a
// __thiscall taking the reader (ret 4); names are placeholders, only the field
// offsets + code bytes are load-bearing.
//
// These records belong to distinct classes (the trigger system, the event
// handler, the grunt-tuning loader, ...) found by caller-tracing each through its
// ILT thunk; the shared reader/registry models below are the load-bearing shape.
#include <rva.h>
#include <Rez/RezList.h>
#include <Gruntz/MgrSettings.h>
#include <string.h> // inline strlen (repne scasb) over the scratch buffer

#include <Gruntz/SerialObjRef.h> // CSerialArchive (reader), CDDrawSubMgrLeaf (registry
#include <Gruntz/GameRegistry.h>
#include <Globals.h>
// leaf + KeyOfValue), CSerialObj - shared with the
// embedded +0x150 sub-record of CProjLoadRec below

// ---------------------------------------------------------------------------
// Every record here streams through the shared WAP32 CSerialArchive (Read @ vtable
// +0x2c = load, Write @ +0x30 = store), pulled in via <Gruntz/SerialObjRef.h> above
// - the former local CStreamReader / CArchiveReader / CDualReader views are all folded
// away. `s->Read` lowers to `mov edx,[esi]; call [edx+0x2c]`; `s->Write` to
// `call [edx+0x30]`. (The +0x30-only CArchiveLoadRec store-serializer reaches Write;
// the dual-mode CGruntStateRec/CProjLoadRec reach both slots off the one type.)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// The game registry's string->object name map (CMapStringToPtr::Lookup @0x1b8008,
// __thiscall): Lookup(key, &out) writes the found object (or leaves out untouched).
// Reached as g_gameReg->m_world->m_10 + 0x10.
// ---------------------------------------------------------------------------
struct CRegNameMap {
    i32 Lookup(char* key, void** out); // 0x1b8008
};
struct CRegSub30 {
    char m_pad00[0x10];
    CDDrawWorkerRegistry* m_10; // +0x10  the name table
};

// The looked-up "type table" value an indexed field resolves through: a bounded
// array (m_14[m_64 .. m_68]) whose element at the read index becomes the field.
struct CRegTypeTable {
    char m_pad00[0x14];
    void** m_elems; // +0x14  element array
    char m_pad18[0x64 - 0x18];
    i32 m_lowerBound; // +0x64  lower bound (inclusive)
    i32 m_upperBound; // +0x68  upper bound (inclusive)
};

// The game registry singleton (0x64556c). The delinker's canonical symbol is the
// extern "C" _g_mgrSettings (the cplay unit owns it); reloc-masked DIR32.
DATA(0x0024556c)
extern "C" CGameRegistry* g_gameReg;

// The serialize sequence counter (0x629ad0, ?g_serialCounter@@3HA): bumped once
// per string field read.
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
    void* out;
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
        reg->m_10->m_10.Lookup(buf, (CObject*&)out);
        m_30 = out;
    } else {
        m_30 = 0;
    }

    g_serialCounter++;
    s->Read(buf, 0x80);
    if (strlen(buf) != 0) {
        out = 0;
        reg->m_10->m_10.Lookup(buf, (CObject*&)out);
        m_34 = out;
    } else {
        m_34 = 0;
    }

    g_serialCounter++;
    s->Read(buf, 0x80);
    if (strlen(buf) != 0) {
        out = 0;
        reg->m_10->m_10.Lookup(buf, (CObject*&)out);
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

    return 1;
}

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
// outparam-zeroinit-scheduling wall (same as CTriggerLoadRec above): logic +
// offsets byte-exact, residual is only the 6 `out = 0` store positions (retail
// sinks, cl hoists). The idx-in-callee-saved-reg regalloc is steered by the
// `i32 i = idx;` copy. ~92%.
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

// ---------------------------------------------------------------------------
// 0x09cab0 (spatially re-homed from src/Stub/BoundaryLowerMethods.cpp). Out-param
// wrapper: call the +0x10 sub's Lookup (0x1b8008 == CMapStringToPtr::Lookup) with a
// zeroed local and return the filled local. @orphan (registry class unrecovered).
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

// ===========================================================================
// CGruntStateRec::Load (0x0ea990) - the dual-mode grunt-state record loader. A
// __thiscall taking (reader, mode, a2, a3), ret 0x10. Bails (0) when the reader
// or the registry sub-object (g_gameReg->m_world) is absent. Mode 7 loads the
// fields as registry refs (the CEventLoadRec indexed-type-ref / name-ref idiom,
// the reader's Read at vtable +0x2c); mode 4 stores them as nested sub-records (the
// CArchiveLoadRec FillDefault + sub-reader idiom, Write at vtable +0x30). Either
// way it tail-chains the base loader (0x1848) and normalises its result to 0/1.
// ===========================================================================

// The reader is the shared WAP32 CSerialArchive: it exposes BOTH serialize entries -
// Read at vtable +0x2c (mode 7 = load) and Write at +0x30 (mode 4 = store) - so the
// dual-mode loader below reaches both slots off the one type (from CSerialObjRef.h).

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
    void* out;
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
        reg->m_10->m_10.Lookup(buf, (CObject*&)out);                                               \
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
        reg->m_10->m_10.Lookup(buf, (CObject*&)out);                                               \
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

// ===========================================================================
// CProjLoadRec::Load (0x0e0d40) - a CProjectile/CTimeBomb-family dual-mode record
// loader. A __thiscall(reader, mode, a2, a3), ret 0x10, bailing (0) when the
// registry sub-object (g_gameReg->m_world) is absent. Mode 7 = READ: a fixed run
// of raw fields, a 7-entry name-ref loop (CMapStringToOb::Lookup @0x1b8438 through
// reg->m_2c->m_10), a single CMapPtrToPtr::Lookup @0x1b8760 (through reg->m_8->m_48)
// gated on the looked-up object's type code (virtual +0x20 == 5), then a g_freeList
// node-splice loop appending 8-byte payloads onto m_204 (CObList::AddTail @0x1b4991).
// Mode 4 = WRITE: re-derives each ref's name via reg->m_2c->KeyOfValue_152d30 and
// writes it back. Either way it tail-chains the base loader (0x16f4a0), then runs an
// embedded CSerialObjRef record at +0x150 (read/write a key name + 0x10 blob, resolve
// through a3->m_7c->m_c->m_2c). Names are placeholders; offsets + bytes load-bearing.
// ===========================================================================

// The g_freeList node allocator (?g_freeList@@3PAXA): the head is a node whose first
// dword is the next pointer; a non-empty pop advances the head and yields node+4.
DATA(0x00245544)
extern void* g_freeList;

// reg->m_8: a sub-registry whose CMapPtrToPtr (Lookup @0x1b8760) sits at +0x48.
struct CProjObjReg {
    char _00[0x48];
    CMapPtrToPtr m_48; // +0x48
};

// g_gameReg->m_world (the game registry's +0x30 sub-registry) viewed by this
// loader: the projectile-object map at +0x8 and the name leaf at +0x2c (the same
// CDDrawSubMgrLeaf type CSerialObjRef resolves through). +0x8 is a CProjObjReg*
// (the retail-correct type). Distinct object from CProjReg in ProjActRegistry.cpp.
SIZE_UNKNOWN(CProjRegSub30);
struct CProjRegSub30 {
    char _00[0x8];
    CProjObjReg* m_8; // +0x08
    char _0c[0x2c - 0xc];
    CDDrawSubMgrLeaf* m_2c; // +0x2c
};

// The FOREIGN CMapPtrToPtr-resolved object whose type code (vtable slot +0x20) gates
// the m_1fc latch; its default-int is read from +0x188 on the write path. Only that
// one slot is dispatched (rest is unreconstructed engine code); pointer-only, never
// constructed. Honest model = a manual vptr into a typed vtable struct naming ONLY
// the used slot as a 4-byte thiscall PMF + char pad[], NO fake virtuals. The vptr
// (m_vtbl) sits at +0x00 exactly where the fake virtuals' vptr did, so the object
// layout (_24 pad, m_188) is byte-identical.
// Real polymorphic view: GetTypeCode is slot 8 (+0x20), a real virtual (8 fillers).
class CProjTypeObj {
public:
    virtual void Slot0();
    virtual void Slot1();
    virtual void Slot2();
    virtual void Slot3();
    virtual void Slot4();
    virtual void Slot5();
    virtual void Slot6();
    virtual void Slot7();
    virtual i32 GetTypeCode(); // slot 8 (+0x20)
    char _24[0x188 - 0x24];
    i32 m_188; // +0x188
    i32 CallGetTypeCode() {
        return GetTypeCode();
    }
};

// One spliced freelist node: next at +0, payload pointer at +8.
struct CProjNode {
    CProjNode* m_next; // +0x00
    i32 m_04;
    void* m_08; // +0x08
};

// The +0x204 list the read path appends payloads to (CObList::AddTail @0x1b4991);
// sized to one pointer so the following fields keep their offsets.
// a3->m_7c->m_c->m_2c is the registry leaf; CSerialObj/CSerialNameHolder give m_7c
// and m_0c, but the inlined +0x150 record reaches m_c (not m_0c) - the same shape at
// +0x0c. Reuse CSerialObj for a3; view its name-holder's +0x0c through CSerialNameHolder.
struct CProjLoadRec {
    i32 Load(CSerialArchive* s, i32 mode, i32 a2, CSerialObj* a3);      // 0x0e0d40
    i32 ChainLoad(CSerialArchive* s, i32 mode, i32 a2, CSerialObj* a3); // 0x16f4a0

    char _00[0x150];
    CSerialObj* m_150;                     // +0x150  a3
    CSerialObj* m_154;                     // +0x154  a3
    CSerialNameHolder* m_158;              // +0x158  a3->m_7c
    CObject* m_15c;                        // +0x15c  resolved value (CMapStringToOb entry)
    i32 m_160, m_164, m_168, m_16c;        // +0x160  the 0x10-byte blob
    i32 m_170, m_174, m_178, m_17c, m_180; // +0x170
    i32 _184;
    i32 m_188, m_18c; // +0x188 (8)
    i32 m_190;        // +0x190
    i32 _194;
    i32 m_198, m_19c;               // +0x198 (8)
    i32 m_1a0, m_1a4;               // +0x1a0 (8)
    i32 m_1a8, m_1ac;               // +0x1a8 (8)
    i32 m_1b0, m_1b4;               // +0x1b0 (8)
    i32 m_1b8, m_1bc;               // +0x1b8 (8)
    i32 m_1c0, m_1c4;               // +0x1c0 (8)
    i32 m_1c8, m_1cc;               // +0x1c8 (8)
    i32 m_1d0, m_1d4, m_1d8, m_1dc; // +0x1d0
    CObject* m_1e0[7];              // +0x1e0..+0x1f8  name refs (CMapStringToOb entries)
    CProjTypeObj* m_1fc;            // +0x1fc  type-5 latch
    i32 m_200;                      // +0x200
    CRezList m_204;                 // +0x204  AddTail target
    CProjNode* m_208;               // +0x208  write-path node list
    i32 _20c;
    i32 m_210; // +0x210
    i32 _214, _218, _21c;
    i32 m_220, m_224; // +0x220, +0x224
};

// @early-stop
// scratch-slot scheduling tail (same family as CTriggerLoadRec/CEventLoadRec/
// CGruntStateRec): the dual-mode switch, every Read/Write field+size, the 7-entry
// name-ref loop, the type-code-gated CMapPtrToPtr lookup, the g_freeList splice +
// AddTail, the inline strlen/strcpy KeyOfValue temps, the g_serialCounter bumps, the
// base tail-chain and the embedded +0x150 sub-record are byte-faithful; residual is
// the MSVC5 scratch-buffer slot assignment + outparam zero-init store positions. Not
// source-steerable.
RVA(0x000e0d40, 0x6c2)
i32 CProjLoadRec::Load(CSerialArchive* s, i32 mode, i32 a2, CSerialObj* a3) {
    CProjRegSub30* reg = (CProjRegSub30*)(void*)g_gameReg->m_world;
    if (reg == 0) {
        return 0;
    }

    char buf[0x80];

    switch (mode) {
        case 7: {
            m_200 = 0;
            s->Read(&m_170, 4);
            s->Read(&m_174, 4);
            s->Read(&m_178, 4);
            s->Read(&m_17c, 4);
            s->Read(&m_180, 4);
            s->Read(&m_188, 8);
            s->Read(&m_190, 4);
            s->Read(&m_198, 8);
            s->Read(&m_1a0, 8);
            s->Read(&m_1a8, 8);
            s->Read(&m_1b0, 8);
            s->Read(&m_1b8, 8);
            s->Read(&m_1c0, 8);
            s->Read(&m_1c8, 8);
            s->Read(&m_1d0, 4);
            s->Read(&m_1d4, 4);
            s->Read(&m_1d8, 4);
            s->Read(&m_1dc, 4);
            s->Read(&m_220, 4);
            s->Read(&m_224, 4);

            for (i32 ni = 0; ni < 7; ni++) {
                g_serialCounter++;
                s->Read(buf, 0x80);
                if (strlen(buf) != 0) {
                    CObject* out = 0;
                    reg->m_2c->m_10.Lookup(buf, out);
                    m_1e0[ni] = out;
                } else {
                    m_1e0[ni] = 0;
                }
            }

            g_serialCounter++;
            i32 key;
            s->Read(&key, 4);
            void* found = 0;
            i32 r;
            if (reg->m_8->m_48.Lookup((void*)key, found) == 0) {
                r = 0;
            } else if (found == 0) {
                r = 0;
            } else {
                r = (((CProjTypeObj*)found)->CallGetTypeCode() == 5) ? (i32)found : 0;
            }
            m_1fc = (CProjTypeObj*)r;
            if (m_1fc == 0 && key != 0) {
                return 0;
            }

            i32 cnt;
            s->Read(&cnt, 4);
            for (i32 ci = 0; ci < cnt; ci++) {
                CProjNode* node = (CProjNode*)g_freeList;
                void* payload = 0;
                if (node->m_next != 0) {
                    g_freeList = node->m_next;
                    payload = &node->m_04;
                }
                s->Read(payload, 8);
                m_204.AddTail((CRezListNode*)payload);
            }
            break;
        }

        case 4: {
            s->Write(&m_170, 4);
            s->Write(&m_174, 4);
            s->Write(&m_178, 4);
            s->Write(&m_17c, 4);
            s->Write(&m_180, 4);
            s->Write(&m_188, 8);
            s->Write(&m_190, 4);
            s->Write(&m_198, 8);
            s->Write(&m_1a0, 8);
            s->Write(&m_1a8, 8);
            s->Write(&m_1b0, 8);
            s->Write(&m_1b8, 8);
            s->Write(&m_1c0, 8);
            s->Write(&m_1c8, 8);
            s->Write(&m_1d0, 4);
            s->Write(&m_1d4, 4);
            s->Write(&m_1d8, 4);
            s->Write(&m_1dc, 4);
            s->Write(&m_220, 4);
            s->Write(&m_224, 4);

            for (i32 wi = 0; wi < 7; wi++) {
                g_serialCounter++;
                memset(buf, 0, sizeof(buf));
                if (m_1e0[wi] != 0) {
                    CString nm = reg->m_2c->KeyOfValue_152d30(m_1e0[wi]);
                    strcpy(buf, nm);
                }
                s->Write(buf, 0x80);
            }

            g_serialCounter++;
            i32 v = 0;
            if (m_1fc != 0) {
                v = m_1fc->m_188;
            }
            s->Write(&v, 4);

            i32 v2 = m_210;
            s->Write(&v2, 4);

            for (CProjNode* n = m_208; n != 0; n = n->m_next) {
                s->Write(n->m_08, 8);
            }
            break;
        }
    }

    if (ChainLoad(s, mode, a2, a3) == 0) {
        return 0;
    }
    if (s == 0) {
        return 0;
    }

    if (mode == 4) {
        char blob[0x80];
        memset(blob, 0, sizeof(blob));
        if (m_15c != 0) {
            CString nm = m_158->m_0c->m_2c->KeyOfValue_152d30(m_15c);
            strcpy(blob, nm);
        }
        s->Write(blob, 0x80);
        s->Write(&m_160, 0x10);
        return 1;
    }
    if (mode != 7) {
        return 1;
    }

    s->Read(buf, 0x80);
    s->Read(&m_160, 0x10);
    m_150 = a3;
    m_154 = a3;
    m_158 = a3->m_7c;
    if (strlen(buf) == 0) {
        m_15c = 0;
        return 1;
    }
    CObject* out = 0;
    m_158->m_0c->m_2c->m_10.Lookup(buf, out);
    m_15c = out;
    return 1;
}
SIZE_UNKNOWN(CArchiveDefaultSub);
SIZE_UNKNOWN(CEventLoadRec);
SIZE_UNKNOWN(CGruntStateRec);
SIZE_UNKNOWN(CProjLoadRec);
SIZE_UNKNOWN(CProjNode);
SIZE_UNKNOWN(CProjObjReg);
SIZE_UNKNOWN(CProjTypeObj);
SIZE_UNKNOWN(CRegNameMap);
SIZE_UNKNOWN(CRegNameTable);
SIZE_UNKNOWN(CRegSub30);
SIZE_UNKNOWN(CRegTypeTable);
SIZE_UNKNOWN(CTriggerLoadRec);

// --- vtable catalog ---
