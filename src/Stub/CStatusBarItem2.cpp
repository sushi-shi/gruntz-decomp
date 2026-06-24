#include <rva.h>
// CStatusBarItem2.cpp - engine-label stubs for CStatusBarItem2 (reloc-correlation).
// NB: the disasm + DSNDMGR.CPP path show this is really a DirectSound-buffer
// wrapper; the CStatusBarItem2 names are placeholders kept per fill-in-place.

// COM-ish sound buffer interface dispatched through its vtable slot 0x30.
struct DSBufferVtbl {
    void* slot[12];                           // [0x00..0x2c]
    i32(__stdcall* Op)(void*, i32, i32, i32); // [0x30]
};
struct DSBuffer {
    DSBufferVtbl* vtbl;
};

// m_10: owning manager; m_78 is the "active" gate.
struct DSMgr {
    char m_pad[0x78];
    i32 m_78; // [+0x78]
};

// Static error reporter (DSNDMGR.CPP): GetErrorString@DirectSoundMgr@@SAXPADHJ@Z
void DSReportError(char*, i32, i32); // 0x138150 (__cdecl)

// Lookup tables (DATA externs).
extern i32 g_dsTable0[]; // 0x653ab8
extern i32 g_dsTable1[]; // 0x653c48

class CStatusBarItem2 {
public:
    i32 Finalize();
    i32 SetField0(i32);
    i32 SetField1(i32);
    i32 SetField2(i32);
    void SetField3(i32);

    // helpers shared with CStatusBarMgr::GetItem (reloc-masked there).
    i32 Sub3f0();      // 0x1353f0 thiscall
    i32 Inner560(i32); // 0x135560 thiscall (SetVolume)
    i32 Inner740(i32); // 0x135740 thiscall
    i32 Inner880(i32); // 0x135880 thiscall

    char m_pad0[0x0c];
    DSBuffer* m_c; // [+0x0c]
    DSMgr* m_10;   // [+0x10]
    i32 m_14;      // [+0x14]
    i32 m_18;      // [+0x18]
    char m_pad1c[0x1c];
    i32 m_38; // [+0x38]
    i32 m_3c; // [+0x3c]
    char m_pad40[0x10];
    i32 m_50;              // [+0x50]
    CStatusBarItem2* m_54; // [+0x54]

private:
    void Update(); // 0x1359a0 thiscall
    i32 Sub340();  // 0x135340 thiscall
};

// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x00136270, 0x8b)
i32 CStatusBarItem2::Finalize() {
    if (!m_10->m_78) {
        return 0;
    }
    i32 hr = (m_c->vtbl->Op(m_c, 0, 0, m_14) != 0);
    if (hr != 0) {
        if (hr == 0x88780096) {
            if (!m_54->Sub340()) {
                return 0;
            }
            i32 hr2 = (m_c->vtbl->Op(m_c, 0, 0, m_14) != 0);
            if (hr2 != 0) {
                DSReportError("C:\\Proj\\Dsndmgr\\DSNDMGR.CPP", 0x34c, hr2);
                return 0;
            }
        } else {
            DSReportError("C:\\Proj\\Dsndmgr\\DSNDMGR.CPP", 0x356, hr);
            return 0;
        }
    }
    return 1;
}
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x001355c0, 0x23)
i32 CStatusBarItem2::SetField0(i32 arg) {
    if (!m_10->m_78) {
        return 0;
    }
    return Inner560(g_dsTable0[arg]);
}
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x001357a0, 0x42)
i32 CStatusBarItem2::SetField1(i32 arg) {
    if (!m_10->m_78) {
        return 0;
    }
    if (arg >= 0) {
        return Inner740(-g_dsTable1[-arg]);
    }
    return Inner740(g_dsTable1[arg]);
}
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x00135920, 0x80)
i32 CStatusBarItem2::SetField2(i32 arg) {
    if (!m_10->m_78) {
        return 0;
    }
    i32 v = arg * m_18 / 100 + m_18;
    if ((u32)v >= 0x186a0) {
        v = 0x1869f;
    }
    if ((u32)v <= 0x64) {
        v = 0x65;
    }
    i32 r = Inner880(v);
    m_3c = arg * m_38 / 100 + m_38;
    Update();
    return r;
}
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x00135510, 0x25)
void CStatusBarItem2::SetField3(i32 arg) {
    if (m_10->m_78) {
        if (arg) {
            m_14 |= 1;
        } else {
            m_14 &= ~1;
        }
    }
}
