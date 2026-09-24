#ifndef GRUNTZ_ZTOOLS_ERROR_H
#define GRUNTZ_ZTOOLS_ERROR_H

#include <rva.h>

#include <Enums.h>
#include <Ints.h>

#include <errno.h>

typedef void(__cdecl* erf_t)(const char*, i32);
unsigned long __caller_ip();
unsigned long __ip();

class zMinErr {
    friend class zErrHandler;
    friend class zErrHandling;

public:
    zMinErr();
    static erf_t set_erf(erf_t f) {
        erf_t t = ef;
        ef = f;
        return t;
    }
    static void quiet() {
        _quiet = 1;
    }
    static void use_error_function() {
        _quiet = 0;
    }
    static unsigned long caller() {
        return caller_ip;
    }

protected:
    static void handle(const char* s, i32 e) {
        caller_ip = __caller_ip();
        if (_quiet) {
            errno = e;
        } else {
            ef(s, e);
        }
    }
    static void handle_inl(const char* s, i32 e) {
        caller_ip = __ip();
        if (_quiet) {
            errno = e;
        } else {
            ef(s, e);
        }
    }

private:
    static unsigned long caller_ip;
    static erf_t ef;
    static i32 _quiet;
    static void catcher(const char*, i32);
};

const i32 MAX_DEDICATED = 32;

class _dhandler {
    friend class zErrHandler;
    void* object;
    erf_t handler;
    short lasterr;
};

class zErrHandler {
    friend class zErrHandling;
    friend class zErrHandler_default;

public:
    GZ_ENUM_BEGIN(error_mode)
        LOGGING = 1,
        FCALL = 2,
        QUICKEST = 4,
        EXCEPTION = 8
    GZ_ENUM_END(error_mode)

    zErrHandler(const char*);
    error_mode setmode(error_mode em) {
        prevmode = mode;
        mode = em;
        return static_cast<error_mode>(prevmode);
    }

protected:
    void log() {
        prevmode = mode;
        mode = LOGGING;
    }
    void callfunc() {
        prevmode = mode;
        mode = FCALL;
    }
    void fast() {
        prevmode = mode;
        mode = QUICKEST;
    }
    void prevstate() {
        mode = prevmode;
    }
    void handle(void*, const char*, i32);
    i32 geterr(void*, i32 = 0);
    i32 ok_set_ef() {
        return ndh < MAX_DEDICATED;
    }
    erf_t set_ef(void*, erf_t);
    erf_t set_default_ef(erf_t);

private:
    i32 srch(void*);
    static _dhandler dl[MAX_DEDICATED];
    static i32 ndh;
    erf_t default_ef;
    i32 slot;
    short evalue;
    GZ_ENUM_STORAGE(error_mode, i32) mode;
    GZ_ENUM_STORAGE(error_mode, i32) prevmode;
    const char* id;
};

typedef zErrHandler::error_mode ehm_t;

class zErrHandling {
public:
    virtual ~zErrHandling();
    void log() {
        hp->log();
    }
    void callfunc() {
        hp->callfunc();
    }
    void fast() {
        hp->fast();
    }
    void prevstate() {
        hp->prevstate();
    }
    void handle(const char* s, i32 e) const {
        zMinErr::caller_ip = __caller_ip();
        // PROVEN: the original const wrapper passes object identity to the void* error table.
        hp->handle(const_cast<zErrHandling*>(this), s, e);
    }
    void handle_inl(const char* s, i32 e) const {
        zMinErr::caller_ip = __ip();
        // PROVEN: the original const wrapper passes object identity to the void* error table.
        hp->handle(const_cast<zErrHandling*>(this), s, e);
    }
    i32 geterr() {
        return hp->geterr(this);
    }
    void reseterr(i32 n = 0) {
        hp->geterr(this, n);
    }
    i32 ok_set_ef() {
        return hp->ok_set_ef();
    }
    erf_t set_ef(erf_t f) {
        return hp->set_ef(this, f);
    }
    erf_t set_default_ef(erf_t f) {
        return hp->set_default_ef(f);
    }

protected:
    zErrHandling(zErrHandler* p = 0);
    static char* _nomem;
    static char* _inval;
    static char* _overflow;
    static char* _nosuch;
    static char* _range;
    static char* _exists;
    static char* _nullparg;
    static char* _badarg;

private:
    zErrHandler* hp;
};

#endif // GRUNTZ_ZTOOLS_ERROR_H
