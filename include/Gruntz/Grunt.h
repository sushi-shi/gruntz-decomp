#ifndef SRC_GRUNTZ_GRUNT_H
#define SRC_GRUNTZ_GRUNT_H

class CAniElement;

class FreeNodePool;

class CDDrawSubMgrLeaf;

class DSoundCloneInst; // the pooled cue player (Dsndmgr/DirectSoundMgr.h)

class DirectSoundMgr;

#include <Mfc.h> // the REAL MFC CPtrList (m_31c/m_338 are value members) + POSITION
#include <Ints.h>
#include <Gruntz/LogicTypeId.h>
#include <rva.h> // SIZE_UNKNOWN/VTBL class-metadata macros used below
#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/UserBaseLink.h>   // shared CUserBaseLink (+0x18 link; ~EngStr 0x16d2a0)
#include <Gruntz/SpriteRefTable.h> // CSpriteRefTable (g_gameReg->m_74; GetSel)
#include <Gruntz/WwdGameReg.h>     // the canonical WwdGameReg singleton layout (g_gameReg)
#include <Gruntz/UserLogic.h>
#include <Gruntz/MovingLogic.h> // the ONE CMovingLogic (fat spine + the band union; CGrunt derives it)

SIZE_UNKNOWN(CCueRect);
typedef struct tagRECT CCueRect;

class CGruntSpawnConfig;                  // the +0x60 registry object (one class, three ex-names)
typedef CGruntSpawnConfig CGruntCueSink; // cue-receiver face: methods live on CGruntSpawnConfig

#include <Gruntz/GameRegistry.h>

#include <Gruntz/String.h>

CString __stdcall operator+(const char* lhs, const CString& rhs);
CString __stdcall operator+(const CString& lhs, const char* rhs);

SIZE_UNKNOWN(CAnimLookupNode);
extern i32 g_movingSeed;

extern "C" i32 GruntRand(); // stub

class CGrunt; // fwd (CueA/CueSpawn first arg; the resolvers below)

i32 CueVisible(i32 viewport, i32 x, i32 y);

SIZE_UNKNOWN(CGruntCell);
class CGruntCell {
public:
    // GetName @0x310f0 IS _zdvec::IndexToPtr; cast at each call.
};

char* GruntStrGetBuffer(void* str, i32 minLen); // 0x1ba11c

void __stdcall EntranceApplyFrame(const char* keyStr, i32 frameNum);

#include <Gruntz/CurPlayer.h> // g_curPlayer (the current local player index)

extern i32 g_cellLo;    // DAT_006bf658
extern i32 g_cellHi;    // DAT_006bf65c
extern i32 g_cellBase;  // DAT_006bf660
extern i32 g_cellRet;   // DAT_006bf664
extern i32 g_cellScale; // DAT_006bf668

extern i32 g_cellRecordBase; // DAT_006bf464
extern i32 g_cellRecordRet;  // DAT_006bf428

extern i32 g_dirAb0[2];     // DAT_00644ab0
extern i32 g_dirAe0[2];     // DAT_00644ae0
extern i32 g_dirAa0[2];     // DAT_00644aa0
extern i32 g_dirB28[2];     // DAT_00644b28
extern i32 g_dirAc0[2];     // DAT_00644ac0
extern i32 g_dirB48[2];     // DAT_00644b48
extern i32 g_dirAd0[2];     // DAT_00644ad0
extern i32 g_dirB18[2];     // DAT_00644b18
extern i32 g_dirB38[2];     // DAT_00644b38
extern double g_dirConst2;  // DAT_005e9a28 = 2.0
extern double g_dirConst1;  // DAT_005e9a30 = 1.0
extern double g_dirConstN1; // DAT_005e9a38 = -1.0

SIZE_UNKNOWN(CAnimScratchString);
struct CAnimScratchString {
    char* m_str; // +0x00  (4-byte stride)
    // Release @0x1b9b93 IS CString::~CString; cast at each call.
};

extern const char g_codeA[]; // 0x60a454 "A"
extern const char g_codeD[]; // 0x60cca4 "D"
extern const char g_codeI[]; // 0x60cca0 "I"
extern const char g_codeG[]; // 0x60cc9c "G"
extern const char g_codeL[]; // 0x60cc98 "L"
extern const char g_codeP[]; // 0x60beb8 "P"
extern const char g_codeO[]; // 0x60dc0c "O"
extern const char g_codeQ[]; // 0x60dc08 "Q"
extern const char g_codeJ[]; // 0x60cc94 "J"
extern const char g_codeN[]; // 0x60dc04 "N"
extern const char g_codeM[]; // 0x60d7f4 "M"
extern const char g_codeK[]; // 0x60d7f8 "K"
extern const char g_codeF[]; // 0x60d2e8 "F"  (PlaySound entrance handler)
extern const char g_codeE[]; // 0x60d2ec "E"  (PlaySound entrance handler)
extern const char g_codeH[]; // 0x60d7fc "H"  (arrival-recycle reject code)

SIZE(GruntEntranceCell, 0xc);
struct GruntEntranceCell {
    i32 col;
    i32 row;
    i32 reason;
};
struct GruntSoundEntry; // map value: per-effect sound entry (factory at +0x10)

SIZE_UNKNOWN(GruntSoundEntry);
struct GruntSoundEntry {
    char m_pad0[0x10];
    DSoundCloneInst* m_10; // +0x10  the sample factory
};

SIZE_UNKNOWN(GruntCoordPool);
extern FreeNodePool g_coordPool; // DAT_00645540

SIZE_UNKNOWN(GruntCoord);
struct GruntCoord {
    i32 m_x; // +0x00
    i32 m_y; // +0x04
};
SIZE_UNKNOWN(GruntCoordNode);
struct GruntCoordNode {
    GruntCoordNode* m_next; // +0x00
    char m_pad4[0x4];
    GruntCoord* m_coord; // +0x08
};

struct CAnimSetNode {
    char m_pad0[0xc];
    i32 m_c;  // +0x0c  the value Lookup returns into the table
    i32 m_10; // +0x10  animation length (toy-swap blend uses this)
};

class CGruntPuddle;
SIZE_UNKNOWN(CGruntLiveNode);
struct CGruntLiveNode {
    CGruntLiveNode* m_next; // +0x00
    char m_pad4[0x8 - 0x4];
    CGruntPuddle* m_entry; // +0x08  placed puddle (tile x/y + pending gate)
};

i32 GruntPointVisible(i32 px, i32 py, i32 cmp);

i32 __stdcall GruntDropReady029b40(CGrunt* g);

SIZE_UNKNOWN(CGruntTypeCatalog);
struct CGruntTypeCatalog {
    char m_pad0[0xc];
    CDDrawSubMgrLeaf* m_c; // +0x0c  owner -> name-id map
};

#include <Gruntz/SerialCounter.h>

SIZE_UNKNOWN(CGruntListNode);
struct CGruntListNode {
    CGruntListNode* m_next; // +0x00
    char m_pad4[0x8 - 0x4];
    u8* m_data;   // +0x08  serialized payload blob (0x2c bytes)
};

class CArchive; // (unused MFC fwd; Save uses CGruntArchive)

SIZE_UNKNOWN(CGruntSub);
class CGruntSub {
public:
    // Free @0x69d60 IS CGrunt::LoadFreezeSpellAssets; cast at each call.
};

SIZE_UNKNOWN(CGruntCellRec);
struct CGruntCellRec {
    CString m_attack; // +0x00  "GRUNTZ_<name>_<DIR>_ATTACK"
    CString m_struck; // +0x04  "GRUNTZ_<name>_<DIR>_STRUCK"
    CString m_walk;   // +0x08  "GRUNTZ_<name>_<DIR>_WALK" / "GRUNTZ_<name>_<DIR>"
    CString m_idle;   // +0x0c  "GRUNTZ_<name>_<DIR>_IDLE"
    CString m_item;   // +0x10  "GRUNTZ_<name>_<DIR>_ITEM"
    i32 m_14;         // +0x14  (serialized record dword)
    i32 m_18;         // +0x18  (serialized record dword)
    i32 m_1c;         // +0x1c  (serialized record dword)
    i32 m_20;         // +0x20  (serialized record dword)
    i32 m_24;         // +0x24  (serialized record dword)
    i32 m_28;         // +0x28  (serialized record dword)
    i32 m_2c;         // +0x2c  (serialized record dword)
    i32 m_30;         // +0x30  (serialized record dword)
    i32 m_34;         // +0x34  (serialized record dword)
    char m_pad38[0x40 - 0x38];
    i32 m_40; // +0x40  (serialized record dword)
    i32 m_44; // +0x44  (serialized record dword)
    // The per-direction movement vector (abs +0x4b0.. as the "+0x4b0 dir-vector
    // table", cell index 3*col+row, stride 0x68): unit direction {m_dirX, m_dirY}
    // + half-tile step offsets {m_stepX, m_stepY}. Activate (@0x5caa0)
    // writes them as doubles ([ecx+13a*8+0x4b0..0x4c8]); the movement-integration
    // tail of MovingSlot16 (@0x5f310) reads all four. The serialize/load path
    // streams raw 4-byte halves of these (the (char*)+4 spellings in Load).
    double m_dirX;    // +0x48  unit direction X
    double m_dirY;    // +0x50  unit direction Y
    double m_stepX;   // +0x58  half-tile step X (+-0.5)
    double m_stepY;   // +0x60  half-tile step Y (+-0.5)
    CGruntCellRec();  // 0x401e9c (per-element ctor; the __ehvec_ctor callback)
    ~CGruntCellRec(); // 0x4023a6 (per-element dtor; reloc-masked)
};
SIZE_UNKNOWN(GruntStrSub);
struct GruntStrSub { // +0x44c / +0x448 / +0x1c0  (~CString 0x1b9cde)
    void CtorImpl(); // 0x1b9b93 (CString default ctor)
    void Dtor();
    GruntStrSub() {
        CtorImpl();
    }
    ~GruntStrSub() {
        Dtor();
    }
};

