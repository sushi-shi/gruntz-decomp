#include <Crypto/BlowfishCopy.h> // this TU's external declarations
#include <Crypto/Blowfish.h>     // BlowfishBlock - the 8-byte block's dword/byte views
#include <Ints.h>
#include <rva.h>
#include <iostream.h>
#include <string.h> // memset - the per-record clear cl inlines to two dword stores

// @early-stop
// regalloc wall (topic:wall topic:regalloc, const-materialize-into-reg-vs-immediate):
// the whole control flow + record read/encipher/write + Blowfish reloc match retail;
// residual is that retail pins the test mask 1 in bl (`movb $1,%bl; testb %bl,mem`) and
// re-zeros the record buffer with a fresh `xor edx` inside the loop, while cl tests with
// an immediate `$1` and hoists the zero into ebx outside the loop. ~84.9%.
RVA(0x0016f6e0, 0x76)
void __stdcall BitStreamBlowfishEncode(istream* src, ostream* dst) {
    i32 last = 0;
    while (!src->eof()) {
        BlowfishBlock rec;
        // memset, not two field stores: the two-store form makes cl hoist the zero into
        // a callee-saved reg for the whole loop, which then costs the `1` its register
        // (retail pins the eof mask in bl and re-zeroes with a volatile `xor edx,edx`).
        memset(rec.m_bytes, 0, 8);
        src->read(rec.m_bytes, 8);
        last = src->gcount();
        Blowfish_encipher(&rec.m_w[0], &rec.m_w[1]);
        dst->write(rec.m_bytes, 8);
    }
    dst->put(static_cast<unsigned char>(last));
}
