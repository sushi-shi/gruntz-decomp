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

    // Parse-arm ctors: ParseAttributeFile's retail arms are `new CButeValue(type, v)`
    // new-expressions - each EH state guards the outer cell across the inlined
    // inner allocation (unwind map @0x604d90: ten alloc states, one per arm).
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

RVA(0x00172040, 0x120)
inline CButeValue* CButeValue::CopyValue(CButeValue* other) {
    // One arm per ButeType, each through the payload's REAL type - the retail jump
    // table (0x17213c) has nine entries over eight bodies, and the one shared pair
    // is cl's own fold of BUTE_FLOAT onto BUTE_DWORD (a float-to-float assignment
    // lowers to the same integer `mov`, so writing the arm as `DWORD` only erases
    // the type).  Each body owns its return epilogue; a shared return is C2-equivalent
    // here but changes the caller's C1 inline accounting.  Every payload is copied as
    // a whole object so both pointers stay in registers - a per-field copy makes cl
    // reload other->payload for each word.
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
