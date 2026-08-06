#include <rva.h>

#include <Ints.h>

#include <stddef.h>

RVA(0x00182a80, 0x2e)
i32 IsPrime(i32 n) {
    i32 d = 2;
    if (d >= n) {
        return 1;
    }
    for (; d < n; d++) {
        if (n % d == 0) {
            return 0;
        }
    }
    return 1;
}

// @early-stop
RVA(0x00182940, 0x13c)
void ScatterSamples(i32* out, i32 start, i32 end, i32 count) {
    if (start > end) {
        return;
    }
    if (start == end) {
        return;
    }

    i32 span = end - start;
    i32 prime;
    if (span < 100000) {
        while (span < 100000) {
            if (IsPrime(span)) {
                prime = span;
                goto have_prime;
            }
            span++;
        }
    }
    prime = count;

have_prime:
    i32* used = new i32[prime];
    if (used == NULL) {
        return;
    }

    i32 step;
    i32 k;
    for (i32 s = 1; s < prime - 1; s++) {
        i32 ok = 1;
        for (k = 0; k < prime; k++) {
            used[k] = 0;
        }
        for (k = 0; k < prime - 1; k++) {
            i32 r = (s * count) % prime;
            if (used[r - 1] != 0) {
                ok = 0;
                break;
            }
            used[r - 1] = 1;
        }
        if (ok != 0) {
            step = s;
            break;
        }
    }

    i32* p = out;
    for (k = count; k > 0; k--) {
        i32 r = (step * count) % prime;
        if (r - 1 <= span) {
            i32 v = r + start - 1;
            i32 c = v < end ? v : end;
            if (c < 0) {
                v = 0;
            } else if (v >= end) {
                v = end;
            }
            *p = v;
            p++;
        }
    }

    delete[] used;
}
