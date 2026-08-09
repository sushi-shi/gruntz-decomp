"""Sieve: a `.data`/`.rdata` word relocates to a DIFFERENT referent than retail's.

The function side of this question has `assert_relocs` (order-independent, over
3500 near-exact bodies). The DATA side had nothing, and data is where the blind
spot is worst: a relocated word's own bytes are a placeholder the linker
overwrites, so both sides hold the same placeholder and a byte comparison cannot
see a wrong referent at all. A vtable slot bound to the wrong method, an RTTI
base-class array pointing at the wrong `??_R1`, a pointer table we ordered
differently -- none of that moves a byte.

It matters now rather than later: size-weighted, `.rdata` is 99.3%, `.data` 95.9%
and `.bss` 99.8%, so a wrong REFERENT is most of what can still be hiding behind
those numbers.

TWO ORACLES, IN DESCENDING ORDER OF AUTHORITY
---------------------------------------------
`retail`  For a datum whose retail RVA we have pinned, the RETAIL IMAGE ITSELF is
          the answer. Its `.reloc` table lists every HIGHLOW fixup, so the set of
          words retail relocates inside the datum's extent is a FACT, and each
          one's stored value is the address retail points at. Nothing about
          either side's symbol NAMES enters into it, which is what makes this
          immune to every naming artefact below. It also does not need the
          delinked object, so it covers data the delinker never carved.

`paired`  For a datum both objects define, each side's referent is resolved to an
          RVA (`symbol_names.csv`, Ghidra's address-carrying auto-labels, the
          delinker's `const_<rva>`) and the ADDRESSES are compared. Where a name
          resolves on neither side, identical canonical names are accepted and a
          disagreement is reported as UNRESOLVED rather than claimed as a defect.

Resolving to addresses rather than comparing names is the whole design, and it is
what the first draft of this tool got wrong. Its name-comparison filters had to
drop "a name only one side has ever heard of" to survive the pooled-literal
naming split -- and an injected wrong vtable slot is exactly that shape, so the
negative control walked straight through the sieve. Addresses have no such hole:
two spellings of one address agree, and one spelling of two addresses does not.

Verdicts, in the vocabulary of `assert_relocs`:

  WRONG    the word points somewhere else than retail's does.
  EXTRA    we relocate a word retail leaves alone.
  MISSING  retail relocates a word we leave alone.
  ADDEND   same referent, different addend (paired oracle only; under the retail
           oracle a bad addend is simply a WRONG address).

NEGATIVE CONTROL
----------------
Three defects injected into `projectile`'s vtable COMDAT -- a slot redirected to
another method, a slot's addend moved 0 -> 4, and a slot's relocation record
deleted -- must come back as WRONG, WRONG and MISSING. `--selftest` runs it.

ARTEFACT FAMILIES, AND WHY THEY DO NOT PRODUCE ROWS
---------------------------------------------------
Each of these cost an earlier lane real time; the address oracle absorbs all of
them structurally rather than by a name heuristic.

  * a pooled literal spelled `??_C@_0BE@MAOF@GAME_ACTIONAREA_RED` on our side and
    `DAT_002126ec` on retail's -- one address, two spellings, agrees;
  * two FID labels on one address (`CGdiObject::Attach` printing as
    `CImageList::Attach`) -- same;
  * the delinker's unsized-datum fallback, which resolves an address it never
    enrolled onto the closest preceding NAMED definition plus an addend
    (`?s_table@...` as `?g_gruntDirNorthEast + 0x2b0`, `_inflate_mask + 0x3db4`
    at the data-attribution frontier) -- the addend is part of the address, so it
    resolves to the right place and agrees;
  * `$S<n>` / `$Sdata_data_<hash>` local-static suffixes and `??_E`/`??_G`, both
    already canonicalized by `canonicalize_data_symbols` in the normalized copies
    this reads (and re-applied here, belt and braces).

WHAT IS NOT COVERED, STATED RATHER THAN HIDDEN
----------------------------------------------
`--coverage`. Of the 10012 relocated words in `.rdata`/`.data`, 8803 are compared
(87.9%): 1078 have a referent that resolves to no RVA (mostly NAFXCW bodies with
no FID label, and `??_R4` COL records) and 131 sit in a datum that is neither
pinned nor paired. A further 4212 words live in `.xdata$x` (the /GX EH state
tables, whose DIR32s name `$L` funclet labels) and `.CRT$XC*` (static-initializer
pointer arrays) -- compiler-generated metadata neither side pins and the delinker
never carves, outside a referent audit's scope and reported separately rather than
folded into the denominator.

The `.xdata$x` half of that is now ENROLLED (`gruntz.build.eh_band` names each
`FuncInfo` and its unwind map after the function that owns them), so those words do
pair. They stay out of scope all the same, and are reported on their own line: what
an EXTRA/MISSING word in an unwind map states is that OUR function constructs a
different number of destructible objects than retail's, which is a code-shape
defect `gruntz.audit.eh_band` owns and reports per function - not a claim about a
referent we chose, which is the only thing this sieve is entitled to gate on.

A datum only ONE side defines is an attribution gap, not a reloc defect, and
`--unpaired` is its own report. `deflate`'s `_configuration_table` -- 10 DIR32s
into `_deflate_*` -- exists only in our object because retail's copy was never
enrolled; objdiff scores that unit's `.data` 100.0% on the six bytes it does
share and says nothing about the other 176.

    python -m gruntz.audit.data_relocs              # the defect list
    python -m gruntz.audit.data_relocs --coverage   # what each oracle reached
    python -m gruntz.audit.data_relocs --calibrate  # the false-positive set
    python -m gruntz.audit.data_relocs --unpaired   # data only one side defines
    python -m gruntz.audit.data_relocs --unresolved # words neither side resolves
    python -m gruntz.audit.data_relocs --sections   # the size-weighted data figure
    python -m gruntz.audit.data_relocs --selftest   # the injected-defect control
    python -m gruntz.audit.data_relocs --gate       # FATAL on any defect row
"""

