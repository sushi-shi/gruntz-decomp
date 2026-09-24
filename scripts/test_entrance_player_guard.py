"""Production-object/original-PE control for the entrance current-player guard.

Run after a full build, in pinned nix develop. This never compiles or runs the
game. It checks one bounded MOV/CMP/Jcc protocol, not general CFG equivalence;
unsupported or missing shapes fail closed instead of silently passing.
"""

from collections import Counter
from dataclasses import dataclass, replace
import re
import struct
import unittest


OWNERS = {
    '?LoadEntranceConfig@CGrunt@@QAEHXZ': (0x67f80, 0x313),
    '?FinishActiveAction@CGrunt@@QAEHXZ': (0x6a6d0, 0x936),
}
PLAYER = '?g_curPlayer@@3HA'
VOICE = '?PlayVoice@CVoiceManager@@QAEHPAVCGrunt@@HHHHH@Z'
RESET = '?ResetCell@CTriggerMgr@@QAEHHHHH@Z'
SAFETY = '?GetDword@CButeMgr@@QAEKPBD0K@Z'
LOOKUP = '?Lookup@CMapStringToPtr@@QBEHPBDAAPAX@Z'
REGISTERS = {'eax', 'ebx', 'ecx', 'edx', 'esi', 'edi', 'ebp', 'esp'}


def require(condition, message):
    if not condition:
        raise AssertionError(message)


@dataclass(frozen=True)
class Instruction:
    offset: int
    size: int
    mnemonic: str
    operands: str

    @property
    def after(self):
        return self.offset + self.size


def decode(data):
    from gruntz.tool import objdump

    result = {}
    for line in objdump.disassemble(data, vma=0, intel=True).splitlines():
        match = re.match(r'^\s*([0-9a-f]+):\t([0-9a-f ]+)(?:\t(.*))?$', line)
        if not match:
            continue
        offset = int(match[1], 16)
        size = len(bytes.fromhex(match[2]))
        assembly = (match[3] or '').strip().split(None, 1)
        if not assembly:
            # Binutils wraps bytes of instructions longer than seven bytes.
            require(result, f'orphan instruction continuation at +0x{offset:x}')
            previous = result[next(reversed(result))]
            require(previous.after == offset, 'noncontiguous instruction continuation')
            result[previous.offset] = replace(previous, size=previous.size + size)
            continue
        require(assembly and not assembly[0].startswith(('.', '(')),
                f'unsupported decode at +0x{offset:x}: {match[3]}')
        result[offset] = Instruction(offset, size, assembly[0],
                                     assembly[1] if len(assembly) == 2 else '')
    require(result and 0 in result, 'empty or incomplete function decode')
    end = 0
    for instruction in result.values():
        require(instruction.offset == end, f'undecoded function bytes at +0x{end:x}')
        end = instruction.after
    require(end == len(data), 'undecoded trailing function bytes')
    return result


@dataclass
class Body:
    label: str
    data: bytes
    calls: dict
    # (instruction-relative fixup offset, kind, semantic target)
    references: tuple

    def __post_init__(self):
        self.instructions = decode(self.data)

    def refs(self, instruction):
        return [name for off, _, name in self.references
                if instruction.offset <= off < instruction.after]

    def target(self, instruction):
        require(re.fullmatch(r'0x[0-9a-f]+', instruction.operands) is not None,
                f'{self.label}: unsupported branch at +0x{instruction.offset:x}')
        target = int(instruction.operands, 16)
        require(target in self.instructions,
                f'{self.label}: branch target +0x{target:x} is not an instruction')
        return target

    def successors(self, offset):
        instruction = self.instructions[offset]
        mnemonic = instruction.mnemonic
        if mnemonic.startswith('ret') or mnemonic == 'int3':
            return ()
        if mnemonic == 'jmp':
            return (self.target(instruction),)
        require(not mnemonic.startswith('loop'), f'{self.label}: unsupported LOOP')
        if mnemonic.startswith('j'):
            return (self.target(instruction), instruction.after)
        return (instruction.after,) if instruction.after in self.instructions else ()

    def reachable(self, start, blocked=None):
        seen, pending = set(), [start]
        while pending:
            offset = pending.pop()
            if offset in seen or offset == blocked:
                continue
            require(offset in self.instructions,
                    f'{self.label}: incomplete decode at +0x{offset:x}')
            seen.add(offset)
            pending.extend(self.successors(offset))
        return seen

    def unique_call(self, name):
        sites = [offset for offset, target in self.calls.items() if target == name]
        require(len(sites) == 1, f'{self.label}: expected one {name}, got {sites}')
        return sites[0]

    def ancestors(self, end):
        predecessors = {off: [] for off in self.instructions}
        for off in self.instructions:
            for successor in self.successors(off):
                require(successor in predecessors, f'{self.label}: incomplete CFG')
                predecessors[successor].append(off)
        seen, pending = set(), [end]
        while pending:
            off = pending.pop()
            if off not in seen:
                seen.add(off)
                pending.extend(predecessors[off])
        return seen


