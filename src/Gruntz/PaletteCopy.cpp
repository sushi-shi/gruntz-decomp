// PaletteCopy.cpp - 0x17ca10: expand a 256-entry RGB palette (3 bytes/entry from
// m_10+0x6c) into the object's 4-byte-per-entry slot array at +0x108, then push it
// to the +0x2c device interface through its vtbl slot 6 (__stdcall).
#include <Ints.h>
#include <rva.h>

// The device interface vtable; only slot 6 (the palette setter) is used.
struct PalDeviceVtbl {
    void* m_slots[6];
    void(__stdcall* SetEntries)(void* self, int a, int b, int count, void* entries);
};
struct PalDevice {
    PalDeviceVtbl* m_vtbl; // +0x00
};

struct CPaletteHost {
    char m_pad0[0x10];
    u8* m_10; // +0x10  RGB source base (entries at +0x6c)
    char m_pad14[0x2c - 0x14];
    PalDevice* m_2c; // +0x2c  device interface
    char m_pad30[0x108 - 0x30];
    u8 m_108[0x400]; // +0x108  256 * 4-byte slots

    void UploadPalette(); // 0x17ca10
};

// ---------------------------------------------------------------------------
// 0x17ca10 - copy RGB triples into the slot array (alpha byte left untouched),
// then hand the slot array to the device.
// ---------------------------------------------------------------------------
// @early-stop
// scheduling+regalloc wall (61%): the RGB->4-byte expand loop and the +0x2c device
// vtbl[6] __stdcall upload are byte-faithful in operation, but retail keeps the
// running dst in edx (started at this+0x109, advanced mid-iteration) and recomputes
// this+0x108 at the end, while cl pins the dst base in ebp (extra push/pop) and
// advances src by 3 up-front instead of inc-per-byte. A loop-scheduling + base-
// register coin-flip; not source-steerable. Logic 100% correct; deferred.
RVA(0x0017ca10, 0x49)
void CPaletteHost::UploadPalette() {
    u8* src = m_10 + 0x6c;
    u8* dst = m_108;
    int n = 0x100;
    do {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst += 4;
        src += 3;
    } while (--n);
    m_2c->m_vtbl->SetEntries(m_2c, 0, 0, 0x100, m_108);
}

// ---------------------------------------------------------------------------
// Class metadata (SIZE sweep) - hosted at TU EOF; labels.py scans tree-wide.
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(PalDevice);
SIZE_UNKNOWN(PalDeviceVtbl);