from __future__ import annotations

import argparse
import collections
import csv
import json
import shutil
import struct
import sys
import tempfile
from pathlib import Path

# One COFF reader and one name canonicaliser for both reloc sieves; `global_refs`
# owns them, so the `$S<hash>` and `??_E`/`??_G` rules cannot drift apart between
# the code side and the data side. `assert_relocs` owns name -> RVA resolution and
# the ILT thunk chase, for the same reason.
from gruntz.audit.assert_relocs import load_symbols, resolve, resolve_thunk
from gruntz.audit.global_refs import DIR32, REPO, _coff, canon
from gruntz.build import eh_band
from gruntz.core.manifest import live_objs
from gruntz.core.pe import IMAGEBASE, PE
from gruntz.core.report import data_measures

NORM = REPO / "build" / "objdiff" / "normalized"
REPORT = REPO / "build" / "objdiff" / "report.json"

MEM_EXECUTE = 0x20000000
MEM_DISCARDABLE = 0x02000000
LNK_INFO = 0x00000200
LNK_REMOVE = 0x00000800
CNT_INITIALIZED = 0x00000040
CNT_UNINITIALIZED = 0x00000080


def _is_data(sec) -> bool:
    """A section that carries bytes and is neither code, debug, nor a directive."""
    ch = sec["chars"]
    if ch & (MEM_EXECUTE | MEM_DISCARDABLE | LNK_INFO | LNK_REMOVE):
        return False
    if not ch & (CNT_INITIALIZED | CNT_UNINITIALIZED):
        return False
    return not sec["name"].startswith((".debug", ".drectve"))


LIBRARY_LABELS = REPO / "config" / "retail" / "library_labels.csv"
# A library label whose own provenance is a vtable slot cannot adjudicate a vtable
# slot: 682 of the 865 words this source reaches are MFC methods labelled by
# `vtable-slot-oracle`, i.e. by reading the very tables under audit. Excluding
# them keeps the oracle independent (our guesses do not get to cite themselves).
CIRCULAR_LABEL_SOURCE = "vtable-slot-oracle"


def pinned_sizes() -> dict[str, int]:
    """{name: reviewed retail size} from symbol_names.csv, where one is claimed.

    Bounds the retail window. cl pads a COMDAT to its alignment, so our extent for
    the LAST symbol in a section runs to the padded end; retail packs, so the same
    span there can reach into the next datum and every relocation in it would read
    as MISSING. Where the pin states a size, that size is the extent.
    """
    out: dict[str, int] = {}
    path = REPO / "build" / "gen" / "symbol_names.csv"
    if not path.is_file():
        return out
    with path.open(encoding="latin-1", newline="") as stream:
        for row in csv.DictReader(stream):
            try:
                size = int(row["size"], 16)
            except (KeyError, TypeError, ValueError):
                continue
            if size > 0:
                out[canon(row["name"])] = max(out.get(canon(row["name"]), 0), size)
    return out


def library_rvas() -> dict[str, int]:
    """{mangled name: rva} for statically-linked CRT/MFC bodies, unambiguous only.

    `symbol_names.csv` covers what WE pinned, so a vtable slot pointing into
    NAFXCW (`?Serialize@CObject@@UAEXAAVCArchive@@@Z`, `?WindowProc@CWnd@@MAEJIIJ@Z`)
    resolves to nothing and the word goes uncompared. The FID census already knows
    those addresses. A name carrying more than one distinct RVA is ambiguous -- the
    two-FID-labels-on-one-address family in reverse -- and is dropped rather than
    guessed.
    """
    seen: dict[str, set[int]] = {}
    if not LIBRARY_LABELS.is_file():
        return {}
    with LIBRARY_LABELS.open(encoding="latin-1", newline="") as stream:
        for row in csv.DictReader(stream):
            if (row.get("confidence") or "").strip().upper() == "LOW":
                continue
            if (row.get("source") or "").strip() == CIRCULAR_LABEL_SOURCE:
                continue
            try:
                seen.setdefault(row["name"], set()).add(int(row["rva"], 16))
            except (KeyError, ValueError):
                continue
    return {n: next(iter(v)) for n, v in seen.items() if len(v) == 1}


