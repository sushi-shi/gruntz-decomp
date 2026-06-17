// Commands.h - Gruntz command-pattern base/leaf classes (C:\Proj\Gruntz).
// RTTI-derived names; layouts sufficient to match ctors/dtors.
//
// CGruntzCommand (@ vftable 0x5e9674) -- base command.
//   Two ctors (default + copy = each 7 B, just the vptr store).
//   One scalar-deleting dtor (32 B).
// CGruntzSingleCommand (@ vftable 0x5e9634) -- single-player command.
//   Lifetime managed by a pool allocator (flag @ 0x62b5dc, pool @ 0x62b5d0).
// CGruntzMultiCommand (@ vftable 0x5e96b4) -- multiplayer command.
//   Lifetime managed by a pool allocator (flag @ 0x62b64c, pool @ 0x62b640).
//
// Field names are placeholders; only the OFFSETS + the emitted code bytes are
// load-bearing (campaign doctrine).
#ifndef SRC_GRUNTZ_COMMANDS_H
#define SRC_GRUNTZ_COMMANDS_H

class CGruntzCommand {
public:
    CGruntzCommand();
    CGruntzCommand(const CGruntzCommand &);
    virtual ~CGruntzCommand();
};

class CGruntzSingleCommand : public CGruntzCommand {
public:
    // The retail constructor at RVA 0x24220 is actually a factory: it checks a
    // pool flag and either tail-calls the pool handler or allocates + stamps
    // the vtable.  Modeled as a static factory here.
    static CGruntzSingleCommand *Create();
};

class CGruntzMultiCommand : public CGruntzCommand {
public:
    static CGruntzMultiCommand *Create();
};

#endif // SRC_GRUNTZ_COMMANDS_H
