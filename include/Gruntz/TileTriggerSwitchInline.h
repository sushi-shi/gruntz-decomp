#ifndef GRUNTZ_GRUNTZ_TILETRIGGERSWITCHINLINE_H
#define GRUNTZ_GRUNTZ_TILETRIGGERSWITCHINLINE_H

static __inline char* AsMutableCStringData(const CString& text) {
    return const_cast<char*>(static_cast<const char*>(text));
}

#endif // GRUNTZ_GRUNTZ_TILETRIGGERSWITCHINLINE_H
