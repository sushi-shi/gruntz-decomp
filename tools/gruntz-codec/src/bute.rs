//! **BUTE** — the Blowfish envelope Gruntz wraps around its attribute text.
//!
//! Two `.TXT` resources in retail `Gruntz.REZ` are not text: `GAME\ATTRIBUTEZ`
//! (150 897 B) and `STATEZ\CREDITZ\PALETTEZ\CHEATZ` (865 B). Both are ECB
//! Blowfish over 8-byte blocks with a one-byte length trailer.
//!
//! # Evidence
//!
//! * The cipher is Blowfish (Schneier 1993) — a published algorithm, verified
//!   here against the published all-zero test vector, not reverse-engineered.
//!   Retail carries the standard pi-digit initialization tables at
//!   `g_bfInitP` @0x0021bef8 / `g_bfInitS` @0x0021bf40.
//! * The **word order is little-endian** — retail loads each block through a
//!   `union { u32 w[2]; char b[8]; }` on x86, so `w[0]` is the first four bytes
//!   read little-endian. This is the one place Monolith deviates from the
//!   reference implementation's big-endian block convention, and it is not
//!   optional: big-endian loading produces noise on the shipped bytes.
//! * The **framing** is `CButeTail::Decode` @0x0016f760, read instruction by
//!   instruction:
//!
//!   ```text
//!   16f77f  push 8 / call istream::read      -> 8 bytes into blk[0]
//!   16f789  mov eax,[esi+8]                  -> gcount
//!   16f78c  cmp eax,1 / jne
//!   16f791  movsx eax, byte [esp+0xc]        -> a 1-byte read IS the length trailer
//!   16f796  test bl,bl                       -> skip the write on the first pass
//!   16f79a  push eax / call ostream::write   -> writes blk[1], the PREVIOUS block
//!   16f7b5  call 0x16fc70                    -> Blowfish_decipher(blk[0])
//!   16f7ba  ...                              -> blk[1] = blk[0]
//!   ```
//!
//!   i.e. the writer runs one block behind the reader, so the block that is
//!   still in hand when the 1-byte trailer arrives is emitted truncated to the
//!   trailer's value. A stream of `8*n + 1` bytes therefore decodes to
//!   `8*(n-1) + tail` bytes.
//! * The **key** is the string literal `"1212C"` @0x20e4a1, passed to
//!   `Blowfish_InitKey` @0x0016f6c0 by `CGruntzMgr::Run` @0x00083450 at the
//!   `GAME_ATTRIBUTEZ` load (`src/Rez/RezSync.cpp`). `Blowfish_InitKey` is three
//!   instructions:
//!
//!   ```text
//!   16f6c0  8b 44 24 04   mov eax,[esp+4]
//!   16f6c4  6a 04         push 4            <- keybytes
//!   16f6c6  50            push eax
//!   16f6c7  e8 ..         call InitializeBlowfish
//!   ```
//!
//!   so the effective key is **four** bytes, `"1212"` — the trailing `C` of the
//!   literal is never read. See [`ATTRIBUTEZ_KEY`].
//!
//! `CHEATZ` does **not** decode under that key (see [`ATTRIBUTEZ_KEY`] docs);
//! its key is still unknown.
//!
//! # Shape of the API
//!
//! [`Blowfish`] is a 4 168-byte key schedule built once and used by reference.
//! Framing follows the crate convention: `decoded_len` sizes the output from
//! the stream header, `decode_into` fills a caller-supplied buffer, and the
//! encoder comes as the `encoded_len` / `encode_into` pair.

use crate::bute_pi::{INIT_P, INIT_S};

/// The key retail uses for `GAME\ATTRIBUTEZ`.
///
/// Four bytes, not five. `Blowfish_InitKey` @0x0016f6c0 hard-codes
/// `keybytes = 4` while its only call site passes the five-character literal
/// `"1212C"`, and `InitializeBlowfish` @0x00170100 wraps every byte index
/// modulo `keybytes`. The `C` is dead.
///
/// This key does *not* decode `STATEZ\CREDITZ\PALETTEZ\CHEATZ`: that resource
/// comes out at 38% printable with a visible 8-byte ECB period, which is what
/// a wrong key on a correctly-framed stream looks like.
pub const ATTRIBUTEZ_KEY: &[u8] = b"1212";

/// Everything that can go wrong decoding a bute stream. Integers only.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ButeError {
    /// The stream is not `8*n + 1` bytes.
    BadStreamLength { have: usize },
    /// The trailer claimed a final-block length outside `0..=8`.
    BadTailLength { tail: u8 },
    /// The caller's output buffer is the wrong size.
    ShortOutput { need: usize, have: usize },
}

impl core::fmt::Display for ButeError {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match *self {
            ButeError::BadStreamLength { have } => {
                write!(f, "bute stream is {have} bytes, not 8*n + 1")
            }
            ButeError::BadTailLength { tail } => {
                write!(f, "bute length trailer is {tail}, not 0..=8")
            }
            ButeError::ShortOutput { need, have } => {
                write!(f, "output buffer is {have} bytes, need {need}")
            }
        }
    }
}

