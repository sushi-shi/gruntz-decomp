// StreamRecordLoaders.cpp - the WAP "binary record load" idiom cluster. Several
// unrelated game classes deserialize a fixed field sequence from a stream-reader
// object (a virtual Read at vtable +0x2c), with string-valued fields read into a
// scratch buffer and interned against the game registry's name->object map
// (g_gameReg->m_30->m_10 + 0x10, CMapStringToPtr::Lookup @0x1b8008). A global
// sequence counter (g_serialCounter) ticks once per string read. Each Load is a
// __thiscall taking the reader (ret 4); names are placeholders, only the field
// offsets + code bytes are load-bearing.
//
// These records belong to distinct classes (the trigger system, the event
// handler, the grunt-tuning loader, ...) found by caller-tracing each through its
// ILT thunk; the shared reader/registry models below are the load-bearing shape.
#include <rva.h>
#include <string.h> // inline strlen (repne scasb) over the scratch buffer

// ---------------------------------------------------------------------------
// CStreamReader - the input stream the records read from. Only the +0x2c virtual
// (Read(buf, count)) is touched; modeled polymorphically so `mov edx,[esi]; call
// [edx+0x2c]` falls out. Never instantiated here, so no vtable is emitted.
// ---------------------------------------------------------------------------
class CStreamReader {
public:
    virtual void s00();
    virtual void s04();
    virtual void s08();
    virtual void s0c();
    virtual void s10();
    virtual void s14();
    virtual void s18();
    virtual void s1c();
    virtual void s20();
    virtual void s24();
    virtual void s28();
    virtual void Read(void* buf, i32 count); // +0x2c
};

// ---------------------------------------------------------------------------
// The game registry's string->object name map (CMapStringToPtr::Lookup @0x1b8008,
// __thiscall): Lookup(key, &out) writes the found object (or leaves out untouched).
// Reached as g_gameReg->m_30->m_10 + 0x10.
// ---------------------------------------------------------------------------
struct CRegNameMap {
    i32 Lookup(char* key, void** out); // 0x1b8008
};
struct CRegNameTable {
    char m_pad00[0x10];
    CRegNameMap m_10map; // +0x10  the embedded name map
};
struct CRegSub30 {
    char m_pad00[0x10];
    CRegNameTable* m_10; // +0x10  the name table
};
struct WwdGameReg {
    char m_pad00[0x30];
    CRegSub30* m_30; // +0x30  the name-table sub-registry
};

// The looked-up "type table" value an indexed field resolves through: a bounded
// array (m_14[m_64 .. m_68]) whose element at the read index becomes the field.
struct CRegTypeTable {
    char m_pad00[0x14];
    void** m_14; // +0x14  element array
    char m_pad18[0x64 - 0x18];
    i32 m_64; // +0x64  lower bound (inclusive)
    i32 m_68; // +0x68  upper bound (inclusive)
};

// The game registry singleton (0x64556c). The delinker's canonical symbol is the
// extern "C" _g_mgrSettings (the cplay unit owns it); reloc-masked DIR32.
DATA(0x0024556c)
extern "C" WwdGameReg* g_mgrSettings;

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
    i32 Load(CStreamReader* s);

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
i32 CTriggerLoadRec::Load(CStreamReader* s) {
    if (s == 0) {
        return 0;
    }
    WwdGameReg* gr = g_mgrSettings;
    if (gr == 0) {
        return 0;
    }
    CRegSub30* reg = gr->m_30;
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
        reg->m_10->m_10map.Lookup(buf, &out);
        m_30 = out;
    } else {
        m_30 = 0;
    }

    g_serialCounter++;
    s->Read(buf, 0x80);
    if (strlen(buf) != 0) {
        out = 0;
        reg->m_10->m_10map.Lookup(buf, &out);
        m_34 = out;
    } else {
        m_34 = 0;
    }

    g_serialCounter++;
    s->Read(buf, 0x80);
    if (strlen(buf) != 0) {
        out = 0;
        reg->m_10->m_10map.Lookup(buf, &out);
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
        reg->m_10->m_10map.Lookup(buf, &out);
        CRegTypeTable* tt = (CRegTypeTable*)out;
        void* r;
        if (tt != 0 && i >= tt->m_64 && i <= tt->m_68) {
            r = tt->m_14[i];
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
        reg->m_10->m_10map.Lookup(buf, &out);
        CRegTypeTable* tt = (CRegTypeTable*)out;
        void* r;
        if (tt != 0 && i >= tt->m_64 && i <= tt->m_68) {
            r = tt->m_14[i];
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
        reg->m_10->m_10map.Lookup(buf, &out);
        CRegTypeTable* tt = (CRegTypeTable*)out;
        void* r;
        if (tt != 0 && i >= tt->m_64 && i <= tt->m_68) {
            r = tt->m_14[i];
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
    i32 Load(CStreamReader* s);

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
i32 CEventLoadRec::Load(CStreamReader* s) {
    if (s == 0) {
        return 0;
    }
    CRegSub30* reg = g_mgrSettings->m_30;
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
        reg->m_10->m_10map.Lookup(buf, &out);
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
        reg->m_10->m_10map.Lookup(buf, &out);
        CRegTypeTable* tt = (CRegTypeTable*)out;
        void* r;
        if (tt != 0 && i >= tt->m_64 && i <= tt->m_68) {
            r = tt->m_14[i];
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
        reg->m_10->m_10map.Lookup(buf, &out);
        CRegTypeTable* tt = (CRegTypeTable*)out;
        void* r;
        if (tt != 0 && i >= tt->m_64 && i <= tt->m_68) {
            r = tt->m_14[i];
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
        reg->m_10->m_10map.Lookup(buf, &out);
        CRegTypeTable* tt = (CRegTypeTable*)out;
        void* r;
        if (tt != 0 && i >= tt->m_64 && i <= tt->m_68) {
            r = tt->m_14[i];
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
        reg->m_10->m_10map.Lookup(buf, &out);
        CRegTypeTable* tt = (CRegTypeTable*)out;
        void* r;
        if (tt != 0 && i >= tt->m_64 && i <= tt->m_68) {
            r = tt->m_14[i];
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
        reg->m_10->m_10map.Lookup(buf, &out);
        CRegTypeTable* tt = (CRegTypeTable*)out;
        void* r;
        if (tt != 0 && i >= tt->m_64 && i <= tt->m_68) {
            r = tt->m_14[i];
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
