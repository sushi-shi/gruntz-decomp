# Pin a named local (or split a pointer-chain into an intermediate) to force MSVC's register/schedule — but the lever is arg-count-dependent
tags: cpp:local | asm:mov asm:push | topic:codegen-idiom topic:regalloc
symptoms: the target caches a value in a callee-saved reg (esi/edi/ebp) across calls / interleaves a load between arg pushes, but your fully-inline expression re-reads it or schedules it after all args
confidence: 8/10

When the target keeps a value in a callee-saved register across calls, or interleaves a pointer
load between arg pushes, a fully-inline expression won't reproduce it (cl re-reads the field or
schedules it late). STEER it by giving the value a named local — this forces cl to allocate the
callee-saved reg and pick the target's schedule. Variants of the lever:

- **Pin `int x = h->m_5c; int y = h->m_60;`** to force the 4th callee-saved reg (+ the matching
  `push ebp`) for a multi-compare visible-bounds gate.
- **Pin `char* tok = m_token; CButeTree* t = Tree();`** reused across two calls to make cl
  allocate ebp (used as a GP reg under /O2) like the target.
- **Split a pointer chain into a named intermediate** (`CSpriteInner* inner = sprite->m_7c;
  CSpriteRegistrar* reg = inner->m_18;`) to flip cl to the eax-in-place + interleaved-load schedule.

CAVEAT — **the optimal lever is ARG-COUNT-dependent**: the `inner`-split HELPS the 3-arg Add
(95.8→99.3%) but HURTS the 2-arg Add (makes it 90.7% vs 91.6% plain). Pick per-function; don't
apply uniformly. Some residues are pure allocator coin-flips no local flips — that's the WALL
(zero-register-pinning.md), not this steerable lever.

STEERABLE (when a local flips it). Evidence: CGrunt::Resolve* x/y locals +~6% (Death 89→95);
ButeMgr::ParseTagLine ebp hint 84→97%; CGrunt sprite creators `inner` split 95.8→99.3% (3-arg).
