// BitStreamBlowfish.cpp - re-homed from src/Stub/Discovered.cpp (0x16f760). The
// dynamic trace mis-attributed this to ClassUnknown_4 via stale ecx: the function
// never reads `this` (ecx) and cleans 2 stack args (ret 8) -> it is a __stdcall
// free function. It is a Blowfish-ECB/CBC bit decode loop: pull 8 "bits" from the
// reader (m_16a510), emit the PREVIOUS decrypted right-half through the sink
// (m_16ab20) skipping the priming block, Blowfish_decipher the (xl,xr) pair, then
// chain. Loops until the reader's end-flag byte (at this+desc->m_4+8) is set.
// Field names are placeholders; reader/sink methods are external (reloc-masked);
// Blowfish_decipher resolves to the matched `blowfish` unit.
#include <rva.h>
#include <Mfc.h>

struct Desc {
    char pad0[4];
    int m_4; // 0x4 - dynamic byte offset of the end-flag within the reader
};

struct BitReader {
    Desc* m_0; // 0x0
    char pad4[4];
    int m_8; // 0x8 - sample mode (==1 => sign-extend a byte)
    void Read16a510(unsigned int* out, int n);
};

struct Sink {
    void Emit16ab20(unsigned int* prevBlock, int sample);
};

// matched in the `blowfish` unit (?Blowfish_decipher@@YAXPAI0@Z)
void Blowfish_decipher(unsigned int* xl, unsigned int* xr);

RVA(0x0016f760, 0x82)
void __stdcall BitStreamBlowfishDecode(BitReader* r, Sink* out) {
    unsigned int blk[4];
    bool first = true;
    while (!(*((char*)r + r->m_0->m_4 + 8) & 1)) {
        r->Read16a510(&blk[0], 8);
        int sample = r->m_8;
        if (sample == 1) {
            sample = *(signed char*)&blk[0];
        }
        if (!first) {
            out->Emit16ab20(&blk[3], sample);
        } else {
            first = false;
        }
        Blowfish_decipher(&blk[0], &blk[1]);
        blk[2] = blk[0];
        blk[3] = blk[1];
    }
}
