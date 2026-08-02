#include <rva.h>

#include <Crypto/FecCrypt.h>

#include <stdlib.h>
#include <time.h>

// Emit TU, wall-blocked: retail's CGruntzMgr::ChangeState calls this ctor
// out-of-line (via the inlined CMoviePlayer ctor, m_decodeStore member), and our
// ChangeState already references it as extern - but converting the body to a
// header inline makes our cl flatten it into ChangeState (caller-budget inline
// divergence, docs/patterns/msvc5-variable-ctor-inline-depth.md), losing this
// label. Dissolves into FecCrypt.h + a gruntzmgr pin when that wall breaks.
RVA(0x0008fea0, 0x6d)
CFecFile::CFecFile() {
    m_openGate = 0;
    m_readOpen = 0;
    m_writeOpen = 0;
    m_nextIndex = 0;
    srand(time(0));
}
