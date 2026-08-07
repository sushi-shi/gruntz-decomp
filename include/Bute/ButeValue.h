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
    ~ButeIntRect() {}
    DWORD a, b, c, d;
};
SIZE(0x10);
struct ButeIntPoint {
    ButeIntPoint() : a(0), b(0) {}
    ~ButeIntPoint() {}
    DWORD a, b;
};
SIZE(0x8);

struct ButeRefLarge {
    double x, y, z;
};
SIZE(0x18);

struct ButeDoubleVector : ButeRefLarge {
    ButeDoubleVector() {
        x = 0;
        y = 0;
        z = 0;
    }
    ~ButeDoubleVector() {}
};
SIZE(0x18);

struct ButeDoubleRange {
    ButeDoubleRange() {
        x = 0;
        y = 0;
    }
    ~ButeDoubleRange() {}
    double x, y;
};
SIZE(0x10);

struct CButeValue {
    ButeType type;
    void* pValue;

    CButeValue() {}

    CButeValue(ButeType type, CButeValue* src) {
        this->type = type;
        this->pValue = new CButeValue(*src);
    }

    // Parse-arm ctors: ParseAttributeFile's retail arms are `new CButeValue(type, v)`
    // new-expressions - each EH state guards the outer cell across the inlined
    // inner allocation (unwind map @0x604d90: ten alloc states, one per arm).
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
    CButeValue(ButeType t, ButeDoubleVector* src) {
        type = t;
        pValue = new ButeDoubleVector(*src);
    }
    CButeValue(ButeType t, ButeDoubleRange* src) {
        type = t;
        pValue = new ButeDoubleRange(*src);
    }
    CButeValue(ButeType t, i32 a, i32 b) {
        type = t;
        i32* p = new i32[2];
        if (p) {
            p[0] = a;
            p[1] = b;
            pValue = p;
        } else {
            pValue = NULL;
        }
    }
    CButeValue(ButeType t, i32 a, i32 b, i32 c, i32 d) {
        type = t;
        i32* p = new i32[4];
        if (p) {
            p[0] = a;
            p[1] = b;
            p[2] = c;
            p[3] = d;
            pValue = p;
        } else {
            pValue = NULL;
        }
    }
    CButeValue(ButeType t, double x, double y) {
        type = t;
        double* p = new double[2];
        if (p) {
            p[0] = x;
            p[1] = y;
            pValue = p;
        } else {
            pValue = NULL;
        }
    }
    CButeValue(ButeType t, double x, double y, double z) {
        type = t;
        double* p = new double[3];
        if (p) {
            p[0] = x;
            p[1] = y;
            p[2] = z;
            pValue = p;
        } else {
            pValue = NULL;
        }
    }

    inline ~CButeValue();

    inline CButeValue* CopyValue(CButeValue* other);
};
SIZE(0x8);

inline CButeValue* CButeValue::CopyValue(CButeValue* other) {
    // The retail jump table (0x17213c) proves the ButeType values AND that the
    // arms are written in value order: 1 and 3 share one arm because cl folded
    // the (identical) BUTE_DWORD body into the BUTE_FLOAT one.  Every payload is
    // copied as a whole object so both pointers stay in registers - a per-field
    // copy makes cl reload other->pValue for each word.
    switch (type) {
        case BUTE_INT:
            *static_cast<i32*>(pValue) = *static_cast<i32*>(other->pValue);
            break;
        case BUTE_DWORD:
            *static_cast<DWORD*>(pValue) = *static_cast<DWORD*>(other->pValue);
            break;
        case BUTE_DOUBLE:
            *static_cast<double*>(pValue) = *static_cast<double*>(other->pValue);
            break;
        case BUTE_FLOAT:
            *static_cast<DWORD*>(pValue) = *static_cast<DWORD*>(other->pValue);
            break;
        case BUTE_STRING:
            *static_cast<CString*>(pValue) = *static_cast<CString*>(other->pValue);
            break;
        case BUTE_RECT:
            *static_cast<ButeIntRect*>(pValue) = *static_cast<ButeIntRect*>(other->pValue);
            break;
        case BUTE_POINT:
            *static_cast<ButeIntPoint*>(pValue) = *static_cast<ButeIntPoint*>(other->pValue);
            break;
        case BUTE_VECTOR:
            *static_cast<ButeDoubleVector*>(pValue) =
                *static_cast<ButeDoubleVector*>(other->pValue);
            break;
        case BUTE_RANGE:
            *static_cast<ButeDoubleRange*>(pValue) = *static_cast<ButeDoubleRange*>(other->pValue);
            break;
    }
    return this;
}

inline CButeValue::~CButeValue() {
    switch (type) {
        case BUTE_STRING:
            delete static_cast<CString*>(pValue);
            break;
        case BUTE_DOUBLE:
        case BUTE_POINT:
            delete static_cast<double*>(pValue);
            break;
        case BUTE_INT:
        case BUTE_FLOAT:
        case BUTE_VECTOR:
            delete static_cast<i32*>(pValue);
            break;
        case BUTE_DWORD:
        case BUTE_RECT:
        case BUTE_RANGE:
            delete static_cast<u32*>(pValue);
            break;
    }
}

void __cdecl ButeValueTeardown(void* pValue);

#endif // SRC_BUTE_BUTEVALUE_H
