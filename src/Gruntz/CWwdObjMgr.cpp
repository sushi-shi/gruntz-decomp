// CWwdObjMgr.cpp - the WWD level-object loader (C:\Proj\Gruntz). One trace method:
//
//   0x15ad30  LoadObjects - iterate the level reader's `count` object descriptors,
//                           dedup each against the active-set map (+0x48), publish
//                           its id to g_wwdObjIdCounter, and dispatch by the object
//                           kind (5 / 0x16 / 0x1b / 0x1c) to the matching CWwdObjMgr
//                           factory; then build + link each object's child record.
//
// CWwdObjMgr is the 0x159xxx-0x15bxxx collection class; its richer view + the
// 0x15abxx list/map methods + CreateObject_159600 live in src/Gruntz/CDDrawSubMgr.cpp.
// This is a separate TU carrying only the minimal view LoadObjects needs. The
// per-kind factories (159440/159600/1598d0/159830), the level-file helpers
// (BuildChild 0x156a90 / Resolve 0x1b8008) and the reader's slot-11 read are all
// external (no body) so their call rel32 / vtable displacements reloc-mask.
// Field names are placeholders; only OFFSETS + emitted bytes are load-bearing.
#include <rva.h>

#include <Gruntz/CWwdObjMgr.h> // the shared object-collection manager class
#include <Mfc.h> // CPtrList, CMapPtrToPtr (real afxcoll, for the m_10/m_2c/m_48 layout)
#include <Globals.h>

// The running WWD object-id counter (?g_wwdObjIdCounter@@3HA @ 0x61ab14; the DATA
// label is owned by CDDrawSubMgr.cpp). LoadObjects publishes each loaded object's
// id here across the factory call, then restores it.

// The per-object descriptor the reader fills (0xa0 bytes). +0x04 is the dedup id,
// +0x08 the kind selector, +0x14 the object's name string (the resolver key).
struct WwdObjDesc {
    i32 m_00;               // +0x00  passed to the factory
    i32 m_04;               // +0x04  dedup key / object id
    i32 m_08;               // +0x08  kind selector
    i32 m_0c;               // +0x0c  ARM-0x1c child tag
    i32 m_10;               // +0x10  merge child-build selector
    char m_14[0x94 - 0x14]; // +0x14  name string (resolver key)
    i32 m_94;               // +0x94
    i32 m_98;               // +0x98
    i32 m_9c;               // +0x9c
};

// The level reader (arg1): a polymorphic stream whose +0x2c slot (index 11) reads
// the next object descriptor into a caller buffer. Modeled with the 12 virtuals so
// the slot-11 call lowers to `mov eax,[obj]; call [eax+0x2c]` (no body -> reloc-mask).
struct WwdReader {
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6();
    virtual void v7();
    virtual void v8();
    virtual void v9();
    virtual void v10();
    virtual void ReadDesc(void* buf, i32 size); // index 11 (+0x2c)
};

// The string-resolve map reached at (m_0c->m_14 + 0x10): Resolve(nameBuf, &out)
// looks the descriptor name up to a value. __thiscall, no body -> reloc-mask.
struct WwdStrResolve {
    void Resolve(const void* nameBuf, void** out); // 0x1b8008
};

// The level-file object (this->m_0c): m_14 fronts the string-resolve map (+0x10);
// BuildChild spawns a sub-record for the object. External, reloc-masked.
struct WwdFile {
    char m_pad0[0x14];
    char* m_14;                                                      // +0x14
    i32 BuildChild(void* reader, i32 tag, i32 selector, void** out); // 0x156a90
};

// The created game object: +0x7c is its aux record, whose +0x18 receives the child
// built in the merge tail.
struct WwdGameObjAux {
    char m_pad0[0x18];
    void* m_18; // +0x18
};
class CWwdGameObject {
public:
    char m_pad0[0x7c];
    WwdGameObjAux* m_7c; // +0x7c
};

