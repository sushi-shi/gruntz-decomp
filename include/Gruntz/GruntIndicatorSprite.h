// GruntIndicatorSprite.h - shared layout for the in-game grunt-indicator sprite
// objects (CGruntSelectedSprite / CGruntPowerupSprite / CGruntToySprite).
//
// Each is a CUserLogic-derived game object (vftables 0x5e705c / 0x5e70b4, the
// CUserLogic / CUserBase pair - proven by each class's 0x44 leaf dtor stamping
// 0x5e705c then 0x5e70b4 and tearing down the +0x18 link via the embedded
// ~EngStr, the SAME shape every UserLogic leaf dtor matches, e.g. ~CTimeBomb
// @0x012a70 / ~CInGameIcon @0x011d00). So despite the trace's "CGruntSprite"
// label, the dtor RTTI says these are plain CUserLogic leaves; the 0x44 is a
// DESTRUCTOR (the CUserLogic-folded leaf-dtor archetype), NOT a ctor.
//
// The accessor/updater methods anchor at +0x54/+0x58 (a tile coord pair) and
// poke the bound renderable (CUserLogic::m_10) + the +0x38 game object. They
// resolve the current grunt for the (m_54,m_58) cell from the global game
// registry's grunt table and copy that grunt's screen position into the bound
// renderable so the indicator tracks the grunt.
//
// Field names are placeholders (m_<hexoffset>); only OFFSETS + code bytes are
// load-bearing (campaign doctrine).
#ifndef GRUNTZ_GRUNTINDICATORSPRITE_H
#define GRUNTZ_GRUNTINDICATORSPRITE_H

#include <rva.h>
#include <Gruntz/GameRegistry.h>

#include <Gruntz/UserLogic.h> // CUserLogic : CUserBase, EngStr, CGameObject

// ---------------------------------------------------------------------------
// The level layer-clamp holder reached as a grunt's m_10->m_194. The Toy
// updater clamps the grunt's layer index into [m_64, m_68] and maps it through
// the +0x14 layer table into m_10->m_198. Only the touched offsets modeled.
// ---------------------------------------------------------------------------
struct CGruntLayerHolder {
    char m_pad00[0x14];
    i32* m_layerTable; // +0x14  the per-index layer table
    char m_pad18[0x64 - 0x18];
    i32 m_layerLo; // +0x64  lo layer bound
    i32 m_layerHi; // +0x68  hi layer bound
};

// ---------------------------------------------------------------------------
// CGruntEntry / CGruntRenderable: FOLDED (VT1) onto their real classes - they were
// views, not classes. One grunt slot in the registry's grunt table (base
// g_gameReg->m_cmdGrid + 0x1c, indexed by m_cellX*15 + m_cellY) is a `CGrunt*`, and
// the "renderable" it hangs at +0x10 is the plain `CGameObject*` every CUserLogic
// already calls m_object. Proof:
//   CGruntEntry == CGrunt      - the table slot was documented as "a dword pointer to
//     a grunt"; +0x10 is CUserLogic::m_object (inherited), +0x198/+0x1d8 are CGrunt's
//     m_198/m_arrived, and the slot-16 stat getters read +0x3f0/+0x3f4/+0x3f8 =
//     CGrunt's m_stamina/m_toyTime/m_wingzTime (identical offsets AND names).
//   CGruntRenderable == CGameObject - identical names at identical offsets
//     (m_screenX +0x5c, m_screenY +0x60) plus +0x4c/+0x50/+0x58 =
//     m_drawFillArg/m_drawFillCmd/m_drawActive, which the same TUs ALREADY spell the
//     CGameObject way; and every use site reached it by casting `(CGruntRenderable*)
//     m_object` - a CGameObject* - which is the cast that proves the type.
// The three role-fields the view carried (+0x190/+0x194/+0x198) are now named union
// members on CGameObject in <Gruntz/UserLogic.h>. CGruntLayerHolder above STAYS: its
// +0x14/+0x64/+0x68 shape matches no existing class (it is NOT CGameObjLayer, whose
// +0x10/+0x14 z-clamps disagree), so it is a real distinct type, not a view.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// CGameRegistry - the minimal game-registry view the indicator updaters use.
// Same singleton as CGameReg (?g_gameReg@@3PA...@@A @ 0x64556c); modeled with
// the grunt table at +0x68 and the bute-set table at +0x78 the powerup setter
// reads. Declared with the registry's own type so the data ref reloc-masks.
//   +0x68  m_68 : grunt table base (entries at +0x1c, dword stride)
//   +0x78  m_78 : the bute lookup table the powerup setter indexes
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// A bound-object sub-object on the +0x38 game object: at +0x1a0 sits a helper
// whose Sync(arg) (0x15c360, __thiscall ret 4) flushes/advances the indicator's
// draw state. g_engineFrameDelta (0x6bf3bc, BSS) is the draw-delta the arg carries; it is a
// single view (extern "C" u32) shared with Projectile/CTeleporter/CGruntPuddle.
// Both are external/no-body so the call + the load reloc-mask.
// ---------------------------------------------------------------------------
struct CIndicatorSyncHelper {};
extern "C" u32 g_engineFrameDelta; // canonical _g_6bf3bc @ 0x6bf3bc (draw-delta mirror)

