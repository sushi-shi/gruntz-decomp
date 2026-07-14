// SpotLightCtor.cpp - the 1-arg CSpotLight constructor (0xb1200), re-homed from
// src/Stub/SpotLight.cpp (C:\Proj\Gruntz).
//
// CSpotLight : CUserLogic (vftable 0x5e75bc). The ctor folds the shared inline
// CUserLogic(CGameObject*) base init (link + RegisterLogicTypesOnce + the three
// AddLogic* + the m_04..m_3c stores), stamps its own vptr, re-resolves the bute
// "A" node into the bound object's aux, snaps the object's screen position onto the
// 0x20 grid and seeds the rotation offsets (m_70/m_78/m_60/m_68/m_80/m_88), then
// reads the SpotLightTime / settings-table tuning and seeds the per-tick state.
//
// The standalone ctor lives in its OWN TU so the existing flat CSpotLight carcass +
// the matched Update (src/Gruntz/SpotLight.cpp) are untouched. Field names are
// placeholders; the OFFSETS + code bytes are the load-bearing facts. The throwing
// CUserBaseLink in the CUserLogic base forces the /GX EH frame -> eh.
#include <Mfc.h>
#include <Io/FileMem.h> // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Gruntz/MovingLogicBase.h> // CMovingLogicBase::Serialize (0x16e7f0) - shared serialize chain
#include <Gruntz/UserLogic.h>       // CUserLogic / CGameObject base init + g_buteMgr
#include <Bute/ButeMgr.h>           // CButeTree / CButeMgr
#include <Gruntz/GameRegistry.h>    // canonical *0x24556c singleton (color table via m_78)
#include <Gruntz/LightFxMgr.h>  // CLightFxMgr - m_logicPump's real class (m_tables[10] @+0x14)
#include <Gruntz/SpriteFactory.h> // CSpriteFactory - m_world->m_8 (embedded GruntObjMap m_objMap @+0x48)
#include <Gruntz/TriggerMgr.h> // CTriggerMgr::CellDispatch (0x6bcb0) - g_gameReg->m_cmdGrid cue dispatch
#include <Gruntz/ActReg.h>        // CActReg coordinate registry (ResolveEntry) for RunAct
#include <Gruntz/SerialArchive.h> // CSerialArchive (Read @+0x2c / Write @+0x30)
#include <Gruntz/SerialObjRef.h>  // the +0x34 serialized-object-reference (Chain @0x8c00)
#include <math.h>                 // sin / cos (the Tick rotation)
#include <rva.h>

// The bute store the "A" activation node is resolved through (g_buteTree @0x6bf620,
// Find @0x16d190); declared extern so the call reloc-masks (the Stub TU owns it).
DATA(0x002bf620)
extern CButeTree g_buteTree;

// The two .rdata scale doubles the per-tick rate folds in (0x5ea3f0 numerator /
// 0x5ea3f8 multiplier). Reloc-masked; the literal value is irrelevant to the match.
DATA(0x001ea3f0)
const double g_spotRateNum = 3.1415927; // 0x5ea3f0
DATA(0x001ea3f8)
const double g_spotRateMul = -1.0; // 0x5ea3f8

// The per-frame light-color table is the canonical CLightFxMgr (g_gameReg->m_logicPump,
// +0x78; <Gruntz/LightFxMgr.h>): the ctor indexes its m_tables[10] shade-table array
// (+0x14) by m_object->m_11c and stores the CShadeTable* as the draw-fill arg; the
// alpha-blend gate is the registry's m_134 discriminator. (The ex CSpotMgrTable view
// is dissolved onto the real class.)
extern "C" CGameRegistry* g_gameReg;

// CSpotLight : CUserLogic is modeled in <Gruntz/SpotLight.h> (canonical header,
// included below). The light eyecandy logic: own fields begin past the CUserLogic
// base (+0x40) - the rotation/offset doubles + the per-tick state ints.
#include <Gruntz/SpotLight.h>

