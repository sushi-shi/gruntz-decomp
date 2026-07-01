#include <rva.h>
#include <Mfc.h>
#include <Ints.h>
// SBI_ImageSet.cpp - Gruntz CSBI_ImageSet (C:\Proj\Gruntz), the frameless methods.
// RTTI .?AVCSBI_ImageSet@@; the most-derived of the SBI image chain
//   CSBI_ImageSet : CSBI_Image : CSBI_RectOnly : CStatusBarItem.
// Vtable @0x5eac4c. The /GX-framed scalar destructor (0x102000) lives in
// SBI_RectOnlyEh.cpp under its true name. Sibling/engine callees are
// ILT/vtable-reloc-masked.

// ---------------------------------------------------------------------------
// Shared engine views (modeled minimally; only touched members/methods are
// load-bearing; every call through them is reloc-masked).

// The serialization stream (arg1): a real polymorphic object with ReadBytes at
// vtable slot 0x2c (index 0xb) and WriteBytes at slot 0x30 (index 0xc), both
// __thiscall (buf, len). Modeled as virtuals so the call lowers to
// `mov edx,[ebx]; call [edx+0x2c|0x30]` with ecx=this (no explicit-self push).
struct CImageSetStream {
    virtual void Vf0();
    virtual void Vf1();
    virtual void Vf2();
    virtual void Vf3();
    virtual void Vf4();
    virtual void Vf5();
    virtual void Vf6();
    virtual void Vf7();
    virtual void Vf8();
    virtual void Vf9();
    virtual void Vfa();
    virtual void ReadBytes(void* buf, i32 len);  // slot 0x2c
    virtual void WriteBytes(void* buf, i32 len); // slot 0x30
};

// The resolved config record (Lookup result): its config name string lives at +0x24.
struct CImageSetCfgRec {
    char m_pad0[0x24];
    char m_name[1]; // +0x24  config name (null-terminated)
};
SIZE_UNKNOWN(CImageSetCfgRec);

// CMapStringToPtr::Lookup-style map (0x1b8008, __thiscall, ret 8) embedded at the
// config-host object's +0x10 (the same map shape SetupImage uses).
struct CImageSetCfgMap {
    i32 Lookup(char* key, CImageSetCfgRec** out); // 0x1b8008
};
SIZE_UNKNOWN(CImageSetCfgMap);
struct CImageSetCfgHost {
    char m_pad0[0x10];
    CImageSetCfgMap m_10; // +0x10  embedded map at +0x10
};
SIZE_UNKNOWN(CImageSetCfgHost);
struct CImageSetRegSub {
    char m_pad0[0x10];
    CImageSetCfgHost* m_10; // +0x10  config host (map embedded at its +0x10)
};
SIZE_UNKNOWN(CImageSetRegSub);

// g_gameReg->m_30 = the active registry/game-manager carrying the config map sub.
struct CImageSetGameReg {
    char m_pad0[0x30];
    CImageSetRegSub* m_30; // +0x30  registry sub-object
};
SIZE_UNKNOWN(CImageSetGameReg);
DATA(0x0024556c)
extern CImageSetGameReg* g_gameReg;

// The serialize-sequence counter bumped once per non-trivial pass.
DATA(0x00229ad0)
extern i32 g_serialCounter;

// ---------------------------------------------------------------------------
// CSBI_ImageSet - adds the slot-1 serialize override (save/load of the config id
// + name) on top of CSBI_Image. Fields are placeholders; offsets are load-bearing.
class CSBI_ImageSet {
public:
    i32 Serialize(CImageSetStream* s, i32 mode, i32 a3, i32 a4);     // vslot 1 (0xe74f0)
    i32 BaseSerialize(CImageSetStream* s, i32 mode, i32 a3, i32 a4); // 0xe6e40 base slot 1

    char m_pad0[0x34];
    CImageSetCfgRec* m_34; // +0x34  resolved config record
    i32 m_38;              // +0x38  serialized config id (4 bytes)
};

// vtable slot 1 (0xe74f0): serialize the config id + name. mode 7 = load (read id,
// read name, resolve record), mode 4 = save (write id, write name from the record);
// either way chain to the base serialize and return its normalized truth.
// @early-stop
// 99.2% (entropy tail): logic + control flow + inline strcpy/strlen + the typed
// vtable (Read/WriteBytes) + the switch dispatch are all byte-exact. The only
// residual is ONE extra `mov [esp+0x18],eax` (retail keeps the dead strlen result
// live before Lookup) + the consequent 1-byte branch-displacement shifts, plus the
// reloc-masked Lookup/Read/WriteBytes/BaseSerialize/g_* operands. Naming the strlen
// result to recover the dead store regresses it (98.4%) - a non-steerable /O2
// dead-store artifact (docs/patterns/reloc-typing-vptr-global.md). Effectively done.
RVA(0x000e74f0, 0x152)
i32 CSBI_ImageSet::Serialize(CImageSetStream* s, i32 mode, i32 a3, i32 a4) {
    if (s == 0) {
        return 0;
    }
    CImageSetRegSub* reg = g_gameReg->m_30;
    if (reg == 0) {
        return 0;
    }
    char buf[0x80];
    switch (mode) {
        case 7:
            s->ReadBytes(&m_38, 4);
            g_serialCounter++;
            s->ReadBytes(buf, 0x80);
            if (strlen(buf)) {
                CImageSetCfgRec* out;
                reg->m_10->m_10.Lookup(buf, &out);
                m_34 = out;
            } else {
                m_34 = 0;
            }
            break;
        case 4:
            s->WriteBytes(&m_38, 4);
            g_serialCounter++;
            memset(buf, 0, 0x80);
            if (m_34) {
                strcpy(buf, m_34->m_name);
            }
            s->WriteBytes(buf, 0x80);
            break;
    }
    return BaseSerialize(s, mode, a3, a4) != 0;
}