// (CVtSlot9 DISSOLVED: DispatchVtbl24's +0x24 self-dispatch is the real slot-9
// virtual, CUserLogic::StepAttackFire.)

void GruntNode_Delete(void* p);

struct CGruntVoiceRec; // defined below (the 3-DWORD by-value voice record)
extern CGruntVoiceRec g_voiceN;  // 0x6448e8  (dx==0, dy>0  -> South: down)
extern CGruntVoiceRec g_voiceS;  // 0x6448d8  (dx==0, dy<0  -> North: up)
extern CGruntVoiceRec g_voiceE;  // 0x6448c8  (shallow +, dx>0 -> East)
extern CGruntVoiceRec g_voiceW;  // 0x6448f8  (shallow +, dx<0 -> West)
extern CGruntVoiceRec g_voiceSE; // 0x644928  (mid +, dx>0)
extern CGruntVoiceRec g_voiceNW; // 0x644918  (mid +, dx<0)
extern CGruntVoiceRec g_voiceNE; // 0x644908  (mid -, dx>0)
extern CGruntVoiceRec g_voiceSW; // 0x644948  (mid -, dx<0)

SIZE_UNKNOWN(CGruntVoiceRec);
struct CGruntVoiceRec {
    i32 m_0;
    i32 m_4;
    i32 m_8;
};

SIZE_UNKNOWN(CGruntMotionBand);
struct CGruntMotionBand {
    void Init(); // 0x136d0 (CMotionState ctor; retail via thunk 0x34db)
    i32 SetParams(
        double a0,
        double a1,
        double a2,
        double a3,
        double a4,
        double a5,
        double a6,
        double a7,
        double a8,
        double a9,
        double a10
    );                   // 0x58bc0 (thunk 0x2ccf)
    void SetZ(double z); // 0x58ca0 (thunk 0x3ea9)
};
extern const double g_movingLogicMin;  // 0x5f04b0 (-2147483647.0)
extern const double g_movingLogicMax;  // 0x5f04b8 (2147483646.0)
extern const double g_gruntSpawnScale; // 0x5e9738 (spawn-seed velocity scale)
extern u32 g_defaultZ;                 // 0x5f04e8 (default-Z int)
extern u32 g_gruntSpawnClock;          // 0x645588 (spawn-seed clock; reloc-masked)
extern "C" u32 g_frameTime;            // 0x645588 (the running game clock; FrameClock.h)

class CProjectile; // canonical full model in <Gruntz/Projectile.h> (MFC-full); pointer-only here

SIZE(CGrunt, 0x8d8);
class CGrunt : public CMovingLogic {
public:
    // vtable overrides in slot order (see the base chain above):
    virtual ~CGrunt() OVERRIDE; // slot 0  @0xf2f0
    virtual i32 SerializeMove(CGruntArchive* ar, i32 mode, i32 a3, i32 a4)
        OVERRIDE; // slot 1  @0x53b80
    RVA(0x0000f2a0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GRUNT;
    } // slot 2  (0xf2a0)
    // slot 3 (0x5d210, 5187 B @stub): receives the serialized type NAME (the base
    // XferName hook) and loads the grunt's per-type tuning constants from it.
    virtual void XferName(char* name) OVERRIDE;
    // RunAct (0x5bcd0): the class's vtable slot-4 (UserLogicVfunc2) activation
    // dispatcher body - a plain method (the no-arg UserLogicVfunc2() base placeholder
    // blocks the int-arg OVERRIDE spelling). Resolves `id`'s handler in the per-class
    // registry g_reg_644af0 and dispatches it as a PMF on `this`; else returns the
    // entry pointer. Same archetype as CPathHazard::RunAct.
    virtual void FireActivation(i32 id) OVERRIDE;

    // ---- the trigger/switch leaves' per-kind hooks ----
    // These are the methods ApplyTriggerA and friends dispatch on a placed grid grunt. Ten
    // of them are the REAL bodies already declared elsewhere in this class (ClearAllSprites
    // 0x4b240, BuildGruntExitAnimation 0x641b0, LoadGruntDeathAnimations 0x60150,
    // StartBombGruntRun 0x68520, RunMoveConfig 0x65630, RectContains 0x51850,
    // CommitNeighbor 0x5b050, BeginAttack 0x5b570, PlayMoveSound 0x511b0,
    // ResetEntranceAnimation 0x62e10) - the fold binds the call sites straight to them.
    //
    // The eight below are the ones the view could not bind. @identity-TODO, and the reason
    // is worth recording: the RVAs the view carried for them (ResetA 0x6a40c, ResetB
    // 0x6a2ae, ResetC 0x6c216, ResetMagic 0x6c498, Disarm 0x6f970, ApplyBox 0x6fc40,
    // Type13Check 0x71f80, Apply13 0x70520) are NOT function starts - several are not even
    // 4-byte aligned and none appears in Ghidra's function list, so they look like call-site
    // operands mis-recorded as entry points. They are declared here so the call sites keep
    // compiling, but they are still declared-never-defined: the honest fix is to recover the
    // real entry points, not to invent addresses for them.
    void ResetA();
    void ResetB();
    void ResetC();
    i32 ResetMagic();
    i32 Disarm(i32 a, i32 b);
    i32 ApplyBox(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f);
    i32 Type13Check();
    void Apply13(i32 a, i32 b);
    i32 Dispatch(i32 kind, i32 a);
    virtual i32 Activate() OVERRIDE;        // slot 6  @0x5caa0
    virtual i32 UserLogicVfunc6() OVERRIDE; // slot 8  (0x62b40)
    virtual i32 StepAttackFire() OVERRIDE;  // slot 9  @0x61cb0 (attack-fire step)
    // slot 9 @0x61cb0 - the per-frame ATTACK-FIRE step (defined in
    // GruntEntranceArrival.cpp; ex the CGruntFireView::Update misbinding): ticks
    // the attack anim; at the fire cue spawns the ranged projectile
    // ("Projectile"/"Boomerang"/"TimeBomb" by tool kind) or delivers the melee
    // hit to the neighbor-cell grunt, then applies the "AttackDowntime" timer.
    // Returns 0. (Base CUserLogic slot 9 is a return-0 default; this is its one
    // known override, hence the slot's name.)
    virtual i32 UserLogicVfunc9() OVERRIDE; // slot 11 @0x48360
    virtual void MovingSlot16() OVERRIDE;   // slot 16 @0x5f310

    i32 CreateHealthSprite();
    i32 CreateToySprite();
    i32 CreateStaminaSprite();
    i32 CreateToyTimeSprite();
    i32 CreateWingzTimeSprite();
    i32 CreatePowerupSprite(i32 a); // (ret 4)
    i32 CreateSelectedSprite();

    void ReadConfigFromButeMgr();
    i32 LoadGruntMovingDeathConfig();
    void LoadAnimNameTable(i32 a, i32 b); // @0x49c60 (ret 8)
    // @0x51850 (ret 8) tile-rect predicate; reconstruction deferred to the final
    // sweep (a register-relative rect-walk regalloc wall - cl folds this+const to
    // absolute loads, overshooting 0x165 B). Called external/reloc-masked here.
    i32 RectContains(i32 x, i32 y);

    // 0x343f0: recycle every occupied-coord node's payload onto g_coordPool's freelist,
    // then RemoveAll the +0x31c CPtrList. A __thiscall ON a grunt - it was homed as the
    // .cpp-local view `GridUnit::RecycleCoords`, and GridUnit IS CGrunt, so it lands here.
    // Body lives in src/Gruntz/BattlezMapConfig.cpp (its retail RVA slot).
    void RecycleCoords(); // 0x343f0
    i32
    RectContainsGated(i32 x, i32 y); // @0x51a20 (ret 8) sibling; m_198 gate, rects +0x2b0/+0x2c0
    i32 CommitNeighbor(i32 a, i32 b, i32 c, i32 d); // @0x5b050 (ret 0x10)
    CGrunt* FindGridNeighbor(i32 validate);         // @0x5b6f0 (ret 4)
    // @0xef6b0 (GruntChargeStep.cpp) - the per-frame pursue/charge behavior step:
    // a scan/move/arrived machine over m_defenderState driving the trigger-mgr
    // grid + the wander fallback. (Ex the GruntChargeStep view family.)
    i32 ChargeStep();
    // @0xf42f0 (ret 0, /GX) - the per-tick nearest-enemy / arrival-target scan
    // (GruntTargetScan.cpp): the ArrivalScan-family nested board scan + reason->priority
    // gate + squared-distance min + PtInRect box + m_defenderState (0/1/2) dispatch +
    // rand-driven idle wander. Big function; parked @early-stop (family regalloc wall).
    i32 ScanNearestTarget();
    i32 UpdateGruntStatus(); // @0x617c0 (ret 0)
    // @0x51c00 (ret 0, /GX) - the per-tick compass-move driver: resolves the grunt's
    // next move tile by the 8-way direction code (m_444), tests/stamps the board
    // occupancy + owner, fires the matching compass grunt-voice record, and commits
    // the move/arrival. Big switch + /GX EH frame + grid raw-offset state machine.
    i32 StepCompassMove();
    // @0x692f0 (ret 0) - the death/struck reaction dispatch: gated on m_1fc, resolves
    // the current anim name + dispatches on its type code (A/D/I/G/L/P/O/J/N/M), then
    // runs the shared arrival/clear-sprites/DEATHZ_FREEZE finalize tail.
    i32 StepArrivalCommit();
    // @0x65630 (2-arg this-method the I-arm of CommitNeighbor runs).
    void RunMoveConfig(i32 a, i32 b);
    // @0x641b0 - tears down the grunt's HUD sprites + plays the "GRUNTZ_EXITZ" exit
    // animation (rand-bucketed ONE/TWO/THREE variant + on-screen cue). __thiscall ret 0.
    i32 BuildGruntExitAnimation();
    // @0x63db0 - (re)loads the vehicle-grunt (gokart/bigwheel) entrance animation set.
    void LoadVehicleGruntAnimations();
    // @0x050a50 (ret 4) - the "toob" (pipe) grunt entrance-anim setup: reset the
    // +0x290/+0x2a0 reach-rect blocks, latch m_coordToggle, pick the TOOB(WATER)GRUNT
    // anim-set into m_animSetName + Register it, run the three reset helpers, gate the
    // entrance re-init, clear the shared type-name registry, and (when the resolved
    // type name is "D") cache the first entrance frame into m_154 + stamp its blit
    // param/descriptor. Body in src/Gruntz/GruntTubeAnim.cpp.
    i32 SetupTubeAnim(i32 isWater);

