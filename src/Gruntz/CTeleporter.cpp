// CTeleporter.cpp - the teleporter tile-logic game object (C:\Proj\Gruntz).
//
// Two trace-discovered CTeleporter methods, defined in ascending retail-RVA order:
//   ~CTeleporter @0x010dd0 - the /GX leaf dtor (folds the CUserLogic teardown).
//   Begin        @0x0419e0 - the anim-driven one-shot geometry/bute bring-up.
//
// CTeleporter : CUserLogic (RTTI .?AVCTeleporter@@). Only offsets / code bytes
// are load-bearing; names are placeholders for the recovered engine identities.
#include <Gruntz/CTeleporter.h>

// The bound CGameObject viewed through m_10 by the bring-up: its +0x7c sub-object
// carries the per-tile-time at +0xbc (the SAME shape CPathHazard reads as
// CPathObj). Only the touched offsets are modeled; it overlays CGameObject.
struct CTeleBoundOwner {
    char m_pad00[0x7c];
    CTeleBoundOwner* m_7c; // +0x7c  per-tile-time owner
    char m_pad80[0xbc - 0x80];
    i32 m_bc; // +0xbc  per-tile time
};

// CTeleporter::~CTeleporter @0x010dd0 - the leaf adds no destructible members
// beyond CUserLogic, so its dtor folds the bare CUserLogic teardown: store the
// CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame. Byte-identical in shape to ~CGruntPuddle
// (0x010d10) / ~CTimeBomb (0x012a70); the empty body is enough for cl.
RVA(0x00010dd0, 0x44)
CTeleporter::~CTeleporter() {}

// CTeleporter::Begin @0x0419e0 - advance the +0x1a0 anim sub-mgr to the current
// draw-delta; once it reports idle (m_28==0 && m_20!=0), run the one-shot
// finalize: snapshot the bound object's per-tile-time / running-clock / bound
// geometry into the leaf fields, apply the "GAME_TELEPORTER" lookup-geometry to
// the bound object, and swap the +0x14 sub-object's "B" bute node. The finalize
// block is the SAME archetype as CGruntPuddle::Place's tail. Returns 0.
//
// @early-stop
// inverse register-pinning wall (~88%, docs/patterns/zero-register-pinning.md):
// every offset / immediate / branch target / call arg / field store is byte-faithful,
// but our MSVC enregisters the reloaded m_38+0x1a0 pointer + the m_7c->m_bc /
// g_645588 values in callee-saved edi (extra push edi/pop edi; folds +0x1a0 into the
// +0x1c8/+0x1c0 field offsets instead of re-adding 0x1a0; reuses eax for the m_1b4
// read) where retail re-reads from memory each time. The SAME coin-flip
// CGruntPuddle::Place / CPlay::ApplyGameOptions carry; no source lever flips it.
RVA(0x000419e0, 0x81)
i32 CTeleporter::Begin() {
    ((CTeleAnimSink*)((char*)m_38 + 0x1a0))->Advance(g_6bf3bc);

    if (((CTeleAnimSink*)((char*)m_38 + 0x1a0))->m_28 == 0) {
        return 0;
    }
    if (((CTeleAnimSink*)((char*)m_38 + 0x1a0))->m_20 != 0) {
        return 0;
    }

    m_60 = ((CTeleBoundOwner*)m_10)->m_7c->m_bc;
    m_64 = 0;
    m_58 = g_645588;
    m_5c = 0;
    m_40 = m_38->m_1b4;
    m_10->ApplyLookupGeometry(g_teleporterGeoKey, 0);
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find(g_iconBute);
    return 0;
}
