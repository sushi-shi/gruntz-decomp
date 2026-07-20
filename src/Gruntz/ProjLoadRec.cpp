// ProjLoadRec.cpp - CProjLoadRec::Load (0x0e0d40), carved out of the conflated
// StreamRecordLoaders.cpp (operation REHOME, package D8). A CProjectile/CTimeBomb-
// family dual-mode record loader; its retail .text sits at 0x0e0d40 - far from the
// CEventLoadRec main block (0x09c650) - a separate obj. Interleaver home (RVA-
// neighbour caller unit): src/Gruntz/Projectile.cpp (the projectile family owns
// g_coordPool.m_freeHead + g_gameReg); homing there is deferred (cross-TU class decl).
//
// Field names are placeholders; only the field offsets + code bytes are load-bearing.
#include <rva.h>
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/SerialCounter.h> // g_serialCounter
#include <Io/FileMem.h>           // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Rez/RezList.h>          // CRezList / CRezListNode (CPtrList::AddTail @0x1b4991)
#include <Gruntz/SerialArchive.h>     // CSerialArchive (the inherited CWapX::Chain arg)
#include <DDrawMgr/DDrawSubMgrLeaf.h> // the anim registry (m_10 Lookup / KeyOfValue; ex SerialObjRef.h pull)
#include <DDrawMgr/DDrawSurfaceMgr.h> // obj->m_0c world root (ex SerialObjRef.h pull)
#include <DDrawMgr/DDrawSurfaceMgr.h> // m_158->m_0c (the world root; m_animRegistry hop)
#include <Gruntz/GameRegistry.h>      // CGameRegistry (g_gameReg->m_world = CDDrawSurfaceMgr*)
#include <DDrawMgr/DDrawChildGroup.h> // CDDrawChildGroup (m_world->m_childGroup; m_map48 key->object map @+0x48)
#include <Gruntz/UserLogic.h> // CGameObject (the resolved object; GetTypeId [8] + m_188)
#include <Gruntz/ProjLoadRec.h> // canonical CProjLoadRec (proven CProjectile; fold deferred)
#include <string.h>           // inline strlen / strcpy over the scratch buffer

// The game registry singleton (0x64556c). Reloc-masked DIR32 (cplay owns the def).

// The serialize sequence counter (0x629ad0, ?g_serialCounter@@3HA): bumped once per
// string field read.

// The g_coordPool.m_freeHead node allocator (?g_coordPool.m_freeHead@@3PAXA): the head is a node whose first
// dword is the next pointer; a non-empty pop advances the head and yields node+4.
#include <Gruntz/FreeNodePool.h> // the coord-node pool object @0x645540
// The pool's INTERIOR FIELDS - m_freeHead (+0x04) and m_linkOffset (+0x0c) are
// fields of g_coordPool (DEFINED in src/Gruntz/GameText.cpp), which is
// why the free-list push/pop code reads exactly [pool+4] and [pool+0xc].

// ===========================================================================
// CProjLoadRec::Load (0x0e0d40) - a CProjectile/CTimeBomb-family dual-mode record
// loader. A __thiscall(reader, mode, a2, a3), ret 0x10, bailing (0) when the
// registry sub-object (g_gameReg->m_world) is absent. Mode 7 = READ: a fixed run
// of raw fields, a 7-entry name-ref loop (CMapStringToPtr::Lookup x1b8438 through
// reg->m_2c->m_10), a single CMapPtrToPtr::Lookup @0x1b8760 (through reg->m_8->m_48)
// gated on the looked-up object's type code (virtual +0x20 == 5), then a g_coordPool.m_freeHead
// node-splice loop appending 8-byte payloads onto m_204 (CPtrList::AddTail @0x1b4991).
// Mode 4 = WRITE: re-derives each ref's name via reg->m_2c->KeyOfValue_152d30 and
// writes it back. Either way it tail-chains the base loader (0x16f4a0), then runs an
// embedded CSerialObjRef record at +0x150 (read/write a key name + 0x10 blob, resolve
// through a3->m_7c->m_0c->m_animRegistry). Names are placeholders; offsets + bytes load-bearing.
// ===========================================================================

// g_gameReg->m_world IS the canonical CDDrawSurfaceMgr (<Gruntz/GameRegistry.h>):
// this loader reaches its projectile-object factory (m_8, a CDDrawChildGroup whose
// embedded key->object map m_map48 @+0x48 has Lookup @0x1b8760) and its name-leaf
// registry (m_animRegistry @+0x2c, the canonical CDDrawSubMgrLeaf serialize
// facet - m_10 name map + KeyOfValue_152d30). Both keyed values ARE created game
// objects (the same map CTriggerMgr::Load resolves, with the `slot-8 GetTypeId()==5`
// gate at +0x20), so the out-param is CGameObject*. (Were the .cpp-local CProjObjReg
// + CProjRegSub30 views.)