    // --- GruntAssetLoaders.cpp cluster (mechanical asset/sprite/tuning loaders) ---
    // @0x68880 (ret 4, /base) - (re)load the wingz-grunt's per-direction sprite
    // name cells (flying ITEM set when enabled, WALK/IDLE set when disabled), the
    // pose-index lookups, then re-stamp the current entrance-cell frame.
    i32 LoadWingzGruntSprites(i32 enable);
    // @0x57100 (ret 4, /base) - the spell-ability tuning loader: fire the attack
    // sound cue, then dispatch on the (random or forced) ability index to build
    // the matching LightFx/rolling-ball effect + tuning.
    i32 LoadGruntAbilityTuning(i32 forced);
    // @0x60150 (ret 8) - the grunt death dispatch: tear down the running anim state,
    // retire the HUD sprites, latch the "C" death anim-set, then switch on the death
    // type to resolve + apply the matching GRUNTZ_DEATHZ_* sprite + cue.
    i32 LoadGruntDeathAnimations(i32 deathType, i32 a2);
    // @0x65e80 (ret 0x14, /base) - the pickup/powerup entrance-sprite loader: gate on
    // grunt-kind/entrance state, bump the per-owner pickup stats, latch the "J" anim-set,
    // then a ~90-way switch on the pickup type resolves the matching GRUNTZ_PICKUPS_*
    // sprite (megaphone runs a 2nd unit-type switch) + fires the on-screen entrance cue.
    i32 LoadPickupSprites(i32 type, i32 a2, i32 a3, i32 a4, i32 a5);
    // Pickup-loader helper this-methods (reloc-masked engine thunks).
    void PickupResetA();                    // thunk 0x214e (__thiscall ret 0)
    void PickupResetB(i32 a, i32 b, i32 c); // thunk 0x136b (__thiscall ret 0xc)

    // @0x57890 (__thiscall ret 0, /GX) - when the entrance reason is a lose-item
    // pose (0x12/0x16/0xe), spawn the one-shot "SingleAnimation" GRUNTZ_<set>_LOSEITEM
    // sprite, fire the on-screen spawn cue, then re-run the type-table step.
    i32 BuildGruntLoseItemAnimation();
    // The big CUserLogic-base step driver reached via thunk 0x3bd9 -> 0x4dd50
    // (LoadGruntTypeTable / SelfImpact); external/reloc-masked here. Returns i32
    // (InGameIcon reads the result; return type is mangling-neutral).
    i32 LoadGruntTypeTable(i32 a, i32 b, i32 c, i32 d);
    // @0x50ca0 (RunEntranceMove tail): reload the type table for `typeId` then reset
    // the move-mode pair (m_moveMode=-1, m_1a4=0). Re-homed from Stub.
    void LoadTypeTableClearMove(i32 typeId);

    // --- arrival / move-step helper cluster (proximity-attributed targets) ---
    void PlayMoveSoundAtTile(i32 tx, i32 ty); // @0x514e0 (ret 8) tile->pixel + PlayMoveSound
    void SnapToLastTile(i32 a);               // @0x517b0 (ret 4) snap m_10 to last tile + commit
    i32 ClaimSwitchTile();                    // @0x52c70 (ret 0) switch-dir tile claim
    void SetArrivalTarget(i32 a, i32 b, i32 c, i32 d); // @0x52ed0 (ret 0x10)
    void ConsiderArrival(i32 a);                       // @0x52f40 (ret 4) arrival/drop gate
    void SelectMoveIcon(i32 a);                        // @0x57800 (ret 4) pick move-cursor icon
    i32 TryPowerupAtTile();                            // @0x57aa0 (ret 0) probe move tile
    // @0x57db0 (ret 0, /GX) - the per-tick grunt path-cell scan (GruntPathScan.cpp):
    // 5x5 dirty box, tracked-coord list scan firing the plane trigger, freelist recycle.
    i32 PathScan57db0();

    // --- animation resolvers (this TU's targets) ---
    i32 ResolveMovingAnimation();
    i32 ResolveDeathAnimation();
    // (NotifyFortUnderAttack @0x45270 is CWarlord's - the CGrunt shadow decl was a
    //  fake-view alias; the one caller downcasts its CUserLogic* to CWarlord*.)
    i32 ResolveAnimation(); // (generic / "_JOY")
    i32 ResolveIdleAnimation();
    i32 ResolveBattlecryAnimation();

    // @0x4a9f0 (ret 0) - the 4-way reachability probe: resolve the grunt under the
    // HUD center (m_tileMgr->FindAtPixel), copy its entrance rect (+0x144) offset by
    // its HUD origin, then test 4 segments (vertical / horizontal / two diagonals,
    // +-1000 px) through the grunt's own HUD center; return 1 on the first hit.
    i32 winapi_04a9f0_CopyRect_OffsetRect();
    // The seg/box probe @0x62b70 is called __THISCALL here (retail loads ecx=this before
    // the call - removing it misaligns the body by 2B/call). 0x62b70's bound symbol
    // CGrunt_SegBoxOverlap is mangled __stdcall (YG) in gruntentrancearrival, so no
    // thiscall-named symbol exists to bind to; this stays a correct-convention UNBOUND.
    i32 RectSegProbe(void* r, void* a, void* b); // call 0x4138 -> 0x62b70 (__thiscall, 3 args)

