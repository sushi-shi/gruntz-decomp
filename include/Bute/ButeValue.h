#ifndef SRC_BUTE_BUTEVALUE_H
#define SRC_BUTE_BUTEVALUE_H

#include <rva.h>

#include <Mfc.h>

#include <Bute/ARange.h>
#include <Bute/AVector.h>
#include <Enums.h>
#include <Ints.h>

GZ_ENUM_BEGIN(ButeType)
    BUTE_INT = 0,
    BUTE_DWORD = 1,
    BUTE_DOUBLE = 2,
    BUTE_FLOAT = 3,
    BUTE_STRING = 4,
    BUTE_RECT = 5,
    BUTE_POINT = 6,
    BUTE_VECTOR = 7,
    BUTE_RANGE = 8
GZ_ENUM_END(ButeType)

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

struct CButeValue {
    ButeType type;
    void* pValue;

    CButeValue() {}

    CButeValue(ButeType t, ButeIntPoint* src) {
        type = t;
        pValue = new ButeIntPoint(*src);
    }

    CButeValue(ButeType t, i32 v) {
        type = t;
        pValue = new i32(v);
    }
    CButeValue(ButeType t, unsigned long v) {
        type = t;
        pValue = new unsigned long(v);
    }
    CButeValue(ButeType t, float v) {
        type = t;
        pValue = new float(v);
    }
    CButeValue(ButeType t, double v) {
        type = t;
        pValue = new double(v);
    }
    CButeValue(ButeType t, const CString& s) {
        type = t;
        pValue = new CString(s);
    }
    CButeValue(ButeType t, ButeIntRect* src) {
        type = t;
        pValue = new ButeIntRect(*src);
    }
    CButeValue(ButeType t, CAVector* src) {
        type = t;
        pValue = new CAVector(*src);
    }
    CButeValue(ButeType t, CARange* src) {
        type = t;
        pValue = new CARange(*src);
    }

    inline ~CButeValue();

    inline CButeValue* CopyValue(CButeValue* other);
};

RVA(0x00172040, 0x120)
inline CButeValue* CButeValue::CopyValue(CButeValue* other) {
    switch (type) {
        case BUTE_INT:
            *static_cast<i32*>(pValue) = *static_cast<i32*>(other->pValue);
            return this;
        case BUTE_DWORD:
            *static_cast<DWORD*>(pValue) = *static_cast<DWORD*>(other->pValue);
            return this;
        case BUTE_DOUBLE:
            *static_cast<double*>(pValue) = *static_cast<double*>(other->pValue);
            return this;
        case BUTE_FLOAT:
            *static_cast<float*>(pValue) = *static_cast<float*>(other->pValue);
            return this;
        case BUTE_STRING:
            *static_cast<CString*>(pValue) = *static_cast<CString*>(other->pValue);
            return this;
        case BUTE_RECT:
            *static_cast<ButeIntRect*>(pValue) = *static_cast<ButeIntRect*>(other->pValue);
            return this;
        case BUTE_POINT:
            *static_cast<ButeIntPoint*>(pValue) = *static_cast<ButeIntPoint*>(other->pValue);
            return this;
        case BUTE_VECTOR:
            *static_cast<CAVector*>(pValue) = *static_cast<CAVector*>(other->pValue);
            return this;
        case BUTE_RANGE:
            *static_cast<CARange*>(pValue) = *static_cast<CARange*>(other->pValue);
            return this;
    }
    return this;
}

inline CButeValue::~CButeValue() {
    switch (type) {
        case BUTE_INT:
            delete static_cast<i32*>(pValue);
            break;
        case BUTE_DWORD:
            delete static_cast<DWORD*>(pValue);
            break;
        case BUTE_DOUBLE:
            delete static_cast<double*>(pValue);
            break;
        case BUTE_FLOAT:
            delete static_cast<float*>(pValue);
            break;
        case BUTE_STRING:
            delete static_cast<CString*>(pValue);
            break;
        case BUTE_RECT:
            delete static_cast<ButeIntRect*>(pValue);
            break;
        case BUTE_POINT:
            delete static_cast<ButeIntPoint*>(pValue);
            break;
        case BUTE_VECTOR:
            delete static_cast<CAVector*>(pValue);
            break;
        case BUTE_RANGE:
            delete static_cast<CARange*>(pValue);
            break;
    }
}

void __cdecl ButeValueTeardown(void* pValue);

#endif // SRC_BUTE_BUTEVALUE_H