# ------------------------------------------------------------------ orphan payloads

DATA_MANIFEST = REPO / "build" / "gen" / "delink_data_manifest.tsv"

# Orphan payload attributions that exist TODAY - NONE, since 2026-08-09. Anything
# not in this set is a new defect and fails the gate; a row leaves the set when its
# owner is proven, never because it became inconvenient. Both entries were closed
# by proving the owner, so the set is empty and must stay that way.
#
#   CLOSED 2026-08-09 - ghidra. config/static_data_copies.tsv's holding unit for
#   three GruntDirStatics blocks (27 `.bss` rows). Each copy's owner is decided by
#   its static initializer: cl emits the `$E` that constructs the nine cells at the
#   HEAD of that TU's .text contribution, so the first named function after the
#   code that stores into them names the unit. The rule reproduces 63 of the 71
#   catalogued blocks unaided, and it homes these three: 0x229548 advancedoptions,
#   0x249620 netlobbydialogs, 0x24bd20 multistartdlg. The second forced a
#   correction - 0x2496e8 was on netlobbydialogs and is multihelpdlg's - since one
#   TU cannot emit the copy twice.
#
#   CLOSED 2026-08-09 - movieplayer. vtables_game.csv attributed
#   ??_7?$CArray@PAUPLAYLISTINFOSTRUCT@@PAU1@@@6B@ (0x1e971c, 0x14, SIX relocated
#   words) to a unit dissolved on 2026-08-06. The `unit` column existed only because
#   data_manifest.vtable_rows() could not spell a template specialization's mangled
#   name from its RTTI key (`.?AV?$CArray@PAU...@@` decodes as the nested scope
#   `PAU1::PAUPLAYLISTINFOSTRUCT::?$CArray`) and withheld it. vtable_rows bridges
#   those names through the catalog now, so the vtable enrolls the ordinary way -
#   once per EMITTING object, all three of arrayserialize/creditsstate/gruntzmgr -
#   the hand-written owner is gone, and vtable_catalog.validate() FAILS the build on
#   a `unit` config/units.toml does not declare.
#
KNOWN_ORPHAN_UNITS = frozenset()

# A live unit objdiff does not really compare. It scores an empty pairing 100.00%
# on EVERY measure with ZERO totals, so the unit reports MATCHING in the per-unit
# table while being entirely unscored - worse than a missing row, because it
# inflates the board instead of showing a hole.
#
#   CLOSED 2026-08-09, in both of its two forms:
#
#   no delinked object    `logicdispatchinit`. Its whole content is one datum, the
#                         `.bss` template static CActRegPool<CEyeCandyAni>::s_table
#                         at 0x246060, and labels.py could not size it: the record
#                         layouts feeding structs.json dropped every template
#                         specialization whose name has a space in it, so all 51
#                         CActRegPool<T>::s_table statics were sizeless, none
#                         enrolled in the data manifest, and this unit - the only
#                         one with nothing else - got no contribution to carve.
#
#   paired with the dummy `stringstaticpool` (one `CString`) AND logicdispatchinit.
#                         Both had a real delinked obj on disk; objdiff.json still
#                         pointed at the empty dummy.obj because configure.py
#                         PREDICTED the pairing from FUNCTION rows only. It reads
#                         the delink output now. This form is invisible to a
#                         file-existence check, which is why the check below also
#                         reads objdiff.json.
KNOWN_UNPAIRED_UNITS = frozenset()

OBJDIFF_JSON = REPO / "build" / "objdiff" / "objdiff.json"


def units_without_a_target(norm=None, project=None) -> list[str]:
    """Live units objdiff cannot score: no delinked object, or paired to the dummy.

    BOTH forms have to be checked. A file-existence test alone missed
    `stringstaticpool`, whose target obj existed while objdiff.json still pointed
    its `target_path` at dummy.obj - the pairing, not the file, is what decides
    whether anything is compared."""
    from gruntz.core.manifest import unit_names

    norm = Path(norm or NORM)
    dummy = set()
    project = Path(project or OBJDIFF_JSON)
    if project.is_file():
        try:
            for unit in json.loads(project.read_text()).get("units", []):
                if Path(unit.get("target_path") or "").name == "dummy.obj":
                    dummy.add(unit.get("name"))
        except (OSError, json.JSONDecodeError, TypeError):
            pass
    return sorted(u for u in unit_names()
                  if (norm / "base" / f"{u}.obj").is_file()
                  and (u in dummy or not (norm / "target" / f"{u}.c.obj").is_file()))


