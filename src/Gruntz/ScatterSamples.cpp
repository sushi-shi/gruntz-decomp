// ScatterSamples.cpp - two file-static __cdecl math helpers in the Gruntz game
// module, reached only from the effect/randomization init at 0x17fe00.
//
// IsPrime(n) (0x182a80): trial-division primality test, returns 1 for n <= 2.
//
// ScatterSamples(out, start, end, count) (0x182940): generates `count`
// distributed sample values into `out` over the inclusive range [start, end].
// It finds the smallest prime >= (end-start) (capped at 100000), then searches
// for a multiplier `step` in [1, count) that is coprime with `count` (a value
// whose multiples (step*k) mod count visit every residue), and scatters
// `count` clamped samples `start + ((step*count*k) mod count) - 1` into `out`.
// Ghidra grouped these as tomalla-49; they are free __cdecl functions, not
// class methods (the caller passes a buffer pointer, there is no `this`).
#include <Ints.h>
#include <rva.h>

// Global operator new / delete (the NAFXCW heap): operator new(0x1b9b46),
// operator delete(0x1b9b82). `new int[n]` over a trivial element lowers to the
// scalar forms with no array cookie.
void* operator new(u32 n);
void operator delete(void* p);

// 0x182a80 - IsPrime(n): n <= 2 is prime; else trial-divide 2..n-1.
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

// 0x182940 - ScatterSamples(out, start, end, count).
// @early-stop
// regalloc/LICM wall (~59%): logic + block structure byte-exact, but MSVC5 /O2
// makes two non-steerable optimizer choices. (1) It assigns prime->ebx + array->ebp
// where retail uses prime->ebp + s->ebx + array->edi (the ebx<->ebp swap cascades).
// (2) It hoists the inner-loop-invariant r=(s*count)%prime out + strength-reduces
// s*count to a running `+= count`, where retail re-reads count from [esp+0x2c] each
// outer iteration (count stays in memory) and recomputes the `imul` inside the loop.
// Same class as docs/patterns/blowfish-feistel-unroll-regalloc.md (identical logic,
// different callee-saved reg pick). Inline-recompute / step=(i32)used both regressed.
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
