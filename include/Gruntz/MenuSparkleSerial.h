// MenuSparkleSerial.h - CMenuSparkle's serialize override (RTTI .?AVCMenuSparkle@@,
// vtable 0x5e82dc -> slot 1 @ 0xae1c0; C:\Proj\Gruntz). The override chains the
// shared CUserLogic base serialize (0x16e7f0, the same slot-1 base every
// CUserLogic-derived Serialize calls - CMovingLogic's included), then a +0x34
// sub-object serialize (0x8c00), then round-trips two 4-byte globals through the
// archive's virtual Read/Write (mode 4 = write, mode 7 = read).
//
// CMenuSparkle IS a real CUserLogic leaf (the canonical ctor/dtor live in
// UserLogic.cpp). This TU models it at CUserLogic's 0x34 boundary
// (<Gruntz/Grunt.h>) so the +0x34 serializable sub-object sits as a real named
// member (m_34) rather than being reached through an offset cast into the fat-view
// base. Field/method names are placeholders; only offsets + emitted bytes are
// load-bearing (campaign doctrine).
#ifndef GRUNTZ_CMENUSPARKLESERIAL_H
#define GRUNTZ_CMENUSPARKLESERIAL_H

#include <rva.h>

// CUserLogic (0x34 base) + CGruntArchive (the serialize stream, Read @+0x2c /
// Write @+0x30) + CUserLogic::SerializeMove (0x16e7f0, the slot-1 base serialize).
#include <Gruntz/Grunt.h>

// The +0x34 sub-object that also serializes (0x8c00) is the shared serialized-
// object-reference; folded onto the canonical CSerialObjRef (Chain @0x8c00).
#include <Gruntz/SerialObjRef.h>

// Grunt.h-world view of CMenuSparkle (includes Grunt.h; overrides Grunt.h's
// CUserLogic::SerializeMove slot-1). NOT the canonical CTileLogic reparent - stays
// `: CUserLogic` (Grunt.h's) until stage 5. (The canonical CMenuSparkle in
// UserLogic.cpp is the CTileLogic one; documented dual-model, never coexist in a TU.)
class CMenuSparkle : public CUserLogic {
public:
    // slot-1 override (0xae1c0); overrides CUserLogic::SerializeMove.
    i32 SerializeMove(CGruntArchive* arc, i32 mode, i32 a3, i32 a4) OVERRIDE;
    // (+0x30 is CUserLogic::m_prevAnimSetNode - the base owns and serializes it;
    // the local `i32 m_30` re-declaration was dropped 2026-07-17, SM1.)
    CSerialObjRef m_34; // +0x34  the +0x34 serialized-object-reference
};

#endif // GRUNTZ_CMENUSPARKLESERIAL_H
