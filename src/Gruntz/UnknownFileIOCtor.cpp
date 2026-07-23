#include <Io/FileStream.h>      // real CFile (also pulls <Mfc.h> -> CByteArray)
#include <Gruntz/FileIOOwner.h> // canonical FileIOOwner (identity @identity-TODO)
#include <rva.h>

#include <stdlib.h> // srand (0x11fed0)
#include <time.h>   // time (0x120210)

RVA(0x0008fea0, 0x6d)
FileIOOwner::FileIOOwner() {
    m_0 = 0;
    m_4 = 0;
    m_8 = 0;
    m_134 = 0;
    srand(time(0));
}
