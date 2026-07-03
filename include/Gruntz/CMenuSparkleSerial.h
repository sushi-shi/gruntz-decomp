// CMenuSparkleSerial.h - CMenuSparkle's serialize override (RTTI .?AVCMenuSparkle@@,
// vtable 0x5e82dc -> slot 1 @ 0xae1c0; C:\Proj\Gruntz). The override chains the
// shared CUserLogic base serialize (0x16e7f0, the same slot-1 base every
// CUserLogic-derived Serialize calls - CMovingLogic's included), then a +0x34
// sub-object serialize (0x8c00), then round-trips two 4-byte globals through the
// archive's virtual Read/Write (mode 4 = write, mode 7 = read).
//
// CMenuSparkle IS a real CUserLogic leaf (the canonical ctor/dtor live in
// UserLogic.cpp). This TU models it at CUserLogic's TRUE 0x30 boundary
// (<Gruntz/Grunt.h>) so the +0x34 serializable sub-object sits as a real named
// member (m_34) rather than being reached through an offset cast into the fat-view
// base. Field/method names are placeholders; only offsets + emitted bytes are
// load-bearing (campaign doctrine).
#ifndef GRUNTZ_CMENUSPARKLESERIAL_H
#define GRUNTZ_CMENUSPARKLESERIAL_H

#include <rva.h>

// CUserLogic (true 0x30 base) + CGruntArchive (the serialize stream, Read @+0x2c /
// Write @+0x30) + CUserLogic::SerializeMove (0x16e7f0, the slot-1 base serialize).
#include <Gruntz/Grunt.h>

// The +0x34 sub-object that also serializes (0x8c00); reloc-masked (no body). This
// is the shared "+0x34 serializable sub-object" (cf. CSerialSub34); typed to the
// real archive so the chain call carries no cast.
class CMenuSparkleSub {
public:
    i32 Serialize(CGruntArchive* arc, i32 mode, i32 a3, i32 a4); // 0x8c00
};

class CMenuSparkle : public CUserLogic {
public:
    // slot-1 override (0xae1c0); overrides CUserLogic::SerializeMove.
    i32 SerializeMove(CGruntArchive* arc, i32 mode, i32 a3, i32 a4) OVERRIDE;
    i32 m_30;             // +0x30
    CMenuSparkleSub m_34; // +0x34
};

#endif // GRUNTZ_CMENUSPARKLESERIAL_H
