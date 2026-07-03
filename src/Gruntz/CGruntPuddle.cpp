// CGruntPuddle.cpp - the grunt-puddle game object (a CUserLogic-derived leaf).
// Four __thiscall methods reconstructed in ascending-RVA order:
//   0x010d10  ~CGruntPuddle  (the bare CUserLogic teardown, /GX frame)
//   0x040c30  Place          (seed the puddle into a tile cell; bind sprite/bute)
//   0x040d20  Remove         (terrain-gate, unlink from the object list, re-bind)
//   0x07d810  SetBute        (swap the +0x1c bute node on the +0x14 sub-object)
//
// CUserLogic / CUserBase / EngStr / CGameObject come from <Gruntz/UserLogic.h>;
// the game-manager singleton (g_gameReg) + the icon factory / tile grid from
// <Gruntz/CInGameIcon.h>. Engine callees are reloc-masked (no body). Confirmed
// CGruntPuddle by its own string constants (see the header).
#include <Gruntz/CGruntPuddle.h>

#include <rva.h>

// ===========================================================================
// CGruntPuddle::~CGruntPuddle  (0x010d10)
// ===========================================================================
// The leaf adds no destructible members, so its dtor folds the bare CUserLogic
// teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link
// (the embedded ~EngStr call), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. The empty body is enough.
RVA(0x00010d10, 0x44)
CGruntPuddle::~CGruntPuddle() {}

// ===========================================================================
// CGruntPuddle::CGruntPuddle  (0x040490)
// ===========================================================================
// Fold the shared CUserLogic(obj) init, then flag the sub-object, lock the draw
// order to 0xa, name + apply the puddle sprite, bind the "A" bute node, snap the
// owner to its tile center, and seed the placed-state fields (+0x5c/+0x60).
// @early-stop
// eh-ctor-vptr-restamp-position wall (docs/patterns/eh-ctor-vptr-restamp-position.md):
// body byte-identical; residual is the /GX leaf-vptr re-stamp position + EH-state ids.
RVA(0x00040490, 0x1ab)
CGruntPuddle::CGruntPuddle(CGameObject* obj) : CUserLogic(obj) {
    m_38->m_flags |= 2;
    if (m_object->m_latchedAnimId != 0xa) {
        m_object->m_latchedAnimId = 0xa;
        m_object->m_flags |= 0x20000;
    }
    m_38->ApplyName("GRUNTZ_GRUNTPUDDLE");
    m_40 = m_38->m_geoId;
    m_38->ApplyLookupGeometry("GRUNTZ_GRUNTPUDDLE_GRUNTPUDDLE1", 0);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->m_stateFlags |= 1;
    m_object->m_screenX = (m_object->m_screenX & ~0x1f) + 0x10;
    m_object->m_screenY = (m_object->m_screenY & ~0x1f) + 0x10;
    m_5c = 1;
    m_60 = 0;
}

// ===========================================================================
// CGruntPuddle::Place  (0x040c30)
// ===========================================================================
// Seed the puddle into a tile cell. Snapshot the owning object's tile (x,y) from
// its world coords (>>5), stash the four call args, resolve an icon record from
// the per-player factory and stamp the place-command back into the owner
// (+0x58/+0x50/+0x4c), clear the owner's "occupied" low bit (+0x40 &= ~1), swap
// the +0x14 sub-object's bute node (g_buteTree.Find("B")). On the a1==0 path it
// finalizes the placement: flag +0x60, clear +0x54, snapshot the geometry id and
// apply the puddle sprite geometry. Returns 1.
//
// @early-stop
// inverse register-pinning wall (docs/patterns/zero-register-pinning.md): the body
// is structurally byte-exact, but retail does NOT enregister the `a1` parameter -
// it re-reads `[esp+0x10]` each use (so `m_5c = 0` is an immediate store and the
// ApplyLookupGeometry flag is `push $0`). Our MSVC 5.0 caches `a1` in callee-saved
// edi (extra push edi/pop edi; `m_5c = edi`; `push edi` flag). All offsets,
// immediates, call args and branch targets match; only the a1 caching differs.
// No init-list/assignment/reorder lever flips the allocator. Deferred.
RVA(0x00040c30, 0xb3)
i32 CGruntPuddle::Place(i32 a0, i32 a1, i32 a2, i32 a3) {
    CGameObject* o = m_object;
    m_54 = o->m_screenX >> 5;
    m_58 = o->m_screenY >> 5;
    m_64 = a3;
    m_68 = a0;
    m_6c = a1;
    i32 rec = ((CIconFactory*)g_gameReg->m_74)->GetByIndex(a1, 0);
    CGameObject* obj = m_object;
    *(i32*)((char*)obj + 0x58) = 1;
    *(i32*)((char*)obj + 0x50) = 0xa;
    *(i32*)((char*)obj + 0x4c) = rec;
    *(i32*)((char*)m_38 + 0x40) &= ~1;
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find(g_iconBute);
    if (a1 == 0) {
        m_60 = 1;
        m_5c = 0;
        m_40 = m_38->m_geoId;
        m_38->ApplyLookupGeometry(g_puddleSpriteKey, 0);
    }
    return 1;
}

