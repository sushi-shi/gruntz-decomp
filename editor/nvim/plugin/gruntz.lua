-- gruntz.nvim - commands + buffer-local keymaps (see lua/gruntz/init.lua).
if vim.g.loaded_gruntz then return end
vim.g.loaded_gruntz = true

local gruntz = require("gruntz")

vim.api.nvim_create_user_command("Gruntz", function(o)
  gruntz.dispatch(o.fargs[1])
end, {
  nargs = "?",
  complete = function() return gruntz.complete() end,
  desc = "gruntz: {target|base|diff|status} asm/diff for the function at cursor",
})

vim.api.nvim_create_user_command("GruntzBuild", function(o)
  gruntz.build(o.fargs)
end, { nargs = "*", desc = "gruntz: recompile (MSVC+wine) and report what moved" })

vim.api.nvim_create_user_command("GruntzLog", function()
  gruntz.show_log()
end, { desc = "gruntz: log of objdiff/build invocations" })

-- Attach the chords + the missing-tool warning + inline % hints on C/C++ buffers
-- only; outside a gruntz checkout the plugin stays inert (attach_keymaps is
-- harmless, check is silent, hints early-return with no project root).
local grp = vim.api.nvim_create_augroup("gruntz", { clear = true })

vim.api.nvim_create_autocmd("FileType", {
  pattern = { "c", "cpp" },
  group = grp,
  callback = function(ev)
    gruntz.load_state(ev.buf) -- restore this checkout's remembered toggles first
    if gruntz.config.keymaps then gruntz.attach_keymaps(ev.buf) end
    gruntz.check(ev.buf)
    gruntz.hints(ev.buf)
  end,
})

-- Refresh the inline % hints on enter/save (a build elsewhere may have moved the
-- numbers; report.json is mtime-cached so this is cheap).
vim.api.nvim_create_autocmd({ "BufWinEnter", "BufEnter", "BufWritePost" }, {
  pattern = { "*.c", "*.cpp", "*.cc", "*.h", "*.hpp" },
  group = grp,
  callback = function(ev) gruntz.hints(ev.buf) end,
})

-- Build-on-save (off by default; `:Gruntz autobuild` toggles): a quiet
-- incremental rebuild on every TU save so the inline %s update as you edit.
vim.api.nvim_create_autocmd("BufWritePost", {
  pattern = { "*.c", "*.cpp", "*.cc" },
  group = grp,
  callback = function(ev) gruntz.on_save(ev.buf) end,
})

-- Initial render for buffers already open when the plugin loads. When nvim is
-- launched on a file (e.g. via the dev-shell wrapper's `--cmd "set rtp^=…"`),
-- that buffer's FileType/BufEnter can fire before this autocmd is registered, so
-- the first render would be missed without this sweep. (hints early-returns for
-- non-project / non-source buffers.)
vim.schedule(function()
  for _, b in ipairs(vim.api.nvim_list_bufs()) do
    if vim.api.nvim_buf_is_loaded(b) then gruntz.hints(b) end
  end
end)