// CSpotLight::~CSpotLight @0x13040 - empty vtable-anchor dtor; folds the CUserLogic
// teardown (the /GX leaf-dtor archetype). Gives CSpotLight a real vftable so the
// ctor's vptr store falls out.
RVA(0x00013040, 0x44)
CSpotLight::~CSpotLight() {}

RVA(0x000b1200, 0x2cb)
// @early-stop
// x87 fp-stack-schedule wall (docs/patterns/x87-fp-stack-schedule.md, topic:wall):
// the /GX EH frame, the folded CUserLogic(obj) base init, the bute "A" re-resolve,
// the 0x20-grid snap + m_124/m_120 adjust, the SpotLightTime/settings tuning and
// every integer field store are byte-faithful; the residual is the rotation block
// that derives m_60/m_68/m_80/m_88 from m_70/m_78 - its fld/fsub/fxch stack
// ordering is not steerable from C (same wall as CSpotLight::Update @0xb1ee0).
// Logic complete; deferred to the final sweep.
CSpotLight::CSpotLight(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->m_flags |= 2;

    i32 ax = (m_object->m_screenX & ~0x1f) + 0x10;
    i32 cx = (m_object->m_screenY & ~0x1f) + 0x10;
    m_70 = (double)ax;
    m_78 = (double)cx;
    i32 nx;
    if (m_object->m_124 == 0) {
        nx = ax - 0x20;
    } else {
        nx = ax - m_object->m_124 * 32;
    }
    m_object->m_screenX = nx;
    m_object->m_screenY = cx;
    m_60 = (double)nx;
    m_68 = m_78;
    if (m_object->m_latchedAnimId != 0xcf850) {
        m_object->m_latchedAnimId = 0xcf850;
        m_object->m_flags |= 0x20000;
    }
    m_80 = m_70 - m_60;
    m_88 = m_78 - m_68;

    u32 v;
    if (m_object->m_120 == 0) {
        v = g_buteMgr.GetDwordDef("Hazardz", "SpotLightTime", 0xbb8);
    } else {
        v = m_object->m_120;
    }
    m_58 = g_spotRateNum / (double)(u32)v;
    if (m_object->m_12c == 1) {
        m_58 = m_58 * g_spotRateMul;
    }
    if (m_object->m_118 == 1) {
        m_90 = 3.1415927;
    } else {
        m_90 = 0;
    }
    i32 looked = (i32)g_gameReg->m_logicPump->m_tables[m_object->m_11c];
    m_object->m_drawActive = 1;
    m_object->m_drawFillCmd = 7;
    m_object->m_drawFillArg = looked;
    m_98 = 0;
    m_object->m_areaL = 0;
    m_object->m_areaR = 0;
    m_object->m_areaT = 0;
    m_object->m_areaB = 0;
    m_9c = -1;
    m_a0 = -1;
    m_a4 = 0;
    if (g_gameReg->m_134 == 1) {
        m_a4 = 1;
    }
}

// CSpotLight's activation-dispatch registry (the untyped .data CActReg @0x646188,
// declared in LogicActRegistrars.cpp; extern here so the loads reloc-mask). Its
// entries hold the per-id handler (a code ptr dispatched __thiscall on this).
extern CActReg g_actReg_646188; // 0x646188
struct CSpotActEntry {
    i32 (CSpotLight::*m_fn)();
};

// CSpotLight::RunAct @0x0b1630 - the class's vtable slot-4 (UserLogicVfunc2) body:
// resolve the registry entry for id and, if a handler is bound, re-resolve and run
// it as a PMF on this, else return the entry pointer. Same archetype as
// CAniCycle::RunAct (ResolveEntry inlined twice). NOTE: this IS CSpotLight's real
// slot-4 override (data-ref ??_7CSpotLight@@6B@+0x10), but the fat base models slot
// 4 with the no-arg UserLogicVfunc2() placeholder, so the int-arg real shape can't
// spell OVERRIDE - kept a plain method; the leaf vtable slot stays base-attributed.
RVA(0x000b1630, 0x102)
i32 CSpotLight::RunAct(i32 id) {
    CSpotActEntry* e = (CSpotActEntry*)g_actReg_646188.ResolveEntry(id);
    if (e->m_fn != 0) {
        return (this->*((CSpotActEntry*)g_actReg_646188.ResolveEntry(id))->m_fn)();
    }
    return (i32)e;
}

