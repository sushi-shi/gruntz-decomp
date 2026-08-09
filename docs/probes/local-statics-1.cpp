// A: non-inline fn, CONSTANT-init local static  -> initialized data
void a() { static int hello = 4; hello++; }
// B: non-inline fn, ZERO-init local static      -> uninitialized data
void b() { static int zed; zed++; }
// C: non-inline fn, DYNAMIC-init local static   -> needs a guard
int side();
void c() { static int dyn = side(); dyn++; }
// D: INLINE fn (as if from a header), const-init local static
inline void d() { static int inl = 7; inl++; }
void use_d() { d(); }
// E: class static, out-of-line definition
struct CFoo { static int s_m; };
int CFoo::s_m = 9;
// F: plain file-scope, for the ordering baseline
int g_first = 1;
int g_zero;
