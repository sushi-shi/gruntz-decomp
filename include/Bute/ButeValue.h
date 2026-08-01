#ifndef SRC_BUTE_BUTEVALUE_H
#define SRC_BUTE_BUTEVALUE_H

#include <Ints.h>
#include <rva.h>

class CString;

enum ButeType {
    kButeInt = 0,
    kButeDword = 1,
    kButeDouble = 2,
    kButeFloat = 3,
    kButeString = 4,
    kButeRect = 5,
    kButePoint = 6,
    kButeVector = 7,
    kButeRange = 8,
};

struct CButeValue {
    i32 type;
    void* pValue;

    CButeValue(i32 type, CButeValue* src);

    ~CButeValue();

    CButeValue* SetInt(i32 type, i32 val);
    CButeValue* SetDword(i32 type, u32 val);
    CButeValue* SetFloat(i32 type, float val);
    CButeValue* SetDouble(i32 type, double val);

    CButeValue* SetString(i32 type, const CString& src);
    CButeValue* SetRect(i32 type, const struct ButeRefSmall* src);
    CButeValue* SetVector(i32 type, const struct ButeRefLarge* src);
    CButeValue* SetRange(i32 type, const struct ButeRefSmall* src);

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
