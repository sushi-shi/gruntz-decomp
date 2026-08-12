#ifndef GRUNTZ_GRUNTZ_TYPECOLLRUNTIME_H
#define GRUNTZ_GRUNTZ_TYPECOLLRUNTIME_H

#include <rva.h>

#include <Ints.h>
#include <Wap32/ZVec.h>

class CString;

class CTypeCollRuntime : public _zdvec {
public:
    CTypeCollRuntime();
    virtual ~CTypeCollRuntime() OVERRIDE;

    char** GetNameRecord(i32 key) {
        return NameOf(SlotOf(key));
    }

    // The raw _zvec resolve: `_zdvec::IndexToPtr` minus its trailing
    // grown-slot construction, which the caller then runs itself.
    char** GetNameRecordRaw(i32 key) {
        return NameOf(ScratchResolve(key));
    }

    static char** NameOf(CString* slot) {

        union {
            CString* m_slot;
            char** m_buffer;
        } view;
        view.m_slot = slot;
        return view.m_buffer;
    }

    CString* ScratchResolve(i32 key) {
        return AsSlot(_zvec::IndexToPtr(key));
    }

    CString* Elem(i32 id) {
        return AsSlot(m_base + (id - m_lo) * m_stride);
    }

    CString* SlotOf(i32 id) {

        union {
            char* m_bytes;
            CString* m_elem;
        } band;
        band.m_bytes = _zdvec::IndexToPtr(id);
        return band.m_elem;
    }
    CString* Slots() {
        return AsSlot(m_alloc);
    }
    CString* Scratch() {
        return AsSlot(m_spare);
    }

private:
    static CString* AsSlot(char* p) {
        union {
            char* m_bytes;
            CString* m_elem;
        } band;
        band.m_bytes = p;
        return band.m_elem;
    }
};

#endif // GRUNTZ_GRUNTZ_TYPECOLLRUNTIME_H
