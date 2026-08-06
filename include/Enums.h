#ifndef GRUNTZ_ENUMS_H
#define GRUNTZ_ENUMS_H

#include <Ints.h>

// One declaration, two expansions - the enum-domain layer.
//
// MSVC 5.0 (the matching compiler) has no scoped enums and no fixed underlying
// type, and it sizes EVERY enum as `int`. A field that retail stores in one byte
// therefore cannot be enum-typed. So a domain is declared once here and expands
// two ways, selected by language level - never by a build flag, so no build can
// disagree with another about what the source says.
//
//   * MSVC 5.0 + the clang readers (clangd, the label/IR pass) take the RETAIL
//     branch. Value domains become real 4-byte `enum`s; narrow fields keep their
//     proven storage type; flag domains stay plain integer arithmetic.
//   * A C++20 clang pass takes the STRICT branch: `enum class` domains, a
//     width-preserving storage proxy, and operators only where the domain has
//     them. It type-checks the model; it never builds the game.
//
// Measured on VC5.0 SP3 (cl 11.00) - see docs/patterns/enum-domains.md:
//   - `enum class` and `enum E : u8` do NOT parse (C2236 / C2059).
//   - `sizeof(enum)` is 4 for every enum, whatever its enumerators.
//   - Retyping a member, parameter, return or switch key from `i32` to a real
//     enum, and replacing literals with enumerators, leaves `.text` BYTE
//     IDENTICAL. Only the mangled name of a signature changes (`H` -> `W4Kind@@`).
//   - An OPAQUE forward declaration (`enum Kind;`) is accepted as a member,
//     parameter and return type, yields the same mangling and the same `.text`,
//     and makes the class 4 bytes wide. Use it to type a header without pulling
//     in the domain's definition, which is what keeps the /O2 regalloc butterfly
//     out of the type-application work.

#if defined(__cplusplus) && __cplusplus >= 202002L
#define GZ_STRICT_ENUMS 1
#else
#define GZ_STRICT_ENUMS 0
#endif

// ---------------------------------------------------------------------------
// Domain declaration
//
//   GZ_ENUM_BEGIN/END(N)              a value domain, 4 bytes everywhere
//   GZ_ENUM_BEGIN/END_SPLIT(N, S)     a value domain ALSO stored narrow
//   GZ_ENUM_CONST_BEGIN/END(N)        a constant bag - counts, extents, masks;
//                                     never a variable's type
//   GZ_ENUM_FLAGS_BEGIN/END(N, S)     a bitflag domain
//   GZ_ENUM_FORWARD(N)                opaque forward declaration
//   GZ_ENUM_FORWARD_SPLIT(N, S)       ditto, for a split domain
//
// A 4-byte field/param/return of domain N is spelled `N` directly - no macro.
// The macros below exist only where retail's type differs from the domain type.
//
//   GZ_ENUM_STORAGE(N, S)             a field of domain N stored in S bytes
//   GZ_ENUM_PARAM(N, S)               a parameter retail passes as S
//   GZ_ENUM_RETURN(N, S)              a return retail declares as S
//   GZ_ENUM_BITFIELD(N, S)            a `: n` bitfield member of domain N
//   GZ_ENUM_FLAGS_OPS(N)              the flag operator set (strict only)
//   GZ_ENUM_STEPPED(N)                the sequence operator set (strict only)
//
// Flag and sequence operators are STRICT-ONLY on purpose: an inline operator in
// a hot header perturbs cl 5.0's /Ob1 inline budget, so the retail branch keeps
// the plain integer arithmetic retail actually compiled.
// ---------------------------------------------------------------------------

#if GZ_STRICT_ENUMS

#define GZ_ENUM_BEGIN(name) enum class name : i32 {
#define GZ_ENUM_END(name)                                                                          \
    }                                                                                              \
    ;                                                                                              \
    using enum name;

#define GZ_ENUM_BEGIN_SPLIT(name, storage) enum class name : storage {
#define GZ_ENUM_END_SPLIT(name, storage)                                                           \
    }                                                                                              \
    ;                                                                                              \
    using enum name;

#define GZ_ENUM_FLAGS_BEGIN(name, storage) enum class name : storage {
#define GZ_ENUM_FLAGS_END(name, storage)                                                           \
    }                                                                                              \
    ;                                                                                              \
    using enum name;

#define GZ_ENUM_FORWARD(name) enum class name : i32
#define GZ_ENUM_FORWARD_SPLIT(name, storage) enum class name : storage

