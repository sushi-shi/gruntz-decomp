#include <Gruntz/RainCloud.h> // CRainCloud : CPathHazard (canonical; pulls PathHazard.h -> GameRegistry.h)
#include <Gruntz/LightFxMgr.h> // reg->m_logicPump (+0x78): the shade-table pump the fill arg reads
#include <Gruntz/GruntzMgr.h>  // complete CGruntzMgr
#include <Gruntz/SoundState.h> // g_sndEnabled/g_sndCueTag (HitTest's kill-sound gate)
#include <Gruntz/TriggerMgr.h> // canonical CTriggerMgr (m_cmdGrid: CellDispatch @0x6bcb0)
#include <Rez/FrameClock.h>    // g_frameTime/g_killCueClock (strike deadline + cue clock)
#include <Bute/ButeMgr.h>      // g_buteMgr (the RainCloudFlashTime window bute)
#include <rva.h>

VTBL(CRainCloud, 0x001e7324); // vtable_names -> code (RTTI game class)
// ~CRainCloud @0x013340 - the CPathHazard-derived rain-cloud leaf's dtor: no
// destructible members of its own, so it folds the bare CUserLogic teardown (store
// the CUserLogic vptr, inline-destruct the +0x18 link's ~EngStr, store the CUserBase
// vptr; the throwing link forces the /GX EH frame). IDENTITY (vtable-owner probe):
// ??_7CRainCloud @0x1e7324 (RTTI-named, <Gruntz/RainCloud.h>) slot 0 -> ILT thunk ->
// the sdd 0x13310 -> THIS body (it was once misbound as ~CPathHazard).
//
// IMPLICIT (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B): a
// user-declared `~CRainCloud() {}` emits the leaf-vptr restamp, and the CWapX base
// EH state blocks the dead-store elision that used to hide it. THIS obj emits
// ??_7CRainCloud -> ??_G -> the implicit ??1 COMDAT (the ctor above is what needs
// the vtable), so the pin resolves here - PathHazard.cpp never emits it.
RVA_COMPGEN(0x00013340, 0x44, ??1CRainCloud@@UAE@XZ)

// CRainCloud::HitTest @0x0b4640 (slot-20 override; ??_7CRainCloud[20] is the
// body's ONLY referent - the base default @0x13230 is what CPathHazard/CUFO
// keep) - arm the strike-window timer (deadline =
// now, window = bute RainCloudFlashTime), fire the cue gate, and play the
// "LEVEL_CLOUDHAZARDKILL" positional sound on the bound object when it is on-screen
// and the per-emitter cooldown has elapsed.  Integer-only; returns 1.  __thiscall,
// 2 args.
// @early-stop
// ~95%: code bytes byte-exact; residual is the same TU-wide reloc-naming artifact
// SiblingTick carries (the obj names g_gameReg as _g_mgrSettings and
// g_strikeClock as _g_645588). Logic byte-for-byte correct.
// @interleaver CRainCloud::HitTest emitted-in <boundary: PathHazardActReg.cpp
// RegisterActs_646250 @0xb3cc0 (before) + CRainCloud::SerializeMove @0xb4cb0
// (after)>. A /Gy first-use COMDAT the linker scattered between OTHER units.
RVA(0x000b4640, 0x104)
i32 CRainCloud::HitTest(i32 a, i32 b) {
    m_strikeArmed = 1;
    m_strikeWindow = static_cast<i64>(
        static_cast<u32>(g_buteMgr.GetDwordDef("Hazardz", "RainCloudFlashTime", 0x7d0))
    );
    m_strikeDeadline = static_cast<i64>(static_cast<u32>(g_frameTime));
    g_gameReg->m_cmdGrid->CellDispatch(a, b, 9, -1);

    CWwdGameObjectA* obj = m_object;
    CGruntzMgr* reg = g_gameReg;
    i32 y = obj->m_screenY;
    i32 x = obj->m_screenX;
    if (x < reg->m_viewOriginR && x >= reg->m_viewOriginL && y < reg->m_viewOriginB
        && y >= reg->m_viewOriginT) {
        CDDrawSubMgrLeafScan* host = reg->m_world->m_soundRegistry;
        if (host->m_emitGate == 0) {
            void* out_ob = 0;
            host->m_10.Lookup("LEVEL_CLOUDHAZARDKILL", out_ob);
            LeafCue* out = static_cast<LeafCue*>(out_ob);
            if (out != 0) {
                i32 enabled = g_sndEnabled;
                i32 tag = g_sndCueTag;
                if (enabled != 0) {
                    u32 now = g_killCueClock;
                    if (static_cast<u32>((now - out->m_14)) >= out->m_18) {
                        out->m_14 = now;
                        out->m_10->ConfigureItem(tag, 0, 0, 0);
                    }
                }
            }
        }
    }
    return 1;
}

RVA(0x000b49b0, 0xa8)
CRainCloud::CRainCloud(CGameObject* obj) : CPathHazard(obj) {
    CWwdGameObjectA* o = m_object;
    i32 n = reinterpret_cast<i32>(g_gameReg->m_logicPump->m_tables[5]); // reg->+0x78->+0x28
    o->m_drawActive = 1;
    o->m_drawFillCmd = 0x7;
    o->m_drawFillArg = n;
    m_value = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry("LEVEL_RAINCLOUD", 0);
    m_object->m_area.left = 1;
    m_object->m_area.right = 1;
    m_object->m_area.top = 1;
    m_object->m_area.bottom = 1;
}

// CRainCloud::SerializeMove @0xb4cb0 (slot-1 override; ??_7CRainCloud[1] is the
// body's ONLY vtable referent - it was misattributed as "CUFO::Method_b4cb0"):
// chain the CPathHazard leg, then on the tag-8 re-seed pass restore the ctor's
// draw-fill triple (fill cmd 7 + the logic pump's shade table) - the exact
// stores CRainCloud::CRainCloud performs at spawn.
RVA(0x000b4cb0, 0x56)
i32 CRainCloud::SerializeMove(CFileMemBase* stream, i32 tag, i32 c, i32 d) {
    if (!CPathHazard::SerializeMove(stream, tag, c, d)) {
        return 0;
    }
    if (tag == 8) {
        CShadeTable* x = g_gameReg->m_logicPump->m_tables[5];
        CWwdGameObjectA* o = m_object;
        o->m_drawActive = 1;
        o->m_drawFillCmd = 7;
        o->m_drawFillArg = reinterpret_cast<i32>(x);
    }
    return 1;
}