def orphan_payloads() -> list[tuple[str, str, str, str]]:
    """Enrolled data whose object objdiff never opens: [(unit, rva, storage, name)].

    A delink manifest row names the object the payload is carved into. If that
    object is not a live `units.toml` unit, the delinker still writes it -- into a
    file `objdiff.json` does not list -- so the payload is withheld from every
    measurement with nothing to report it. `vtable_catalog.validate` checks names
    and RVAs, not the unit column, so this is the ordinary-data twin of the
    vtables_game.csv row that started this: silently unscored, no gate.
    """
    from gruntz.core.manifest import unit_names
    from gruntz.core.vtable_catalog import LIBRARY_HOLDING_UNIT

    if not DATA_MANIFEST.is_file():
        return []
    live = unit_names()
    out = []
    with DATA_MANIFEST.open(encoding="latin-1", newline="") as stream:
        for row in csv.DictReader(stream, delimiter="\t"):
            unit = row["object"].split("/")[-1]
            for suffix in (".c.obj", ".obj", ".c"):
                if unit.endswith(suffix):
                    unit = unit[:-len(suffix)]
                    break
            if unit == LIBRARY_HOLDING_UNIT:
                # The DELIBERATE holding object for library data no cl output
                # can re-emit (CDialog's messageMap, type_info's vtable): the
                # carve is what binds every reference by name, and keeping it
                # out of the compared set is the point, not a defect - a game
                # unit hosting it showed the payload as a permanently
                # unpairable target symbol. See vtable_catalog.
                continue
            if unit not in live:
                out.append((unit, row["rva"], row["storage"], row["name"]))
    return out


# --------------------------------------------------------------------------- model

class Datum:
    """One defined data symbol and the relocations inside its own window."""

    __slots__ = ("name", "section", "start", "lo", "hi", "rel")

    def __init__(self, name, section, start, lo, hi, rel):
        self.name, self.section = name, section
        self.start, self.lo, self.hi = start, lo, hi
        self.rel = rel                       # {rel_offset: (canon_name, addend)}


def _data(secs, dropped) -> dict[str, Datum]:
    """{canon symbol: Datum} for every defined data symbol in one object.

    The window is [previous owner's offset, next owner's offset), except that the
    FIRST owner also owns everything before it. That is not a detail: a /GR vtable
    COMDAT begins with the `??_R4` complete-object-locator pointer four bytes
    AHEAD of the `??_7` symbol, and that word is one of the ones worth checking.
    Relative offsets can therefore be negative, and they still line up across the
    two sides because both put the COL in the same place relative to the symbol.

    Windowing per symbol rather than per section is what makes this survive the
    shape difference between the sides: cl emits one COMDAT per literal and per
    vtable, the delinker PACKS a unit's data into one section, so section offsets
    never line up (`?g_projPhase0@@3NB` is `.rdata`+0 for us and `.rdata$r`+0x28
    for the delinker) but symbol-relative ones always do.
    """
    out: dict[str, Datum] = {}
    for sec in secs:
        if not _is_data(sec):
            continue
        owners = sec["owners"]
        if not owners:
            if sec["rel"]:
                dropped["relocation in a data section with no defined symbol"] += \
                    len(sec["rel"])
            continue
        for i, (val, nm) in enumerate(owners):
            lo = 0 if i == 0 else val
            hi = owners[i + 1][0] if i + 1 < len(owners) else sec["size"]
            rel = {off - val: (canon(rnm), addend)
                   for off, rnm, ty, addend in sec["rel"]
                   if ty == DIR32 and lo <= off < hi}
            key = canon(nm)
            if key in out:
                dropped["symbol defined twice in one object"] += 1
                continue
            out[key] = Datum(key, sec["name"], val, lo - val, hi - val, rel)
    return out


class Row:
    __slots__ = ("unit", "datum", "off", "verdict", "oracle", "base", "target",
                 "clean")

    def __init__(self, unit, datum, off, verdict, oracle, base, target, clean):
        self.unit, self.datum, self.off = unit, datum, off
        self.verdict, self.oracle = verdict, oracle
        self.base, self.target, self.clean = base, target, clean

    def __str__(self) -> str:
        at = "" if self.off == 0 else f" {'-' if self.off < 0 else '+'} {abs(self.off):#x}"
        return (f"{self.verdict:<7} [{self.oracle}]  {self.unit}  {self.datum}{at}"
                f"\n            ours   {self.base or '-'}"
                f"\n            retail {self.target or '-'}")


def _ref(name, addend, rva) -> str:
    s = name + (f" + {addend:#x}" if addend else "")
    return f"{s}  ({rva + IMAGEBASE:#010x})" if rva is not None else f"{s}  (unresolved)"


# --------------------------------------------------------------------------- scan

