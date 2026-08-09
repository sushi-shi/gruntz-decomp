int side();
struct CBar { int x; CBar(); };
inline void iz() { static int zi; zi++; }            // inline, ZERO-init
inline void id() { static int dv = side(); dv++; }   // inline, DYNAMIC-init
inline void io() { static CBar obj; obj.x++; }       // inline, ctor
void use() { iz(); id(); io(); }
