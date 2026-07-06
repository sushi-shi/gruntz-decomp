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
extern "C" CGameRegistry* g_mgrSettings;

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
    i32 Load(CSerialArchive* s);

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
i32 CTriggerLoadRec::Load(CSerialArchive* s) {
    if (s == 0) {
        return 0;
    }
    CGameRegistry* gr = g_mgrSettings;
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
// ret 4. Note: unlike CTriggerLoadRec it does NOT null-check g_mgrSettings, only
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
    CRegSub30* reg = (CRegSub30*)g_mgrSettings->m_world;
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

// ===========================================================================
// The CArchive-store / CString-default flavor of the record serializer: a larger
// record streamed through the shared CSerialArchive's +0x30 slot (Write, the store
// entry - not the +0x2c load slot the sibling loaders use), whose string fields take
// a CString/default-buffer copy rather than the name-registry Lookup. `this` is held
// in ebp; the archive in ebx. `s->Write(buf,n)` lowers to `mov edx,[ebx]; call
// [edx+0x30]`. __thiscall, ret 4; returns 0 when the archive or the bound manager
// (m_c) is absent, else 1. NB the CArchiveLoadRec::Load method name is the recovered-
// symbol placeholder; only the +0x30 slot offset is load-bearing.
// ===========================================================================

// The bound manager (this->m_c): the string-default helper at 0x155630 is a
// __thiscall on its +0x10 sub-object, taking (obj, scratch, &outInt).
struct CArchiveMgr {
    char m_pad00[0x10];
    CDDrawWorkerRegistry* m_10; // +0x10
    char m_pad14[0x24 - 0x14];  //
    char m_name24[0x80 - 0x24]; // +0x24  inline name string (default-copied into scratch)
};

// The +0x188 default-int object one raw field seeds from (m_4e4 below).
struct CArchiveDefInt;

// The global default sink one raw field reads into (DAT_00612618).

// One outer entry of the m_3a8 nested-array block: {void** base; i32 count}.
struct CArchiveSubArray {
    void** m_base; // +0x00
    i32 m_count;   // +0x04
    char m_pad08[0x14 - 0x08];
};

struct CArchiveLoadRec {
    i32 Load(CSerialArchive* s);

    char m_pad00[0x0c];
    CArchiveMgr* m_c; // +0x0c  bound manager (null -> bail)
    char m_pad10[0x1bc - 0x10];
    i32 m_1bc; // +0x1bc
    i32 m_1c0; // +0x1c0
    i32 m_1c4; // +0x1c4
    char m_pad1c8[0x1cc - 0x1c8];
    i32 m_1cc; // +0x1cc
    char m_pad1d0[0x2d8 - 0x1d0];
    i32 m_2d8; // +0x2d8
    char m_pad2dc[0x2ec - 0x2dc];
    i32 m_2ec;        // +0x2ec
    i32 m_2f0;        // +0x2f0
    i32 m_2f4;        // +0x2f4
    i32 m_2f8;        // +0x2f8
    i32 m_2fc, m_300; // +0x2fc  (8 bytes)
    char m_pad304[0x360 - 0x304];
    i32 m_360, m_364;   // +0x360  (8 bytes)
    i32 m_368;          // +0x368
    i32 m_36c;          // +0x36c
    i32 m_370;          // +0x370
    void** m_elemArray; // +0x374  element-ptr array (paired count m_elemCount)
    i32 m_elemCount;    // +0x378  element count
    char m_pad37c[0x384 - 0x37c];
    char m_384[0x3a8 - 0x384]; // +0x384  4 fixed 8-byte entries
    CArchiveSubArray m_3a8[4]; // +0x3a8  4 nested {base,count} sub-arrays
    char m_pad3f8[0x408 - 0x3f8];
    i32 m_408;                             // +0x408
    i32 m_40c;                             // +0x40c
    char* m_defaultStr;                    // +0x410  default string (copied into the scratch)
    i32 m_414, m_418, m_41c, m_420, m_424; // +0x414..+0x424
    i16 m_428;                             // +0x428  (2 bytes)
    char m_pad42a[0x470 - 0x42a];
    i32 m_470, m_474, m_478, m_47c, m_480, m_484; // +0x470..+0x484
    char m_pad488[0x48c - 0x488];
    void** m_tailArray; // +0x48c  trailing element-ptr array (paired count m_tailCount)
    i32 m_tailCount;    // +0x490  trailing element count
    char m_pad494[0x49c - 0x494];
    i32 m_49c; // +0x49c
    char m_pad4a0[0x4b0 - 0x4a0];
    i32 m_4b0; // +0x4b0
    char m_pad4b4[0x4cc - 0x4b4];
    CArchiveMgr* m_4cc;        // +0x4cc  object whose +0x24 is an inline name string
    void* m_4d0;               // +0x4d0  object passed to the default helper
    i32 m_4d4;                 // +0x4d4
    i32 m_4d8, m_4dc, m_4e0;   // +0x4d8..+0x4e0
    CArchiveDefInt* m_4e4_obj; // +0x4e4  object whose +0x188 seeds a default int
    i32 m_4e8, m_4ec, m_4f0, m_4f4, m_4f8, m_4fc, m_500, m_504; // +0x4e8..+0x504
    char m_pad508[0x514 - 0x508];
    i32 m_514; // +0x514
};

// The +0x188 default-int field on the m_4e4 object.
struct CArchiveDefInt {
    char m_pad00[0x188];
    i32 m_188; // +0x188
};

// @early-stop
// scratch-slot scheduling tail (~99.8%): every serialize field/size, the two unsigned
// count loops, the nested sub-array loop, the inline strlen/strcpy default copies,
// the g_serialCounter bumps, the conditional default-helper call and the final
// signed element loop are byte-faithful. The sole residual is the MSVC5 scheduler
// parking one extra scratch slot (frame 0x294 vs retail 0x28c) + a few zero-init
// store positions - the entropy tail the CTriggerLoadRec/CEventLoadRec siblings
// share; not source-steerable.
RVA(0x000d79d0, 0x537)
i32 CArchiveLoadRec::Load(CSerialArchive* s) {
    if (s == 0) {
        return 0;
    }
    CArchiveMgr* mc = m_c;
    if (mc == 0) {
        return 0;
    }

    s->Write(&m_1bc, 4);
    s->Write(&m_1c0, 4);
    s->Write(&m_1cc, 4);
    s->Write(&m_2d8, 4);
    s->Write(&m_2ec, 4);
    s->Write(&m_2f0, 4);
    s->Write(&m_2f4, 4);
    s->Write(&m_2f8, 4);
    s->Write(&m_2fc, 8);
    s->Write(&m_360, 8);
    s->Write(&m_368, 4);
    s->Write(&m_36c, 4);

    i32 c0 = m_elemCount;
    s->Write(&c0, 4);
    for (u32 i0 = 0; i0 < (u32)c0; i0++) {
        s->Write(m_elemArray[i0], 8);
    }

    char* p = m_384;
    for (i32 k0 = 4; k0 != 0; k0--) {
        s->Write(p, 8);
        p += 8;
    }

    CArchiveSubArray* e = m_3a8;
    for (i32 k1 = 4; k1 != 0; k1--) {
        i32 cnt = e->m_count;
        s->Write(&cnt, 4);
        for (u32 i1 = 0; i1 < (u32)cnt; i1++) {
            s->Write(e->m_base[i1], 8);
        }
        e++;
    }

    s->Write(&m_408, 4);

    g_serialCounter++;
    {
        char buf[0x200];
        memset(buf, 0, sizeof(buf));
        strcpy(buf, m_defaultStr);
        s->Write(buf, 0x200);
    }

    s->Write(&m_40c, 4);
    s->Write(&g_archiveDefault612618, 4);

    g_serialCounter++;
    {
        char buf[0x80];
        memset(buf, 0, sizeof(buf));
        i32 v = 0;
        if (m_4d0 != 0) {
            mc->m_10->AnyValueMatches_155630((i32)m_4d0, (i32)buf, (i32)&v);
        }
        s->Write(buf, 0x80);
        s->Write(&v, 4);
    }

    g_serialCounter++;
    {
        char buf[0x80];
        memset(buf, 0, sizeof(buf));
        if (m_4cc != 0) {
            strcpy(buf, m_4cc->m_name24);
        }
        s->Write(buf, 0x80);
    }

    s->Write(&m_4d8, 4);
    s->Write(&m_4dc, 4);
    s->Write(&m_4e0, 4);

    g_serialCounter++;
    {
        i32 v = 0;
        if (m_4e4_obj != 0) {
            v = m_4e4_obj->m_188;
        }
        s->Write(&v, 4);
    }

    s->Write(&m_4e8, 4);
    s->Write(&m_4ec, 4);
    s->Write(&m_4f4, 4);
    s->Write(&m_1c4, 4);
    s->Write(&m_484, 4);
    s->Write(&m_4f8, 4);
    s->Write(&m_4fc, 4);
    s->Write(&m_500, 4);
    s->Write(&m_4f0, 4);
    s->Write(&m_504, 4);
    s->Write(&m_414, 4);
    s->Write(&m_418, 4);
    s->Write(&m_41c, 4);
    s->Write(&m_420, 4);
    s->Write(&m_424, 4);
    s->Write(&m_428, 2);
    s->Write(&m_470, 4);
    s->Write(&m_474, 4);
    s->Write(&m_478, 4);
    s->Write(&m_47c, 4);
    s->Write(&m_480, 4);
    s->Write(&m_4b0, 4);
    s->Write(&m_4d4, 4);
    s->Write(&m_49c, 4);
    s->Write(&m_514, 4);

    i32 c1 = m_tailCount;
    s->Write(&c1, 4);
    for (i32 fi = 0; fi < m_tailCount; fi++) {
        void* el = m_tailArray[fi];
        if (el != 0) {
            s->Write(el, 8);
        }
    }

    return 1;
}

// ===========================================================================
// CGruntStateRec::Load (0x0ea990) - the dual-mode grunt-state record loader. A
// __thiscall taking (reader, mode, a2, a3), ret 0x10. Bails (0) when the reader
// or the registry sub-object (g_mgrSettings->m_world) is absent. Mode 7 loads the
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
    CRegSub30* reg = (CRegSub30*)g_mgrSettings->m_world;
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
// registry sub-object (g_mgrSettings->m_world) is absent. Mode 7 = READ: a fixed run
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

// g_mgrSettings->m_world (the game registry's +0x30 sub-registry) viewed by this
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
    CProjRegSub30* reg = (CProjRegSub30*)(void*)g_mgrSettings->m_world;
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
SIZE_UNKNOWN(CArchiveDefInt);
SIZE_UNKNOWN(CArchiveDefaultSub);
SIZE_UNKNOWN(CArchiveLoadRec);
SIZE_UNKNOWN(CArchiveMgr);
SIZE_UNKNOWN(CArchiveSubArray);
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
VTBL(CProjTypeObj, 0x001e8cb4);
