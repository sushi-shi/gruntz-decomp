#ifndef GRUNTZ_SAFEDELETE_H
#define GRUNTZ_SAFEDELETE_H

#define SAFE_DELETE(p)                                                                             \
    {                                                                                              \
        if (p) {                                                                                   \
            delete (p);                                                                            \
            (p) = NULL;                                                                            \
        }                                                                                          \
    }

#define SAFE_DELETE_ARRAY(p)                                                                       \
    {                                                                                              \
        if (p) {                                                                                   \
            delete[] (p);                                                                          \
            (p) = NULL;                                                                            \
        }                                                                                          \
    }

#define SAFE_RELEASE(p)                                                                            \
    {                                                                                              \
        if (p) {                                                                                   \
            (p)->Release();                                                                        \
            (p) = NULL;                                                                            \
        }                                                                                          \
    }

#endif // GRUNTZ_SAFEDELETE_H
