"""Bounded production/original control for seven reroll-span Max consumers.

Read-only; run after a real build in pinned nix. Checks the signed selector,
second-on-tie transfer, route-count guards and SetEntrancePos receiver/arguments/
referent. It does not prove span producers, RNG, whole CFG or caller equivalence.
Unsupported shapes fail closed. Decode only the prefix through this operation:
some owners have unrelated trailing tables or differing call/inline topology.
"""

import re
import struct
import unittest

from test_entrance_player_guard import decode, require
from test_rng_helper_consumers import Consumer, production


SET_ENTRANCE = '?SetEntrancePos@CGrunt@@QAEXHH@Z'
# stem, method, original RVA, original extent, original selector CMP RVA.
OWNERS = (
    ('gruntdefenselean', 'StepMagicWandGruntBehavior', 0xf8240, 0x5b9, 0xf8783),
    ('gruntarrivalscan', 'StepBomberBehavior', 0xec670, 0x298, 0xec881),
    ('gruntscantarget', 'StepSmartChaserBehavior', 0xf42f0, 0x15c0, 0xf53fe),
    ('gruntdefensestep', 'StepScrollGruntBehavior', 0xf2b20, 0x6e1, 0xf3173),
    ('gruntchargestep', 'StepDumbChaserBehavior', 0xef6b0, 0x61d, 0xefca1),
    ('gruntwanderstep', 'StepHitAndRunnerBehavior', 0xed9f0, 0x900, 0xeddb0),
    ('gruntarrivalupdate', 'StepGauntletGruntBehavior', 0xf0130, 0x7c0, 0xf07bd),
)
# Find the local operation independently of branch conditions, register choices
# and call displacement, so mutations reach semantic checks instead of silently
# disappearing behind a target-name filter.
SPAN = re.compile(
    rb'\x8b[\x80-\x87]\x28\x03\x00\x00\x85\xc0[\x70-\x7f].'
    rb'\x3b.[\x70-\x7f].\x8b.\x3b.[\x70-\x7f].'
    rb'(?:\x6a.\x6a.\x8b.|\x8b\x4c\x24.\x6a.\x6a.)'
    rb'\xe8....', re.S)


def expect(instruction, mnemonic, operands=None):
    require(instruction.mnemonic == mnemonic
            and (operands is None or instruction.operands == operands),
            f'unsupported selector protocol at +{instruction.offset:x}: '
            f'{instruction.mnemonic} {instruction.operands}; expected {mnemonic} {operands}')


def target(instruction):
    require(re.fullmatch(r'0x[0-9a-f]+', instruction.operands),
            'unsupported selector branch destination')
    return int(instruction.operands, 16)


def check(code):
    matches = list(SPAN.finditer(code.data))
    require(len(matches) == 1, 'expected one complete reroll-span protocol')
    match = matches[0]
    prefix = list(decode(code.data[:match.end()]).values())
    positions = {instruction.offset: n for n, instruction in enumerate(prefix)}
    require(match.start() in positions, 'selector load is not an instruction boundary')
    start = positions[match.start()]
    local = prefix[start:]
    require(len(local) == 12, 'unexpected selector instruction census')
    load, zero, empty, comparison, greater, move, count, enough = local[:8]
    expect(load, 'mov')
    receiver = re.fullmatch(r'eax,DWORD PTR \[(eax|esi)\+0x328\]', load.operands)
    require(receiver, 'route count width/member/receiver changed')
    expect(zero, 'test', 'eax,eax')
    expect(empty, 'je')
    expect(comparison, 'cmp')
    operands = re.fullmatch(r'(ebx|esi|edi|ebp),(ebx|esi|edi|ebp)', comparison.operands)
    require(operands and operands[1] != operands[2], 'unsupported signed span inputs')
    first, second = operands.groups()
    expect(greater, 'jg')
    expect(move, 'mov', f'{first},{second}')
    require(target(greater) == count.offset, 'greater edge does not skip exactly second transfer')
    expect(count, 'cmp', f'eax,{first}')
    expect(enough, 'jle')
    skip = target(empty)
    require(target(enough) == skip, 'route-count guards disagree on skip destination')
    call = local[-1]
    expect(call, 'call')
    require(call.after <= skip < len(code.data), 'count guard does not bypass guarded call')
    # Decoding exactly to the destination proves it is a boundary, without
    # requiring or comparing unrelated trailing instructions/tables.
    decode(code.data[:skip])
    setup = local[8:-1]
    if receiver[1] == 'esi':
        expect(setup[0], 'push', '0x1')
        expect(setup[1], 'push', '0x1')
        expect(setup[2], 'mov', 'ecx,esi')
    else:
        require(start > 0, 'missing saved receiver load')
        saved = prefix[start - 1]
        expect(saved, 'mov')
        slot = re.fullmatch(r'eax,DWORD PTR \[esp\+0x([0-9a-f]+)\]', saved.operands)
        require(slot, 'unsupported stack-saved receiver')
        expect(setup[0], 'mov', f'ecx,DWORD PTR [esp+0x{slot[1]}]')
        expect(setup[1], 'push', '0x1')
        expect(setup[2], 'push', '0x1')
    if code.pe is None:
        refs = [(site, kind, name) for site, kind, name in code.references
                if call.offset <= site < call.after]
        require(refs == [(call.offset + 1, 0x14, SET_ENTRANCE)],
                'production SetEntrancePos referent changed')
        require(struct.unpack_from('<I', code.data, call.offset + 1)[0] == 0,
                'production SetEntrancePos referent addend changed')
    else:
        address = code.rva + call.after + struct.unpack_from('<i', code.data, call.offset + 1)[0]
        require(address == 0x1401, 'original SetEntrancePos thunk referent changed')
        thunk = code.pe.read(address, 5)
        require(thunk is not None and thunk[0] == 0xe9, 'missing original entrance thunk')
        require(address + 5 + struct.unpack_from('<i', thunk, 1)[0] == 0x4d060,
                'original SetEntrancePos implementation referent changed')
    return dict(load=load, comparison=comparison, greater=greater, move=move,
                empty=empty, enough=enough, call=call)


class RerollSpanMaxTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        from gruntz.core.paths import BUILD
        from gruntz.core.pe import Pe

        pe = Pe()
        cls.pairs = []
        for stem, method, rva, size, comparison in OWNERS:
            data = pe.read(rva, size)
            require(data is not None, f'missing original owner {method}')
            cls.pairs.append((
                production(BUILD / f'objdiff/base/{stem}.obj',
                           f'?{method}@CGrunt@@QAEHXZ'),
                Consumer(f'original {method}', data, (), rva, pe),
                comparison,
            ))

    def test_all_seven_production_and_original_consumers(self):
        self.assertEqual(len(self.pairs), 7)
        for compiled, original, expected_cmp in self.pairs:
            for code in (compiled, original):
                with self.subTest(owner=code.label):
                    result = check(code)
                    if code.pe is not None:
                        self.assertEqual(code.rva + result['comparison'].offset, expected_cmp)

    def test_unsigned_or_first_on_tie_selector_is_rejected(self):
        for compiled, original, _ in self.pairs:
            for code in (compiled, original):
                site = check(code)['greater'].offset
                for opcode in (b'\x77', b'\x7d'):  # JA or JGE instead of JG.
                    with self.subTest(owner=code.label, opcode=opcode):
                        with self.assertRaises(AssertionError):
                            check(code.changed(site, opcode))

    def test_swapped_transfer_destination_is_rejected(self):
        for compiled, original, _ in self.pairs:
            for code in (compiled, original):
                site = check(code)['move'].offset
                self.assertEqual(code.data[site], 0x8b)
                modrm = code.data[site + 1]
                self.assertEqual(modrm & 0xc0, 0xc0)
                swapped = (modrm & 0xc0) | ((modrm & 7) << 3) | ((modrm >> 3) & 7)
                damaged = code.changed(site + 1, bytes([swapped]))
                self.assertEqual(damaged.references, code.references)
                self.assertEqual(len(damaged.data), len(code.data))
                with self.subTest(owner=code.label), self.assertRaises(AssertionError):
                    check(damaged)

    def test_wrong_route_guard_condition_or_edge_is_rejected(self):
        for compiled, original, _ in self.pairs:
            for code in (compiled, original):
                sites = check(code)
                enough, empty, call = (sites[key] for key in ('enough', 'empty', 'call'))
                # Wrong unsigned domain, equality behavior, or a zero-route edge
                # redirected directly to the call while preserving instruction count.
                controls = ((enough.offset, b'\x76'), (enough.offset, b'\x7c'),
                            (empty.offset + 1, struct.pack('<b', call.offset - empty.after)))
                for offset, payload in controls:
                    with self.subTest(owner=code.label, offset=offset, payload=payload):
                        with self.assertRaises(AssertionError):
                            check(code.changed(offset, payload))

    def test_wrong_actual_callee_is_rejected(self):
        for compiled, original, _ in self.pairs:
            for code in (compiled, original):
                call = check(code)['call']
                if code.pe is None:
                    refs = tuple((site, kind, '_rand' if site == call.offset + 1 else name)
                                 for site, kind, name in code.references)
                    damaged = Consumer(code.label + ' wrong COFF referent', code.data, refs)
                    self.assertEqual(damaged.data, code.data)
                else:
                    damaged = code.changed(call.offset + 1,
                                           struct.pack('<i', 0x11fee0 - code.rva - call.after))
                with self.subTest(owner=code.label), self.assertRaisesRegex(AssertionError, 'referent'):
                    check(damaged)


if __name__ == '__main__':
    unittest.main()
