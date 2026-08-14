"""gruntz.match - matching-progress tooling (CLI + library helpers).

  status         make matching progress + REGRESSIONS queriable (the match CLI).
  fingerprints   per-function source fingerprints (the helper status imports).
  residual_queue the exhaustive and weighted-middle matching queues.
  verify_stubs / verify_unique_names / verify_library_overlap
                 measurement-integrity build gates.
  gate_selftest  negative controls for ALL gates (here + gruntz/cleanliness/).

The drive-to-0 quality board + its gates live in gruntz/cleanliness/; the

Run the CLI as `python -m gruntz.match.status <cmd>`; import the helpers with
`from gruntz.match.fingerprints import cpp_hash, load_cache`.
"""