    // Data members. vptr(+0), m_10(+0x10), m_14(+0x14) are in CUserBase; the +0x18
    // EngStr link is CUserLogic::m_18. CGrunt's own members begin at +0x34.
    // +0x30 m_prevAnimSetNode is INHERITED from CUserLogic (<Gruntz/UserLogic.h>),
    // which owns and serializes it: CGrunt::SerializeMove @0x53b80 chains the base
    // 0x16e7f0, and that body read/writes this+0x30 - so +0x30 cannot be CGrunt's
    // own field. The local re-declaration was dropped 2026-07-17 (SM1); the name,
    // type (void*) and offset are unchanged, so every use site is untouched.
    // (+0x34..+0x14f is the CMovingLogic base's - incl. the grunt arm of its band
    // union: m_animPlayer/m_activeAnimDesc/m_typeName/the *GeoSrc handles/the move
    // seeds/m_animResolved/m_deathCueArg all live there now. <Gruntz/MovingLogic.h>.)
    // The +0x54 type name viewed as a CString at its five concat sites
    // (codegen-neutral: same CString lvalue; the raw char* member keeps ~CGrunt
    // from auto-destructing it).
    CString& TypeName() {
        return *reinterpret_cast<CString*>(&m_typeName);
    }
    // (+0x114/+0x118/+0x124 were migrated here last batch as "placement params". That was
    //  WRONG and is reverted: those writes happen on the CreateSprite RESULT - a CGameObject,
    //  not a grunt - and <Gruntz/UserLogic.h>'s CGameObject already models them (m_114 /
    //  m_118 / m_124) with recovered roles. The old CTmCell view conflated the SPRITE with
    //  the LOGIC, and this was a piece of that conflation leaking onto CGrunt.)
    // ---------------------------------------------------------------------
    // +0x150..+0x16f IS THE CWapX SECOND BASE, spelled flat (MI1, 2026-07-17).
    // PROVEN, not suspected - three independent lines agree:
    //  1. RTTI: CGrunt's CHD @VA 0x5f2c40 is attributes=1 (MI) and, decoding the
    //     numContainedBases nesting, its DIRECT bases are CMovingLogic@0x0 +
    //     CWapX@0x150. (CMovingLogic's own CHD @0x5f3cf0 is attributes=0 with NO
    //     CWapX, so it does not arrive through the spine.)
    //  2. BYTES: the ctor @0x47a10 emits `mov [esi+0x150],ebp; mov [esi+0x154],ebp;
    //     mov ecx,[ebp+0x7c]; mov [esi+0x158],ecx` - exactly the three-store CWapX
    //     seed the +0x34 tile leaves emit, at +0x150.
    //  3. SHAPE: the five fields below == CWapX's m_34/m_38/m_3c/m_value/m_blob,
    //     field for field, including this header's own independently-recovered notes
    //     ("m_158 = obj->m_7c", "m_15c = m_154->m_1a0.m_14 cache", a 0x10-byte pad),
    //     and CGrunt's real own data resumes at +0x170 = 0x150 + sizeof(CWapX).
    //
    // NOT YET CONVERTED. Step 1 (the name collisions) is DONE and committed; step 2
    // (the spine) is written, ATTEMPTED, and blocked on ONE named thing - read this
    // before you retry, it is cheap to re-derive wrongly:
    //
    //   (1) DONE - the collision renames, USR-exact + byte-neutral (0 functions moved):
    //         CGrunt::m_38 -> m_animPlayer  (+0x38; collided with CWapX::m_38 @+0x150)
    //         m_150/m_154/m_158/m_prevEntranceDesc -> m_34/m_38/m_3c/m_value
    //       so the +0x150 use sites already read the base's names. NB clangd's renamer
    //       does NOT rewrite MACRO BODIES - three macros kept expanding to the dead
    //       m_154 and only the build caught them. Rename FIRST, then delete: with the
    //       decls still present a missed site is a compile error, not a silent rebind.
    //
    //   (2) BLOCKED - `: public the lean CMovingLogic, public CWapX` needs the PRIMARY base
    //       to be the 0x150 spine (a second base lands right after the primary's size,
    //       so with today's LEAN base CWapX would sit at +0x34, not +0x150). Moving
    //       +0x34..+0x14f into the lean CMovingLogic is mechanical and byte-neutral (base
    //       data precedes own data -> offsets unchanged). ATTEMPTED 2026-07-17: it
    //       compiles down to exactly ONE class of error, and it is the OLD, KNOWN
    //       CGrunt-ODR blocker #4 (the i32/void slot-signature split; see the NOTE in
    //       <Gruntz/UserLogic.h>) surfacing at the PMF table: making CGrunt MI widens
    //       `i32 (CGrunt::*)()` to 8 bytes where retail stores 4, so GruntActHandler
    //       must be declared on the SI base (retail's own RTTI names the table
    //       `zDArray<int (CUserLogic::*)(void)>`, and it mixes CGrunt with
    //       CGruntBehaviorLeaf handlers, so the base IS its true type - that part is
    //       right). But g_reg_644af0's handlers are a MIX of void- and i32-returning
    //       methods, and MSVC5 rejects `static_cast<i32 (CUserLogic::*)()>(&CGrunt::)<void method>`.
    //       So the PMF retype cannot land until the void/i32 return split is
    //       reconciled from the BODIES (which arm actually materializes eax) - the same
    //       reconciliation blocker #4 already describes. Do that FIRST; the spine move
    //       itself is ready and is not the hard part.
    //
    // The equivalent conversion on CProjectile (same base, same +0x150) is DONE and
    // byte-verified - see <Gruntz/Projectile.h> for the worked shape. CProjectile did
    // not hit this because its handler table's return types are uniform.
    // ---------------------------------------------------------------------
    CGameObject* m_34; // +0x150  (CWapX::m_34; == the bound object, m_38's twin slot)
    CWwdGameObjectA* m_38; // +0x154  the created entrance-anim sprite object (the ex
                        //         CEntranceAnimPlayer view - the player IS the
                        //         created CGameObject; see UserLogic.h's tail note)
    // +0x158: the sprite's worker record. IDENTITY PROVEN by the ctor tail
    // (Grunt.cpp): `m_158 = obj->m_7c` - the bound object's AnimWorkerObj. Its
    // hop chain
    // m_0c->m_28->m_30: worker->m_0c is the owner/world context (the
    // CDDrawSurfaceMgr facet) whose +0x28 is the CSndHost cue registry
    // (emit gate +0x30, CMapStringToPtr map +0x10).
    AnimWorkerObj* m_3c;            // +0x158 (the bound object's worker record)
    CAniElement* m_value; // +0x15c (= m_154->m_1a0.m_14 cache)
    char m_pad160[0x170 - 0x160];
    // +0x170 (entrance-reason / movement state). The attack-fire step (UserLogicVfunc7)
    // reads this slot as the grunt's current TOOL/attack kind (switched over the
    // GRUNTZ tool ids 2=Boomerang/9=Gunhat/10=Nerfgun/11=Rock/17=TimeBomb/
    // 21=Welder/22=Wingz; >0x16 = melee) and forwards it as the projectile kind -
    // the slot multiplexes the current-action kind; reconcile the name when the
    // entrance machines' reading is re-verified.
    i32 m_entranceReason;
    i32 m_entrancePxX; // +0x174 (SetEntrancePos: committed entrance position X, pixel)
    i32 m_entrancePxY; // +0x178 (SetEntrancePos: committed entrance position Y, pixel)
    i32 m_lastTilePxX; // +0x17c (LoadEntranceConfig: last occupied tile X, pixel; -1 = none)
    i32 m_lastTilePxY; // +0x180 (LoadEntranceConfig: last occupied tile Y, pixel; -1 = none)
    i32 m_commitPxX; // +0x184 (committed position snapshot X, pixel; = m_lastTilePxX after switch)
    i32 m_commitPxY; // +0x188 (committed position snapshot Y, pixel; = m_lastTilePxY after switch)
    i32 m_18c;       // +0x18c
    i32 m_toyBlendPct;              // +0x190 (anim-name loader: TOY1/TOY2 blend percent)
    i32 m_194;                      // +0x194
    i32 m_198;                      // +0x198
    i32 m_19c;                      // +0x19c
    i32 m_moveMode;                 // +0x1a0
    i32 m_1a4;                      // +0x1a4
    i32 m_1a8;                      // +0x1a8 (serialized)
    i32 m_1ac;                      // +0x1ac (serialized)
    i32 m_1b0;                      // +0x1b0 (serialized)
    i32 m_1b4;                      // +0x1b4 (serialized)
    CWwdGameObjectA* m_selectedSprite;  // +0x1b8
    CWwdGameObjectA* m_toySprite;       // +0x1bc
    CString m_animSetName;          // +0x1c0  (anim-name loader: "GRUNTZ_"+m_animSetName+...)
    CWwdGameObjectA* m_healthSprite;    // +0x1c4
    CWwdGameObjectA* m_staminaSprite;   // +0x1c8
    CWwdGameObjectA* m_toyTimeSprite;   // +0x1cc
    CWwdGameObjectA* m_wingzTimeSprite; // +0x1d0
    CWwdGameObjectA* m_powerupSprite;   // +0x1d4
    i32 m_arrived;                  // +0x1d8 (entrance-arrival gate)
    i32 m_1dc;                      // +0x1dc
    i32 m_1e0;                      // +0x1e0
    i32 m_entranceActive;           // +0x1e4 (entrance: set to 1)
    i32 m_arrivalPending;           // +0x1e8 (SnapToLastTile/ClaimSwitchTile arrival-commit latch)
    i32 m_tileOwnerHi;              // +0x1ec
    i32 m_tileOwnerLo;              // +0x1f0
    i32 m_1f4_moveIcon;             // +0x1f4 (SelectMoveIcon: clamped icon index, [0,0x11))
    i32 m_1f8;                      // +0x1f8 (serialized)
    i32 m_entranceCommitted;        // +0x1fc (entrance: cleared)
    i32 m_neighborCol;              // +0x200 (grid-neighbor: column, -1 = none)
    i32 m_neighborRow;              // +0x204 (grid-neighbor: row, -1 = none)
    i32 m_208;                      // +0x208
    i32 m_20c;                      // +0x20c
    i32 m_210;                      // +0x210
    i32 m_214;                      // +0x214
    i32 m_combatActive; // +0x218 (combat/attack-active latch; set with m_poweredUp on attack, gates stamina display)
    i32 m_neighborValid;   // +0x21c (grid-neighbor: cleared on miss)
    i32 m_poweredUp;       // +0x220 (powered-up gate; 0 = run entrance reset)
    i32 m_224;             // +0x224 (serialized)
    i32 m_entranceStamped; // +0x228 (one-time entrance/toy-break stamp latch; set 1 at rest)
    i32 m_22c;             // +0x22c (entrance-drop: latched anim re-init gate)
    i32 m_arrivalActive; // +0x230 (arrival commit armed; seeded 1 w/ defender block, gates commit)
    i32 m_coordToggle;   // +0x234 (parity bit toggled per coord commit; arg to OnCoordCommit)
    i32 m_wingzEnabled;  // +0x238
    i32 m_freezeDelayDone; // +0x23c (freeze finalize: 0 = sparkle-delay window running, 1 = elapsed)
    i32 m_freezeUnfrozen;  // +0x240 (freeze finalize: set 1 when DEATHZ_UNFREEZE applied)
    i32 m_resetApplied;    // +0x244 (entrance-reset: 0 then 1 = "applied" flag)
    i32 m_arrivalFlags;    // +0x248 (arrival flag word; |= 0x18040402)
    i32 m_24c;             // +0x24c
    i32 m_250;             // +0x250 (serialized)
    i32 m_254;             // +0x254 (serialized)
    i32 m_gruntKind;       // +0x258 (grunt type/kind; ==0x37 -> halve TimePerTile)
    i32 m_entranceArmed;   // +0x25c (entrance: set to 1)
    // +0x260  the path/occupancy board == the ONE CTriggerMgr (same object as
    // thunk-resolution table above). Fwd-declared via <Gruntz/GameRegistry.h>;
    // TUs that dispatch on it include <Gruntz/TriggerMgr.h>.
    class CTriggerMgr* m_tileMgr;
    i32 m_struckCount;   // +0x264 (struck-reaction counter; cue tier 5/0xa)
    i32 m_struckClockLo; // +0x268 (= g_frameTime game clock at last struck)
    i32 m_struckClockHi; // +0x26c (= 0)
    i32 m_struckTimerLo; // +0x270 (= 0xfa0 struck cooldown window)
    i32 m_struckTimerHi; // +0x274 (= 0)
    i32 m_278;           // +0x278
    i32 m_27c;           // +0x27c
    i32 m_280;           // +0x280
    i32 m_284;           // +0x284
    i32 m_288;           // +0x288 (serialized)
    i32 m_28c;           // +0x28c (serialized)
    // The grunt's reach/collision bounds rect (tile-space {left,top,right,bottom};
    // RectContains reads it via &m_reachRectLeft). m_reachRadius is the rect's right
    // edge AND a scalar reach radius (GruntInRadius radius-sum). GruntTubeAnim seeds
    // it {-1,-1,1,1}.
    i32 m_reachRectLeft;   // +0x290
    i32 m_reachRectTop;    // +0x294
    i32 m_reachRadius;     // +0x298 (grunt reach/collision radius; also reach-rect right edge)
    i32 m_reachRectBottom; // +0x29c
    i32 m_2a0;             // +0x2a0
    i32 m_2a4;             // +0x2a4
    i32 m_2a8;             // +0x2a8
    i32 m_2ac;             // +0x2ac
    i32 m_2b0;             // +0x2b0
    i32 m_2b4;             // +0x2b4
    i32 m_2b8;             // +0x2b8
    i32 m_2bc;             // +0x2bc
    i32 m_2c0;             // +0x2c0
    i32 m_2c4;             // +0x2c4
    i32 m_2c8;             // +0x2c8
    i32 m_2cc;             // +0x2cc
    i32 m_arrivalState;    // +0x2d0 (arrival: = 4)
    i32 m_defenderState;   // +0x2d4 (arrival defender dispatch sub-state: switch 0/1/2/3)
    i32 m_2d8;             // +0x2d8
    i32 m_defenderRadius;  // +0x2dc (defender radius / arrival kind)
    i32 m_2e0;             // +0x2e0 (serialized)
    i32 m_2e4;             // +0x2e4 (serialized)
    i32 m_2e8;             // +0x2e8 (serialized)
    i32 m_dwell;           // +0x2ec
    i32 m_arrivalCol;      // +0x2f0 (arrival target grid col; grid index = 15*col+row; -1 = none)
    i32 m_arrivalRow;      // +0x2f4 (arrival target grid row)
    i32 m_2f8;             // +0x2f8
    i32 m_2fc;             // +0x2fc
    i32 m_defenderX;       // +0x300 (arrival: = m_lastTilePxX)
    i32 m_defenderY;       // +0x304 (arrival: = m_lastTilePxY)
    i32 m_arrivalRerollLo; // +0x308 (arrival re-roll idle timer: anchor clock lo; i64 w/ m_arrivalRerollHi)
    i32 m_arrivalRerollHi;       // +0x30c (arrival re-roll idle timer: anchor clock hi)
    i32 m_arrivalRerollWindowLo; // +0x310 (arrival re-roll idle window lo = GruntRand()%0x7530 + 0x7530)
    i32 m_arrivalRerollWindowHi; // +0x314 (arrival re-roll idle window hi)
    i32 m_318;                   // +0x318
    // The two owned lists are REAL MFC CPtrLists (0x1c B each), not views: the ctor
    // calls 0x1b4867 and ~CGrunt calls 0x1b48c6 - the band whose vtable (0x1eb054)
    // slot-0 GetRuntimeClass returns the CRuntimeClass naming "CPtrList". Their
    // interior IS the CPtrList field run:
    //   m_31c: vptr@31c  head@320  tail@324  count@328  free@32c blocks@330 blk@334
    //   m_338: vptr@338  head@33c  tail@340  count@344  free@348 blocks@34c blk@350
    // Read them through the public inline accessors (GetCount/GetHeadPosition/
    // GetHead/GetTail) - they compile to the same [esi+0x320]/[esi+0x328] loads.
    // A POSITION here IS a GruntCoordNode* (MFC CNode = {pNext, pPrev, data}, and
    // GruntCoord* m_coord is the +8 data slot).
    CPtrList m_31c; // +0x31c  occupied-coord list (block size 0xa); ends +0x338
    CPtrList m_338; // +0x338  serialized-payload list (block size 0xa); ends +0x354

