// ProjLoadRec.cpp - CProjLoadRec::Load (0x0e0d40), carved out of the conflated
// StreamRecordLoaders.cpp (operation REHOME, package D8). A CProjectile/CTimeBomb-
// family dual-mode record loader; its retail .text sits at 0x0e0d40 - far from the
// CEventLoadRec main block (0x09c650) - a separate obj. Interleaver home (RVA-
// neighbour caller unit): src/Gruntz/Projectile.cpp (the projectile family owns
// g_coordPool.m_freeHead + g_gameReg); homing there is deferred (cross-TU class decl).
//
// Field names are placeholders; only the field offsets + code bytes are load-bearing.
#include <rva.h>
#include <Rez/RezList.h>         // CRezList / CRezListNode (CObList::AddTail @0x1b4991)
#include <Gruntz/SerialObjRef.h> // CSerialArchive, CDDrawSubMgrLeaf (KeyOfValue), CSerialObj
#include <Gruntz/GameRegistry.h> // CGameRegistry (g_gameReg->m_world)
#include <string.h>              // inline strlen / strcpy over the scratch buffer

// The game registry singleton (0x64556c). Reloc-masked DIR32 (cplay owns the def).
DATA(0x0024556c)
extern "C" CGameRegistry* g_gameReg;

// The serialize sequence counter (0x629ad0, ?g_serialCounter@@3HA): bumped once per
// string field read.
DATA(0x00229ad0)
extern i32 g_serialCounter;

// The g_coordPool.m_freeHead node allocator (?g_coordPool.m_freeHead@@3PAXA): the head is a node whose first
// dword is the next pointer; a non-empty pop advances the head and yields node+4.
#include <Gruntz/FreeNodePool.h> // the coord-node pool object @0x645540
// The pool's INTERIOR FIELDS - m_freeHead (+0x04) and m_linkOffset (+0x0c) - used to be
// declared here as the standalone globals g_coordPool.m_freeHead / g_coordPool.m_linkOffset. They are not
// globals: they are fields of g_coordPool (DEFINED in src/Gruntz/GameText.cpp), which is
// why the free-list push/pop code reads exactly [pool+4] and [pool+0xc].
extern FreeNodePool g_coordPool;

// ===========================================================================
// CProjLoadRec::Load (0x0e0d40) - a CProjectile/CTimeBomb-family dual-mode record
// loader. A __thiscall(reader, mode, a2, a3), ret 0x10, bailing (0) when the
// registry sub-object (g_gameReg->m_world) is absent. Mode 7 = READ: a fixed run
// of raw fields, a 7-entry name-ref loop (CMapStringToOb::Lookup @0x1b8438 through
// reg->m_2c->m_10), a single CMapPtrToPtr::Lookup @0x1b8760 (through reg->m_8->m_48)
// gated on the looked-up object's type code (virtual +0x20 == 5), then a g_coordPool.m_freeHead
// node-splice loop appending 8-byte payloads onto m_204 (CObList::AddTail @0x1b4991).
// Mode 4 = WRITE: re-derives each ref's name via reg->m_2c->KeyOfValue_152d30 and
// writes it back. Either way it tail-chains the base loader (0x16f4a0), then runs an
// embedded CSerialObjRef record at +0x150 (read/write a key name + 0x10 blob, resolve
// through a3->m_7c->m_c->m_2c). Names are placeholders; offsets + bytes load-bearing.
// ===========================================================================

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
// name-ref loop, the type-code-gated CMapPtrToPtr lookup, the g_coordPool.m_freeHead splice +
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
                CProjNode* node = (CProjNode*)g_coordPool.m_freeHead;
                void* payload = 0;
                if (node->m_next != 0) {
                    g_coordPool.m_freeHead = node->m_next;
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
SIZE_UNKNOWN(CProjLoadRec);
SIZE_UNKNOWN(CProjNode);
SIZE_UNKNOWN(CProjObjReg);
SIZE_UNKNOWN(CProjTypeObj);
