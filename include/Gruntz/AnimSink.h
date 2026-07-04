// AnimSink.h - the shared view of the animation sub-object embedded at
// CGameObject+0x1a0 (the bound object is CUserLogic::m_38). Its setter (0x15c360,
// __thiscall, 1 arg) re-targets the active animation to the draw-delta passed in;
// modeled NO-body so the call reloc-masks (the body lives in the engine sub-mgr
// TU). The SAME engine method every eye-candy / grunt-puddle / projectile
// render-obj TU calls (CSimpleAnimation, CBehindCandyAni, CFrontCandyAni,
// CFortressFlag, CGruntCreationPoint, CParticlez).
//
// Placeholder name; only the OFFSET/signature + emitted code bytes are load-bearing.
#ifndef GRUNTZ_GRUNTZ_CANIMSINK_H
#define GRUNTZ_GRUNTZ_CANIMSINK_H

#include <rva.h>

struct CAnimSink {
    i32 SetAnim(u32 ctx); // 0x15c360
};

#endif // GRUNTZ_GRUNTZ_CANIMSINK_H