// The bute store the powerup setter seeds the "A" node from (g_buteTree.Find).
// Also the shared activation-name registry types/globals (CActColl / CVariantSlot /
// g_actCache / g_retAddrBreadcrumb / GetRetAddr / g_buteTree / g_nextActId / s_actKeyA)
// the RegisterActs id->entry resolve uses.
#include <Bute/ButeMgr.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h> // the shared CActReg coordinate-registry archetype
// NOTE: the g_gameReg singleton is NOT declared here - a header extern would force the
// CGameRegistry view on every includer, clashing with the WwdGameReg view Grunt.cpp uses
// (which now includes the concrete HUD-sprite headers). The consuming .cpp files declare
// `extern "C" CGameRegistry* g_gameReg;` locally.

// ---------------------------------------------------------------------------
// CIndicatorActReg - the per-class activation-coordinate registry singleton each
// indicator sprite owns (the SAME shape as CParticlez's g_partColl @0x644870 /
// CGruntVoice's g_vactColl @0x6514d8). Each class has a tiny free init function
// (the trace's 0x15 method) that constructs its registry over the fixed
// coordinate range [2000, 2010] via the shared ctor FUN_00408710 (__thiscall,
// ret 8: stores the registry vtable 0x5e70fc and a base CDWordArray, m_1c =
// count). The ctor is external/no-body so the call reloc-masks; modeled as a
// Construct(lo,hi) method on the registry so `mov ecx,&g_reg; push hi; push lo;
// call ctor` falls out byte-exact.
//
// CIndicatorActReg is the shared <Gruntz/ActReg.h> CActReg archetype (the fast
// [m_lo, m_hi] range path yields m_base + (id-m_lo)*m_stride; the slow Find
// (0x16da80) / GetRetAddr (0x16d990) / coll2 Insert (0x16d850) path yields m_cur).
// It was a per-file duplicate of that layout + ResolveEntry; it keeps its own
// placeholder name so the DATA-pinned globals below are unchanged.
// ---------------------------------------------------------------------------
struct CIndicatorActReg : public CActReg {};

// These four registry singletons are DATA-bound in their owning sprite .cpp files
// (GruntHealthSprite.cpp / GruntPowerupSprite.cpp / GruntSelectedSprite.cpp /
// GruntToySprite.cpp): labels.py scans DATA() only in the TU source, not headers.
extern CIndicatorActReg g_healthActReg;   // 0x644d80
extern CIndicatorActReg g_powerupActReg;  // 0x644d30
extern CIndicatorActReg g_selectedActReg; // 0x644da8
extern CIndicatorActReg g_toyActReg;      // 0x644d58

#endif // GRUNTZ_GRUNTINDICATORSPRITE_H
