# A TU's COFF section ORDER homes an uncarved body — bracket it in .text AND .data at once
tags: cpp:global cpp:string cpp:template msvc5:gy msvc5:gf | topic:identity topic:layout topic:data
symptoms: dead function in a census hole, "no admitted row covers this address", unclaimed pooled literal between two units, which TU owns this, link_order band gap, `$E` dynamic-init block
confidence: 9/10

cl 5.0 emits an obj's sections in a FIXED order and the linker concatenates each
obj's contributions in that order, so both `.text` and `.data` carry the same
per-TU sequence. Two independent brackets on one dead body therefore pin its
owner even when the retail band tables leave a hole. The order, measured on our
own objs (`llvm-objdump -h/-t build/objdiff/base/<unit>.obj`):

    .drectve | initialized .data | .bss | .CRT$XCU
    | the `$E` dynamic-init COMDAT pairs (thunk 0x10 + body 0x20, one pair per
      file-scope static needing dynamic init)   <-- ALWAYS FIRST IN .text
    | for each function IN SOURCE ORDER: its `.text`, its `.text$x` EH funclets,
      then ITS OWN pooled `??_C@` literals in REVERSE intern order (right-to-left
      per call: `fopen(name, mode)` interns "mode" first, so "name" gets the
      LOWER address), then its `.xdata$x`
    | `??_R0` RTTI type descriptors where their vtable is emitted (AFTER strings)
    | template instantiation COMDATs                      <-- ALWAYS LAST

Worked case — the dead `fopen("c:\\foo.log","wb")` pair at 0x0942e0 / 0x094310,
sitting in a census hole between levelrezpath (ends 0x0941b3) and gruntzwnd:

* `.text`: gruntzwnd's `$E` block starts at 0x094370, and `$E` is always the
  FIRST `.text` of its obj — so everything below 0x094370 is an EARLIER obj, and
  levelrezpath is the only candidate. (Scan for it: the block is 9 pairs of
  `jmp <body>` / `mov ds:<static>,<imm>` runs, easily missed by a fill scan that
  treats `0x00` as padding.)
* `.data`: levelrezpath's four literals run LEVEL%i 0x21110c, TRAINING%i
  0x211118, AREA%i_WORLDZ 0x211128, GAME_MULTI 0x211138 — reverse of their use
  in BuildLevelRezPath. `c:\foo.log` 0x211148 and `wb` 0x211158 continue that
  run and stop exactly at gruntzwnd's first `.data`, its `??_R0?AVCGameWnd@@@8`
  at 0x211160. A function's strings follow its own `.text`, so the pair belongs
  to a levelrezpath function emitted AFTER BuildLevelRezPath.
* Corroboration: `??0?$CArray@PAUPLAYLISTINFOSTRUCT@@PAU1@@@QAE@XZ` at 0x094340
  closes the obj — template instantiations are emitted last.

Identity, not a score lever. `.bss` proves NOTHING here — retail's uninitialized
globals are COMMON-pooled and interleave units (bootystateactivate's run reappears
inside gruntzmgr's), so `g_logFile` @0x245510 sitting beside bootystateactivate's
globals is adjacency, not ownership; its only two referrers are the two helpers.
Both bodies scored 100.0000 in levelrezpath.