    // Typed views of the two lists' interior. MFC's GetCount/GetHeadPosition/
    // GetTailPosition are inline, so these compile to the very same [esi+0x320] /
    // [esi+0x324] / [esi+0x328] loads the hand-declared fields used to emit. The
    // POSITION -> node cast is the one language-forced cast here: MFC keeps CNode
    // private, and a CPtrList POSITION *is* that node ({pNext, pPrev, data}), whose
    // +8 data slot is GruntCoord* m_coord.
    GruntCoordNode* CoordHead() const {
        return reinterpret_cast<GruntCoordNode*>(m_31c.GetHeadPosition());
    }
    GruntCoordNode* CoordTail() const {
        return reinterpret_cast<GruntCoordNode*>(m_31c.GetTailPosition());
    }
    i32 CoordCount() const {
        return m_31c.GetCount();
    }
    CGruntListNode* PayloadHead() const {
        return reinterpret_cast<CGruntListNode*>(m_338.GetHeadPosition());
    }
    i32 PayloadCount() const {
        return m_338.GetCount();
    }

    i32 m_354;       // +0x354 (serialized)
    i32 m_358;       // +0x358
    i32 m_35c;       // +0x35c
    i32 m_deathType; // +0x360 (last LoadGruntDeathAnimations kind; serialized w/ m_364)
    i32 m_entranceDropActive; // +0x364 (entrance-drop safe period active; set 1 on drop commit, gates combat reaction)
    i32 m_deathAnimStarted; // +0x368 (death-animation-started latch)
    i32 m_36c;              // +0x36c
    i32 m_370;             // +0x370 (death-notify arg a2; serialized; also a CObArray view in Load)
    i32 m_374;             // +0x374
    i32 m_378;             // +0x378 (serialized)
    i32 m_moveKind;        // +0x37c (move-kind fallback; randomized if 0, arg to ApplyMoveKind)
    i32 m_moveVariant;     // +0x380 (selected entrance/move variant index 1..N; from m_374 or rand)
    i32 m_coordRetryCount; // +0x384 (MovingSlot16: head-coord tile-claim retry budget, <=5)
    i32 m_toyTileIndex;    // +0x388 (toy-tile index; gated < Grunt ToyTiles config count)
    i32 m_38c;             // +0x38c
    i32 m_390;             // +0x390
    // The per-pose animation-name index table (LoadAnimNameTable @0x49c60 fills
    // it from "GRUNTZ_"+m_animSetName+"_<POSE>" lookups). The entrance code reads
    // the IDLE1/2/3 slots (m_poseIdle/m_3b0/m_3b4) as its geometry-source triple.
    CAniElement* m_poseWalk;       // +0x394 (_WALK)
    CAniElement* m_poseAttack1;    // +0x398 (_ATTACK1)
    CAniElement* m_poseAttack2;    // +0x39c (_ATTACK2)
    CAniElement* m_poseAttackIdle; // +0x3a0 (_ATTACK-IDLE)
    CAniElement* m_poseStruck1;    // +0x3a4 (_STRUCK1)
    CAniElement* m_poseStruck2;    // +0x3a8 (_STRUCK2)
    CAniElement* m_poseIdle[3];    // +0x3ac (_IDLE1/2/3) (entrance geometry-source triple [0..2])
    CAniElement* m_poseIdle4;      // +0x3b8 (_IDLE4)
    CAniElement* m_poseIdle5;      // +0x3bc (_IDLE5)
    CAniElement* m_poseDeath;      // +0x3c0 (_DEATH)
    CAniElement* m_poseToy1;       // +0x3c4 (_TOY1)
    CAniElement* m_poseToy2;       // +0x3c8 (_TOY2)
    CAniElement* m_poseToyBreak;   // +0x3cc (_TOY-BREAK)
    CAniElement* m_poseItem;       // +0x3d0 (_ITEM)
    CAniElement* m_poseItem2;      // +0x3d4 (_ITEM2)
    i32 m_pickupGeoSrc; // +0x3d8  (LoadPickupSprites: resolved pickup-sprite handle / SetGeometry source; serialized by name)
    i32 m_3dc;          // +0x3dc (serialized)
    i32 m_3e0;          // +0x3e0 (serialized)
    i32 m_moveTileX; // +0x3e4 (destination tile X; ArrivalNotify6/Load6 arg, = PlayMoveSoundAtTile x)
    i32 m_moveTileY; // +0x3e8 (destination tile Y; ArrivalNotify6/Load6 arg, = PlayMoveSoundAtTile y)
    i32 m_health;    // +0x3ec
    i32 m_stamina;   // +0x3f0
    i32 m_toyTime;   // +0x3f4
    i32 m_wingzTime; // +0x3f8
    char m_pad3fc[0x400 - 0x3fc];
    double m_400;      // +0x400
    double m_408;      // +0x408
    double m_410;      // +0x410
    i32 m_418;         // +0x418
    i32 m_timePerTile; // +0x41c (TimePerTile config; ComputeFacing time divisor; halved for kind 0x37)
    i32 m_tileClaimed;                // +0x420 (arrival-claimed latch)
    DirectSoundMgr* m_struckSlotSound;            // +0x424 (struck-slot sound sample; freed via StopAndRewind)
    DirectSoundMgr* m_struckVoiceSound;            // +0x428 (struck-voice sound sample; freed via StopAndRewind)
    i32 m_42c;                        // +0x42c
    i32 m_430;                        // +0x430
    i32 m_434;                        // +0x434
    i32 m_438;                        // +0x438
    GruntEntranceCell m_entranceCell; // +0x43c (entrance-cell triple {col, row, reason})
    CString m_448;                    // +0x448 (real CString; ex the GruntStrSub shell)
    CString m_44c;                    // +0x44c (real CString; ex the GruntStrSub shell)
    i32 m_arrivalPhase;    // +0x450 (arrival/update dispatch phase: 2 = in-flight, 3 = committing)
    i32 m_454;             // +0x454 (serialized)
    i32 m_458;             // +0x458 (serialized)
    i32 m_45c;             // +0x45c (serialized)
    i32 m_lowStaminaCued;  // +0x460 (low-stamina off-screen cue latch)
    i32 m_arrivalNotified; // +0x464 (entrance-reset latch flag)
    // +0x468 owned-cell array (9 x 0x68, +0x468..+0x810; entrance-cell record table,
    // 0x68-byte stride). Value array so ~CGrunt auto-emits the __ehvec_dtor teardown.
    CGruntCellRec m_cells[9]; // +0x468..+0x810  (per-direction anim-name cell records)
    // +0x810..+0x83f: three timer i64 pairs. Each is a union so the 64-bit
    // arithmetic sites keep the i64 while the ctor's interleaved dword zeroing
    // (lo,lo,hi,hi per block - retail's store order) is typed, not offset-cast.
    union { i64 m_toyClock; struct { i32 m_toyClockLo, m_toyClockHi; }; };          // +0x810
    union { i64 m_toyDuration; struct { i32 m_toyDurationLo, m_toyDurationHi; }; }; // +0x818
    union { i64 m_idleAnchor; struct { i32 m_idleAnchorLo, m_idleAnchorHi; }; };    // +0x820
    union { i64 m_idleDelay; struct { i32 m_idleDelayLo, m_idleDelayHi; }; };       // +0x828
    union { i64 m_idleTimer; struct { i32 m_idleTimerLo, m_idleTimerHi; }; };       // +0x830
    union { i64 m_idleWindow; struct { i32 m_idleWindowLo, m_idleWindowHi; }; };    // +0x838 (= 0x3a98)
    i32 m_entranceClockLo;    // +0x840 (entrance: = g_frameTime game clock, low dword)
    i32 m_entranceClockHi;    // +0x844 (entrance: = 0, high dword)
    i32 m_entranceSafeTimeLo; // +0x848 (entrance: = EntranceSafeTime config)
    i32 m_entranceSafeTimeHi; // +0x84c (entrance: = 0)
    i32 m_850;                // +0x850 (timer record base, SerializeMove)
    i32 m_854;                // +0x854
    i32 m_858;                // +0x858 (entrance: = 0)
    i32 m_85c;                // +0x85c (entrance: = 0)
    // +0x860..+0x86f: the attack-downtime timer record (same {clock i64, duration
    // i64} shape as the combat/wingz timers below; SerializeMove round-trips it
    // from m_860). The attack-fire step (UserLogicVfunc7) stamps it at each impact:
    // clock = g_frameTime (lo) / 0 (hi), duration = "AttackDowntime" bute (lo) / 0.
    i32 m_860;              // +0x860 (attack timer: anchor clock lo = g_frameTime)
    i32 m_864;              // +0x864 (attack timer: anchor clock hi = 0)
    i32 m_attackDowntimeLo; // +0x868 (attack timer: duration lo = AttackDowntime config)
    i32 m_attackDowntimeHi; // +0x86c (attack timer: duration hi = 0)
    // Combat/wingz state timers (the GruntAssetLoaders cluster fills them).
    i32 m_combatClockLo; // +0x870 (combat timer: anchor clock lo = g_frameTime; i64 w/ m_combatClockHi)
    i32 m_combatClockHi;   // +0x874 (combat timer: anchor clock hi = 0)
    i32 m_combatTimeoutLo; // +0x878 (combat timer: duration lo = CombatTimeout config)
    i32 m_combatTimeoutHi; // +0x87c (combat timer: duration hi = 0)
    i32 m_880; // +0x880 (timer record base, SerializeMove; == combat timer base = game clock)
    i32 m_884; // +0x884
    // +0x888 - the trigger leaves read this as the combat timeout the +0x880 timer base is
    // compared against. CGrunt already names a CombatTimeout at +0x878/+0x87c, so this one
    // keeps its offset name until the two roles are reconciled (do not guess).
    i32 m_888;          // +0x888
    i32 m_88c;          // +0x88c
    i32 m_wingzClockLo; // +0x890 (wingz timer: anchor clock lo = g_frameTime; i64 w/ m_wingzClockHi)
    i32 m_wingzClockHi;    // +0x894 (wingz timer: anchor clock hi = 0)
    i32 m_wingzDurationLo; // +0x898 (wingz timer: duration lo = (long)(m_wingzTime*scale-bias))
    i32 m_wingzDurationHi; // +0x89c (wingz timer: duration hi = 0)
    i32 m_8a0; // +0x8a0 (sub-ser record base, SerializeMove)
    i32 m_8a4; // +0x8a4
    i32 m_8a8; // +0x8a8
    i32 m_8ac; // +0x8ac
    i32 m_8b0; // +0x8b0 (sub-ser record base, SerializeMove)
    i32 m_8b4; // +0x8b4
    i32 m_8b8; // +0x8b8
    i32 m_8bc; // +0x8bc
    i32 m_8c0; // +0x8c0
    i32 m_8c4; // +0x8c4
    i32 m_8c8; // +0x8c8
    i32 m_8cc; // +0x8cc
    i32 m_8d0; // +0x8d0
    i32 m_8d4; // +0x8d4  (trailing member; sizeof(CGrunt) == 0x8d8, the `new CGrunt` size)

