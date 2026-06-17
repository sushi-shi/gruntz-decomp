// StatusBarUpdate.cpp - destruct-button status-bar animation step.
// UpdateDestructButtonStatusBar @0x10b320 (359 B) - __thiscall void method on
// a game-world-level object (offsets +0x558..+0x570). Implements a two-phase
// timer-driven counter: warning-on (state=1) decrements counter (clamp >= 2),
// warning-cooldown (state=2) increments it (clamp <= 6). At each tick re-reads
// the delay from CButeMgr "StatusBar"/"DestructButtonWarningDelay" via GetIntDef
// (3-arg version @0x1721e0) and resets the 64-bit timer (m_560). Forwards the
// current counter to a status-bar item at +0x570 via vtable slot +0x30.
//
// Plain /O2 /MT (no /GX): scalar leaf, no stack C++ object / EH frame.
#include "../Wap32/Wap32.h"

// The running frame-clock for the 64-bit elapsed check (VA 0x645588).
// @data: 0x245588
extern unsigned int g_645588;

// The global CButeMgr config tree (VA 0x6453d8).
extern struct CButeMgr {
    int GetIntDef(const char *tag, const char *key, int defValue);  // @0x1721e0
} g_buteMgr;

// The status-bar item (only slot +0x30 used for SetCounter).
struct StatusBarItem {
    virtual void s00();  virtual void s01();  virtual void s02();
    virtual void s03();  virtual void s04();  virtual void s05();
    virtual void s06();  virtual void s07();  virtual void s08();
    virtual void s09();  virtual void s0a();  virtual void s0b();
    virtual void SetCounter(int counter);   // slot 12 (+0x30)
};

// Placeholder class for the object that owns the destruct-button state.
class CStatusBarOwner {
public:
    void UpdateDestructButtonStatusBar();
private:
    char m_pad[0x558];
    int  m_state;        // +0x558  1=warning-on, 2=cooldown
    int  m_counter;      // +0x55c  current tick counter
    unsigned __int64 m_start;   // +0x560  64-bit start timestamp
    unsigned __int64 m_delay;   // +0x568  64-bit delay interval (from config)
    void *m_item;        // +0x570  StatusBarItem pointer (or null)
};

// ---------------------------------------------------------------------------
// UpdateDestructButtonStatusBar  @ RVA 0x10b320  (thiscall void)
//
// @address: 0x10b320
// @size:    0x167
// ---------------------------------------------------------------------------
void CStatusBarOwner::UpdateDestructButtonStatusBar()
{
    int state = m_state;

    if (state == 1) {
        unsigned int now = g_645588;
        if ((unsigned __int64)now - m_start >= m_delay) {
            int c = m_counter - 1;
            m_counter = c;
            if (c <= 2) {
                m_counter = 2;
                m_state = 1;
            }

            m_delay = g_buteMgr.GetIntDef("StatusBar",
                "DestructButtonWarningDelay", 0x32);
            m_start = now;

            if (m_item != 0)
                ((StatusBarItem *)m_item)->SetCounter(m_counter);
        }
    } else if (state == 2) {
        unsigned int now = g_645588;
        if ((unsigned __int64)now - m_start >= m_delay) {
            int c = m_counter + 1;
            m_counter = c;
            if (c >= 6) {
                m_counter = 6;
                m_state = 2;
            }

            m_delay = g_buteMgr.GetIntDef("StatusBar",
                "DestructButtonWarningDelay", 0x32);
            m_start = now;

            if (m_item != 0)
                ((StatusBarItem *)m_item)->SetCounter(m_counter);
        }
    }
}