impl core::error::Error for ButeError {}

/// A Blowfish key schedule: the 18-entry P-array and four 256-entry S-boxes,
/// after the key has been folded in.
#[derive(Clone)]
pub struct Blowfish {
    p: [u32; 18],
    s: [[u32; 256]; 4],
}

impl core::fmt::Debug for Blowfish {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        // The schedule IS the key; printing 4 KiB of it helps nobody.
        write!(f, "Blowfish {{ p[0]: {:#010x}, .. }}", self.p[0])
    }
}

impl Blowfish {
    /// The retail `GAME\ATTRIBUTEZ` schedule — [`ATTRIBUTEZ_KEY`] over four
    /// key bytes.
    pub fn attributez() -> Blowfish {
        Blowfish::with_key_bytes(ATTRIBUTEZ_KEY, 4)
    }

    /// The ordinary Blowfish key schedule: the whole of `key` is used.
    ///
    /// # Panics
    /// If `key` is empty.
    pub fn new(key: &[u8]) -> Blowfish {
        Blowfish::with_key_bytes(key, key.len())
    }

    /// The schedule as retail spells it: `InitializeBlowfish(key, key_bytes)`
    /// @0x00170100, which indexes `key` **modulo `key_bytes`** and therefore
    /// ignores anything past `key_bytes`.
    ///
    /// Exposed separately because retail's `key_bytes` (4) and its key literal
    /// (`"1212C"`, 5 bytes) disagree, and that disagreement is load-bearing —
    /// folding it away would make this function unable to reproduce retail.
    ///
    /// # Panics
    /// If `key_bytes` is 0 or larger than `key.len()`.
    pub fn with_key_bytes(key: &[u8], key_bytes: usize) -> Blowfish {
        let mut bf = Blowfish {
            p: INIT_P,
            s: INIT_S,
        };
        bf.rekey(key, key_bytes);
        bf
    }

    /// Rebuild the schedule in place. Same result as [`Blowfish::with_key_bytes`];
    /// exists so a key search can reuse one 4 KiB schedule instead of moving a
    /// fresh one per candidate.
    ///
    /// # Panics
    /// If `key_bytes` is 0 or larger than `key.len()`.
    pub fn rekey(&mut self, key: &[u8], key_bytes: usize) {
        assert!(key_bytes > 0, "blowfish key_bytes must be non-zero");
        assert!(
            key_bytes <= key.len(),
            "blowfish key_bytes {key_bytes} exceeds the {} byte key",
            key.len()
        );
        let bf = self;
        bf.p = INIT_P;
        bf.s = INIT_S;

        let mut j = 0usize;
        for i in 0..18 {
            // Retail reads key[j] unwrapped and the other three modulo
            // key_bytes; with key_bytes a multiple of 4 (the only case retail
            // uses) `j` never leaves 0, so the two spellings agree. Wrap it
            // anyway so an odd key_bytes cannot index out of bounds.
            let b = |k: usize| u32::from(key[k % key_bytes]);
            let data = (b(j) << 24) | (b(j + 1) << 16) | (b(j + 2) << 8) | b(j + 3);
            bf.p[i] ^= data;
            j = (j + 4) % key_bytes;
        }

        let (mut l, mut r) = (0u32, 0u32);
        let mut i = 0;
        while i < 18 {
            let (nl, nr) = bf.encipher(l, r);
            l = nl;
            r = nr;
            bf.p[i] = l;
            bf.p[i + 1] = r;
            i += 2;
        }
        for box_ in 0..4 {
            let mut k = 0;
            while k < 256 {
                let (nl, nr) = bf.encipher(l, r);
                l = nl;
                r = nr;
                bf.s[box_][k] = l;
                bf.s[box_][k + 1] = r;
                k += 2;
            }
        }
    }

    #[inline]
    fn f(&self, x: u32) -> u32 {
        let s = &self.s;
        // wrapping, not checked: Blowfish's F is defined modulo 2^32, so a
        // wrap here is the algorithm, not an overflow bug.
        (s[0][(x >> 24) as usize].wrapping_add(s[1][((x >> 16) & 0xff) as usize])
            ^ s[2][((x >> 8) & 0xff) as usize])
            .wrapping_add(s[3][(x & 0xff) as usize])
    }

    /// One 16-round encryption. Returns the halves in retail's order — the
    /// reference implementation's trailing swap is already applied.
    pub fn encipher(&self, xl: u32, xr: u32) -> (u32, u32) {
        let (mut l, mut r) = (xl, xr);
        l ^= self.p[0];
        for i in 1..17 {
            if i & 1 == 1 {
                r ^= self.p[i] ^ self.f(l);
            } else {
                l ^= self.p[i] ^ self.f(r);
            }
        }
        r ^= self.p[17];
        (r, l)
    }

