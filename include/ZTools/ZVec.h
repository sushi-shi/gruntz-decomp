#ifndef GRUNTZ_ZTOOLS_ZVEC_H
#define GRUNTZ_ZTOOLS_ZVEC_H

#include <rva.h>

#include <Enums.h>
#include <Ints.h>
#include <ZTools/Error.h>

#include <stddef.h>

GZ_ENUM_CONST_BEGIN(ZVecSentinel)
    ZVEC_NO_SCRATCH_ADDRESS = 1
GZ_ENUM_CONST_END(ZVecSentinel)

inline char* ZVecNoScratch() {

    // byte-evidenced: ztools marks "no scratch" with a fixed non-null address.
    return reinterpret_cast<char*>(ZVEC_NO_SCRATCH_ADDRESS);
}

class _zvec : public zErrHandling {
public:
    i32 high() const {
        return hi;
    }
    i32 low() const {
        return lo;
    }
    static ehm_t class_error_mode(ehm_t em) {
        return ceh.setmode(em);
    }

protected:
    _zvec(size_t s, i32 l, i32 h, void* overflow);
    virtual ~_zvec() OVERRIDE;
    void* get(i32 i) {
        return (i < lo || hi < i) ? (handle_inl(_range, ERANGE), ovf) : vec + (i - lo) * size;
    }
    i32 lo;
    i32 hi;
    char* vec;
    void* ovf;
    size_t size;
    static zErrHandler ceh;
};

class _zdvec : public _zvec {
protected:
    _zdvec(size_t s, i32 l, i32 h, void* overflow);
    void* get(i32 i) {
        initcount = 0;
        return (i < lo || i > hi)
                   ? (realloc(i) ? vec + size * (i - lo) : (handle_inl(_nomem, ENOMEM), ovf))
                   : vec + size * (i - lo);
    }
    i32 realloc(i32 i, i32 batch = 0);
    void* init;
    i32 initcount;
};

#endif // GRUNTZ_ZTOOLS_ZVEC_H
