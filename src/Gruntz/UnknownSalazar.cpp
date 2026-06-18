// UnknownSalazar.cpp - getLookupTableValue and initializeUnknownLookupTable,
// plus computeScaleFactor, of the tomalla-named class UnknownSalazar.
//
// getLookupTableValue is a framed leaf that maps an input value through a two-part
// branch (100->0, 0->-10000) then a chain of arithmetic: value / A, B / that,
// pow(C, result), acos of that, acos of D, ratio, negate, multiply by A,
// convert to int. The double constants A/B/C/D are global engine globals at
// the .rdata addresses below; the CRT helpers _CIpow (pow), _CIacos (acos),
// and __ftol (double->int) are reloc-masked LIB/CRT calls.
//
// initializeUnknownLookupTable fills a 100-element static int table with
// getLookupTableValue results for indices 0..99.
//
// computeScaleFactor maps an int through: value/100 -> sqrt -> mul/sub chain
// -> ftol or negate+ftol based on sign.
//
// Field names are placeholders; only OFFSETS + emitted code bytes are load-
// bearing (campaign doctrine).
// ---------------------------------------------------------------------------

// The four double constants (DIR64 in .rdata, reloc-masked VA operands).
// @data: 0x1ef698
extern double g_salazarConstA;      // 0x5ef698  first divisor
// @data: 0x1ef6a0
extern double g_salazarConstB;      // 0x5ef6a0  second divisor/reverse
// @data: 0x1ef6a8
extern double g_salazarConstC;      // 0x5ef6a8  base for acos+pow
// @data: 0x1ef6b0
extern double g_salazarConstD;      // 0x5ef6b0  divisor under acos denominator

// CRT math helpers (intrinsic names per MSVC 5.0 LIBCMT).
// Declared as extern "C" so the calls resolve to _CIpow / _CIacos.
extern "C" double _CIpow(double, double);
extern "C" double _CIacos(double);
extern "C" double _CIsqrt(double);
extern "C" long __ftol(double);

// The lookup table buffer (binary @0x653ab8, 100 ints). Written by
// initializeUnknownLookupTable with getLookupTableValue results.
// @data: 0x653ab8
static int g_salazarLookupTable[100];

class UnknownSalazar {
public:
    static int getLookupTableValue(int value);
    static void initializeUnknownLookupTable();
    static int computeScaleFactor(int value);
};

// ---------------------------------------------------------------------------
// UnknownSalazar::getLookupTableValue  @0x1350b0  (static, ret 4)
// Maps the input value through the computed formula:
//   result = -A * acos(pow(C, A * B / value)) / acos(D)
// Special cases: value == 100 -> 0; value == 0 -> -10000.
// ---------------------------------------------------------------------------
// @address: 0x1350b0
// @size:    0x5d
int UnknownSalazar::getLookupTableValue(int value)
{
    if (value == 100)
        return 0;
    if (value == 0)
        return -10000;

    double t = (double)value;
    double ab = g_salazarConstA * g_salazarConstB;
    double p = _CIpow(g_salazarConstC, ab / t);
    double n = _CIacos(p);
    double d = _CIacos(g_salazarConstD);
    double ratio = n / d;

    return (int)(-g_salazarConstA * ratio);
}

// ---------------------------------------------------------------------------
// UnknownSalazar::initializeUnknownLookupTable  @0x1351a0  (static, void)
// Fills the 100-element static lookup table at 0x653ab8 with
// getLookupTableValue(i) for i = 0..99. Plain /O2 /MT leaf: no SEH frame.
// ---------------------------------------------------------------------------
// @address: 0x1351a0
// @size:    0x23
void UnknownSalazar::initializeUnknownLookupTable()
{
    for (int i = 0; i < 100; i++)
        g_salazarLookupTable[i] = getLookupTableValue(i);
}

// ---------------------------------------------------------------------------
// UnknownSalazar::computeScaleFactor  @0x135110  (static, ret 4)
// Computes: A - A*(B - sqrt(-value/100 / C)) with sign-based negation.
// Special case: value == 0 -> return 100.
// Returns __ftol(result) for value<0, __ftol(-result) for value>=0.
// ---------------------------------------------------------------------------
// @address: 0x135110
// @size:    0x8e
int UnknownSalazar::computeScaleFactor(int value)
{
    if (value == 0)
        return 100;

    int scaled = value / 100;
    double t;
    if (value < 0) {
        t = (double)(-scaled);
    } else {
        t = (double)(scaled);
    }

    double result = g_salazarConstA - g_salazarConstA
                    * (g_salazarConstB - _CIsqrt(-t / g_salazarConstC));

    if (value < 0)
        return __ftol(result);
    else
        return __ftol(-result);
}