// SerializeMove's +0x98 focus slot is a serialized object reference. The referent
// carries its id at +0x188 and a type tag via vtable slot 8 (+0x20). The Read path
// resolves the id through the world sprite factory's embedded id->object map
// (g_gameReg->m_world->m_8->m_objMap, the canonical GruntObjMap @+0x48, Lookup
// @0x1b8760), keeping the object only when its type tag is 5. The per-serialize round
// counter g_serialCounter bumps each pass. (The ex CSpotResMgr view is dissolved onto
// the real CSpriteFactoryHolder / CSpriteFactory.)
extern i32 g_serialCounter; // 0x629ad0
struct CSpotFocus {
    virtual i32 s0();
    virtual i32 s1();
    virtual i32 s2();
    virtual i32 s3();
    virtual i32 s4();
    virtual i32 s5();
    virtual i32 s6();
    virtual i32 s7();
    virtual i32 GetType(); // slot 8 (+0x20): the type tag (5 == a valid focus)
    char m_pad04[0x188 - 0x04];
    i32 m_188; // +0x188  the serialized id
};

// CSpotLight::SerializeMove @0x0b2050 (vtable slot 1) - chain the base + the +0x34
// object-reference, then transfer the light's own state through the archive keyed on
// the serialize mode: 4 = Write / 7 = Read the eight rotation/offset doubles
// (m_58..m_90) + the +0x98 focus reference (serialize/resolve its id) + the three
// tail ints (m_9c/m_a0/m_a4); 8 = re-apply the level's draw-fill color.
// @early-stop
// 99.96% - entropy-tail regalloc coin-flip (topic:regalloc): the whole body (the two
// chains, the mode switch, all sixteen 8-byte double transfers, the g_serialCounter
// bump, the mode-8 draw-fill, the Write serialize-id, and the Read MFC CMapPtrToPtr
// Lookup + branchless `(GetType()==5)?obj:0` resolve) is byte-faithful. Sole residual:
// the Write-id load `id = m_98->m_188` uses ecx here vs eax in retail (a 1-byte
// callee-saved reg choice), not source-steerable under MSVC5 /O2.
RVA(0x000b2050, 0x295)
i32 CSpotLight::SerializeMove(CGruntArchive* arc, i32 mode, i32 c, i32 d) {
    if (((CMovingLogicBase*)this)->Serialize((CSerialArchive*)((i32)arc), mode, c, d) == 0) {
        return 0;
    }
    if (((CSerialObjRef*)&m_34)->Chain((CSerialArchive*)arc, mode, c, (CSerialObj*)d) == 0) {
        return 0;
    }
    CGameRegistry* reg = g_gameReg;
    CSerialArchive* s = (CSerialArchive*)arc;
    switch (mode) {
        case 4: // Write
            s->Write(&m_58, 8);
            s->Write(&m_60, 8);
            s->Write(&m_68, 8);
            s->Write(&m_70, 8);
            s->Write(&m_78, 8);
            s->Write(&m_80, 8);
            s->Write(&m_88, 8);
            s->Write(&m_90, 8);
            g_serialCounter++;
            {
                i32 id = 0;
                if (m_98 != 0) {
                    id = ((CSpotFocus*)m_98)->m_188;
                }
                s->Write(&id, 4);
            }
            s->Write(&m_9c, 4);
            s->Write(&m_a0, 4);
            s->Write(&m_a4, 4);
            break;
        case 7: // Read
            s->Read(&m_58, 8);
            s->Read(&m_60, 8);
            s->Read(&m_68, 8);
            s->Read(&m_70, 8);
            s->Read(&m_78, 8);
            s->Read(&m_80, 8);
            s->Read(&m_88, 8);
            s->Read(&m_90, 8);
            g_serialCounter++;
            {
                i32 id;
                s->Read(&id, 4);
                CSpotFocus* out = 0;
                i32 resolved = reg->m_world->m_8->m_objMap.Lookup((void*)id, (void*&)out);
                if (resolved != 0) {
                    if (out == 0) {
                        resolved = 0;
                    } else {
                        resolved = (out->GetType() == 5) ? (i32)out : 0;
                    }
                }
                m_98 = resolved;
                if (m_98 == 0 && id != 0) {
                    return 0;
                }
            }
            s->Read(&m_9c, 4);
            s->Read(&m_a0, 4);
            s->Read(&m_a4, 4);
            break;
        case 8: { // re-apply the level draw-fill color
            CGameObject* o = m_object;
            i32 fill = (i32)reg->m_logicPump->m_tables[o->m_11c];
            o->m_drawActive = 1;
            o->m_drawFillArg = fill;
            o->m_drawFillCmd = 7;
            break;
        }
    }
    return 1;
}

