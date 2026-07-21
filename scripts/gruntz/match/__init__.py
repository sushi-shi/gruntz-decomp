"""gruntz.match - matching-progress tooling (CLI + library helpers).

  status         make matching progress + REGRESSIONS queriable (the match CLI).
  fingerprints   per-function source fingerprints (the helper status imports).
  high_water     the MAX-% high-water ratchet (config/match-max.tsv).
  residual_queue the exhaustive live non-exact function queue.
  verify_stubs / verify_unique_names / verify_library_overlap
                 measurement-integrity build gates.
  gate_selftest  negative controls for ALL gates (here + gruntz/cleanliness/).

The drive-to-0 quality board + its gates live in gruntz/cleanliness/; the
source-permutation climbers live in gruntz/permute/.
Run the CLI as `python -m gruntz.match.status <cmd>`; import the helpers with
`from gruntz.match.fingerprints import cpp_hash, load_cache`.
"""
