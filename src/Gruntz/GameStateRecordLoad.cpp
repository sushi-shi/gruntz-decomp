#include <Gruntz/GameStateRecordLoad.h> // this TU's external declarations
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


#define SERIALREF(field)                                                                             \
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
        (field) = static_cast<CWwdGameObjectA*>(r);                                                \
        if (r == 0 && id != 0) {                                                                   \
            return 0;                                                                              \
        }                                                                                          \
    } while (0)
#define READCSTR(field)                                                                              \
    do {                                                                                           \
        ++g_serialCounter;                                                                         \
        ar->Read(buf, 0x80);                                                                       \
        (field) = buf;                                                                             \
    } while (0)
#define NAMEREF(field)                                                                               \
    do {                                                                                           \
        ++g_serialCounter;                                                                         \
        ar->Read(buf, 0x80);                                                                       \
        if (strlen(buf) != 0) {                                                                    \
            obj = 0;                                                                               \
            dir->m_animRegistry->m_10.Lookup(buf, obj);                                            \
            (field) = static_cast<CAniElement*>(obj);                                              \
        } else {                                                                                   \
            (field) = 0;                                                                           \
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

    m_struckSlotSound = 0;
    m_struckVoiceSound = 0;
    m_struckCount = 0;
    m_struckClockLo = 0;
    m_struckTimerLo = 0;
    m_struckClockHi = 0;
    m_struckTimerHi = 0;

    // 7 serial-id object refs (unrolled).
    SERIALREF(m_selectedSprite);
    SERIALREF(m_toySprite);
    SERIALREF(m_healthSprite);
    SERIALREF(m_staminaSprite);
    SERIALREF(m_toyTimeSprite);
    SERIALREF(m_wingzTimeSprite);
    SERIALREF(m_powerupSprite);

    // 3 CString fields.
    READCSTR(m_animSetName);
    READCSTR(m_448);
    READCSTR(m_44c);

    // 18 name-ref fields (0x394..0x3d8 step 4, unrolled).
    NAMEREF(m_poseWalk);
    NAMEREF(m_poseAttack1);
    NAMEREF(m_poseAttack2);
    NAMEREF(m_poseAttackIdle);
    NAMEREF(m_poseStruck1);
    NAMEREF(m_poseStruck2);
    NAMEREF(m_poseIdle[0]);
    NAMEREF(m_poseIdle[1]);
    NAMEREF(m_poseIdle[2]);
    NAMEREF(m_poseIdle4);
    NAMEREF(m_poseIdle5);
    NAMEREF(m_poseDeath);
    NAMEREF(m_poseToy1);
    NAMEREF(m_poseToy2);
    NAMEREF(m_poseToyBreak);
    NAMEREF(m_poseItem);
    NAMEREF(m_poseItem2);
    NAMEREF(m_pickupGeoSrc);

    // ~100 plain scalar/struct reads (in retail order).
    ar->Read(&m_18c, 4);
    ar->Read(&m_toyBlendPct, 4);
    ar->Read(&m_194, 4);
    ar->Read(&m_entranceReason, 4);
    ar->Read(&m_198, 4);
    ar->Read(&m_19c, 4);
    ar->Read(&m_moveMode, 4);
    ar->Read(&m_1a4, 4);
    ar->Read(&m_1a8, 4);
    ar->Read(&m_1ac, 4);
    ar->Read(&m_1b0, 4);
    ar->Read(&m_1b4, 4);
    ar->Read(&m_arrived, 4);
    ar->Read(&m_entrancePxX, 8);
    ar->Read(&m_lastTilePxX, 8);
    ar->Read(&m_commitPxX, 8);
    ar->Read(&m_1dc, 8);
    ar->Read(&m_entranceActive, 4);
    ar->Read(&m_arrivalPending, 4);
    ar->Read(&m_tileOwnerHi, 4);
    ar->Read(&m_tileOwnerLo, 4);
    ar->Read(&m_1f4_moveIcon, 4);
    ar->Read(&m_1f8, 4);
    ar->Read(&m_entranceCommitted, 4);
    ar->Read(&m_neighborCol, 8);
    ar->Read(&m_208, 8);
    ar->Read(&m_210, 4);
    ar->Read(&m_214, 4);
    ar->Read(&m_combatActive, 4);
    ar->Read(&m_neighborValid, 4);
    ar->Read(&m_poweredUp, 4);
    ar->Read(&m_224, 4);
    ar->Read(&m_entranceStamped, 4);
    ar->Read(&m_22c, 4);
    ar->Read(&m_arrivalActive, 4);
    ar->Read(&m_reachRectLeft, 0x10);
    ar->Read(&m_2a0, 0x10);
    ar->Read(&m_2b0, 0x10);
    ar->Read(&m_2c0, 0x10);
    ar->Read(&m_health, 4);
    ar->Read(&m_stamina, 4);
    ar->Read(&m_toyTime, 4);
    ar->Read(&m_wingzTime, 4);
    ar->Read(&m_400, 8);
    ar->Read(&m_418, 4);
    ar->Read(&m_42c, 4);
    ar->Read(&m_430, 4);
    ar->Read(&m_434, 4);
    ar->Read(&m_438, 4);
    ar->Read(&m_arrivalState, 4);
    ar->Read(&m_defenderState, 4);
    ar->Read(&m_2d8, 4);
    ar->Read(&m_defenderRadius, 4);
    ar->Read(&m_2e0, 4);
    ar->Read(&m_2e4, 4);
    ar->Read(&m_dwell, 4);
    ar->Read(&m_arrivalCol, 8);
    ar->Read(&m_defenderX, 8);
    ar->Read(&m_354, 4);
    ar->Read(&m_358, 4);
    ar->Read(&m_35c, 4);
    ar->Read(&m_3dc, 8);
    ar->Read(&m_moveTileX, 8);
    ar->Read(&m_arrivalPhase, 4);
    ar->Read(&m_timePerTile, 4);
    ar->Read(&m_408, 8);
    ar->Read(&m_410, 8);
    ar->Read(&m_8d0, 4);
    ar->Read(&m_coordToggle, 4);
    ar->Read(&m_wingzEnabled, 4);
    ar->Read(&m_freezeDelayDone, 4);
    ar->Read(&m_freezeUnfrozen, 4);
    ar->Read(&m_resetApplied, 4);
    ar->Read(&m_arrivalFlags, 4);
    ar->Read(&m_24c, 4);
    ar->Read(&m_gruntKind, 4);
    ar->Read(&m_entranceArmed, 4);
    ar->Read(&m_deathType, 4);
    ar->Read(&m_entranceDropActive, 4);
    ar->Read(&m_318, 4);
    ar->Read(&m_2f8, 8);
    ar->Read(&m_36c, 4);
    ar->Read(&m_454, 4);
    ar->Read(&m_370, 4);
    ar->Read(&m_tileClaimed, 4);
    ar->Read(&m_deathAnimStarted, 4);
    ar->Read(&m_458, 8);
    ar->Read(&m_250, 4);
    ar->Read(&m_254, 4);
    ar->Read(&m_374, 4);
    ar->Read(&m_moveKind, 4);
    ar->Read(&m_moveVariant, 4);
    ar->Read(&m_coordRetryCount, 4);
    ar->Read(&m_toyTileIndex, 4);
    ar->Read(&m_390, 4);
    ar->Read(&m_378, 4);
    ar->Read(&m_38c, 4);
    ar->Read(&m_lowStaminaCued, 4);
    ar->Read(&m_2e8, 4);
    ar->Read(&m_288, 8);

    // 3x3 array of 0x68-byte sub-records (outer stride 0x138, inner 0x68).
    CGruntCellRec* row = m_cells;
    for (i32 gi = 0; gi < 3; ++gi) {
        CGruntCellRec* cell = row;
        for (i32 gj = 0; gj < 3; ++gj) {
            if ((reinterpret_cast<GruntDataRecord*>(cell))->DeserializeStrings(ar) == 0) {
                return 0;
            }
            cell += 1;
        }
        row += 3;
    }

    // Drain the m_320 list back to the engine free-list, then RemoveAll(m_31c).
    if (m_31c.GetCount() != 0) {
        GruntCoordNode* node = CoordHeadOf(m_31c);
        if (node != 0) {
            CoordPoolNode* fl = g_coordPool.m_freeHead;
            do {
                GruntCoordNode* next = node->m_next;
                char* buf = reinterpret_cast<char*>(node->m_coord);
                if (buf != 0) {
                    CoordPoolNode* n2 = g_coordPool.NodeOf(buf);
                    n2->m_next = fl;
                    fl = n2;
                    g_coordPool.m_freeHead = n2;
                }
                node = next;
            } while (node != 0);
        }
        (&m_31c)->RemoveAll();
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
        (&m_31c)->AddTail(item);
    }

    // Drain + free the m_338 list.
    while (m_338.GetCount() != 0 && GruntListHeadOf(m_338)->m_data != 0) {
        void* rem = (&m_338)->RemoveHead();
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
        (&m_338)->AddTail(item);
    }

    // Push the level-config event(s) into the grunt's HUD object (the
    // m_4c/m_50/m_58 move-icon triple, the SelectMoveIcon idiom).
    i32 flag = (m_entranceReason >= 0x17);
    CShadeTable* r = g_gameReg->m_spriteFactory->GetSel(m_1f4_moveIcon, flag);
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
