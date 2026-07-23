#include <Gruntz/GruntDataRecord.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Rez/RezAlloc.h>             // RezAlloc/RezFree
#include <Gruntz/Grunt.h>             // canonical CGrunt (this) + CGruntHud + CDDrawChildGroup
#include <DDrawMgr/DDrawSubMgrLeaf.h> // CDDrawSubMgrLeaf (the name map host, holder +0x2c)
#include <Wwd/WwdGameObjectFamily.h>  // CGameObject::GetClassId (the ==5 probe)
#include <Io/FileMem.h> // the serialize stream (CFileMemBase == the real CFileMemBase)
#include <Gruntz/SpriteRefTable.h>
#include <Bute/ButeMgr.h>         // CButeMgr (GetIntDef) + CString
#include <Gruntz/GruntzMgr.h>     // CGruntzMgr (the game-manager singleton; one true shape)
#include <Gruntz/SerialArchive.h> // the shared CFileMemBase stream (Read @+0x2c)
#include <Mfc.h>                  // CPtrList (CRecPtrList fold)
#include <rva.h>
#include <string.h> // inline strlen / memset (rep scas / rep stos)

#include <Gruntz/FreeNodePool.h> // the coord-node pool object @0x645540

static const char s_Powerupz[] = "Powerupz";                                 // 0x60d9b4
static const char s_GruntGhostTransparencyOn[] = "GruntGhostTransparencyOn"; // 0x60d900

void* operator new(u32 n); // 0x1b9b46

#define SERIALREF(off)                                                                             \
    do {                                                                                           \
        ++g_serialCounter;                                                                         \
        ar->Read(&id, 4);                                                                          \
        obj = 0;                                                                                   \
        void* r;                                                                                   \
        if (dir->m_childGroup->m_map48.Lookup(reinterpret_cast<void*>(id), obj) != 0               \
            && obj != 0) {                                                                         \
            r = ((reinterpret_cast<CGameObject*>(obj))->GetClassId() == CLASSID_SERIALREF) ? obj   \
                                                                                           : 0;    \
        } else {                                                                                   \
            r = 0;                                                                                 \
        }                                                                                          \
        *reinterpret_cast<void**>(p + (off)) = r;                                                  \
        if (r == 0 && id != 0) {                                                                   \
            return 0;                                                                              \
        }                                                                                          \
    } while (0)
#define READCSTR(off)                                                                              \
    do {                                                                                           \
        ++g_serialCounter;                                                                         \
        ar->Read(buf, 0x80);                                                                       \
        *reinterpret_cast<CString*>(p + (off)) = buf;                                              \
    } while (0)
#define NAMEREF(off)                                                                               \
    do {                                                                                           \
        ++g_serialCounter;                                                                         \
        ar->Read(buf, 0x80);                                                                       \
        if (strlen(buf) != 0) {                                                                    \
            obj = 0;                                                                               \
            dir->m_animRegistry->m_10.Lookup(buf, obj);                                            \
            *reinterpret_cast<void**>(p + (off)) = obj;                                            \
        } else {                                                                                   \
            *reinterpret_cast<void**>(p + (off)) = 0;                                              \
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
i32 CGrunt::LoadStateRecord(CFileMemBase* ar) {
    char* p = reinterpret_cast<char*>(this);
    if (ar == 0) {
        return 0;
    }
    CDDrawSurfaceMgr* dir = g_gameReg->m_world;
    if (dir == 0) {
        return 0;
    }

    i32 id;
    void* obj; // the CMapPtrToPtr value type; the CObject-map call below re-types it
    char buf[0x80];

    *reinterpret_cast<void**>((p + 0x424)) = 0;
    *reinterpret_cast<void**>((p + 0x428)) = 0;
    *reinterpret_cast<void**>((p + 0x264)) = 0;
    *reinterpret_cast<void**>((p + 0x268)) = 0;
    *reinterpret_cast<void**>((p + 0x270)) = 0;
    *reinterpret_cast<void**>((p + 0x26c)) = 0;
    *reinterpret_cast<void**>((p + 0x274)) = 0;

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
            if ((reinterpret_cast<GruntDataRecord*>(cell))->DeserializeStrings(ar) == 0) {
                return 0;
            }
            cell += 0x68;
        }
        row += 0x138;
    }

    // Drain the m_320 list back to the engine free-list, then RemoveAll(m_31c).
    if (*reinterpret_cast<void**>((p + 0x328)) != 0) {
        void* node = *reinterpret_cast<void**>((p + 0x320));
        if (node != 0) {
            CoordPoolNode* fl = g_coordPool.m_freeHead;
            do {
                void* next = *static_cast<void**>(node);
                char* buf = *reinterpret_cast<char**>((reinterpret_cast<char*>(node) + 8));
                if (buf != 0) {
                    CoordPoolNode* n2 = g_coordPool.NodeOf(buf);
                    n2->m_next = fl;
                    fl = n2;
                    g_coordPool.m_freeHead = n2;
                }
                node = next;
            } while (node != 0);
        }
        (reinterpret_cast<CPtrList*>((p + 0x31c)))->RemoveAll();
    }

    // Rebuild m_31c from a count of 8-byte free-list nodes.
    i32 count;
    ar->Read(&count, 4);
    for (i32 a = 0; a < count; ++a) {
        CoordPoolNode* slot = g_coordPool.m_freeHead;
        CoordPoolNode* nf = slot->m_next;
        void* item = 0;
        if (nf != 0) {
            item = &slot->m_coord;
            g_coordPool.m_freeHead = nf;
        }
        ar->Read(item, 8);
        (reinterpret_cast<CPtrList*>((p + 0x31c)))->AddTail(item);
    }

    // Drain + free the m_338 list.
    while (*reinterpret_cast<void**>((p + 0x344)) != 0
           && *reinterpret_cast<i32*>(
                  (reinterpret_cast<char*>(*reinterpret_cast<void**>((p + 0x33c))) + 8)
              ) != 0) {
        void* rem = (reinterpret_cast<CPtrList*>((p + 0x338)))->RemoveHead();
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
        (reinterpret_cast<CPtrList*>((p + 0x338)))->AddTail(item);
    }

    // Push the level-config event(s) into the grunt's HUD object (the
    // m_4c/m_50/m_58 move-icon triple, the SelectMoveIcon idiom).
    i32 flag = (m_entranceReason >= 0x17);
    i32 r = g_gameReg->m_spriteFactory->GetSel(m_1f4_moveIcon, flag);
    CWwdGameObjectA* cb = m_object;
    cb->m_drawActive = 1;
    cb->m_drawFillCmd = 0xa;
    cb->m_drawFillArg = r;

    if (m_gruntKind == 0x36) {
        CWwdGameObjectA* cb2 = m_object;
        i32 v = g_buteMgr.GetIntDef(s_Powerupz, s_GruntGhostTransparencyOn, 0xe0);
        cb2->m_drawActive = 1;
        cb2->m_drawFillCmd = 0xb;
        cb2->m_fillFraction = v;
    }
    return 1;
}