// CWwdObjMgr is the shared <Gruntz/CWwdObjMgr.h> class (the parent file handle at
// +0x0c and the active-set dedup map at +0x48 are what LoadObjects reads here).

// ===========================================================================
// CWwdObjMgr::LoadObjects @0x15ad30
// ===========================================================================
// @early-stop
// jump-table + foreign-chain plateau (>512 B): logic + the four kind arms
// (Resolve + the matching factory), the +0x48 dedup gate, the g_wwdObjIdCounter
// publish/restore, the merge createdObj/m_7c guards, and the child-build link are
// reconstructed in shape + order. Residual: the switch lowers to MSVC5's
// byte-index jump table (reloc-typed scoring artifact) and the heavy descriptor
// stack-buffer + null-register (ebp=0) regalloc across the arms is non-steerable
// under /O2; the reader/level-file chains are modeled by raw offset. Final sweep.
RVA(0x0015ad30, 0x2be)
i32 CWwdObjMgr::LoadObjects(WwdReader* reader, u32 count, i32 unused) {
    i32 savedCounter = 0;
    if (reader == 0) {
        return 0;
    }
    for (u32 i = 0; i < count; i++) {
        WwdObjDesc desc;
        reader->ReadDesc(&desc, 0xa0);

        void* found;
        if (m_48.Lookup((void*)desc.m_04, found) && found != 0) {
            return 0;
        }

        savedCounter = g_wwdObjIdCounter;
        g_wwdObjIdCounter = desc.m_04;

        CWwdGameObject* createdObj = 0;
        switch (desc.m_08) {
            case 5: {
                void* val;
                ((WwdStrResolve*)(m_0c->m_14 + 0x10))->Resolve(desc.m_14, &val);
                if (val != 0) {
                    createdObj = CreateObject_159600(
                        desc.m_00,
                        desc.m_94,
                        desc.m_98,
                        desc.m_9c,
                        (i32)val,
                        0
                    );
                }
                break;
            }
            case 0x16: {
                void* val;
                ((WwdStrResolve*)(m_0c->m_14 + 0x10))->Resolve(desc.m_14, &val);
                createdObj = CreateObject_159440(desc.m_00, desc.m_9c, (i32)val, 0);
                break;
            }
            case 0x1b: {
                void* val;
                ((WwdStrResolve*)(m_0c->m_14 + 0x10))->Resolve(desc.m_14, &val);
                if (val != 0) {
                    createdObj = CreateObject_1598d0(
                        desc.m_00,
                        desc.m_94,
                        desc.m_98,
                        desc.m_9c,
                        (i32)val,
                        0
                    );
                }
                break;
            }
            case 0x1c: {
                void* rec = 0;
                if (m_0c->BuildChild(reader, 0xa, desc.m_0c, &rec) == 0) {
                    return 0;
                }
                if (rec == 0) {
                    return 0;
                }
                *(i32*)((char*)rec + 4) = desc.m_00;
                if (Init_159830(rec, desc.m_94, desc.m_98, desc.m_9c, desc.m_14, 0) == 0) {
                    return 0;
                }
                createdObj = (CWwdGameObject*)rec;
                break;
            }
            default:
                break;
        }

        g_wwdObjIdCounter = savedCounter;
        if (createdObj == 0) {
            return 0;
        }
        if (createdObj->m_7c == 0) {
            return 0;
        }
        if (desc.m_10 != 0) {
            void* child = 0;
            if (m_0c->BuildChild(reader, 9, desc.m_10, &child) == 0) {
                return 0;
            }
            if (child == 0) {
                return 0;
            }
            createdObj->m_7c->m_18 = child;
        }
    }
    return 1;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CWwdGameObject);
SIZE_UNKNOWN(WwdGameObjAux);
SIZE_UNKNOWN(WwdObjDesc);
SIZE_UNKNOWN(WwdReader);
SIZE_UNKNOWN(WwdStrResolve);
