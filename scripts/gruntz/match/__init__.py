"""gruntz.match - matching-progress tooling (CLI + library helpers).

  status       make matching progress + REGRESSIONS queriable (the match CLI).
  fingerprints per-function source fingerprints (the helper status imports).
  verify_stubs verify source-backed @stub metadata (the build gate).

Run the CLI as `python -m gruntz.match.status <cmd>`; import the helpers with
`from gruntz.match.fingerprints import cpp_hash, load_cache`.
"""
