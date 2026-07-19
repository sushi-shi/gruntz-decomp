// BlowfishCopy.cpp - BitStreamBlowfishEncode (0x16f6e0), the encode half of the
// BitStreamBlowfish encode/decode pair (decode is 0x16f760, the very next COMDAT).
//
// Split out of Blowfish.cpp into its own TU: it needs the CRT <iostream.h> istream/
// ostream, and pulling that header into the Blowfish.cpp unit destabilises the mirror
// schedule of Blowfish_decipher (a knife-edge ~99.5% match). As its own single-function
// unit (like bitstreamblowfish) it uses the real iostream types with zero collateral.
//
// While the source istream is not at EOF, read an 8-byte record, Blowfish_encipher it,
// and write it to the ostream; remember the last gcount(); finally put() that count.
// __stdcall(src, dst). The streams are the statically-linked CRT iostreams (recovered
// identity): istream::read @0x16a510, ostream::write @0x16ab20,
// ostream::put(unsigned char) @0x16aab0, loop gate istream::eof() (ios vbase state &
// eofbit). Re-homed from src/Stub/ReconBatch2.cpp.
#include <Ints.h>
#include <rva.h>
#include <iostream.h>

// matched in the `blowfish` unit (?Blowfish_encipher@@YAXPAI0@Z)
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
    dst->put((unsigned char)last);
}
