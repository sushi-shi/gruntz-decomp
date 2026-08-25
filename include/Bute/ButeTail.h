#ifndef SRC_BUTE_BUTETAIL_H
#define SRC_BUTE_BUTETAIL_H

#include <rva.h>

struct CButeTail {
    CButeTail();
    CButeTail(const char* key);

    ~CButeTail();

    void InitKey(const char* key);
    void Decode(class istream* in, class ostream* out);
    void Encode(class istream* src, class ostream* dst);
};

#endif // SRC_BUTE_BUTETAIL_H
