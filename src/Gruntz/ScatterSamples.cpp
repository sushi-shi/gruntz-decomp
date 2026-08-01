#include <Ints.h>
#include <rva.h>

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
    if (start >= end) {
        return;
    }

    i32 span = end - start;
    i32 prime = span;
    if (span < 100000) {
        while (!IsPrime(prime)) {
            prime++;
            if (prime >= 100000) {
                prime = count;
                break;
            }
        }
    } else {
        prime = count;
    }

    i32* used = static_cast<i32*>(operator new(prime * sizeof(i32)));
    if (used == 0) {
        return;
    }

    i32 step = 1;
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

    operator delete(used);
}