    // The grunt's spawn constructor @0x47a10 (__thiscall, the CMovingLogic-base
    // moving-object ctor: base CUserLogic(owner), the CMotionState motion band at
    // +0x38 + the twelve default coordinate bounds, then the huge field-init block).
    CGrunt(void* owner);

    // Engine-label backlog stubs. (0x048400 is CGrunt::ReadConfigFromButeMgr, declared above.)
    void LoadCellAnimNames(i32 a, i32 b); // (2-arg; called from LoadEntranceConfig tail)
    void ResetEntranceAnimation(i32 a, i32 b, i32 c); // (ret 0xc) - 3-arg entrance reset
    void ResolveEntranceArrival();
    void EntrancePrepare(); // thunk_FUN_0044b240 (void this-method, external)
    void BuildEntranceAnimation(i32 mode);
    void LoadEntranceConfig();
    // LoadEntranceConfig tail helpers (this-methods reached via incremental-link
    // thunks; external/no-body, reloc-masked).
    void EntranceFinishWire(i32 a, i32 b);  // thunk_FUN_00449c60 (2-arg)
    void EntranceOnReleased();              // thunk_FUN_0044b130 (0-arg)
    void EntranceArrivalHook(i32 a, i32 b); // thunk_FUN_0044d060 (2-arg; arrival commit)

    // ---- the CGrunt method cluster ----
    // (~CGrunt / SerializeMove / Activate / UserLogicVfunc9 / MovingSlot16
    // are the vtable slots declared at the top of the class.)
    void EnsureStruckSlot(const char* key); // @0x57b70 lazily build/play the +0x424 sample
    i32 UpdateEntranceAnim();               // @0x690a0 entrance-anim/arrival update step
    void ApplyMoveKind(i32 v);              // @0x57100 (thunk_0x3c29) 1-arg move-kind apply
    i32 Save(CGruntArchive* ar);            // @0x53f90 serialize
    // @0x555e0 (4856 B; body in GameStateRecordLoad.cpp) - the game-state-record
    // load counterpart of Save: SerializeMove's mode-7 arm, dispatched on this
    // same grunt.
    // (Load @0xd8060 is CPlay::SyncRead2f7c, the
    //  play-state read serializer; SyncState calls it on the PLAY state. The
    //  CGrunt attribution was a same-offsets overlay; see Play.cpp.)
    i32 LoadStateRecord(CGruntArchive* ar);
    void ClearAllSprites();    // @0x4b240
    i32 CommitArrival();       // @0x4b130
    void ClearSubA();          // @0x57c10
    void ClearSubB();          // @0x57ce0
    void ReapplyVoiceParams(); // @0x57d10 replay both voices on the registry gate
    void DestroyAnims();       // @0x57d80
    // @0x31c70 (ret 4) - write the grunt's HUD tile coords (m_10->m_5c/m_60 >> 5)
    // into the caller's {x,y} out slot and return it.
    GruntTilePos* GetTilePos(GruntTilePos* out); // 0x31c70 (out-of-line in Grunt.cpp)
    // @0x57c40 (ret 4) - lazily build + play the grunt's struck-voice sample for the
    // given sound key (stored into the +0x428 slot ClearSubB frees).
    void EnsureStruckVoice(const char* key);
    // DestroyAnims' two teardown steps ARE ClearSubA/ClearSubB (0x57c10/0x57ce0),
    // declared above - no separate AnimTeardownA/B shadow.
    // Legacy placeholder decls retained: CommitArrival now calls the real creators +
    // SetEntrancePos, but dropping these shifts the unit's symbol set and drifts an
    // unrelated function's fuzzy score (matching-patterns.md symbol-set sensitivity).
    void ArrivalClaim(i32 a, i32 b);
    void ArrivalHook0();
    void ArrivalHook1();
    void ArrivalHook2();
    void ArrivalHook3();
    void ArrivalHook4();
    void ArrivalHook5();
    i32 CanShowStamina();              // @0x514a0
    void SetEntrancePos(i32 a, i32 b); // @0x4d060 (ret 8)
    void EntranceTileOffset(i32* out); // @0x56f80 (ret 4) adjacent-tile pixel pos
    void ComputeFacing(double dt);     // @0x57060 (ret 8)
    i32 ResetGeometry();               // @0x616e0
    void DispatchVtbl24();             // 0x6b260 (out-of-line in Grunt.cpp)

    void PlayMoveSound(i32 x, i32 y);              // @0x511b0 (ret 8)
    void PlaySound(i32 range, CGruntVoiceRec rec); // @0x4ac10 (ret 0x10) external
    void OnStruck(i32 wasHit);                     // @0x588f0 (ret 4)
    i32 ResolveArrivalNeighbor();                  // @0xf26f0 (ret 0)
    void RearmEntranceDrop();                      // @0x68370 (ret 0)

    // ---- the move/timer record serializer (@0x53b80, ret 0x10) ----
    // SerializeMove(ar, mode) drives the grunt move/idle-timer state through an
    // archive whose Read/Write are vtable slots +0x2c/+0x30. mode 4 = save, 7 =
    // load. The eight 16-byte (double-pair) records at +0x810..+0x880 stream
    // directly; the sub-records at +0x150/+0x43c/+0x278/+0x308/+0x890..+0x8c0
    // serialize through their own engine helpers (external/reloc-masked).
    // (SerializeMove is the vtable slot-1 override, declared at the top of CGrunt.)
    // The head sub-serializer is the inherited CUserLogic::SerializeMove (0x16e7f0); the mode-4/7
    // pre-steps are CGrunt::Save (0x53f90) / CGrunt::Load (0x555e0) - all
    // real bound callees now.

    // ---- grunt movement / anim-name dispatch state machines (this TU) ----
    // The 5 big per-pose/anim-name resolution state machines: each resolves the
    // grunt's current anim name (via g_typeColl) and dispatches on its
    // single-letter type code (A/D/I/G/L/P/O/Q/J/N/M/K), driving the grunt's
    // movement/arrival state + occupied-coord recycle + a re-latch of m_14->m_1c.
    // The arrival/update dispatch trio (CGrunt methods - every member offset they
    // touch is in this layout). All three drive the grunt's
    // arrival/entrance bookkeeping + the occupied-slot recycle.
    i32 ArrivalRecycle(i32 a, i32 b, i32 mode, i32 d, i32 e); // @0x59230 (ret 0x14)
    // @0x597a0 (ret 0x20) - the combat-hit megahandler (GruntCombat.cpp): conversion
    // hit / death-touch damage, the hit/block launch-cue resolve, the knockback
    // direction-octant resolver + occupied-coord recycle. (Every offset it touches is
    // this layout - m_31c, m_400..m_410.)
    i32 LoadGruntCombatAnimations(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7);
    // (Activate is the vtable slot-6 override, declared at the top of CGrunt.)
    i32 UpdateArrival(i32 a1, i32 a2); // @0x62110 (ret 0x8)