def clean_units() -> set[str]:
    """Units whose every `.data`/`.rdata` section objdiff scores at exactly 100.0.

    The calibration set. Those sections agree byte-for-byte AND, as the injected
    control below demonstrates, objdiff does compare a data relocation's target
    NAME -- redirecting one vtable slot took `boomerang`'s `.rdata` from 100.00 to
    99.37. So a row inside one of these is a bug in this tool, not a finding.
    `.bss` is excluded: no raw bytes, so no relocations to be right or wrong about.
    """
    if not REPORT.is_file():
        return set()
    doc = json.loads(REPORT.read_text())
    out = set()
    for u in doc.get("units", []):
        rows = [s for s in u.get("sections", []) if s["name"] in (".data", ".rdata")]
        if rows and all(s.get("fuzzy_match_percent", 0.0) >= 100.0 for s in rows):
            out.add(u["name"])
    return out


def scan(unit_filter=None, norm=None):
    """([Row], [unpaired], [unresolved], stats, dropped, [eh Row]) per live unit."""
    norm = Path(norm or NORM)
    clean = clean_units()
    sym, data, _dups = load_symbols()
    pe = PE()
    rows, unpaired, unresolved, eh_state = [], [], [], []
    dropped: collections.Counter = collections.Counter()
    stats: collections.Counter = collections.Counter()

    libs, sizes = library_rvas(), pinned_sizes()

    def rva_of(name, addend=0):
        r = resolve(sym, data, "DIR32", name, addend)
        if r is None:
            lib = libs.get(name, libs.get(name.lstrip("_")))
            if lib is not None:
                r = resolve_thunk((lib + addend) & 0xFFFFFFFF)
        return r

    for unit, base in live_objs(norm / "base"):
        if unit_filter and unit != unit_filter:
            continue
        target = norm / "target" / f"{unit}.c.obj"
        bsecs = _coff(base)
        bdata = _data(bsecs, dropped)
        tdata = {}
        if target.is_file():
            tdata = _data(_coff(target), dropped)
        else:
            dropped["unit whose delinked side does not exist"] += 1
        stats["units"] += 1

        for nm in sorted(set(bdata) - set(tdata)):
            unpaired.append((unit, "base-only", nm, len(bdata[nm].rel)))
        for nm in sorted(set(tdata) - set(bdata)):
            unpaired.append((unit, "retail-only", nm, len(tdata[nm].rel)))

        for nm in sorted(bdata):
            b = bdata[nm]
            stats["data symbols"] += 1
            pin = rva_of(nm)

            # ---- oracle 1: the retail image's own base-relocation table -------
            if pin is not None:
                stats["data symbols pinned"] += 1
                # Retail was linked INCREMENTALLY, so a vtable slot holds the
                # address of the 5-byte ILT `jmp` thunk, not of the body. Both
                # sides go through resolve_thunk, or every /GR vtable in the tree
                # reads as 18 wrong slots (it did: 4725 rows, 3530 of them inside
                # 100.00%-exact sections).
                hi = min(b.hi, sizes[nm]) if nm in sizes else b.hi
                retail = {site - pin: resolve_thunk(va - IMAGEBASE)
                          for site, va in pe.relocs_in(pin + b.lo, pin + hi)}
                for off in sorted(set(b.rel) | set(retail)):
                    if off >= hi:
                        dropped["word past the datum's reviewed retail size"] += 1
                        continue
                    br, tv = b.rel.get(off), retail.get(off)
                    if br is None:
                        stats["words compared (retail)"] += 1
                        rows.append(Row(unit, nm, off, "MISSING", "retail", None,
                                        _ref("<retail .reloc>", 0, tv),
                                        unit in clean))
                        continue
                    ours = rva_of(*br)
                    if ours is None:
                        dropped["our referent does not resolve to an rva"] += 1
                        continue
                    stats["words compared (retail)"] += 1
                    if tv is None:
                        rows.append(Row(unit, nm, off, "EXTRA", "retail",
                                        _ref(br[0], br[1], ours), None,
                                        unit in clean))
                    elif tv != ours:
                        rows.append(Row(unit, nm, off, "WRONG", "retail",
                                        _ref(br[0], br[1], ours),
                                        _ref("<retail .reloc>", 0, tv),
                                        unit in clean))
                continue

            # ---- oracle 2: the delinked object, resolved to addresses ---------
            t = tdata.get(nm)
            if t is None:
                # `.xdata$x` (the /GX EH state tables, whose DIR32s name `$L`
                # funclet labels) and `.CRT$XC*` (the static-initializer pointer
                # arrays) are compiler-generated metadata that neither side pins
                # and the delinker never carves. They are 4212 of the 14224 words
                # in base data sections and are outside a referent audit's scope;
                # counting them as "uncompared" would understate the real coverage
                # of `.rdata`/`.data` by a third.
                if b.rel:
                    kind = b.section.split("$")[0]
                    dropped[f"{kind}: datum neither pinned nor paired"] += len(b.rel)
                continue
            stats["data symbols paired"] += 1
            for off in sorted(set(b.rel) | set(t.rel)):
                br, tr = b.rel.get(off), t.rel.get(off)
                ba = rva_of(*br) if br else None
                ta = rva_of(*tr) if tr else None
                if br and tr and br == tr:
                    stats["words compared (paired)"] += 1
                    continue
                if br and tr and (ba is None or ta is None):
                    # Two spellings we cannot turn into addresses. Claiming a
                    # defect here is what the name-comparison draft did wrong.
                    unresolved.append((unit, nm, off, br, tr))
                    dropped["neither side's referent resolves to an rva"] += 1
                    continue
                stats["words compared (paired)"] += 1
                if br and tr:
                    if ba != ta:
                        verdict = "WRONG"
                    elif br[1] != tr[1]:
                        verdict = "ADDEND"
                    else:
                        continue
                else:
                    verdict = "EXTRA" if br else "MISSING"
                row = Row(unit, nm, off, verdict, "paired",
                          _ref(*br, ba) if br else None,
                          _ref(*tr, ta) if tr else None,
                          unit in clean)
                # THE /GX EH STATE TABLES ARE OUT OF THIS AUDIT'S SCOPE, as its own
                # coverage note has always said - they are compiler-generated
                # metadata about the OWNER's scopes, not a referent we chose. Now
                # that `gruntz.build.eh_band` enrolls them they PAIR, so a divergence
                # would arrive here as an EXTRA/MISSING word; but what it states is
                # that our function builds a different number of destructible objects
                # than retail's, which is a code-shape defect `gruntz.audit.eh_band`
                # owns and reports per function. Route it there instead of gating the
                # tree on it here, where it would say nothing about a referent.
                (eh_state if eh_band.is_band_data_symbol(nm) else rows).append(row)
    return rows, unpaired, unresolved, stats, dropped, eh_state


