#ifndef UTILS_RANDOMNUMBER_INL
#define UTILS_RANDOMNUMBER_INL

// Include after the Win32 declarations, in the scope that owns the RNG state.
__inline int GetRandomNumber() {
    static long holdrand = timeGetTime();
    return (((holdrand = holdrand * 214013L + 2531011L) >> 16) & 0x7fff);
}

#endif // UTILS_RANDOMNUMBER_INL
