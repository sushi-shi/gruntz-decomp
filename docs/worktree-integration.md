# Worktree Integration Checkpoints

This records the last completed commit inspected from long-running worktrees so
later integrations can resume without rescanning their committed history.
Dirty worktree state is never covered by a checkpoint.

| Worktree | Last inspected commit | Notes |
| :-- | :-- | :-- |
| `matcher-1` | `0768b092f7e0bec003181f14f84018bd15f26306` | Its committed history is already in `main`. Imported from the dirty tree: MoviePlayer destructor ownership, REZ archive version validation, GameStats reset order, runnable-link resources, and the input/shade startup block. The remaining dirty state is still active. |