# --------------------------------------------------------------------------- data %

def section_measures():
    """(`core.report.data_measures` rows, report.json's own `measures` block)."""
    doc = json.loads(REPORT.read_text())
    return data_measures(doc), doc["measures"]


# --------------------------------------------------------------------------- control

def selftest(unit="projectile") -> int:
    """Inject three known defects into one object and require all three back.

    A sieve nobody has watched FAIL reports a clean tree whether or not the tree
    is clean; the first draft of this one returned 0 rows over 9806 words and was
    simply blind. The three shapes are the three verdicts the retail oracle can
    emit: a redirected referent, a moved addend, and a deleted relocation record.
    """
    src = NORM / "base" / f"{unit}.obj"
    if not src.is_file():
        print(f"selftest: {src} is missing (run a build first)")
        return 1
    expect = {"redirect": "WRONG", "addend": "WRONG", "delete": "MISSING"}
    bad = []
    for mode, want in expect.items():
        with tempfile.TemporaryDirectory() as td:
            lab = Path(td)
            (lab / "base").mkdir()
            (lab / "target").mkdir()
            shutil.copy2(src, lab / "base" / f"{unit}.obj")
            tgt = NORM / "target" / f"{unit}.c.obj"
            if tgt.is_file():
                shutil.copy2(tgt, lab / "target" / tgt.name)
            _inject(lab / "base" / f"{unit}.obj", mode)
            rows, _up, _ur, _st, _dr, _eh = scan(unit, norm=lab)
            got = [r.verdict for r in rows]
            ok = want in got
            print(f"  {mode:<9} expect {want:<8} got {got or ['(nothing)']}"
                  f"   {'ok' if ok else 'FAIL'}")
            if not ok:
                bad.append(mode)
    print("selftest: " + ("pass" if not bad else f"FAIL ({', '.join(bad)})"))
    return 1 if bad else 0


