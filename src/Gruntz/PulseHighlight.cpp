// PulseHighlight.cpp - a CUserLogic-derived highlight/selection sprite whose
// per-frame Tick (0x8440) pulses a brightness value on its bound anim sink every
// 500 ms, toggling between two ramps; Serialize (0x8600) chains the shared
// CUserLogic serialize + the +0x34 sub-object, then transfers the pulse-timer
// state (m_54 phase / m_58 timestamp i64 / m_60 duration i64).
//
// The owning class is an orphan-COMDAT pair with no surviving vtable reference, so
// the class name is a PLACEHOLDER; only the OFFSETS + emitted code bytes are
// load-bearing (campaign doctrine). All callees (the brightness setter 0x1524d0,
// SerializeChain 0x16e7f0, CSerialObjRef::Chain, the archive Read/Write slots) are
// external/no-body so their call rel32 / DIR32 reloc-mask.
#include <Gruntz/UserLogic.h>     // CUserLogic + SerializeChain (0x16e7f0)
#include <Gruntz/CSerialObjRef.h> // CSerialArchive (Read @+0x2c / Write @+0x30) + CSerialObjRef

// The global millisecond tick (_g_645588). The DWORD load reloc-masks.
DATA(0x00245588)
extern "C" u32 g_645588;

// The bound anim sink: m_38 -> anim, anim->m_194 -> sink, sink->Set(brightness).
struct CPulseSink {
    void Set(i32 v); // 0x1524d0 (__thiscall, 1 arg)
};
struct CPulseAnim {
    char _00[0x194];
    CPulseSink* m_194; // +0x194
};

class CPulseHighlight : public CUserLogic {
public:
    i32 Tick();                                               // 0x8440
    i32 Serialize(CSerialArchive* ar, i32 tag, i32 c, i32 d); // 0x8600
};

// 0x8440: per-frame pulse. Every 500 ms the phase flag toggles and the timestamp
// resets; the elapsed time since the timestamp drives a brightness ramp (phase-on:
// (1 - t*0.002)*50 + 155; phase-off: t*0.1 + 155) pushed to the bound anim sink.
RVA(0x00008440, 0xfe)
i32 CPulseHighlight::Tick() {
    i64* ts = (i64*)((char*)this + 0x58);
    i32* phase = (i32*)((char*)this + 0x54);
    if ((i64)(u32)g_645588 - *ts >= *(i64*)((char*)this + 0x60)) {
        *phase = (*phase == 0);
        *(i64*)((char*)this + 0x60) = 0x1f4;
        *ts = (u32)g_645588;
    }
    if (*phase != 0) {
        i64 d2 = (i64)(u32)g_645588 - *ts;
        double t = (double)(u32)(d2 < 0 ? 0 : (u32)d2);
        (*(CPulseAnim**)((char*)this + 0x38))
            ->m_194->Set((i32)((1.0 - t * 0.002) * 50.0 - (-155.0)));
    } else {
        i64 d2 = (i64)(u32)g_645588 - *ts;
        double t = (double)(u32)(d2 < 0 ? 0 : (u32)d2);
        (*(CPulseAnim**)((char*)this + 0x38))->m_194->Set((i32)(t * 0.1 - (-155.0)));
    }
    return 0;
}

// 0x8600: Serialize override - chain the base serialize + the +0x34 sub-object,
// then transfer the pulse-timer fields (tag 4 = write, tag 7 = read).
RVA(0x00008600, 0xcd)
i32 CPulseHighlight::Serialize(CSerialArchive* ar, i32 tag, i32 c, i32 d) {
    if (ar == 0) {
        return 0;
    }
    if (!SerializeChain((i32)ar, tag, c, d)) {
        return 0;
    }
    if (!((CSerialObjRef*)((char*)this + 0x34))->Chain(ar, tag, c, (CSerialObj*)d)) {
        return 0;
    }
    char* p = (char*)this + 0x58;
    switch (tag) {
        case 4:
            ar->Write(p, 8);
            ar->Write(p + 8, 8);
            break;
        case 7:
            ar->Read(p, 8);
            ar->Read(p + 8, 8);
            break;
    }
    switch (tag) {
        case 4:
            ar->Write((char*)this + 0x54, 4);
            break;
        case 7:
            ar->Read((char*)this + 0x54, 4);
            break;
    }
    return 1;
}
SIZE_UNKNOWN(CPulseAnim);
SIZE_UNKNOWN(CPulseHighlight);
SIZE_UNKNOWN(CPulseSink);