// The per-tick laser-update externs (all reloc-masked): the hit/spawn probe (0x32ce),
// the per-cell sound-cue emitter (0x2e96, __thiscall on g_gameReg->m_68), the target's
// activate (0x4322, __thiscall on the probed object), and the sound-play (0x1360d0).
extern "C" void* Probe_32ce(i32 x, i32 y, void* rect, i32* outA, i32* outB, i32 flag);
extern "C" void Activate_4322(void* target, i32 f);
extern "C" i32 SoundPlay_1360d0(i32 a, i32 b, i32 c, i32 d);
// The per-cell sound-cue emitter is CTriggerMgr::CellDispatch (0x6bcb0, reached
// through the ILT thunk 0x2e96) on the registry's +0x68 command grid
// (g_gameReg->m_cmdGrid); see <Gruntz/GameRegistry.h> / <Gruntz/TriggerMgr.h>.
// The seeded PRNG (inline LCG) the laser id draws from: seed once from timeGetTime.
extern "C" unsigned char g_randSeeded; // 0x6c127d
extern "C" i32 g_randSeed;             // 0x6c1288
extern u32 (*g_pTimeGetTime)();        // 0x6c4650
extern "C" u32 g_frameDelta;           // frame-time delta
// The activation-key "B" the update re-resolves through the bute tree.
extern char s_actKeyB[]; // 0x60d1bc "B"
// The laser-sound format string + the sound-play gate globals.
// @undefined-data: a char[] datum here is a STRING (or a run of them); its
// extent is not boundable from the named-symbol gaps (the unnamed $SG literals
// in between get swallowed). Inline the literal at its use site instead.
extern char s_LEVEL_UFOHAZARDLASER[]; // 0x611c54 "LEVEL_UFOHAZARDLASER%d"
extern i32 g_sndEnabled;              // 0x61ab20
extern i32 g_sndCueTag;               // 0x61ab24
// The probed hit-target: its +0x258 is a type tag (0x38 == self), +0x10 its object.
struct CSpotTarget {
    char m_pad00[0x10];
    CGameObject* m_10; // +0x10  the target's bound object (coords at +0x5c/+0x60)
    char m_pad14[0x258 - 0x14];
    i32 m_258; // +0x258  type tag
};
// The looked-up laser sound record: the update reads its move-delta at +0x5c/+0x60.
struct CSpotLaser {
    char m_pad00[0x5c];
    i32 m_5c; // +0x5c
    i32 m_60; // +0x60
};

