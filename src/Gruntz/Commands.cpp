// Commands.cpp - Gruntz CGruntzCommand family (C:\Proj\Gruntz).
// Five byte-exact targets in a single plain /O2 /MT TU.
//
// CGruntzCommand   default ctor           @0x242f0  (7 B)  vptr store
// CGruntzCommand   copy ctor              @0x24430  (7 B)  vptr store
// CGruntzCommand   scalar-deleting dtor   @0x24330  (32 B)  vptr + delete-flag
// CGruntzSingleCommand  factory ("ctor")  @0x24220  (43 B)  pool flag -> handler / new+vptr
// CGruntzMultiCommand   factory ("ctor")  @0x24360  (43 B)  pool flag -> handler / new+vptr
//
// Field names are placeholders; only the OFFSETS + the emitted code bytes are
// load-bearing (campaign doctrine).
#include "Commands.h"

// ---------------------------------------------------------------------------
// The three vftables, referenced as DIR32 data (RVA = VA - 0x400000).
// @data: 0x1e9674
extern void *g_cgruntzCommandVtbl;         // VA 0x5e9674  CGruntzCommand
// @data: 0x1e9634
extern void *g_cgruntzSingleCommandVtbl;   // VA 0x5e9634  CGruntzSingleCommand
// @data: 0x1e96b4
extern void *g_cgruntzMultiCommandVtbl;    // VA 0x5e96b4  CGruntzMultiCommand
// ---------------------------------------------------------------------------
// The two command-pool globals (SingleCommand: flag @0x62b5dc, pool @0x62b5d0).
// @data: 0x225bdc
extern int g_cmdSinglePoolFlag;      // DAT_0062b5dc  VA 0x62b5dc
// @data: 0x225bd0
extern int g_cmdSinglePoolParam;     // DAT_0062b5d0  VA 0x62b5d0
// ---------------------------------------------------------------------------
// The two command-pool globals (MultiCommand: flag @0x62b64c, pool @0x62b640).
// @data: 0x225b4c
extern int g_cmdMultiPoolFlag;       // DAT_0062b64c  VA 0x62b64c
// @data: 0x225b40
extern int g_cmdMultiPoolParam;      // DAT_0062b640  VA 0x62b640

// Pool-allocator handler at 0x1b4a27.  Takes one __fastcall param in ecx,
// returns void*.  External/no-body; the call displacement is reloc-masked.
void *__fastcall CmdPoolAllocHandler(int param);

// ---------------------------------------------------------------------------
// CGruntzCommand::CGruntzCommand()  @0x242f0
// Trivial default ctor: just stores the vtable pointer, returns this.
//
// @address: 0x242f0
// @size:    0x7
// ---------------------------------------------------------------------------
CGruntzCommand::CGruntzCommand()
{
}

// ---------------------------------------------------------------------------
// CGruntzCommand::CGruntzCommand(const CGruntzCommand&)  @0x24430
// Trivial copy ctor: stores the vtable pointer, returns this.
//
// @address: 0x24430
// @size:    0x7
// ---------------------------------------------------------------------------
CGruntzCommand::CGruntzCommand(const CGruntzCommand &)
{
}

// ---------------------------------------------------------------------------
// CGruntzCommand::~CGruntzCommand()  @0x24330
// Scalar-deleting destructor: stores the vtable (for the dtor body),
// then if the ~0x1 flag is set, calls operator delete(this).
//
// @address: 0x24330
// @size:    0x20
// ---------------------------------------------------------------------------
CGruntzCommand::~CGruntzCommand()
{
}

// ---------------------------------------------------------------------------
// CGruntzSingleCommand::Create()  @0x24220
// Factory: checks the pool flag; if active, tail-calls the pool allocator
// with the pool param in ecx.  Otherwise `operator new(0x14)` and stamps
// the vtable into the raw block.  Returns null on alloc failure.
//
// In the retail binary this function bears the mangling of the default ctor
// (??0CGruntzSingleCommand@@QAE@XZ) because the C++ source combined the
// "new" expression and the ctor body into one TU.  We model it as a static
// `Create` factory; the @address annotation bridges the RVA.
//
// @address: 0x24220
// @size:    0x2b
// ---------------------------------------------------------------------------
CGruntzSingleCommand *CGruntzSingleCommand::Create()
{
    if (g_cmdSinglePoolFlag) {
        return (CGruntzSingleCommand *)CmdPoolAllocHandler(g_cmdSinglePoolParam);
    }
    CGruntzSingleCommand *raw = (CGruntzSingleCommand *)operator new(0x14);
    if (raw != 0) {
        *(void **)raw = &g_cgruntzSingleCommandVtbl;
    }
    return raw;
}

// ---------------------------------------------------------------------------
// CGruntzMultiCommand::Create()  @0x24360
// Same factory shape as SingleCommand but with the Multi pool globals and
// the Multi vtable (@0x5e96b4).
//
// @address: 0x24360
// @size:    0x2b
// ---------------------------------------------------------------------------
CGruntzMultiCommand *CGruntzMultiCommand::Create()
{
    if (g_cmdMultiPoolFlag) {
        return (CGruntzMultiCommand *)CmdPoolAllocHandler(g_cmdMultiPoolParam);
    }
    CGruntzMultiCommand *raw = (CGruntzMultiCommand *)operator new(0x14);
    if (raw != 0) {
        *(void **)raw = &g_cgruntzMultiCommandVtbl;
    }
    return raw;
}
