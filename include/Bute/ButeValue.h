#ifndef SRC_BUTE_BUTEVALUE_H
#define SRC_BUTE_BUTEVALUE_H

#include <rva.h>

#include <Mfc.h>

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

struct ButeRefLarge {
    double x, y, z;
};

struct ButeDoubleVector : ButeRefLarge {
    ButeDoubleVector() {
        x = 0;
        y = 0;
        z = 0;
    }
    ButeDoubleVector(double x_, double y_, double z_) {
        x = x_;
        y = y_;
        z = z_;
    }
};

struct ButeDoubleRange {
    ButeDoubleRange() {
        x = 0;
        y = 0;
    }
    ButeDoubleRange(double x_, double y_) {
        x = x_;
        y = y_;
    }
    double x, y;
};

union ButeValuePayload {
    i32* m_int;
    DWORD* m_dword;
    double* m_double;
    float* m_float;
    CString* m_string;
    ButeIntRect* m_rect;
    ButeIntPoint* m_point;
    ButeDoubleVector* m_vector;
    ButeDoubleRange* m_range;
};

struct CButeValue {
    ButeType type;
    ButeValuePayload payload;

    CButeValue() {}

    CButeValue(ButeType t, ButeIntPoint* src) {
        type = t;
        payload.m_point = new ButeIntPoint(*src);
    }

    CButeValue(ButeType t, i32 v) {
        type = t;
        payload.m_int = new i32(v);
    }
    CButeValue(ButeType t, unsigned long v) {
        type = t;
        payload.m_dword = new unsigned long(v);
    }
    CButeValue(ButeType t, float v) {
        type = t;
        payload.m_float = new float(v);
    }
    CButeValue(ButeType t, double v) {
        type = t;
        payload.m_double = new double(v);
    }
    CButeValue(ButeType t, const CString& s) {
        type = t;
        payload.m_string = new CString(s);
    }
    CButeValue(ButeType t, ButeIntRect* src) {
        type = t;
        payload.m_rect = new ButeIntRect(*src);
    }
    CButeValue(ButeType t, ButeDoubleVector* src) {
        type = t;
        payload.m_vector = new ButeDoubleVector(*src);
    }
    CButeValue(ButeType t, ButeDoubleRange* src) {
        type = t;
        payload.m_range = new ButeDoubleRange(*src);
    }

    inline ~CButeValue();

    inline CButeValue* CopyValue(CButeValue* other);
};

RVA(0x00172320, 0x120)
inline CButeValue* CButeValue::CopyValue(CButeValue* other) {
    switch (type) {
        case BUTE_INT:
            *payload.m_int = *other->payload.m_int;
            return this;
        case BUTE_DWORD:
            *payload.m_dword = *other->payload.m_dword;
            return this;
        case BUTE_DOUBLE:
            *payload.m_double = *other->payload.m_double;
            return this;
        case BUTE_FLOAT:
            *payload.m_float = *other->payload.m_float;
            return this;
        case BUTE_STRING:
            *payload.m_string = *other->payload.m_string;
            return this;
        case BUTE_RECT:
            *payload.m_rect = *other->payload.m_rect;
            return this;
        case BUTE_POINT:
            *payload.m_point = *other->payload.m_point;
            return this;
        case BUTE_VECTOR:
            *payload.m_vector = *other->payload.m_vector;
            return this;
        case BUTE_RANGE:
            *payload.m_range = *other->payload.m_range;
            return this;
    }
    return this;
}

inline CButeValue::~CButeValue() {
    switch (type) {
        case BUTE_INT:
            delete payload.m_int;
            break;
        case BUTE_DWORD:
            delete payload.m_dword;
            break;
        case BUTE_DOUBLE:
            delete payload.m_double;
            break;
        case BUTE_FLOAT:
            delete payload.m_float;
            break;
        case BUTE_STRING:
            delete payload.m_string;
            break;
        case BUTE_RECT:
            delete payload.m_rect;
            break;
        case BUTE_POINT:
            delete payload.m_point;
            break;
        case BUTE_VECTOR:
            delete payload.m_vector;
            break;
        case BUTE_RANGE:
            delete payload.m_range;
            break;
    }
}

void __cdecl ButeValueTeardown(void* pValue);

#endif // SRC_BUTE_BUTEVALUE_H