def current_player_guard(body):
    """Recognize only the local field/global MOV, optional register copies, CMP/Jcc."""
    instructions = body.instructions
    global_loads = [i for i in instructions.values() if PLAYER in body.refs(i)]
    require(len(global_loads) == 1,
            f'{body.label}: expected one current-player reference, got {len(global_loads)}')
    load = global_loads[0]
    lookup, safety = body.unique_call(LOOKUP), body.unique_call(SAFETY)
    require(lookup < load.offset < safety,
            f'{body.label}: current-player reference outside entrance Lookup/safety region')

    # Identify the incoming receiver saved before the first call. Prove the
    # save dominates this guard and no path to it overwrites the saved register.
    first_call = min(body.calls)
    saves = [i for i in instructions.values() if i.offset < first_call
             and i.mnemonic == 'mov'
             and re.fullmatch(r'(ebx|esi|edi|ebp),ecx', i.operands)]
    require(len(saves) == 1, f'{body.label}: unsupported receiver prologue')
    save = saves[0]
    receiver = save.operands.split(',')[0]
    for i in instructions.values():
        if i.offset >= save.offset:
            break
        operands = i.operands.split(',')
        require(not (operands[0] == 'ecx' and i.mnemonic not in ('cmp', 'test', 'push'))
                and not (i.mnemonic == 'xchg' and 'ecx' in operands),
                f'{body.label}: incoming receiver modified before save')
    require(load.offset not in body.reachable(0, save.offset),
            f'{body.label}: receiver save does not dominate guard')
    before_guard = body.ancestors(load.offset)
    for off in before_guard & body.reachable(save.after):
        i = instructions[off]
        destination = i.operands.split(',')[0]
        if i.mnemonic == 'xchg' and receiver in i.operands.split(','):
            raise AssertionError(f'{body.label}: unsupported receiver XCHG')
        if destination == receiver and i.mnemonic not in ('cmp', 'test', 'push'):
            raise AssertionError(f'{body.label}: receiver overwritten at +0x{off:x}')

    # Start at the basic-block entry containing the global load. The bounded
    # local def-use census accepts register copies, not arbitrary expressions,
    # partial-register normalization, stack spills, or cross-block values.
    leaders = {0}
    for i in instructions.values():
        if i.mnemonic.startswith('j'):
            leaders.add(body.target(i))
            leaders.add(i.after)
    start = max(off for off in leaders if off <= load.offset)
    tags, comparison = {}, None
    offset = start
    for _ in range(32):
        require(offset in instructions, f'{body.label}: incomplete guard decode')
        i = instructions[offset]
        operands = i.operands.split(',')
        if i.mnemonic in ('je', 'jne') and comparison is not None:
            require(comparison == {'player', 'current'},
                    f'{body.label}: comparison does not identify both player operands')
            taken, fallthrough = body.target(i), i.after
            equal, unequal = (taken, fallthrough) if i.mnemonic == 'je' else (fallthrough, taken)
            require(i.offset not in body.reachable(0, lookup),
                    f'{body.label}: animation lookup does not dominate guard')
            return i, equal, unequal
        if i.mnemonic == 'cmp' and len(operands) == 2:
            comparison = {tags.get(operand) for operand in operands}
        elif i.mnemonic == 'mov' and len(operands) == 2 and operands[0] in REGISTERS:
            destination, source = operands
            if PLAYER in body.refs(i):
                require(source.startswith('DWORD PTR'), f'{body.label}: player address, not load')
                tags[destination] = 'current'
            elif source == f'DWORD PTR [{receiver}+0x1ec]':
                tags[destination] = 'player'
            else:
                tags[destination] = tags.get(source)
        elif i.mnemonic != 'nop':
            raise AssertionError(f'{body.label}: unsupported guard instruction +0x{offset:x}: '
                                 f'{i.mnemonic} {i.operands}')
        offset = i.after
    raise AssertionError(f'{body.label}: current-player comparison not found within local block')


def guarded_traces(body, start):
    """All local paths to the unique safety call; no symbolic execution needed."""
    safety = body.unique_call(SAFETY)
    pending = [(start, (), frozenset())]
    traces = set()
    while pending:
        offset, events, seen = pending.pop()
        require(offset not in seen, f'{body.label}: cycle before safety call')
        require(len(seen) < 96, f'{body.label}: unsupported oversized player branch')
        require(offset in body.instructions, f'{body.label}: missing local instruction')
        i = body.instructions[offset]
        if i.mnemonic == 'call':
            target = body.calls.get(offset)
            require(target in (VOICE, RESET, SAFETY),
                    f'{body.label}: unexpected call at +0x{offset:x}: {target}')
            events += (target,)
        if offset == safety:
            traces.add(events)
            continue
        successors = body.successors(offset)
        require(successors, f'{body.label}: exit before safety call at +0x{offset:x}')
        for successor in successors:
            pending.append((successor, events, seen | {offset}))
    return traces