// One spliced freelist node is the canonical CoordPoolNode (read: link @+0, inline
// {x,y} payload @+4) / CoordNode (write list m_208: link @+0, Coord* payload @+8) -
// <Gruntz/CoordNode.h> / FreeNodePool.h. Was the .cpp-local CProjNode view.

// The +0x204 list the read path appends payloads to (CPtrList::AddTail @0x1b4991);
// sized to one pointer so the following fields keep their offsets.
// a3->m_7c->m_0c->m_animRegistry is the registry leaf; the canonical types give m_7c
// and m_0c, but the inlined +0x150 record reaches m_c (not m_0c) - the same shape at
// +0x0c. Reuse CGameObject for a3; view its name-holder's +0x0c through AnimWorkerObj.
// @identity-recovered: CProjLoadRec IS CProjectile, and Load @0x0e0d40 IS
// CProjectile::SerializeMove (vtable slot 1). PROOF: vtable_hierarchy --class
// CProjectile resolves slot 1 (ILT thunk 0x0034b3) to this RVA under the src name
// "Load"; the field layout mirrors CProjectile exactly (m_1e0[7] = the seven frame
// sprites, m_1fc = m_shadow, m_204 = m_hitList, m_220/m_224 = m_targetId/m_ownerId,
// and the +0x150 sub-record `m_150=a3; m_154=a3; m_158=a3->m_7c` restores the same
// owner/sprite/m_7c triple the ctor sets). DEFERRED-FOLD: 0x0e0d40 lives in
// Projectile.cpp's RVA band (0xdec60..0xe2213), so folding onto CProjectile means
// re-homing this whole @early-stop body into Projectile.cpp AND reconciling
// m_1e0[]/m_204 with CProjectile's named-field/CPtrList layout - risks the byte
// match; left for the projectile-serialize rehome. Struct def: <Gruntz/ProjLoadRec.h>.

// @early-stop
// scratch-slot scheduling tail (same family as CTriggerLoadRec/CEventLoadRec/
// CGruntStateRec): the dual-mode switch, every Read/Write field+size, the 7-entry
// name-ref loop, the type-code-gated CMapPtrToPtr lookup, the g_coordPool.m_freeHead splice +
// AddTail, the inline strlen/strcpy KeyOfValue temps, the g_serialCounter bumps, the
// base tail-chain and the embedded +0x150 sub-record are byte-faithful; residual is
// the MSVC5 scratch-buffer slot assignment + outparam zero-init store positions. Not
// source-steerable.
RVA(0x000e0d40, 0x6c2)
i32 CProjLoadRec::Load(CSerialArchive* s, i32 mode, i32 a2, CGameObject* a3) {
    CDDrawSurfaceMgr* reg = g_gameReg->m_world;
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
                    void* out = 0; // CMapStringToPtr::Lookup (0x1b8438) takes a void&
                    reg->m_animRegistry->m_10.Lookup(buf, out);
                    m_1e0[ni] = static_cast<CObject*>(out);
                } else {
                    m_1e0[ni] = 0;
                }
            }

            g_serialCounter++;
            i32 key;
            s->Read(&key, 4);
            CGameObject* found = 0;
            i32 r;
            if (reg->m_childGroup->m_map48.Lookup(reinterpret_cast<void*>(key), reinterpret_cast<void*&>(found)) == 0) {
                r = 0;
            } else if (found == 0) {
                r = 0;
            } else {
                r = (found->GetTypeId() == 5) ? reinterpret_cast<i32>(found) : 0;
            }
            m_1fc = reinterpret_cast<CGameObject*>(r);
            if (m_1fc == 0 && key != 0) {
                return 0;
            }

            i32 cnt;
            s->Read(&cnt, 4);
            for (i32 ci = 0; ci < cnt; ci++) {
                CoordPoolNode* node = static_cast<CoordPoolNode*>(g_coordPool.m_freeHead);
                void* payload = 0;
                if (node->m_next != 0) {
                    g_coordPool.m_freeHead = node->m_next;
                    payload = &node->m_coord;
                }
                s->Read(payload, 8);
                m_204.AddTail(static_cast<CRezListNode*>(payload));
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
                    CString nm = reg->m_animRegistry->KeyOfValue_152d30(m_1e0[wi]);
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

            for (CoordNode* n = m_208; n != 0; n = n->m_next) {
                s->Write(n->m_coord, 8);
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
            CString nm = m_158->m_0c->m_animRegistry->KeyOfValue_152d30(m_15c);
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
    void* out = 0; // CMapStringToPtr::Lookup (0x1b8438) takes a void&
    m_158->m_0c->m_animRegistry->m_10.Lookup(buf, out);
    m_15c = static_cast<CObject*>(out);
    return 1;
}
