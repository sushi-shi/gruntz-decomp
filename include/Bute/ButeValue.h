#ifndef SRC_BUTE_BUTEVALUE_H
#define SRC_BUTE_BUTEVALUE_H

#include <rva.h>

#include <Mfc.h>

#include <Enums.h>
#include <Ints.h>

GZ_ENUM_BEGIN(ButeType)
    kButeInt = 0,
    kButeDword = 1,
    kButeDouble = 2,
    kButeFloat = 3,
    kButeString = 4,
    kButeRect = 5,
    kButePoint = 6,
    kButeVector = 7,
    kButeRange = 8
GZ_ENUM_END(ButeType)

struct CButeValue {
    ButeType type;
    void* pValue;

    CButeValue(ButeType type, CButeValue* src);

    // Parse-arm ctors: ParseAttributeFile's retail arms are `new CButeValue(type, v)`
    // new-expressions - each EH state guards the outer cell across the inlined
    // inner allocation (unwind map @0x604d90: ten alloc states, one per arm).
    CButeValue(ButeType t, i32 v) {
        type = t;
        i32* p = new i32;
        if (p) {
            *p = v;
            pValue = p;
        } else {
            pValue = 0;
        }
    }
    CButeValue(ButeType t, unsigned long v) {
        type = t;
        unsigned long* p = new unsigned long;
        if (p) {
            *p = v;
            pValue = p;
        } else {
            pValue = 0;
        }
    }
    CButeValue(ButeType t, float v) {
        type = t;
        float* p = new float;
        if (p) {
            *p = v;
            pValue = p;
        } else {
            pValue = 0;
        }
    }
    CButeValue(ButeType t, double v) {
        type = t;
        double* p = new double;
        if (p) {
            *p = v;
            pValue = p;
        } else {
            pValue = 0;
        }
    }
    CButeValue(ButeType t, const CString& s) {
        type = t;
        pValue = new CString(s);
    }
    CButeValue(ButeType t, i32 a, i32 b) {
        type = t;
        i32* p = new i32[2];
        if (p) {
            p[0] = a;
            p[1] = b;
            pValue = p;
        } else {
            pValue = 0;
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
            pValue = 0;
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
            pValue = 0;
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
            pValue = 0;
        }
    }

    ~CButeValue();

    CButeValue* SetInt(ButeType type, i32 val);
    CButeValue* SetDword(ButeType type, u32 val);
    CButeValue* SetFloat(ButeType type, float val);
    CButeValue* SetDouble(ButeType type, double val);

    CButeValue* SetString(ButeType type, const CString& src);
    CButeValue* SetRect(ButeType type, const struct ButeRefSmall* src);
    CButeValue* SetVector(ButeType type, const struct ButeRefLarge* src);
    CButeValue* SetRange(ButeType type, const struct ButeRefSmall* src);

    CButeValue* CopyValue(CButeValue* other);
};
SIZE(0x8);

struct ButeRefSmall {
    i32 w[4];
};
SIZE(0x10);

struct ButeRefLarge {
    i32 w[6];
};
SIZE(0x18);

void __cdecl ButeValueTeardown(void* pValue);

#endif // SRC_BUTE_BUTEVALUE_H
