// GameStateRecordLoad.cpp - the big game-state-record deserializer @0x555e0
// (?Load@... , 4856 B). Reads an entire saved game-state record off a CRecReader:
//   - 7 serial-id object refs (read a 4-byte serial, look it up in the engine
//     object directory's serial map, type-tag-check (==5), store the pointer;
//     fail the load if a non-zero serial resolves to nothing),
//   - 3 CString fields (read a 0x80 text buffer, CString::operator= it),
//   - 18 name-ref object fields (read a 0x80 name, look it up by name in the
//     directory's name map, store the pointer),
//   - ~100 plain Read(&field, N) scalar/struct fields,
//   - a 3x3 array of 0x68-byte sub-records (each Load'd via 0x3ee0),
//   - two engine free-list-backed CRecPtrList rebuilds (count-prefixed),
//   - a couple of event-buffer pushes + one bute GetIntDef.
//
// `this` is accessed by raw byte offset throughout (the offsets ARE the on-disk
// record layout - the load-bearing spec); the archive Read / object type-tag are
// modeled as typed vtable structs so the `mov edx,[esi]; call [edx+0x2c]` /
// `[eax+0x20]` virtual dispatches fall out with no cast. Non-EH (base) profile -
// no destructible locals (the CString targets are members, the text buffer is a
// trivial char[]).
#include <Gruntz/GruntDataRecord.h>
#include <Gruntz/SpriteRefTable.h>
#include <Bute/ButeMgr.h>         // CButeMgr (GetIntDef) + CString
#include <Gruntz/GruntzMgr.h>     // CGruntzMgr (the game-manager singleton; one true shape)
#include <Gruntz/SerialArchive.h> // the shared CSerialArchive stream (Read @+0x2c)
#include <Mfc.h>                  // CObList (CRecPtrList fold)
#include <rva.h>
#include <string.h> // inline strlen / memset (rep scas / rep stos)

// Engine globals (reloc-masked; names match the delinked symbols).
extern i32 g_serialCounter;    // 0x629ad0  per-object load serial
#include <Gruntz/FreeNodePool.h> // the coord-node pool object @0x645540
// The pool's INTERIOR FIELDS - m_freeHead (+0x04) and m_linkOffset (+0x0c) - used to be
// declared here as the standalone globals g_coordPool.m_freeHead / g_coordPool.m_linkOffset. They are not
// globals: they are fields of g_coordPool (DEFINED in src/Gruntz/GameText.cpp), which is
// why the free-list push/pop code reads exactly [pool+4] and [pool+0xc].
extern FreeNodePool g_coordPool;
extern CButeMgr g_buteMgr;     // 0x6453d8  the global bute manager

// The two bute tag/key literals the tail GetIntDef reads.
static const char s_Powerupz[] = "Powerupz";                                 // 0x60d9b4
static const char s_GruntGhostTransparencyOn[] = "GruntGhostTransparencyOn"; // 0x60d900

// The record reads from the shared WAP32 CSerialArchive stream (Read @ vtable +0x2c),
// now the one modeled class in <Gruntz/SerialArchive.h> - the former local `CRecReader`
// view is folded away. `ar->Read` lowers to `mov edx,[ar]; call [edx+0x2c]`.

// A directory object whose runtime type tag (virtual slot 0x20 = index 8) gates
// the serial-ref store (only tag==5 objects are accepted).
struct CDirObj {
    virtual void dv0();
    virtual void dv1();
    virtual void dv2();
    virtual void dv3();
    virtual void dv4();
    virtual void dv5();
    virtual void dv6();
    virtual void dv7();
    virtual i32 GetTag(); // slot 8 -> +0x20
};

// The two engine lookup maps (external __thiscall, reloc-masked). The serial map
// keys on the 4-byte id; the name map keys on the text name.
// serial-map @0x1b8760 = MFC CMapPtrToPtr::Lookup (int key); name-map @0x1b8438 =
// CMapStringToOb::Lookup (string key). Both cast at the call.
struct CSerialMap {};
struct CNameMap {};