// CSpotLight::Tick_0b1af0 @0x0b1af0 - the per-tick laser update. Unless the game is
// in the easy-mode gate, probe the cell under the light (Probe_32ce) for a live
// non-self target; if found, re-resolve the "B" bute node, copy the target's coords,
// and either (m_object->m_114 == 1) emit the cell sound-cue + spawn a numbered
// "LEVEL_UFOHAZARDLASER%d" laser sound (inline-seeded rand id) + play it, or else
// activate the target and emit the alternate cue. Otherwise fall through to the 2D
// rotation of the light offset (angle m_90 advanced by g_frameDelta * rate m_58).
// @early-stop
// ~52% - megafunction frame/regalloc + x87 fp-stack-schedule wall
// (docs/patterns/x87-fp-stack-schedule.md, topic:wall topic:regalloc): the control
// flow (mode gate, probe, target/type checks, "B" bute re-resolve, coord copy, the
// m_114 laser-vs-activate branch, and the 2D rotation) is a complete reconstruction,
// but retail's frame reserves 0x18 (the CString name + the fp scratch temps
// [esp+0x10..0x20]) vs our 0xc, and it hoists g_gameReg->m_68 into the prologue - so
// every [esp+N] slot + register assignment shifts, cascading through the whole body.
// Compounded by the inlined seed*0x343fd+0x269ec3 rand LCG (lea-chain) and the
// fld/fsin/fcos/fxch rotation stack-scheduling (the SAME wall the CSpotLight ctor +
// Update_0b1ee0 carry). Logic complete; the frame/regalloc/fp codegen is the wall.
RVA(0x000b1af0, 0x318)
i32 CSpotLight::Tick_0b1af0() {
    CGameRegistry* reg = g_gameReg;
    if (reg->m_isEasyMode == 0 || *(i32*)((char*)reg + 0x134) != 1) {
        char* o = (char*)m_object;
        CSpotTarget* tgt = (CSpotTarget*)
            Probe_32ce(*(i32*)(o + 0x5c), *(i32*)(o + 0x60), o + 0x144, &m_9c, &m_a0, 0);
        if (tgt != 0 && tgt->m_258 != 0x38 && !(m_a4 != 0 && m_9c != 0)) {
            m_prevAnimSetNode = m_objAux->m_1c;
            m_objAux->m_1c = g_buteTree.Find(s_actKeyB);
            char* t = (char*)tgt->m_10;
            *(i32*)(o + 0x5c) = *(i32*)(t + 0x5c);
            *(i32*)(o + 0x60) = *(i32*)(t + 0x60);
            if (*(i32*)(o + 0x114) == 1) {
                reg->m_cmdGrid->CellDispatch(m_9c, m_a0, 5, -1);
                i32 seed;
                if ((g_randSeeded & 1) == 0) {
                    g_randSeeded |= 1;
                    seed = (i32)g_pTimeGetTime();
                } else {
                    seed = g_randSeed;
                }
                g_randSeed = seed * 0x343fd + 0x269ec3;
                i32 laser = (((g_randSeed >> 16) & 0x7fff) & 1) + 1;
                CString name;
                name.Format(s_LEVEL_UFOHAZARDLASER, laser);
                CSndHost* obj = reg->m_world->m_28; // the name->cue map host
                if (obj->m_emitGate == 0) {
                    void* out = 0;
                    if (obj->m_10.Lookup(name, out) && out != 0 && g_sndEnabled != 0) {
                        SoundPlay_1360d0((i32)out, 0, 0, g_sndCueTag);
                    }
                }
            } else {
                Activate_4322(tgt, 1);
                reg->m_cmdGrid->CellDispatch(m_9c, m_a0, 0xa, -1);
            }
            return 0;
        }
    }
    // 2D rotation of the light offset (the fp-stack-schedule wall)
    double s = sin(m_90);
    double c = cos(m_90);
    double dt = (double)(i32)g_frameDelta;
    CSpotLaser* mv = (CSpotLaser*)m_98;
    double rx = m_80 * c - m_88 * s;
    double ry = m_80 * s + m_88 * c;
    if (mv != 0) {
        m_70 = (double)mv->m_5c;
        m_78 = (double)mv->m_60;
    }
    m_60 = m_70 + rx;
    m_68 = m_78 + ry;
    m_90 = m_90 + dt * m_58;
    *(i32*)((char*)m_object + 0x5c) = (i32)m_60;
    *(i32*)((char*)m_object + 0x60) = (i32)m_68;
    return 0;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CSpotActEntry);
SIZE_UNKNOWN(CSpotFocus);
SIZE_UNKNOWN(CSpotTarget);
SIZE_UNKNOWN(CSpotLaser);
