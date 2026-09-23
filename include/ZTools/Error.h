#ifndef GRUNTZ_ZTOOLS_ERROR_H
#define GRUNTZ_ZTOOLS_ERROR_H

#include <rva.h>

#include <Enums.h>
#include <Ints.h>

struct CVariantSlot;

extern CVariantSlot g_zBitSetErrorSlot;
extern CVariantSlot g_globalErrorSlot;
extern CVariantSlot g_dynamicArrayErrorSlot;
extern CVariantSlot g_rezArchiveErrorSlot;
extern void* g_retAddrBreadcrumb;
extern i32 g_variantOverrideCount;

extern char* g_errDataInvalid;
extern char* g_errOverflow;
extern char* g_errOutOfRange;
extern char* g_errNullArg;
extern char* g_errExists;
extern char* g_errBadArg;
extern char* g_errNoFile;
extern char* g_errOutOfMem;

void* GetRetAddr();
void* GetCallerRetAddr();
void TmErrorHandler(char* prefix, i32 errNum);

class zErrHandling {
public:
    zErrHandling(CVariantSlot* errSink);
    virtual ~zErrHandling();

    void handle(const char* message, i32 code) const;
    void Report(char* message, i32 code);

    CVariantSlot* m_errSink;
};

typedef void(__cdecl* VariantCallback)(char* message, i32 value);

GZ_ENUM_BEGIN(VariantSlotKind)
    VARIANT_SLOT_RECORD_VALUE = 1,
    VARIANT_SLOT_CALLBACK = 2,
    VARIANT_SLOT_DIRECT_VALUE = 4
GZ_ENUM_END(VariantSlotKind)

struct CVariantSlot {
    CVariantSlot(char* label);
    void Set(zErrHandling* obj, char* item, i32 b);
    CVariantSlot* EnsureTmErrorCallback();
    i32 Find(i32 key);
    void* Add(void* key, void* value);
    VariantCallback m_callback;
    i32 m_searchIndex;
    u16 m_valueWord;
    VariantSlotKind m_typeTag;
    i32 m_reserved10;
    char* m_label;
};

inline void zErrHandling::handle(const char* message, i32 code) const {
    g_retAddrBreadcrumb = GetCallerRetAddr();
    m_errSink->Set(const_cast<zErrHandling*>(this), const_cast<char*>(message), code);
}

inline void zErrHandling::Report(char* message, i32 code) {
    g_retAddrBreadcrumb = GetRetAddr();
    m_errSink->Set(this, message, code);
}

struct TypeKeyRec {
    TypeKeyRec() {}
    i32 m_key;
    VariantCallback m_callback;
    short m_value;
};

#endif // GRUNTZ_ZTOOLS_ERROR_H