    void StepArrivalDrop(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f); // @0x4b370 (ret 0x18, /GX)
    i32 StepGruntMovement(); // @0x4c170 (ret 0)         - the per-tick move step
    i32 StepAnimDispatchA(i32 a, i32 b, i32 c, i32 d); // @0x52fb0 (ret 0x10)
    // (MovingSlot16 is the vtable slot-16 override, declared at the top of CGrunt.)
    i32 StepAnimDispatchB(); // @0x6a6d0 (ret 0)
    // @0x637a0 (ret 0) - the I-code entrance re-stamp dispatch step: D/L reject,
    // reset the +0x8c0 struck timer, on the "I" anim re-notify the tile mgr, then
    // (if the grunt's head tile / HUD point is unobstructed) re-latch a fresh anim
    // set and re-stamp the first entrance-cell frame.
    i32 StepEntranceReinit();
    // @0x67850 (ret 0) - the entrance-move update step: drive the geometry source,
    // gate on the armed-but-not-running sub-player, resolve the current anim name
    // (scratch form), re-latch on "D", create the HUD stat sprites on arrival, then
    // dispatch the +0x1a0 move mode.
    i32 RunEntranceMove();
    // @0x64540 (ret 0) - the anim-code "C" act handler (PROVEN: RegisterActs_644af0
    // @0x5be30 stores its ILT thunk 0x13cf into the g_reg_644af0 registry under the
    // "C" key @0x60cc90; RunAct dispatches those PMFs on the grunt). Advance the
    // entrance player's cursor; once arrived-and-idle: if the death kind is the
    // warp (m_deathType == 0xc) and "WORLDZ\LEVEL%i" (level+100, the secret level)
    // exists in the level bank, post the level-switch command (WM_COMMAND 0x807f)
    // to the game window; then (unless m_36c suppresses) notify the owner tile
    // cell and retire the entrance player. (Ex CUserLogic::winapi_064540_PostMessageA
    // + the CWarpLeaf/CWarpM154/CWarpMgr/CWarpMgrWnd/CWarpLevelReg views - every
    // offset is this layout: m_154/m_tileOwnerHi/Lo/m_tileMgr/m_deathType/m_36c.)
    i32 StepWarpExit(); // @0x64540

    // The engine helpers these machines call (all external/no-body, reloc-masked;
    // modeled as __thiscall methods on the grunt so `mov ecx,this; ...; call`
    // falls out). Names describe the observed effect, not a recovered symbol.
    i32 IsDropReady(i32 a = 0); // thunk_0x17df (drop-ready predicate; 1-arg __thiscall)
    void ApplySetState1(i32 v); // thunk_0x4322 (1-arg state apply)
    i32 SetMoveStateA(i32 v, i32 a, i32 b, i32 c); // thunk_0x3bd9 (4-arg; nonzero = re-roll)
    void SetMoveStateB(i32 v, i32 a, i32 b, i32 c, i32 d); // thunk_0x1401 (5-arg-ish)
    void EmitMoveCueQ(i32 a);                              // thunk_0x4336 (1-arg cue/state)
    void EmitMoveCueShort(i32 a, i32 b, i32 c);            // thunk_0x1163 (3-arg cue on m_10)
    void ReseedIdleReset(i32 a, i32 b, i32 c);             // thunk_0x136b (3-arg idle reset)
    // Attack-fire step (UserLogicVfunc7 @0x61cb0, ProjectileUpdate.cpp) helpers
    // (external/no-body thunks, reloc-masked; names = observed roles):
    void GetSpawnPos(i32* out); // thunk_0x1a73 (TimeBomb placement pos, {x,y} out-pair)
    void FinishAttackPowered(); // thunk_0x3dd7 (finish-tail hook when m_poweredUp is set)
    void NotifyAttackImpact();  // thunk_0x22de (impact hook when m_healthSprite exists)
    // Melee hit delivery: the attacker's fire step runs this ON THE TARGET grunt
    // (the neighbor-cell m_grid occupant) with the attacker's tool kind, cell and
    // screen pos; the target's reaction machinery takes over. thunk_0x1bf9, 8 args.
    void
    TakeHit(i32 kind, i32 a2, i32 ownerHi, i32 ownerLo, i32 px, i32 py, i32 a7, i32 attackerKind);
    void OnMoveFinishA(i32 a);             // thunk_0x3ea4 (1-arg finish)
    void CommitMoveA(i32 a, i32 b, i32 c); // thunk_0x3dfa (3-arg move commit)
    void StepCoordTick();                  // thunk_0x245a (0-arg coord tick)
    void OnCoordCommit(i32 a);             // thunk_0x1e47 (1-arg commit)
    void NotifyDrop();                     // thunk_0x119a (0-arg drop notify)
    i32 ProbeRetry();                      // thunk_0x3c0b (retry predicate)
    void OnReanchor(i32 a);                // thunk_0x3cce (1-arg reanchor)
    void StepDropApply();                  // thunk (drop-apply tail)
    i32 ApplyMoveMode(i32 v);              // thunk_0x3b75 -> 0x50ca0 (the >=0x32 / <0x17 mode arm)

    // ---- chunk-2 attributed targets (RearmAttack family + entrance-move tail) ----
    // @0x5b570 (ret 8) - begin the grunt's attack/combat reaction: gated on the
    // entrance being committed (m_1fc) AND the current anim NOT being "F" AND
    // m_stamina>=0x64; fires the directional move-sound, latches the powered-up /
    // combat-timer state, builds the HUD health sprite, and re-arms the ATTACK2 anim.
    i32 BeginAttack(i32 a, i32 b); // @0x5b570
    // @0x61940 (ret 8) - re-arm the grunt's attack/struck anim by entrance-reason:
    // gated on m_entranceReason<0x17; latches neighbor col/row, re-latches the "F"
    // anim set, switches on (m_entranceReason-2) to pick the powered/random branch,
    // latches the combat-timer state, fires the focused-grunt drop cue when visible,
    // marks HUD dirty, drives the ATTACK1/ATTACK2 geometry, re-stamps the cell frame.
    i32 RearmAttackAnim(i32 col, i32 row); // @0x61940
    // @0x61bc0 (ret 0) - the simple ATTACK2 re-arm: re-latch "F" anim set, drive the
    // m_poseAttack2 geometry, re-stamp the entrance-cell frame, set m_214.
    i32 RearmAttackAnim2(); // @0x61bc0
    // @0x67b00 (ret 8) - the grunt-in-radius predicate: given a cell coord (col,row)
    // resolve the occupant grunt via the tile-mgr's 15-wide cell grid, gate it (live,
    // committed, not state 0x36), then test whether the squared tile-distance from
    // this grunt to it is within the (radius-sum)^2 threshold.
    i32 GruntInRadius(i32 col, i32 row); // @0x67b00
    // @0x0f7d90 (body in ObjectTracker.cpp) - the per-tick peer-tracking behavior
    // step (this grunt's; the one retail
    // caller, LoadGruntTuningConstants @0x5d210 - itself data-ref'd in
    // ??_7CGrunt@@6B@ - dispatches thunk 0x2806 on its own `this` @0x5dd4e, and
    // all 12 fields land on CGrunt members at identical offsets). Publish the
    // defended position, then track the nearest board enemy: relay its screen pos
    // to the trigger grid, or (m_dwell > 1000) re-place at its tile + cue 0x366.
    i32 StepPeerTracking(); // @0x0f7d90
    // @0x69fd0 (ret 0) - finish the entrance move: arm the entrance geometry source,
    // gate on the armed-but-not-running sub-player, notify the tile-mgr of the drop
    // (unless m_36c set), then retire the entrance player (m_154->m_8 |= 0x10000).
    i32 FinishEntranceMove(); // @0x69fd0
    // @0x69d60 (ret 0) - the freeze-spell entrance-anim finalize step (DEATHZ_SPARKLE
    // -> idle-delay window -> DEATHZ_UNFREEZE + on-screen entrance cue).
    i32 LoadFreezeSpellAssets(); // @0x69d60
    // @0xec670 (ret 0 -> 1) - the arrival-reposition step: resolve the tile occupant,
    // gate on in-radius, commit its slot (or, when no occupant, re-roll a random
    // in-region target after the idle window elapses) + fire the on-screen entrance cue.
    i32 ResolveArrivalReposition(); // @0xec670
    // @0xf2b20 (ret 0 -> 1) - the multi-state arrival-defender step. Latch the
    // defender position to the last tile, then dispatch on m_2d4 (0/1/2): resolve
    // the cell occupant (the m_tileMgr 15-wide grid in states 1/2, GetOccupant in
    // state 0), gate it (in-radius, committed, settled, on-screen via RectContains),
    // and commit/neighbor-link onto it or, on the no-occupant path, re-roll a random
    // in-region defender target + fire the on-screen entrance cue + reset the idle timer.
    i32 StepArrivalDefense();
    // @0xf8240 (ret 0 -> 1) - the leaner sibling of StepArrivalDefense. Gated on the
    // current anim not being "I"; same m_2d4 (0/1/2) defender dispatch over the grid
    // occupant, but without the m_neighborValid/m_198 CommitTileSlot arms (straight to
    // CommitNeighbor), sets the +0x2ec dwell to 0x1f4 on a state-1 latch, and the
    // state-0 path commits the occupant's tile slot on a rand%100 roll + re-rolls a
    // random in-region target / resets the idle timer.
    i32 StepArrivalDefenseLean();
    // @0xf1c70 (ret 0 -> 1) - the powered-up arrival-defender variant
    // (every offset/helper proves CGrunt). Sets
    // m_arrivalFlags |= 0x40000, then either runs the powered-up release gate
    // (m_poweredUp!=0: FindGridNeighbor + clear-state) or the m_2d4 (0/1/2/3) defender
    // dispatch: GetOccupant/grid-occupant settle + CommitNeighbor, the 4-way
    // StepArrivalDrop tile walk toward m_defenderX/Y, and the on-screen entrance cue.
    i32 StepArrivalDefenseAlt();
    // @0xf60f0 (ret 4, /GX) - the arrival/relocation phase step. Gated on the grunt
    // type name (g_typeColl.Lookup(m_14->m_1c) vs "F"); drives the m_defenderState
    // machine (0/2/4/0x19/0x1a) recomputing the target tile, building the 16 border
    // cells of the 5x5 block into a point accumulator, random-picking a free cell to
    // relocate/arrive on (m_tileMgr TileSwitch / CommitTileSlot2), and recycling the
    // visited-coord nodes onto the shared free list.
    i32 PhaseStep();
    // The grunt per-tick arrival/scan/wander step machines (every offset they touch is a
    // CGrunt member, every helper a CGrunt
    // method - so they are CGrunt methods). All __thiscall ret 4, drive the tile-to-tile
    // move via the tile-mgr grid + the on-screen CueA cue.
    i32 UpdateArrival(); // @0xf0130  (GruntUpdateStep.cpp) "I"-grunt arrival update
    i32 SeekTarget();    // @0xf71c0  (GruntUpdateStep.cpp) seek/scan variant
    i32 WanderStep();    // @0xed9f0  (WanderIdleStep.cpp) idle/wander 6-phase step
    i32 ArrivalScanA();  // @0xecc90  (GruntArrivalScan.cpp) grid-cell box scan
    i32 ArrivalScanB();  // @0xf0e20  (GruntArrivalScan.cpp) live-grunt-list scan
    i32 ArrivalScanC();  // @0xf36a0  (GruntArrivalScan.cpp) 0x10000-flag box scan
    // GetScreenPos (0x29a50, copies m_10->{m_5c,m_60} into the out point) is
    // CUserLogic's own leaf accessor, inherited here - no CGrunt re-declaration
    // (a shadow would emit ?GetScreenPos@CGrunt@@ which does not resolve to 0x29a50).
    // (TileSwitch6 is GONE - it was a SECOND name for TileSwitch @0x4b320, declared
    //  below; the convention conflation is settled __thiscall - see that decl.)
    // @0x6a060 (ret 0) - the SINK/FALL death-finalize step the death-anim loader runs
    // after the entrance-drop notify. External/no-body (reloc-masked here).
    void Step6a060();

