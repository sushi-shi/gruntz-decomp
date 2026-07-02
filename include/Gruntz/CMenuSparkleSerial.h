// CMenuSparkleSerial.h - CMenuSparkle's serialize override (RTTI .?AVCMenuSparkle@@,
// vtable 0x5e82dc -> slot 1 @ 0xae1c0; C:\Proj\Gruntz). The override chains the
// shared base Serialize (0x16e7f0, the same engine base used by CMovingLogic's
// serialize), then a +0x34 sub-object Serialize (0x8c00), then round-trips two
// 4-byte globals through the archive's virtual Read/Write (mode 4 = write, mode 7
// = read).
//
// Modeled as a standalone serialize view (the main CMenuSparkle lives in
// UserLogic.cpp; this TU only needs `this`, the +0x34 sub, and the base call).
// Field/method names are placeholders; only offsets + emitted bytes are
// load-bearing (campaign doctrine).
#ifndef GRUNTZ_CMENUSPARKLESERIAL_H
#define GRUNTZ_CMENUSPARKLESERIAL_H

#include <Ints.h>
#include <rva.h>

// The serialize/archive stream: the shared WAP32 stream interface (Read @ +0x2c /
// Write @ +0x30), a real declared-only virtual class.
#include <Gruntz/SerialArchive.h>

// The +0x34 sub-object that also serializes (0x8c00); reloc-masked (no body).
class CMenuSparkleSub {
public:
    i32 Serialize(CSerialArchive* arc, i32 mode, i32 a3, i32 a4); // 0x8c00
};

// The shared serialize base (0x16e7f0) carrying the +0x00..+0x33 region; the
// derived Serialize calls it non-virtually on `this`. Reloc-masked (no body).
class CMenuSparkleBase {
public:
    i32 BaseSerialize(CSerialArchive* arc, i32 mode, i32 a3, i32 a4); // 0x16e7f0
    char m_00[0x34];                                                  // +0x00..+0x33
};

class CMenuSparkle : public CMenuSparkleBase {
public:
    i32 Serialize(CSerialArchive* arc, i32 mode, i32 a3, i32 a4); // 0xae1c0
    CMenuSparkleSub m_34;                                         // +0x34
};

#endif // GRUNTZ_CMENUSPARKLESERIAL_H
