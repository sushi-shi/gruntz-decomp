//! Lossless integer conversions, so that no other crate in this workspace has
//! to write `as`.
//!
//! `as` between integer types is the one Rust operator that silently discards
//! information, and a codec is exactly the place where that costs you a day:
//! a truncated length reads as a short sprite, not as a compile error.
//!
//! The tools target **x86-64 only** — that is asserted below, not assumed — so
//! `u32 -> usize` and `usize -> u64` are provably lossless and get infallible
//! methods. Anything that *could* lose information is deliberately absent:
//! use `TryFrom` and handle the failure.
//!
//! Deliberate truncation still happens (retail keeps only the low byte of a
//! fill colour, for instance). That is what [`LowByte`] is for: it is loud,
//! greppable, and its name says the information loss is the point.

#![no_std]
// `as_*(self)` on a Copy scalar is exactly the convention the standard library
// uses (`u32::to_le_bytes(self)`, `char::to_ascii_uppercase(self)`); taking a
// `&u32` here would be strictly worse. The lint is aimed at `as_*` on owning
// types, which none of these are.
#![allow(clippy::wrong_self_convention)]

const _: () = assert!(
    usize::BITS == 64,
    "gruntz tools target x86-64; on a 32-bit host u32 -> usize would truncate"
);

/// Widen to `usize`. Only implemented where the conversion cannot lose bits on
/// a 64-bit target.
pub trait AsUsize {
    fn as_usize(self) -> usize;
}

/// Widen to `u64`.
pub trait AsU64 {
    fn as_u64(self) -> u64;
}

/// Widen to `i64`.
pub trait AsI64 {
    fn as_i64(self) -> i64;
}

macro_rules! impl_as_usize {
    ($($t:ty),*) => { $(
        impl AsUsize for $t {
            #[inline(always)]
            fn as_usize(self) -> usize {
                // Infallible on a 64-bit target; the const assert above proves it.
                usize::try_from(self).unwrap_or_else(|_| unreachable!())
            }
        }
    )* };
}

macro_rules! impl_as_u64 {
    ($($t:ty),*) => { $(
        impl AsU64 for $t {
            #[inline(always)]
            fn as_u64(self) -> u64 {
                u64::try_from(self).unwrap_or_else(|_| unreachable!())
            }
        }
    )* };
}

macro_rules! impl_as_i64 {
    ($($t:ty),*) => { $(
        impl AsI64 for $t {
            #[inline(always)]
            fn as_i64(self) -> i64 {
                i64::from(self)
            }
        }
    )* };
}

impl_as_usize!(u8, u16, u32, u64, usize);
impl_as_u64!(u8, u16, u32, u64, usize);
impl_as_i64!(i8, i16, i32, u8, u16, u32);

/// The low byte of a wider value — an *intentional* truncation.
///
/// Named rather than spelled `as u8` so that every place the format throws
/// information away is greppable and reviewable.
pub trait LowByte {
    fn low_byte(self) -> u8;
}

macro_rules! impl_low_byte {
    ($($t:ty),*) => { $(
        impl LowByte for $t {
            #[inline(always)]
            fn low_byte(self) -> u8 {
                self.to_le_bytes()[0]
            }
        }
    )* };
}

impl_low_byte!(u16, u32, u64, usize, i16, i32, i64, isize);

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn widening_is_value_preserving() {
        assert_eq!(u32::MAX.as_usize(), 4_294_967_295);
        assert_eq!(usize::MAX.as_u64(), u64::MAX);
        assert_eq!(i32::MIN.as_i64(), -2_147_483_648);
    }

    #[test]
    fn low_byte_takes_the_low_byte() {
        assert_eq!(0x1234_5678u32.low_byte(), 0x78);
        assert_eq!((-1i32).low_byte(), 0xff);
    }
}