    // @0x68520 (ret 0 -> 0) - begin the bomb-grunt run reaction: retire the HUD stat
    // sprites, latch the entrance/struck state, then either re-notify the move or (when
    // re-rolling) pick a random adjacent tile, play the move sound, latch the "M" anim,
    // load RunningTimePerTile, fire the on-screen spawn cue, and re-stamp the cell frame.
    i32 StartBombGruntRun(); // @0x68520

    // @0x5ecd0 (vtable slot-5 override; `ret 4`, void - NO exit materializes eax;
    // the base signature is settled so the OVERRIDE spelling works now). Finalize
    // (direct base call, retail `call 0x3913`) + slot-16 tick, gated anim-code
    // cleanups (L/G -> ClearSubA, off-screen -> ClearSubB), then on the "O" (or
    // scratch-resolved) anim code smoothly interpolate the grunt's HUD position
    // toward the target tile using the per-cell velocity records (m_cells[base]
    // +0x48..+0x60 doubles), clamping on overshoot, and mark the HUD scroll
    // dirty. (Was the plain RVA method RunPositionInterpStep.)
    virtual void FinalizeStep(i32 arg) OVERRIDE;
    // (FinalizeStep is GONE from here - it was a PHANTOM duplicate: nothing ever
    //  defined ?FinalizeStep@CGrunt@@, and 0x8b90 is CUserLogic::FinalizeStep, which
    //  CGrunt INHERITS. Its call site resolves to the base method, cast-free.)

    // --- entrance/arrival per-tick steps (RunAct-dispatched; GruntEntranceArrival.cpp) ---
    // Each advances the entrance geometry sub-player (m_154->m_1a0.Advance),
    // and (once the sub-player is armed-but-not-running: m_1a0.m_28!=0 && m_1a0.m_20==0)
    // runs an arrival/entrance commit driven by the tile-mgr + tile-occupancy board.
    i32 StepEntranceRelatchA(); // @0x62840 (re-latch "A" anim, board-gated commit/HUD)
    i32 StepArrivalReroll();    // @0x63b60 (idle re-roll timer -> on-screen entrance cue)
    i32 StepArrivalCommitA();   // @0x65300 (health-gated tile commit / drop / reset)
    i32 StepArrivalCommitB();   // @0x654b0 (snap+commit then the same board-gated tail)
    i32 StepEntranceRelatchB(); // @0x65c20 (re-latch "D" anim, board-gated icon place)

    // @0x646b0 (ret 0x20, 8 stack args) - the combat-reaction anim-dispatch state
    // machine. MISATTRIBUTED to CUserLogic by proximity bracketing; the +0x1fc gate,
    // g_typeColl grunt anim codes (A/D/I/G/L/P/O/Q/J/N), m_10 HUD dirty bit
    // and "Grunt"/"CombatTimeout" bute section all prove `this` is a CGrunt (CUserLogic
    // is only 0x40 bytes; +0x1fc is impossible). Re-homed here.
    i32 StepCombatReaction(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7);

    // StepCombatReaction's engine thunks (external/no-body, reloc-masked).
    void UpdateCombatTimer();   // call 0x243c (0-arg tail step)
    void OnTileMismatch(i32 v); // call 0x2cb6 (1-arg)
    i32 ForwardCombatStep(
        i32 a0,
        i32 a1,
        i32 a2,
        i32 a3,
        i32 a4,
        i32 a5,
        i32 a6,
        i32 a7
    );                                                 // call 0x1451
    i32 IsInCombatRange(i32 x, i32 y);                 // call 0x3c4c (2-arg predicate)
    void CommitCombatMove(i32 a, i32 b, i32 c, i32 d); // call 0x302b (4-arg)
    // The tile-switch trigger @0x4b320 (thunk 0x1640): scale the (col,row) grid pair
    // to tile-pixel centres (*0x20+0x10) and forward all six args to the engine
    // helper. __thiscall on the grunt - the body
    // never reads `this`, but EVERY retail call site (58 through the thunk) loads a
    // grunt into ecx first (`mov ecx,esi/ebx/ebp` / `mov ecx,[esp+..]`), which only a
    // thiscall member reproduces. Body in Grunt.cpp (callee-cleans 0x18 either way).
    i32 TileSwitch(i32 col, i32 row, i32 flags, i32 a4, i32 a5, i32 a6); // 0x4b320 (thunk 0x1640)

    // The toy/vehicle grunt sprite loader - latch the kind (m_198),
    // reset m_moveMode, seed the m_2b0/m_2c0 region blocks per toy kind, build the
    // "<NAME>GRUNT" namespace, and on a switch tile (0x41/0x42) at the committed
    // tile re-fire ApplySwitch + WireTileSwitchLogic on m_tileMgr.
    i32 LoadVehicleGruntSprites(i32 kind); // 0x50ce0

    // @0xee800 (ret 4, /GX) - the arrival/defender reticle scan (mis-homed under
    // ?winapi_0ee800_IntersectRect_PtInRect@CUserLogic before xref recovery: this
    // is CGrunt - the this-object extends to +0x3f0 and the caller (0x5d210, CGrunt
    // vtable slot 3) runs it on a CGrunt). Snapshots the reach-box grid cells into a
    // local CByteArray, marks the tiles within m_defenderRadius of the defender tile,
    // clips against the tile grid + viewport (PtInRect/IntersectRect), and recycles
    // occupied-coord nodes through g_coordPool. Reconstruction in GruntReticle.cpp.
    i32 ArrivalReticleScan(); // 0xee800

    // CombatCue per-grunt spell effects (external/no-body, reloc-masked):
    //   TeleportMove(dx,dy,a,b) thunk 0x2f3b (ret 0x10; nonzero = moved)
    //   FreezeApply()           thunk 0x28d8 (0-arg freeze)
    i32 TeleportMove(i32 dx, i32 dy, i32 a, i32 b); // 0x2f3b
    void FreezeApply();                             // 0x28d8
};

typedef i32 (CGrunt::*GruntActHandler)();
struct CGruntActEntry {
    GruntActHandler m_fn;
};
SIZE(CGruntActEntry, 0x4);

SIZE_UNKNOWN(GruntBox);
struct GruntBox {
    i32 m_0; // +0x00 x0
    i32 m_4; // +0x04 y0
    i32 m_8; // +0x08 x1
    i32 m_c; // +0x0c y1
};
SIZE_UNKNOWN(GruntSegEnd);
struct GruntSegEnd {
    i32 m_0; // +0x00 x
    i32 m_4; // +0x04 y
};
i32 __stdcall CGrunt_SegBoxOverlap(GruntBox* p, GruntSegEnd* e1, GruntSegEnd* e2);

bool CGrunt_IsSameType(CGrunt* a, CGrunt* b);

void GruntRecycleCoords(CGrunt* g); // 0x343f0
i32 __stdcall GruntTileSwitchImpl(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f);

extern char s_codeD[]; // "D" (0x0060cca4)
extern char s_codeF[]; // "F" (0x0060d2e8)
extern char s_codeH[]; // "H" (0x0060d7fc)
extern char s_codeK[]; // "K" (0x0060d7f8)
extern char s_codeM[]; // "M" (0x0060d7f4)
extern char s_codeN[]; // "N" (0x0060dc04)
extern char s_codeO[]; // "O" (0x0060dc0c)
extern char s_codeQ[]; // "Q" (0x0060dc08)

#endif // SRC_GRUNTZ_GRUNT_H
