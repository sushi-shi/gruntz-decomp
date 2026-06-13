// Wap32.h - WAP32 engine class declarations (Brian Goble's engine; shared
// C:\Proj\Incs\). Minimal reconstructions sufficient to byte-match the small
// self-contained constructors. Field names are placeholders (m_<hexoffset>);
// only the OFFSETS are load-bearing (they are what the byte-exact ctor proves).
#ifndef WAP32_H
#define WAP32_H

// ---------------------------------------------------------------------------
// CGameWnd - WAP32 window wrapper.
//   vftable @0x5ea344. ctor (RVA 0x13cf00, 17 bytes) zeroes m_4 (+0x04) and
//   m_c (+0x0c); vptr stored first (natural single-class form).
// ---------------------------------------------------------------------------
class CGameWnd {
public:
    CGameWnd();
    virtual ~CGameWnd();
    virtual int Wap32GameWndVfunc0();

    int  m_4;   // +0x04  zeroed by ctor
    int  m_8;   // +0x08  (not touched by ctor)
    int  m_c;   // +0x0c  zeroed by ctor
};

// ---------------------------------------------------------------------------
// CGameApp - WAP32 application object.
//   vftable @0x5e9b0c. ctor (RVA 0x13d590, 60 bytes) zeroes a handful of
//   fields then bumps a file-scope instance counter at 0x653c6c.
//   The ctor schedule emits the +0x10 store BEFORE the +0x0c store, which the
//   source mirrors (m_10 initialised before m_c).
// ---------------------------------------------------------------------------
class CGameApp {
public:
    CGameApp();
    virtual ~CGameApp();
    virtual int Wap32GameAppVfunc0();

    int  m_4;            // +0x04
    int  m_8;            // +0x08
    int  m_c;            // +0x0c
    int  m_10;           // +0x10
    char m_pad14[0x240 - 0x14];
    int  m_240;          // +0x240
    int  m_244;          // +0x244 (not touched)
    int  m_248;          // +0x248
    int  m_24c;          // +0x24c
    int  m_250;          // +0x250
};

#endif // WAP32_H