    /// One 16-round decryption — [`Blowfish::encipher`] with the P-array walked
    /// backwards (`Blowfish_decipher` @0x0016fc70).
    pub fn decipher(&self, xl: u32, xr: u32) -> (u32, u32) {
        let (mut l, mut r) = (xl, xr);
        l ^= self.p[17];
        for (n, i) in (1..17).rev().enumerate() {
            if n & 1 == 0 {
                r ^= self.p[i] ^ self.f(l);
            } else {
                l ^= self.p[i] ^ self.f(r);
            }
        }
        r ^= self.p[0];
        (r, l)
    }

    /// Decrypt one 8-byte block in place, little-endian halves.
    pub fn decipher_block(&self, block: &mut [u8; 8]) {
        let l = u32::from_le_bytes([block[0], block[1], block[2], block[3]]);
        let r = u32::from_le_bytes([block[4], block[5], block[6], block[7]]);
        let (l, r) = self.decipher(l, r);
        block[..4].copy_from_slice(&l.to_le_bytes());
        block[4..].copy_from_slice(&r.to_le_bytes());
    }

    /// Encrypt one 8-byte block in place, little-endian halves.
    pub fn encipher_block(&self, block: &mut [u8; 8]) {
        let l = u32::from_le_bytes([block[0], block[1], block[2], block[3]]);
        let r = u32::from_le_bytes([block[4], block[5], block[6], block[7]]);
        let (l, r) = self.encipher(l, r);
        block[..4].copy_from_slice(&l.to_le_bytes());
        block[4..].copy_from_slice(&r.to_le_bytes());
    }
}

/// How many plaintext bytes `stream` decodes to.
///
/// The stream is `n` cipher blocks plus a one-byte trailer; the trailer is the
/// valid length of the **last** block, so the answer is `8*(n-1) + tail`.
pub fn decoded_len(stream: &[u8]) -> Result<usize, ButeError> {
    if stream.is_empty() || !(stream.len() - 1).is_multiple_of(8) {
        return Err(ButeError::BadStreamLength { have: stream.len() });
    }
    let blocks = (stream.len() - 1) / 8;
    if blocks == 0 {
        return Ok(0);
    }
    let tail = stream[blocks * 8];
    if tail > 8 {
        return Err(ButeError::BadTailLength { tail });
    }
    Ok((blocks - 1) * 8 + usize::from(tail))
}

/// Decrypt a whole bute stream into `out`, which must be exactly
/// [`decoded_len`] bytes. Returns that length.
pub fn decode_into(bf: &Blowfish, stream: &[u8], out: &mut [u8]) -> Result<usize, ButeError> {
    let need = decoded_len(stream)?;
    if out.len() < need {
        return Err(ButeError::ShortOutput {
            need,
            have: out.len(),
        });
    }
    let blocks = (stream.len() - 1) / 8;
    let mut at = 0;
    for b in 0..blocks {
        let mut blk = [0u8; 8];
        blk.copy_from_slice(&stream[b * 8..b * 8 + 8]);
        bf.decipher_block(&mut blk);
        let take = if b + 1 == blocks { need - at } else { 8 };
        out[at..at + take].copy_from_slice(&blk[..take]);
        at += take;
    }
    Ok(need)
}

/// How many stream bytes `plain` encodes to.
///
/// `CButeTail::Encode` @0x0016f6e0 always writes a whole block per read and
/// then one trailer byte, so an exact multiple of 8 costs an extra all-zero
/// block: the final `read` returns 0 bytes and the block written for it is the
/// encryption of the zeroed buffer.
pub fn encoded_len(plain: &[u8]) -> usize {
    plain.len() / 8 * 8 + 9
}

/// Encrypt `plain` into `out`, which must be exactly [`encoded_len`] bytes.
/// Returns that length.
pub fn encode_into(bf: &Blowfish, plain: &[u8], out: &mut [u8]) -> Result<usize, ButeError> {
    let need = encoded_len(plain);
    if out.len() < need {
        return Err(ButeError::ShortOutput {
            need,
            have: out.len(),
        });
    }
    let full = plain.len() / 8;
    let tail = plain.len() - full * 8;
    for b in 0..full {
        let mut blk = [0u8; 8];
        blk.copy_from_slice(&plain[b * 8..b * 8 + 8]);
        bf.encipher_block(&mut blk);
        out[b * 8..b * 8 + 8].copy_from_slice(&blk);
    }
    // The short (possibly empty) last read: retail memsets the block first, so
    // the pad is zeroes and not stack residue.
    let mut blk = [0u8; 8];
    blk[..tail].copy_from_slice(&plain[full * 8..]);
    bf.encipher_block(&mut blk);
    out[full * 8..full * 8 + 8].copy_from_slice(&blk);
    out[full * 8 + 8] = tail as u8;
    Ok(need)
}
