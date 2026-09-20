#ifndef SRC_BUTE_BUTEVALUE_H
#define SRC_BUTE_BUTEVALUE_H

#include <Mfc.h>

#include <Bute/ARange.h>
#include <Bute/AVector.h>
#include <Ints.h>

struct ButeIntRect {
    ButeIntRect() : m_a(0), m_b(0), m_c(0), m_d(0) {}
    ButeIntRect(DWORD a_, DWORD b_, DWORD c_, DWORD d_) : m_a(a_), m_b(b_), m_c(c_), m_d(d_) {}
    DWORD m_a, m_b, m_c, m_d;
};
struct ButeIntPoint {
    ButeIntPoint() : m_a(0), m_b(0) {}
    ButeIntPoint(DWORD a_, DWORD b_) : m_a(a_), m_b(b_) {}
    DWORD m_a, m_b;
};

#endif // SRC_BUTE_BUTEVALUE_H