def check_guard(body):
    branch, equal, unequal = current_player_guard(body)
    actual = (guarded_traces(body, equal), guarded_traces(body, unequal))
    expected = ({(VOICE, RESET, SAFETY)}, {(SAFETY,)})
    require(actual == expected,
            f'{body.label}: current-player guard +0x{branch.offset:x}: '
            f'equal/unequal call traces {actual!r}, expected {expected!r}')
    return actual


def production_body(path, symbol):
    from gruntz.delink.coffx import Obj
    from gruntz.walls.pairscan import functions, fn_relocs

    require(path.is_file(), f'missing production object {path}; run full gruntz build')
    obj = Obj(path)
    require(symbol in functions(obj), f'{path}: missing production owner {symbol}')
    section, start, end = functions(obj)[symbol]
    data = obj.section_payload(section)[start:end].rstrip(b'\xcc').rstrip(b'\x90')
    references = tuple((off - start, kind, name)
                       for off, name, kind, _ in fn_relocs(obj, section, start, end))
    body = Body(f'production {symbol}', data, {}, references)
    for i in body.instructions.values():
        if i.mnemonic == 'call':
            targets = [name for off, kind, name in references
                       if i.offset <= off < i.after and kind == 0x14]
            require(len(targets) == 1, f'{body.label}: unresolved call at +0x{i.offset:x}')
            body.calls[i.offset] = targets[0]
    return body


def retail_body(image, names, symbol, rva, size):
    data = image.read(rva, size)
    require(data is not None, f'missing original retail owner {symbol}')
    references = [(off - rva, 6, names.get(target, f'rva:{target:x}'))
                  for off, target in image.relocs_in(rva, rva + size)]
    body = Body(f'original retail {symbol}', data, {}, ())
    for i in body.instructions.values():
        if i.mnemonic != 'call':
            continue
        require(data[i.offset] == 0xe8, f'{body.label}: unsupported indirect call')
        target = rva + i.after + struct.unpack_from('<i', data, i.offset + 1)[0]
        seen = set()
        while True:
            require(target not in seen, f'{body.label}: retail thunk cycle')
            seen.add(target)
            forwarded = image.jmp_target(target)
            if forwarded is None:
                break
            target = forwarded
        name = names.get(target, f'rva:{target:x}')
        body.calls[i.offset] = name
        references.append((i.offset + 1, 0x14, name))
    body.references = tuple(sorted(references))
    return body


class EntrancePlayerGuardTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        from gruntz.core.paths import BUILD
        from gruntz.model import resolve
        from gruntz.sema.image import Image

        model = resolve()
        names = {row.rva: row.name for row in model.functions + model.data if row.name}
        require(names.get(0x244c54) == PLAYER, 'retail current-player identity changed')
        image = Image()
        cls.pairs = []
        for symbol, (rva, size) in OWNERS.items():
            require(names.get(rva) == symbol, f'retail owner identity changed at {rva:x}')
            cls.pairs.append((production_body(BUILD / 'objdiff/base/gruntentrancemove.obj', symbol),
                              retail_body(image, names, symbol, rva, size)))

    def test_real_production_owners_against_original_retail(self):
        for production, retail in self.pairs:
            with self.subTest(owner=production.label):
                self.assertEqual(check_guard(production), check_guard(retail))

    def test_redirected_retail_edge_keeps_counts_and_referents_but_fails(self):
        for _, retail in self.pairs:
            with self.subTest(owner=retail.label):
                check_guard(retail)
                branch, _, _ = current_player_guard(retail)
                self.assertEqual(branch.mnemonic, 'jne')
                self.assertEqual(retail.data[branch.offset], 0x75)
                # Land after PlayVoice, executing ResetCell for the wrong player.
                reset_setup = retail.instructions[retail.unique_call(VOICE)].after
                displacement = reset_setup - branch.after
                self.assertTrue(-128 <= displacement <= 127)
                damaged = bytearray(retail.data)
                struct.pack_into('<b', damaged, branch.offset + 1, displacement)
                negative = replace(retail, label=retail.label + ' redirected', data=bytes(damaged))
                self.assertEqual([off for off, (a, b) in
                                  enumerate(zip(retail.data, negative.data)) if a != b],
                                 [branch.offset + 1])
                self.assertFalse(any(off <= branch.offset + 1 < off + 4
                                     for off, _, _ in retail.references))
                self.assertEqual(len(retail.data), len(negative.data))
                self.assertEqual(Counter(i.mnemonic for i in retail.instructions.values()),
                                 Counter(i.mnemonic for i in negative.instructions.values()))
                self.assertEqual(retail.calls, negative.calls)
                self.assertEqual(retail.references, negative.references)
                with self.assertRaisesRegex(AssertionError, 'equal/unequal call traces'):
                    check_guard(negative)


if __name__ == '__main__':
    unittest.main()
