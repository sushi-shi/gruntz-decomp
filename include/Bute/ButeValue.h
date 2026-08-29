#ifndef SRC_BUTE_BUTEVALUE_H
#define SRC_BUTE_BUTEVALUE_H

#include <Mfc.h>

#include <Bute/ARange.h>
#include <Bute/AVector.h>
#include <Ints.h>

struct ButeIntRect {
    ButeIntRect() : a(0), b(0), c(0), d(0) {}
    ButeIntRect(DWORD a_, DWORD b_, DWORD c_, DWORD d_) : a(a_), b(b_), c(c_), d(d_) {}
    DWORD a, b, c, d;
};
struct ButeIntPoint {
    ButeIntPoint() : a(0), b(0) {}
    ButeIntPoint(DWORD a_, DWORD b_) : a(a_), b(b_) {}
    DWORD a, b;
};

#endif // SRC_BUTE_BUTEVALUE_H
