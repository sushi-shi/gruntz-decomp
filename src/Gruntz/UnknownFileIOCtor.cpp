// The out-of-line CFecFile default constructor (0x8fea0). It sits in its own
// contribution between CGruntzMgr::ChangeState (0x8fab0) and the rest of the
// GruntzMgr band because it is a COMDAT: CFecFile is only ever constructed as
// CMoviePlayer::m_decodeStore, and ChangeState's stack-local player is the site
// that forced the copy out.
//
// IDENTITY RESOLVED 2026-07-28 (was `FileIOOwner`, @identity-TODO): this ctor runs
// on [esp+0x558] inside ChangeState, which is player+0x540 == CMoviePlayer::
// m_decodeStore - the exact member the same function later tears down with
// CFecFile::Close (0x17b570) and ~CFecFile (0x390a0). The ex-view's layout was
// CFecFile's field for field: m_0/m_4/m_8 == m_openGate/m_readOpen/m_writeOpen, the
// destructible CFile at +0x124 == m_stream, m_134 == m_nextIndex, and the
// CDWordArray at +0x138 == m_index.
#include <Crypto/FecCrypt.h> // canonical CFecFile (also pulls <Mfc.h> -> CFile/CDWordArray)
#include <rva.h>

#include <stdlib.h> // srand (0x11fed0)
#include <time.h>   // time (0x120210)

RVA(0x0008fea0, 0x6d)
CFecFile::CFecFile() {
    m_openGate = 0;
    m_readOpen = 0;
    m_writeOpen = 0;
    m_nextIndex = 0;
    srand(time(0));
}
