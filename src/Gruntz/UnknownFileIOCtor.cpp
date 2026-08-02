

#include <Crypto/FecCrypt.h>
#include <rva.h>

#include <stdlib.h>
#include <time.h>

// Retail copy sits in the gruntzmgr band, but no reconstructed code constructs a
// CFecFile yet, so this TU is the only emitter; dissolves into a header inline +
// gruntzmgr pin once the FEC-using gruntzmgr function is reconstructed.
RVA(0x0008fea0, 0x6d)
CFecFile::CFecFile() {
    m_openGate = 0;
    m_readOpen = 0;
    m_writeOpen = 0;
    m_nextIndex = 0;
    srand(time(0));
}
