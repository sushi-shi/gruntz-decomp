#include <rva.h>

#include <Ints.h>

#include <stddef.h>
#include <stdlib.h>

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
    // The search walks a COPY: [esp+0x18] still holds the original span at the
    // `r - 1 <= span` test below, so the loop must not clobber it.
    i32 probe = span;
    while (probe < 100000) {
        if (IsPrime(probe)) {
            prime = probe;
            goto have_prime;
        }
        probe++;
    }
    prime = count;

have_prime:
    i32* used = new i32[prime];
    if (used == NULL) {
        return;
    }

    i32 k;
    i32 r;
    i32 step;
    for (i32 s = 1; s < prime - 1; s++) {
        i32 ok = 1;
        for (k = 0; k < prime; k++) {
            used[k] = 0;
        }
        // The residue is loop-carried: retail keeps it in edx across the idiv,
        // so each step multiplies the PREVIOUS residue, seeded with `count`.
        r = count;
        for (k = 0; k < prime - 1; k++) {
            r = (s * r) % prime;
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

    r = count;
    for (k = 0; k < prime; k++) {
        r = (step * r) % prime;
        if (r - 1 <= span) {
            i32 v = r + start - 1;
            *out++ = __max(0, __min(v, end));
        }
    }

    delete[] used;
}
