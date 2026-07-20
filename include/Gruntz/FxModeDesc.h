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

SIZE_UNKNOWN(CFxModeT2);
class CFxModeT2 : public CFxModeDesc {
public:
    CFxModeT2(); // 0x17e840
    i32 m_14;
    i32 m_18;
    i32 m_1c;
    i32 m_20;
}; // 0x24

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
