

#include <Crypto/FecCrypt.h>
#include <rva.h>

#include <stdlib.h>
#include <time.h>

RVA(0x0008fea0, 0x6d)
CFecFile::CFecFile() {
    m_openGate = 0;
    m_readOpen = 0;
    m_writeOpen = 0;
    m_nextIndex = 0;
    srand(time(0));
}
