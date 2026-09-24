#ifndef GRUNTZ_GRUNTZ_TILETRIGGERSWITCHINLINE_H
#define GRUNTZ_GRUNTZ_TILETRIGGERSWITCHINLINE_H

#include <Mfc.h>

static __inline char* PbStr(const CString& s) {
    return const_cast<char*>(static_cast<const char*>(s));
}

#endif // GRUNTZ_GRUNTZ_TILETRIGGERSWITCHINLINE_H
