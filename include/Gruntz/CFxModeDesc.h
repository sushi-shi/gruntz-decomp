// CFxModeDesc.h - a small non-polymorphic mode/effect descriptor record (trace
// placeholder tomalla-47). A type discriminator at +0x00 plus a handful of
// int parameter fields; CFxModeDesc is the base (ctor 0x17e7b0 zeroes the tag)
// and the concrete modes derive from it - each derived ctor runs the base ctor
// then stamps its type tag (1..6) and that type's defaults. Only the type-3
// variant (0x17e880) is reconstructed here. Names are placeholders; offsets +
// code bytes are load-bearing.
#ifndef GRUNTZ_CFXMODEDESC_H
#define GRUNTZ_CFXMODEDESC_H

#include <Ints.h>
#include <rva.h>

SIZE_UNKNOWN(CFxModeDesc);
class CFxModeDesc {
public:
    CFxModeDesc(); // 0x17e7b0 base init (type = 0)

    i32 m_type; // +0x00 discriminator
    i32 m_04;
    i32 m_08;
    i32 m_0c;
    i32 m_10;
}; // 0x14 - the shared base; the upper fields (m_14..m_20) belong to whichever
// variants actually carry them, so each variant's stack temp is sized to it.

// The concrete modes: each derived ctor runs the base ctor, then stamps its type
// tag (2..6) and that type's defaults. The upper fields a variant writes are
// declared here (offsets continue at +0x14); offsets + store order are
// load-bearing, and the per-variant size determines the caller's stack frame.
SIZE_UNKNOWN(CFxModeT2);
class CFxModeT2 : public CFxModeDesc {
public:
    CFxModeT2(); // 0x17e840
    i32 m_14;
    i32 m_18;
    i32 m_1c;
    i32 m_20;
}; // 0x24

// The type-3 mode: base + (type=3, m_0c=1, m_10=0xf). No upper fields => 0x14.
SIZE_UNKNOWN(CFxModeT3);
class CFxModeT3 : public CFxModeDesc {
public:
    CFxModeT3(); // 0x17e880
}; // 0x14

SIZE_UNKNOWN(CFxModeT4);
class CFxModeT4 : public CFxModeDesc {
public:
    CFxModeT4(); // 0x17e8b0
    i32 m_14;
}; // 0x18

SIZE_UNKNOWN(CFxModeT5);
class CFxModeT5 : public CFxModeDesc {
public:
    CFxModeT5(); // 0x17e8e0
    i32 m_14;
}; // 0x18

SIZE_UNKNOWN(CFxModeT6);
class CFxModeT6 : public CFxModeDesc {
public:
    CFxModeT6(); // 0x17e910
    i32 m_14;
    i32 m_18;
    i32 m_1c;
    i32 m_20;
}; // 0x24

#endif // GRUNTZ_CFXMODEDESC_H