def _inject(path: Path, mode: str) -> None:
    """Rewrite one vtable relocation in a disposable copy of a base object."""
    d = bytearray(path.read_bytes())
    symptr, nsym = struct.unpack_from("<II", d, 8)
    strtab = symptr + nsym * 18
    names, i = {}, 0
    while i < nsym:
        o = symptr + i * 18
        raw = d[o:o + 8]
        names[i] = (d[strtab + struct.unpack_from("<I", d, o + 4)[0]:
                      d.find(b"\0", strtab + struct.unpack_from("<I", d, o + 4)[0])]
                    .decode("latin1") if raw[:4] == b"\0\0\0\0"
                    else raw.rstrip(b"\0").decode("latin1"))
        i += 1 + d[o + 17]
    nsec = struct.unpack_from("<H", d, 2)[0]
    optsz = struct.unpack_from("<H", d, 16)[0]
    for s in range(nsec):
        o = 20 + optsz + s * 40
        chars = struct.unpack_from("<I", d, o + 36)[0]
        rawoff = struct.unpack_from("<I", d, o + 20)[0]
        relptr = struct.unpack_from("<I", d, o + 24)[0]
        nrel = struct.unpack_from("<H", d, o + 32)[0]
        if chars & MEM_EXECUTE or nrel < 4:
            continue
        recs = [(struct.unpack_from("<IIH", d, relptr + r * 10), relptr + r * 10)
                for r in range(nrel)]
        vt = [r for r in recs if names[r[0][1]].startswith("?")
              and "@@UAE" in names[r[0][1]]]
        if len(vt) < 2:
            continue
        site, other = vt[0], vt[1]
        if mode == "redirect":
            struct.pack_into("<I", d, site[1] + 4, other[0][1])
        elif mode == "addend":
            struct.pack_into("<I", d, rawoff + site[0][0], 4)
        elif mode == "delete":
            del d[site[1]:site[1] + 10]
            struct.pack_into("<H", d, o + 32, nrel - 1)
            for t in range(nsec):
                to = 20 + optsz + t * 40
                for off in (20, 24):
                    v = struct.unpack_from("<I", d, to + off)[0]
                    if v > site[1]:
                        struct.pack_into("<I", d, to + off, v - 10)
            if symptr > site[1]:
                struct.pack_into("<I", d, 8, symptr - 10)
        path.unlink()                      # never write through a hardlink
        path.write_bytes(bytes(d))
        return
    raise SystemExit(f"selftest: no injectable vtable relocation in {path}")


# --------------------------------------------------------------------------- cli

