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

struct CButeValue {
    ButeType type;
    void* pValue;

    CButeValue() {}

    CButeValue(ButeType t, ButeIntPoint* src) {
        type = t;
        pValue = new ButeIntPoint(*src);
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

    inline ~CButeValue();

    inline CButeValue* CopyValue(CButeValue* other);
};

inline CButeValue* CButeValue::CopyValue(CButeValue* other) {
    // One arm per ButeType, each through the payload's REAL type - the retail jump
    // table (0x17213c) has nine entries over eight bodies, and the one shared pair
    // is cl's own fold of BUTE_FLOAT onto BUTE_DWORD (a float-to-float assignment
    // lowers to the same integer `mov`, so writing the arm as `DWORD` only erases
    // the type).  Every payload is copied as a whole object so both pointers stay in
    // registers - a per-field copy makes cl reload other->pValue for each word.
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
            *static_cast<float*>(pValue) = *static_cast<float*>(other->pValue);
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

// One `delete` per ButeType, each through the payload's REAL type.  Retail's arm
// bodies prove both halves of this shape: only the BUTE_STRING arm null-tests the
// pointer, so the other eight payload types have TRIVIAL destructors, and cl
// tail-merges the eight identical arms back into the 4-body / 9-entry jump table
// at 0x1721b4.  Transcribing that FOLD as four merged arms is what cost the whole
// Set<T> family ~11 points of /Ob1 inline accounting.
// docs/patterns/inline-callee-frontend-cost-drives-ob1-budget.md
inline CButeValue::~CButeValue() {
    switch (type) {
        case BUTE_INT:
            delete static_cast<i32*>(pValue);
            break;
        case BUTE_DWORD:
            delete static_cast<u32*>(pValue);
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
            delete static_cast<ButeDoubleVector*>(pValue);
            break;
        case BUTE_RANGE:
            delete static_cast<ButeDoubleRange*>(pValue);
            break;
    }
}

void __cdecl ButeValueTeardown(void* pValue);

#endif // SRC_BUTE_BUTEVALUE_H
