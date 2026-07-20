#include <Ints.h>
#include <rva.h>
#include <iostream.h>

void Blowfish_encipher(u32* xl, u32* xr);

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
        unsigned int rec[2];
        rec[0] = 0;
        rec[1] = 0;
        src->read(reinterpret_cast<char*>(rec), 8);
        last = src->gcount();
        Blowfish_encipher(&rec[0], &rec[1]);
        dst->write(reinterpret_cast<const char*>(rec), 8);
    }
    dst->put(static_cast<unsigned char>(last));
}