def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--unit", help="restrict to one objdiff unit")
    ap.add_argument("--datum", help="substring filter on the data symbol")
    ap.add_argument("--calibrate", action="store_true",
                    help="only rows inside 100.00%%-exact data sections "
                         "-- the false-positive set")
    ap.add_argument("--coverage", action="store_true",
                    help="what each oracle reached, and what neither did")
    ap.add_argument("--unpaired", action="store_true",
                    help="data symbols only ONE side defines (an attribution gap, "
                         "not a reloc defect)")
    ap.add_argument("--unresolved", action="store_true",
                    help="words where the two sides disagree on the SPELLING and "
                         "neither resolves to an rva (no verdict is claimed)")
    ap.add_argument("--sections", action="store_true",
                    help="the size-weighted data figure behind `matched_data`")
    ap.add_argument("--selftest", action="store_true",
                    help="inject three known defects and require all three back")
    ap.add_argument("--orphans", action="store_true",
                    help="enrolled data carved into an object objdiff never opens")
    ap.add_argument("--gate", action="store_true",
                    help="exit 1 on any defect row or any NEW orphan payload")
    ap.add_argument("--top", type=int, default=40)
    args = ap.parse_args(argv)

    if args.selftest:
        return selftest()

    # A gate nobody has watched FAIL reports a clean tree whether or not the tree
    # is clean, and this one HAS been blind (see selftest's docstring). So the gate
    # proves itself on every run before it is allowed to report a pass.
    if args.gate and selftest():
        print("FAIL: the data-reloc sieve did not catch its own injected defects; "
              "its verdict on the tree means nothing.")
        return 1

    if args.orphans:
        orphans = orphan_payloads()
        print(f"enrolled data whose object objdiff never opens: {len(orphans)} row(s)")
        for unit, rva, storage, name in orphans:
            print(f"  {unit:<14} {rva:>9} {storage:<6} {name}")
        empty = units_without_a_target()
        print(f"\nlive units objdiff does not compare - no delinked object, or "
              f"paired to dummy.obj (it scores the empty pairing 100.00%): "
              f"{len(empty)}")
        for u in empty:
            print(f"  {u}")
        return 0

    if args.sections:
        rows, measures = section_measures()
        print("size-weighted data match, from the SAME report.json section rows\n")
        for k in [n for n in sorted(rows) if n != "total"] + ["total"]:
            r = rows[k]
            print(f"  {k:<7} {r['sections']:>4} sections  {r['bytes']:>7} B  "
                  f"size-weighted {100.0 * r['weighted'] / r['bytes']:6.2f}%  "
                  f"all-or-nothing {100.0 * r['exact'] / r['bytes']:6.2f}%")
        print(f"\n  report.json matched_data: {measures['matched_data']}"
              f"/{measures['total_data']} = {measures['matched_data_percent']:.2f}%"
              "  (all-or-nothing per section)")
        return 0

    rows, unpaired, unresolved, stats, dropped, eh_state = scan(args.unit)
    fp = [r for r in rows if r.clean]
    orphans = orphan_payloads()
    new_orphans = [o for o in orphans if o[0] not in KNOWN_ORPHAN_UNITS]
    empty = units_without_a_target()
    new_empty = [u for u in empty if u not in KNOWN_UNPAIRED_UNITS]

    print(f"objs: {NORM.relative_to(REPO)}   reloc: DIR32 in data sections")
    print(f"units {stats['units']}   data symbols {stats['data symbols']} "
          f"({stats['data symbols pinned']} pinned, "
          f"{stats['data symbols paired']} paired-only)")
    print(f"words compared: {stats['words compared (retail)']} against the retail "
          f"image, {stats['words compared (paired)']} against the delinked object")
    if dropped:
        print("uncompared: " + ", ".join(f"{k}={v}" for k, v in sorted(dropped.items())))
    print(f"rows: {len(rows)}   of those inside a 100.00%%-exact data section "
          f"(FALSE POSITIVES): {len(fp)}")
    by = collections.Counter(r.verdict for r in rows)
    print("verdicts: " + (", ".join(f"{k}={v}" for k, v in sorted(by.items())) or "-"))
    if eh_state:
        owners = sorted({r.datum.split("$", 1)[1] for r in eh_state})
        print(f"/GX EH state tables (out of scope here - a different unwind-state "
              f"COUNT is a code-shape defect `python -m gruntz.audit.eh_band` owns): "
              f"{len(eh_state)} word(s) over {len(owners)} function(s)")
        for owner in owners:
            print(f"    {owner}")
    print(f"enrolled data carved into an object objdiff never opens: {len(orphans)} "
          f"({len(new_orphans)} new); live units objdiff does not compare at all: "
          f"{len(empty)} ({len(new_empty)} new) -- --orphans")

    if args.coverage:
        # `.xdata$x` (/GX EH state tables) and `.CRT$XC*` (static-init pointer
        # arrays) are compiler-generated metadata neither side pins and the
        # delinker never carves. Reporting them inside the denominator understates
        # the real `.rdata`/`.data` coverage by a third, so they are their own row.
        meta = sum(v for k, v in dropped.items()
                   if k.startswith((".xdata", ".CRT")))
        done = stats["words compared (retail)"] + stats["words compared (paired)"]
        rest = sum(v for k, v in dropped.items()
                   if not k.startswith((".xdata", ".CRT"))
                   and "delinked side" not in k)
        print(f"\ngame data words (.rdata/.data/.bss): {done + rest}   "
              f"compared {done} ({100.0 * done / max(done + rest, 1):.1f}%)")
        print(f"compiler-generated metadata out of scope (.xdata EH state tables, "
              f".CRT initializer arrays): {meta} words")
        print(f"unpaired data symbols: {len(unpaired)} "
              f"({sum(1 for r in unpaired if r[3])} carrying relocations, "
              f"{sum(r[3] for r in unpaired)} words)")
        print(f"spelling-only disagreements neither side resolves: {len(unresolved)}")
        return 0

    if args.unpaired:
        print(f"\nunpaired data symbols: {len(unpaired)}")
        seen = collections.Counter(side for _u, side, _n, _r in unpaired)
        print("  " + ", ".join(f"{k}={v}" for k, v in sorted(seen.items())))
        withrel = [r for r in unpaired if r[3]]
        print(f"  carrying relocations (unverifiable pointers): {len(withrel)}, "
              f"{sum(r[3] for r in withrel)} words")
        for unit, side, nm, nrel in sorted(withrel, key=lambda r: -r[3])[:args.top]:
            print(f"  {side:<11} {nrel:>3} rel  {unit}  {nm}")
        return 0

    if args.unresolved:
        print(f"\nspelling-only disagreements, no verdict claimed: {len(unresolved)}")
        for unit, nm, off, br, tr in unresolved[:args.top]:
            print(f"  {unit}  {nm} + {off:#x}\n      ours   {br}\n      retail {tr}")
        return 0

    shown = fp if args.calibrate else [r for r in rows if not r.clean]
    if args.datum:
        shown = [r for r in rows if args.datum in r.datum]
    print()
    for r in shown[:args.top]:
        print(r)
    if len(shown) > args.top:
        print(f"\n... {len(shown) - args.top} more")

    if args.gate:
        if rows:
            print(f"\nFAIL: {len(rows)} data word(s) do not relocate where retail's do.")
            return 1
        if new_orphans:
            print(f"\nFAIL: {len(new_orphans)} enrolled datum(s) are carved into an "
                  "object objdiff never opens, so their payload is withheld from "
                  "every measurement:")
            for unit, rva, storage, name in new_orphans[:args.top]:
                print(f"  {unit:<14} {rva:>9} {storage:<6} {name}")
            print("Attribute the row to a live config/units.toml unit, or add the "
                  "unit to KNOWN_ORPHAN_UNITS with the evidence.")
            return 1
        if new_empty:
            print(f"\nFAIL: {len(new_empty)} live unit(s) are not compared at all - "
                  "no delinked object, or objdiff.json still pairs them with "
                  "dummy.obj. Either way objdiff scores the empty pairing 100.00% "
                  "on every measure: " + ", ".join(new_empty))
            return 1
        print(f"data-relocs: {stats['words compared (retail)']}"
              f"+{stats['words compared (paired)']} relocated data words point where "
              f"retail's do; {len(orphans)} known orphan payload(s)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