// ===========================================================================
// CGruntPuddle::Remove  (0x040d20)
// ===========================================================================
// Tear the puddle off a cell. When placed (+0x60), read the tile cell's terrain
// flags from g_gameReg->m_tileGrid (out-of-bounds -> a synthetic 1); if the cell is
// passable (flags & 0x939 or & 0x2) mark the owner dirty (+0x8 |= 0x10000) and
// unlink this puddle's node from g_gameReg->m_68. Either way notify the owner's
// +0x1a0 sink, then re-bind/finalize: if the owner is fully constructed
// (+0x1c8 && !+0x1c0) and we were not yet placed, apply the puddle geometry and
// flag +0x60; otherwise set the owner's +0x40 low bit. Returns 0.
//
// @early-stop
// register-allocation wall (docs/patterns/zero-register-pinning.md): the body is
// structurally byte-exact - every offset, immediate (0x939/0x2/0x10000/0x1a0),
// branch target and call arg matches retail. The sole residual is the callee-saved
// scratch register: retail allocates `edi` (push esi/push edi) where our MSVC 5.0
// allocates `ebx` (push ebx/push esi), cascading the name through the tile-index
// `tx*7`, the `+0x8 |= 0x10000` temp and the list-walk node. The advance-then-test
// loop ordering was steered to match retail (+`next` local, ~70->71%); the ebx/edi
// coin-flip is not source-steerable. Deferred.
RVA(0x00040d20, 0xe3)
i32 CGruntPuddle::Remove() {
    if (m_60 != 0) {
        CGameRegistry* reg = g_gameReg;
        i32 ty = m_58;
        CTileGrid* grid = reg->m_tileGrid;
        i32 tx = m_54;
        i32 flags;
        if ((u32)tx < (u32)grid->m_c && (u32)ty < (u32)grid->m_10) {
            flags = ((i32*)grid->m_8[ty])[tx * 7];
        } else {
            flags = 1;
        }
        if ((flags & 0x939) != 0 || (flags & 0x2) != 0) {
            *(i32*)((char*)m_38 + 0x8) |= 0x10000;
            CObjList* list = (CObjList*)g_gameReg->m_68;
            CObjListNode* node = list->m_head;
            while (node != 0) {
                CObjListNode* next = node->m_next;
                if (node->m_data == this) {
                    list->RemoveAt(node);
                    return 0;
                }
                node = next;
            }
        }
    }
    ((CGruntPuddleSink*)((char*)m_38 + 0x1a0))->Notify(g_6bf3bc);
    CGameObject* o = m_38;
    if (*(i32*)((char*)o + 0x1c8) != 0 && *(i32*)((char*)o + 0x1c0) == 0) {
        if (m_60 != 0) {
            *(i32*)((char*)o + 0x40) |= 1;
        } else {
            m_40 = *(i32*)((char*)o + 0x1b4);
            ((CGameObject*)o)->ApplyLookupGeometry(g_puddleSpriteKey, 0);
            m_60 = 1;
            m_5c = 0;
        }
    }
    return 0;
}

// ===========================================================================
// CGruntPuddle::SetBute  (0x07d810)
// ===========================================================================
// Swap the +0x14 sub-object's bute node for the one keyed by `key`, stashing the
// previous node in CUserLogic::m_prevAnimSetNode. The shared tail of Place's bute re-bind.
RVA(0x0007d810, 0x25)
void CGruntPuddle::SetBute(char* key) {
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find(key);
}
