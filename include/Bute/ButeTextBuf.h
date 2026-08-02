#ifndef SRC_BUTE_BUTETEXTBUF_H
#define SRC_BUTE_BUTETEXTBUF_H

#include <rva.h>

#include <iostream.h>

struct CButeTextBuf {
    char m_pad00[0xc];
    ostream accum;
};
SIZE_UNKNOWN();

#endif // SRC_BUTE_BUTETEXTBUF_H
