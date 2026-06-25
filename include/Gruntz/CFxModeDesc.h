// CFxModeDesc.h - a small non-polymorphic mode/effect descriptor record (trace
// placeholder ClassUnknown_47). A type discriminator at +0x00 plus a handful of
// int parameter fields; CFxModeDesc is the base (ctor 0x17e7b0 zeroes the tag)
// and the concrete modes derive from it - each derived ctor runs the base ctor
// then stamps its type tag (1..6) and that type's defaults. Only the type-3
// variant (0x17e880) is reconstructed here. Names are placeholders; offsets +
// code bytes are load-bearing.
#ifndef GRUNTZ_CFXMODEDESC_H
#define GRUNTZ_CFXMODEDESC_H

#include <Ints.h>
#include <rva.h>

class CFxModeDesc {
public:
    CFxModeDesc(); // 0x17e7b0 base init (type = 0)

    i32 m_type; // +0x00 discriminator
    i32 m_04;
    i32 m_08;
    i32 m_0c;
    i32 m_10;
};

// The type-3 mode: base + (type=3, m_0c=1, m_10=0xf).
class CFxModeT3 : public CFxModeDesc {
public:
    CFxModeT3(); // 0x17e880
};

#endif // GRUNTZ_CFXMODEDESC_H