#define GZ_ENUM_STORAGE(name, storage) GzEnumStorage<name, storage>
#define GZ_ENUM_PARAM(name, storage) name
#define GZ_ENUM_RETURN(name, storage) name
#define GZ_ENUM_BITFIELD(name, storage) name

// A domain can appear in fields of several widths. Keep each field's exact
// representation while presenting the domain type to strict-build expressions.
// Conversion TO the enum is implicit; conversion to an integer is explicit, so a
// domain value can never silently become an array index or another domain.
template<typename Enum, typename Storage> class GzEnumStorage {
public:
    GzEnumStorage() = default;
    constexpr GzEnumStorage(Enum value) : m_value(static_cast<Storage>(value)) {}
    constexpr GzEnumStorage(Storage value) : m_value(value) {}

    constexpr operator Enum() const {
        return static_cast<Enum>(m_value);
    }

    explicit constexpr operator i32() const {
        return static_cast<i32>(m_value);
    }

    GzEnumStorage& operator=(Enum value) {
        m_value = static_cast<Storage>(value);
        return *this;
    }

    GzEnumStorage& operator=(Storage value) {
        m_value = value;
        return *this;
    }

    GzEnumStorage& operator|=(Enum value) {
        m_value = static_cast<Storage>(m_value | static_cast<Storage>(value));
        return *this;
    }

    GzEnumStorage& operator&=(Enum value) {
        m_value = static_cast<Storage>(m_value & static_cast<Storage>(value));
        return *this;
    }

    GzEnumStorage& operator^=(Enum value) {
        m_value = static_cast<Storage>(m_value ^ static_cast<Storage>(value));
        return *this;
    }

private:
    Storage m_value;
};

template<typename Enum, typename Storage>
constexpr bool operator==(GzEnumStorage<Enum, Storage> lhs, Enum rhs) {
    return static_cast<Enum>(lhs) == rhs;
}

template<typename Enum, typename Storage>
constexpr bool operator!=(GzEnumStorage<Enum, Storage> lhs, Enum rhs) {
    return !(lhs == rhs);
}

template<typename Enum, typename LeftStorage, typename RightStorage>
constexpr bool
operator==(GzEnumStorage<Enum, LeftStorage> lhs, GzEnumStorage<Enum, RightStorage> rhs) {
    return static_cast<Enum>(lhs) == static_cast<Enum>(rhs);
}

template<typename Enum, typename LeftStorage, typename RightStorage>
constexpr bool
operator!=(GzEnumStorage<Enum, LeftStorage> lhs, GzEnumStorage<Enum, RightStorage> rhs) {
    return !(lhs == rhs);
}

template<typename Enum, typename Storage>
constexpr bool operator<(GzEnumStorage<Enum, Storage> lhs, Enum rhs) {
    return static_cast<Enum>(lhs) < rhs;
}

template<typename Enum, typename Storage>
constexpr bool operator>=(GzEnumStorage<Enum, Storage> lhs, Enum rhs) {
    return static_cast<Enum>(lhs) >= rhs;
}

template<typename Enum, typename Storage>
constexpr i32 GzEnumIndex(GzEnumStorage<Enum, Storage> value) {
    return static_cast<i32>(value);
}

template<typename Value> constexpr i32 GzEnumIndex(Value value) {
    return static_cast<i32>(value);
}

#define GZ_ENUM_FLAGS_OPS(name)                                                                    \
    inline constexpr name operator|(name a, name b) {                                              \
        return static_cast<name>(static_cast<i32>(a) | static_cast<i32>(b));                       \
    }                                                                                              \
    inline constexpr name operator&(name a, name b) {                                              \
        return static_cast<name>(static_cast<i32>(a) & static_cast<i32>(b));                       \
    }                                                                                              \
    inline constexpr name operator^(name a, name b) {                                              \
        return static_cast<name>(static_cast<i32>(a) ^ static_cast<i32>(b));                       \
    }                                                                                              \
    inline constexpr name operator~(name a) {                                                      \
        return static_cast<name>(~static_cast<i32>(a));                                            \
    }                                                                                              \
    inline constexpr name& operator|=(name& a, name b) {                                           \
        return a = a | b;                                                                          \
    }                                                                                              \
    inline constexpr name& operator&=(name& a, name b) {                                           \
        return a = a & b;                                                                          \
    }                                                                                              \
    inline constexpr name& operator^=(name& a, name b) {                                           \
        return a = a ^ b;                                                                          \
    }                                                                                              \
    inline constexpr bool operator!(name a) {                                                      \
        return !static_cast<i32>(a);                                                               \
    }

