// ButeTextBuf.h - the value-text accumulator host CButeMgr+0xa4 (m_pText) points at.
//
// In write-back mode the parser reconstructs each value's text into a CRT ostream
// embedded at +0x0c of this host (Parse reaches it as `mov ecx,[esi+0xa4]; add
// ecx,0xc; call ??6ostream@@...`). The host is a statically-linked CRT stream
// (an ofstream/ostrstream whose ostream base sub-object sits at +0x0c after a
// 12-byte fstreambase/strstreambase prefix); only the +0x0c offset and the fact
// that `accum` is a real ostream are load-bearing. @identity-TODO: pin the exact
// CRT host class (the 12-byte prefix). This lives in a header (not a .cpp) so it is
// a shared model, not a per-TU view; forward-declared as the type of CButeMgr::m_pText.
#ifndef SRC_BUTE_BUTETEXTBUF_H
#define SRC_BUTE_BUTETEXTBUF_H

#include <iostream.h> // the real MSVC 5.0 CRT ostream (the accumulator sub-object)

struct CButeTextBuf {
    char m_pad00[0xc]; // +0x00  the CRT stream prefix (fstreambase/strstreambase)
    ostream accum;     // +0x0c  the value-text accumulator (the real CRT ostream)
};

#endif // SRC_BUTE_BUTETEXTBUF_H