// The engine object directory (g_gameReg->m_world): the serial-map host at +8
// (map at +0x48), the name-map host at +0x2c (map at +0x10).
struct CObjDir {
    char _00[8];
    char* m_serialHost; // +0x08  (serial map @ +0x48)
    char _0c[0x2c - 0xc];
    char* m_nameHost; // +0x2c  (name map @ +0x10)
};

// The game-manager singleton (the one true CGruntzMgr shape lives in
// <Gruntz/GruntzMgr.h>): the object directory at +0x30 (canonical m_world, viewed
// here as the serial/name-map host), an engine helper at +0x74 the tail invokes
// (0x4165). Both sub-objects are engine carcasses reached by a struct-view cast.
DATA(0x0024556c)
extern "C" CGruntzMgr* g_gameReg; // 0x64556c

// The event/command buffer at this+0x10 the tail writes (type/value/flag slots).
struct CCmdBuf {
    char _00[0x4c];
    i32 m_4c; // +0x4c
    i32 m_50; // +0x50
    i32 m_54; // +0x54
    i32 m_58; // +0x58
};

// CRecPtrList (engine; AddTail/RemoveHead/RemoveAll reloc-masked __thiscall).

// A 0x68-byte sub-record (3x3 grid) with its own loader (0x3ee0).

// Global operator new / free (engine NAFXCW; reloc-masked).
void* operator new(u32 n);        // 0x1b9b46
extern "C" void RezFree(void* p); // 0x1b9b82  (operator delete / free)

class CGameStateRecord {
public:
    i32 Load(CSerialArchive* ar);
};

// The three repeating block shapes, expanded inline + unrolled (retail unrolls
// each and shares one 0x80 text buffer + the id/obj scratch locals - a helper
// function with a 0x80 local won't inline under MSVC5's budget, so the blocks
// are open-coded as macros over the Load-local `p`/`ar`/`dir`/`buf`/`id`/`obj`).
//   SERIALREF: read a 4-byte id, look it up, accept only a tag==5 object, store;
//              fail the load if a non-zero id resolved to nothing.
#define SERIALREF(off)                                                                             \
    do {                                                                                           \
        ++g_serialCounter;                                                                         \
        ar->Read(&id, 4);                                                                          \
        obj = 0;                                                                                   \
        void* r;                                                                                   \
        if (((CMapPtrToPtr*)(dir->m_serialHost + 0x48))->Lookup((void*)id, obj) != 0               \
            && obj != 0) {                                                                         \
            r = (((CDirObj*)obj)->GetTag() == 5) ? obj : 0;                                        \
        } else {                                                                                   \
            r = 0;                                                                                 \
        }                                                                                          \
        *(void**)(p + (off)) = r;                                                                  \
        if (r == 0 && id != 0) {                                                                   \
            return 0;                                                                              \
        }                                                                                          \
    } while (0)
//   READCSTR: read a 0x80 text buffer, CString::operator= it.
#define READCSTR(off)                                                                              \
    do {                                                                                           \
        ++g_serialCounter;                                                                         \
        ar->Read(buf, 0x80);                                                                       \
        *(CString*)(p + (off)) = buf;                                                              \
    } while (0)
//   NAMEREF: read a 0x80 name, look it up by name (if non-empty), store.
#define NAMEREF(off)                                                                               \
    do {                                                                                           \
        ++g_serialCounter;                                                                         \
        ar->Read(buf, 0x80);                                                                       \
        if (strlen(buf) != 0) {                                                                    \
            obj = 0;                                                                               \
            ((CMapStringToOb*)(dir->m_nameHost + 0x10))->Lookup(buf, (CObject*&)obj);              \
            *(void**)(p + (off)) = obj;                                                            \
        } else {                                                                                   \
            *(void**)(p + (off)) = 0;                                                              \
        }                                                                                          \
    } while (0)

