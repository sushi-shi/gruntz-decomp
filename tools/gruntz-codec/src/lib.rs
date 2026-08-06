//! Clean-room codecs for Gruntz resource formats — `no_std`, no `alloc`.
//!
//! The image grammars in this crate were derived from two sources only:
//!
//! 1. retail `GRUNTZ.EXE` disassembly (`gruntz sema disasm <rva> --target`), and
//! 2. the archived bytes in `GRUNTDEM.REZ` / `Gruntz.REZ`.
//!
//! XMI additionally uses the format owner's declarations in the vendored Miles
//! SDK header. It deliberately does **not** import another codec implementation
//! or consult the C++ reconstruction under `src/`.
//! That is the whole point: where this crate and `src/` disagree, one of them
//! is wrong, and the disagreeing sprite is the reproducer.
//!
//! Image-codec public items cite the retail RVA that proves them; container
//! formats identify their archive or format-owner evidence in module docs.
//!
//! # Shape of the API
//!
//! Zero-copy throughout, and therefore genuinely `no_std` — not `no_std` with
//! an `alloc` fig leaf:
//!
//! * headers **borrow** (`split` hands back `&[u8]` slices into the resource);
//! * decoders write into a caller-supplied `&mut [u8]` and return the number of
//!   stream bytes consumed — the output size is `width * height`, known from
//!   the header before any work happens;
//! * encoders come in pairs: `..._len` sizes the output, `..._into` writes it;
//! * token walks are `Iterator`s over borrowed slices, never a collected `Vec`;
//! * errors carry integers only, never a formatted `String`.
//!
//! No third-party dependency either. Every multi-byte field is read explicitly
//! little-endian rather than cast from a struct, so the codec is correct on a
//! big-endian host; a `bytemuck` struct cast would have quietly given that up.

#![no_std]

pub mod ani;
pub mod bmp;
pub mod pal;
pub mod pcx;
pub mod pid;
pub mod rid;
pub mod rle16;
pub mod xmi;

/// A byte sink that either counts or writes — the one primitive that lets each
/// encoder be written **once** and used both to size and to emit.
pub(crate) enum Sink<'a> {
    Count(usize),
    Write { buf: &'a mut [u8], at: usize },
}

impl Sink<'_> {
    pub(crate) fn push(&mut self, b: u8) -> bool {
        match self {
            Sink::Count(n) => {
                *n += 1;
                true
            }
            Sink::Write { buf, at } => match buf.get_mut(*at) {
                Some(slot) => {
                    *slot = b;
                    *at += 1;
                    true
                }
                None => false,
            },
        }
    }

    pub(crate) fn extend(&mut self, bytes: &[u8]) -> bool {
        match self {
            Sink::Count(n) => {
                *n += bytes.len();
                true
            }
            Sink::Write { buf, at } => match buf.get_mut(*at..*at + bytes.len()) {
                Some(slot) => {
                    slot.copy_from_slice(bytes);
                    *at += bytes.len();
                    true
                }
                None => false,
            },
        }
    }

    pub(crate) fn len(&self) -> usize {
        match self {
            Sink::Count(n) => *n,
            Sink::Write { at, .. } => *at,
        }
    }
}
