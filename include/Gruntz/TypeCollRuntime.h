#ifndef GRUNTZ_GRUNTZ_TYPECOLLRUNTIME_H
#define GRUNTZ_GRUNTZ_TYPECOLLRUNTIME_H

#include <rva.h>

#include <Mfc.h>

#include <Ints.h>
#include <Wap32/ZVec.h>

class CTypeCollRuntime : public _zdvec {
public:
    CTypeCollRuntime();
    virtual ~CTypeCollRuntime() OVERRIDE;

    CString* GetNameRecord(i32 key) {
        return SlotOf(key);
    }

    CString* GetNameRecordRaw(i32 key) {
        return ScratchResolve(key);
    }

    CString* ScratchResolve(i32 key) {
        return AsSlot(_zvec::IndexToPtr(key));
    }

    CString* Elem(i32 id) {
        return AsSlot(m_base + (id - m_lo) * m_stride);
    }

    CString* SlotOf(i32 id) {
        return AsSlot(_zdvec::IndexToPtr(id));
    }
    CString* Slots() {
        return AsSlot(m_alloc);
    }
    CString* Scratch() {
        return AsSlot(m_spare);
    }

private:
    static CString* AsSlot(char* p) {
        return static_cast<CString*>(static_cast<void*>(p));
    }
};

#endif // GRUNTZ_GRUNTZ_TYPECOLLRUNTIME_H