// @early-stop
// 91.76% - COMPLETE, correct reconstruction of the whole 4856-byte deserializer
// (all 7 serial refs, 3 CStrings, 18 name refs, ~100 scalar reads, the 3x3
// sub-record loop, both free-list-backed list rebuilds, and the tail event
// pushes + GetIntDef), verified instruction-by-instruction against the disasm.
// The callee-saved pinning matches retail exactly (ebx=this, esi=ar, edi=dir,
// ebp=null-reg) and every opcode / immediate / call target / member offset /
// branch matches. The residual is purely the stack-FRAME LAYOUT: the recompile
// reserves a 0x94 frame and assigns the four scratch locals as obj@0x10 /
// dir-spill@0x14 / id@0x18 / count@0x1c, while retail reserves 0x90 with id@0x10
// / obj@0x14 / dir-spill@0x18 / count@0x1c. That one-slot-bigger frame + the
// permuted slot assignment shifts the `[esp+N]` displacement of every scratch
// access (id/obj/dir/count) across all 28 unrolled blocks + the param load
// (`[esp+0xa8]` vs `[esp+0xa0]`) - same instructions, different disp byte. This
// is MSVC5's internal frame-layout choice and is not steerable from source
// (merging the two count locals dropped 0x98->0x94 but the last slot + the
// id<->obj permutation persist; reordering the scratch declarations had no
// effect). The documented large-function regalloc/frame-layout wall; reconstructed
// in full per the no-stub mandate.
RVA(0x000555e0, 0x12f8)
i32 CGameStateRecord::Load(CSerialArchive* ar) {
    char* p = (char*)this;
    if (ar == 0) {
        return 0;
    }
    CObjDir* dir = (CObjDir*)g_gameReg->m_world;
    if (dir == 0) {
        return 0;
    }

    i32 id;
    void* obj; // the CMapPtrToPtr value type; the CObject-map call below re-types it
    char buf[0x80];

    *(void**)(p + 0x424) = 0;
    *(void**)(p + 0x428) = 0;
    *(void**)(p + 0x264) = 0;
    *(void**)(p + 0x268) = 0;
    *(void**)(p + 0x270) = 0;
    *(void**)(p + 0x26c) = 0;
    *(void**)(p + 0x274) = 0;

    // 7 serial-id object refs (unrolled).
    SERIALREF(0x1b8);
    SERIALREF(0x1bc);
    SERIALREF(0x1c4);
    SERIALREF(0x1c8);
    SERIALREF(0x1cc);
    SERIALREF(0x1d0);
    SERIALREF(0x1d4);

    // 3 CString fields.
    READCSTR(0x1c0);
    READCSTR(0x448);
    READCSTR(0x44c);

    // 18 name-ref fields (0x394..0x3d8 step 4, unrolled).
    NAMEREF(0x394);
    NAMEREF(0x398);
    NAMEREF(0x39c);
    NAMEREF(0x3a0);
    NAMEREF(0x3a4);
    NAMEREF(0x3a8);
    NAMEREF(0x3ac);
    NAMEREF(0x3b0);
    NAMEREF(0x3b4);
    NAMEREF(0x3b8);
    NAMEREF(0x3bc);
    NAMEREF(0x3c0);
    NAMEREF(0x3c4);
    NAMEREF(0x3c8);
    NAMEREF(0x3cc);
    NAMEREF(0x3d0);
    NAMEREF(0x3d4);
    NAMEREF(0x3d8);

    // ~100 plain scalar/struct reads (in retail order).
    ar->Read(p + 0x18c, 4);
    ar->Read(p + 0x190, 4);
    ar->Read(p + 0x194, 4);
    ar->Read(p + 0x170, 4);
    ar->Read(p + 0x198, 4);
    ar->Read(p + 0x19c, 4);
    ar->Read(p + 0x1a0, 4);
    ar->Read(p + 0x1a4, 4);
    ar->Read(p + 0x1a8, 4);
    ar->Read(p + 0x1ac, 4);
    ar->Read(p + 0x1b0, 4);
    ar->Read(p + 0x1b4, 4);
    ar->Read(p + 0x1d8, 4);
    ar->Read(p + 0x174, 8);
    ar->Read(p + 0x17c, 8);
    ar->Read(p + 0x184, 8);
    ar->Read(p + 0x1dc, 8);
    ar->Read(p + 0x1e4, 4);
    ar->Read(p + 0x1e8, 4);
    ar->Read(p + 0x1ec, 4);
    ar->Read(p + 0x1f0, 4);
    ar->Read(p + 0x1f4, 4);
    ar->Read(p + 0x1f8, 4);
    ar->Read(p + 0x1fc, 4);
    ar->Read(p + 0x200, 8);
    ar->Read(p + 0x208, 8);
    ar->Read(p + 0x210, 4);
    ar->Read(p + 0x214, 4);
    ar->Read(p + 0x218, 4);
    ar->Read(p + 0x21c, 4);
    ar->Read(p + 0x220, 4);
    ar->Read(p + 0x224, 4);
    ar->Read(p + 0x228, 4);
    ar->Read(p + 0x22c, 4);
    ar->Read(p + 0x230, 4);
    ar->Read(p + 0x290, 0x10);
    ar->Read(p + 0x2a0, 0x10);
    ar->Read(p + 0x2b0, 0x10);
    ar->Read(p + 0x2c0, 0x10);
    ar->Read(p + 0x3ec, 4);
    ar->Read(p + 0x3f0, 4);
    ar->Read(p + 0x3f4, 4);
    ar->Read(p + 0x3f8, 4);
    ar->Read(p + 0x400, 8);
    ar->Read(p + 0x418, 4);
    ar->Read(p + 0x42c, 4);
    ar->Read(p + 0x430, 4);
    ar->Read(p + 0x434, 4);
    ar->Read(p + 0x438, 4);
    ar->Read(p + 0x2d0, 4);
    ar->Read(p + 0x2d4, 4);
    ar->Read(p + 0x2d8, 4);
    ar->Read(p + 0x2dc, 4);
    ar->Read(p + 0x2e0, 4);
    ar->Read(p + 0x2e4, 4);
    ar->Read(p + 0x2ec, 4);
    ar->Read(p + 0x2f0, 8);
    ar->Read(p + 0x300, 8);
    ar->Read(p + 0x354, 4);
    ar->Read(p + 0x358, 4);
    ar->Read(p + 0x35c, 4);
    ar->Read(p + 0x3dc, 8);
    ar->Read(p + 0x3e4, 8);
    ar->Read(p + 0x450, 4);
    ar->Read(p + 0x41c, 4);
    ar->Read(p + 0x408, 8);
    ar->Read(p + 0x410, 8);
    ar->Read(p + 0x8d0, 4);
    ar->Read(p + 0x234, 4);
    ar->Read(p + 0x238, 4);
    ar->Read(p + 0x23c, 4);
    ar->Read(p + 0x240, 4);
    ar->Read(p + 0x244, 4);
    ar->Read(p + 0x248, 4);
    ar->Read(p + 0x24c, 4);
    ar->Read(p + 0x258, 4);
    ar->Read(p + 0x25c, 4);
    ar->Read(p + 0x360, 4);
    ar->Read(p + 0x364, 4);
    ar->Read(p + 0x318, 4);
    ar->Read(p + 0x2f8, 8);
    ar->Read(p + 0x36c, 4);
    ar->Read(p + 0x454, 4);
    ar->Read(p + 0x370, 4);
    ar->Read(p + 0x420, 4);
    ar->Read(p + 0x368, 4);
    ar->Read(p + 0x458, 8);
    ar->Read(p + 0x250, 4);
    ar->Read(p + 0x254, 4);
    ar->Read(p + 0x374, 4);
    ar->Read(p + 0x37c, 4);
    ar->Read(p + 0x380, 4);
    ar->Read(p + 0x384, 4);
    ar->Read(p + 0x388, 4);
    ar->Read(p + 0x390, 4);
    ar->Read(p + 0x378, 4);
    ar->Read(p + 0x38c, 4);
    ar->Read(p + 0x460, 4);
    ar->Read(p + 0x2e8, 4);
    ar->Read(p + 0x288, 8);

    // 3x3 array of 0x68-byte sub-records (outer stride 0x138, inner 0x68).
    char* row = p + 0x468;
    for (i32 gi = 0; gi < 3; ++gi) {
        char* cell = row;
        for (i32 gj = 0; gj < 3; ++gj) {
            if (((GruntDataRecord*)cell)->DeserializeStrings(ar) == 0) {
                return 0;
            }
            cell += 0x68;
        }
        row += 0x138;
    }

    // Drain the m_320 list back to the engine free-list, then RemoveAll(m_31c).
    if (*(void**)(p + 0x328) != 0) {
        void* node = *(void**)(p + 0x320);
        if (node != 0) {
            void* fl = g_coordPool.m_freeHead;
            do {
                void* next = *(void**)node;
                char* buf = *(char**)((char*)node + 8);
                if (buf != 0) {
                    buf -= g_coordPool.m_linkOffset;
                    *(void**)buf = fl;
                    fl = buf;
                    g_coordPool.m_freeHead = buf;
                }
                node = next;
            } while (node != 0);
        }
        ((CObList*)(p + 0x31c))->RemoveAll();
    }

    // Rebuild m_31c from a count of 8-byte free-list nodes.
    i32 count;
    ar->Read(&count, 4);
    for (i32 a = 0; a < count; ++a) {
        char* slot = (char*)g_coordPool.m_freeHead;
        void* nf = *(void**)slot;
        char* item = 0;
        if (nf != 0) {
            item = slot + 4;
            g_coordPool.m_freeHead = nf;
        }
        ar->Read(item, 8);
        ((CObList*)(p + 0x31c))->AddTail((CObject*)item);
    }

    // Drain + free the m_338 list.
    while (*(void**)(p + 0x344) != 0 && *(i32*)((char*)*(void**)(p + 0x33c) + 8) != 0) {
        void* rem = ((CObList*)(p + 0x338))->RemoveHead();
        RezFree(rem);
    }

    // Rebuild m_338 from a count of new(0x2c) nodes (zero-init, read 0x2c each).
    ar->Read(&count, 4);
    for (i32 b = 0; b < count; ++b) {
        void* mem = operator new(0x2c);
        void* item = 0;
        if (mem != 0) {
            memset(mem, 0, 0xb * 4);
            item = mem;
        }
        ar->Read(item, 0x2c);
        ((CObList*)(p + 0x338))->AddTail((CObject*)item);
    }

    // Push the level-config event(s) into the command buffer at +0x10.
    i32 m170 = *(i32*)(p + 0x170);
    i32 m1f4 = *(i32*)(p + 0x1f4);
    i32 flag = (m170 >= 0x17);
    i32 r = ((CSpriteRefTable*)g_gameReg->m_spriteFactory)->GetSel(m1f4, flag);
    CCmdBuf* cb = *(CCmdBuf**)(p + 0x10);
    cb->m_58 = 1;
    cb->m_50 = 0xa;
    cb->m_4c = r;

    if (*(i32*)(p + 0x258) == 0x36) {
        CCmdBuf* cb2 = *(CCmdBuf**)(p + 0x10);
        i32 v = g_buteMgr.GetIntDef(s_Powerupz, s_GruntGhostTransparencyOn, 0xe0);
        cb2->m_58 = 1;
        cb2->m_50 = 0xb;
        cb2->m_54 = v;
    }
    return 1;
}

SIZE_UNKNOWN(CDirObj);
SIZE_UNKNOWN(CSerialMap);
SIZE_UNKNOWN(CNameMap);
SIZE_UNKNOWN(CObjDir);
SIZE_UNKNOWN(CMgr74);
SIZE_UNKNOWN(CCmdBuf);
SIZE_UNKNOWN(CSubRecord);
SIZE_UNKNOWN(CGameStateRecord);

// --- vtable catalog ---