#define GZ_ENUM_STEPPED(name)                                                                      \
    inline constexpr name operator+(name a, i32 amount) {                                          \
        return static_cast<name>(static_cast<i32>(a) + amount);                                    \
    }                                                                                              \
    inline constexpr name operator-(name a, i32 amount) {                                          \
        return static_cast<name>(static_cast<i32>(a) - amount);                                    \
    }                                                                                              \
    inline constexpr i32 operator-(name a, name b) {                                               \
        return static_cast<i32>(a) - static_cast<i32>(b);                                          \
    }                                                                                              \
    inline name& operator+=(name& a, i32 amount) {                                                 \
        return a = a + amount;                                                                     \
    }                                                                                              \
    inline name& operator-=(name& a, i32 amount) {                                                 \
        return a = a - amount;                                                                     \
    }                                                                                              \
    inline name& operator++(name& a) {                                                             \
        return a = a + 1;                                                                          \
    }                                                                                              \
    inline name operator++(name& a, i32) {                                                         \
        name old = a;                                                                              \
        ++a;                                                                                       \
        return old;                                                                                \
    }                                                                                              \
    inline name& operator--(name& a) {                                                             \
        return a = a - 1;                                                                          \
    }                                                                                              \
    inline name operator--(name& a, i32) {                                                         \
        name old = a;                                                                              \
        --a;                                                                                       \
        return old;                                                                                \
    }

#else // !GZ_STRICT_ENUMS - the retail (MSVC 5.0) branch

// Value domains are REAL enums here. Measured byte-neutral, and it keeps the
// manglings the tree already pins (`?GetTypeTag@...@@UAE?AW4LogicTypeId@@XZ`).
// Gruntz has no PDB, so nothing external constrains the declared type - unlike
// homm2-decomp, whose CodeView stream forced `typedef i32` on every domain.
#define GZ_ENUM_BEGIN(name) enum name {
#define GZ_ENUM_END(name)                                                                          \
    }                                                                                              \
    ;

// A split domain's public type is still the 4-byte enum; only the narrow FIELDS
// differ, and they say so with GZ_ENUM_STORAGE.
#define GZ_ENUM_BEGIN_SPLIT(name, storage) enum name {
#define GZ_ENUM_END_SPLIT(name, storage)                                                           \
    }                                                                                              \
    ;

// Flag domains stay integers: retail combined and cleared bits with plain
// arithmetic, and an inline operator would perturb the /Ob1 inline budget.
#define GZ_ENUM_FLAGS_BEGIN(name, storage) enum {
#define GZ_ENUM_FLAGS_END(name, storage)                                                           \
    }                                                                                              \
    ;                                                                                              \
    typedef i32 name;

#define GZ_ENUM_FORWARD(name) enum name
#define GZ_ENUM_FORWARD_SPLIT(name, storage) enum name

#define GZ_ENUM_STORAGE(name, storage) storage
#define GZ_ENUM_PARAM(name, storage) storage
#define GZ_ENUM_RETURN(name, storage) storage
#define GZ_ENUM_BITFIELD(name, storage) storage

#define GZ_ENUM_FLAGS_OPS(name)
#define GZ_ENUM_STEPPED(name)

#endif // GZ_STRICT_ENUMS

// A constant bag is never a variable's type, so it expands the same in both
// branches: an anonymous enum whose name exists only to group the constants.
#define GZ_ENUM_CONST_BEGIN(name) enum {
#define GZ_ENUM_CONST_END(name)                                                                    \
    }                                                                                              \
    ;

// Spell the intent without moving a byte. Retail expands each to the bare
// expression, so these are free.
//
// AT(array, value) indexes an array whose INDEX SPACE is a domain - the pose
// tables, the per-player slots, the icon rows. This is the one place a domain
// legitimately becomes a number, so it gets its own spelling instead of leaving
// a bare IDX at every subscript.
//
// IDX(x) is then what is left: a genuine numeric conversion at a boundary the
// type system cannot follow (a Win32 message parameter, a checksum sum). It is
// NOT a way to silence a comparison - `a == IDX(B)` means one side is mis-typed,
// and `case IDX(x):` means the switch key belongs to a domain nobody declared.
#if GZ_STRICT_ENUMS
#define AT(a, i) (a)[GzEnumIndex(i)]
#define IDX(x) GzEnumIndex(x)
#define HAS(flags, bit) (IDX((flags) & (bit)))
#define BIT(x) (1 << IDX(x))
#else
#define AT(a, i) (a)[i]
#define IDX(x) (x)
#define HAS(flags, bit) ((flags) & (bit))
#define BIT(x) (1 << (x))
#endif

#endif // GRUNTZ_ENUMS_H
