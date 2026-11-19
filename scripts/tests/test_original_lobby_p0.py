import hashlib
import struct
import unittest
from pathlib import Path
from xml.etree import ElementTree

try:
    from unicorn import UC_ARCH_X86, UC_HOOK_CODE, UC_MODE_32, Uc, UcError
except ModuleNotFoundError as error:
    if error.name != 'unicorn':
        raise
    message = 'P0 branch replay requires unicorn==2.1.4 (see ORIGIN-REBUILD.md)'
    if __name__ == '__main__':
        raise SystemExit(message) from error
    raise unittest.SkipTest(message) from error

from unicorn.x86_const import (
    UC_X86_REG_EAX as EAX,
    UC_X86_REG_EBP as EBP,
    UC_X86_REG_EBX as EBX,
    UC_X86_REG_ECX as ECX,
    UC_X86_REG_EDI as EDI,
    UC_X86_REG_EDX as EDX,
    UC_X86_REG_EIP as EIP,
    UC_X86_REG_ESI as ESI,
    UC_X86_REG_ESP as ESP,
)


ORIGINAL = Path(__file__).resolve().parents[2] / 'Tenhou.Client.Origin' / 'tenhou' / 'tenhou.exe'
SHA256 = '96164cdb63e043541c27572a22b895072a8f9a0a0ee97d877a3cb340e327ffe6'
IMAGE = ORIGINAL.read_bytes()
STACK = 0x110000
STATE = 0x120000
STOP = 0x200000
QUEUE_IDS = range(0xFAF, 0xFBB)
SEND_FORMAT = 0x415B10
GET_ITEM = STOP + 0x10
SEND_ITEM = STOP + 0x20


class Branch:
    def __init__(self):
        if hashlib.sha256(IMAGE).hexdigest() != SHA256:
            raise ValueError('The P0 branch addresses require the frozen original EXE')
        self.cpu = Uc(UC_ARCH_X86, UC_MODE_32)
        self.cpu.mem_map(0x400000, 0x120000)
        for address, offset, size in (
            (0x401000, 0x1000, 0xE3B92),
            (0x4E5000, 0xE5000, 0xE8DE),
            (0x4F4000, 0xF4000, 0x10000),
        ):
            self.cpu.mem_write(address, IMAGE[offset:offset + size])
        self.cpu.mem_map(STACK - 0x10000, 0x30000)
        self.cpu.mem_map(STOP, 0x1000)
        self.cpu.reg_write(ESP, STACK)
        self.cpu.reg_write(EBP, 0xFA1)
        self.cpu.reg_write(EBX, STATE)
        self.cpu.reg_write(ESI, GET_ITEM)
        self.cpu.reg_write(EDI, SEND_ITEM)
        self.stubs = {}
        self.calls = []
        self.cpu.hook_add(UC_HOOK_CODE, self.dispatch)
        self.stub(GET_ITEM, 2, lambda parent, control: control)

    def put(self, address, *values):
        self.cpu.mem_write(address, struct.pack(f'<{len(values)}I', *values))

    def dword(self, address):
        return struct.unpack('<I', self.cpu.mem_read(address, 4))[0]

    def text(self, address):
        return bytes(self.cpu.mem_read(address, 128)).split(b'\0', 1)[0].decode('cp932')

    def stub(self, address, count, callback, *, caller_cleanup=False):
        self.cpu.mem_write(address, b'\xc3' if caller_cleanup else b'\xc2' + struct.pack('<H', count * 4))
        self.stubs[address] = count, callback

    def imported(self, slot, count, callback):
        address = STOP + 0x100 + len(self.stubs) * 0x10
        self.put(slot, address)
        self.stub(address, count, callback)

    def dispatch(self, cpu, address, size, data):
        if address in self.stubs:
            count, callback = self.stubs[address]
            stack = cpu.reg_read(ESP)
            result = callback(*(self.dword(stack + index * 4) for index in range(1, count + 1)))
            cpu.reg_write(EAX, result or 0)

    def run(self, start, stop=STOP):
        try:
            self.cpu.emu_start(start, stop, timeout=1_000_000, count=10_000)
        except UcError as error:
            raise AssertionError(f'Branch {start:#x} stopped at {self.cpu.reg_read(EIP):#x}: {error}') from error
        if self.cpu.reg_read(EIP) != stop:
            raise AssertionError(f'Branch did not reach {stop:#x}')

    def wire(self, fmt, *values):
        self.calls.append(self.text(fmt) % values)


def ranked_case(tab, pf4, pf3, expire, opt, reservations, room=0):
    branch = Branch()
    branch.put(STATE + 4, opt, expire)
    branch.put(STATE + 0x1C, *pf4, *pf3, 0)
    branch.put(STATE + 0x0C + tab * 4, reservations)
    branch.put(STATE + 0x2C, room)
    branch.put(STACK + 0x64, 1)
    branch.cpu.reg_write(EAX, tab)
    branch.stub(0x443E60, 2, lambda out, reason: out, caller_cleanup=True)
    branch.stub(0x415160, 3, lambda *args: None)
    branch.stub(0x414380, 1, lambda *args: None)
    branch.imported(0x4E53C0, 2, lambda control, enabled: branch.calls.append(('enable', control, enabled)))
    branch.imported(0x4E53D8, 1, lambda count: 1)
    branch.imported(0x4E53DC, 8, lambda handle, control, after, px, py, width, height, flags:
                    branch.calls.append(('show', control, flags)) or 1)
    branch.run(0x439D81, 0x43A4FB)
    visible = {control - 0xFAF for action, control, flags in branch.calls if action == 'show' and flags == 0x47}
    enabled = {control - 0xFAF for action, control, value in branch.calls if action == 'enable' and value}
    return visible, enabled


class OriginalLobbyP0(unittest.TestCase):
    def test_ln_sentinel_slots(self):
        joined = list(range(1000, 1113))
        gaming = list(range(2000, 2113))
        payload = '<LN n="81,82,83,84" j="{}" g="{}" />'.format(
            ','.join(map(str, joined)), ','.join(map(str, gaming)))
        reply = ElementTree.fromstring(payload)
        totals, joined, gaming = (list(map(int, reply.attrib[key].split(','))) for key in ('n', 'j', 'g'))
        branch = Branch()
        branch.put(STATE + 0xC, STATE + 0x100, STATE + 0x110)
        branch.put(STATE + 0x100, *totals)
        branch.cpu.reg_write(EDI, STATE)
        branch.cpu.reg_write(EBP, 0)
        branch.stub(0x428290, 5, lambda out, fmt, *values: branch.calls.append((fmt, *values)), caller_cleanup=True)
        branch.run(0x427F79, 0x427FE1)
        self.assertEqual(branch.calls, [(0x4F6E8C, 81, 83, 84)])

        branch = Branch()
        branch.put(STACK + 0x4C, STATE)
        branch.put(STATE + 0x18, STATE + 0x100)
        branch.put(STATE + 0x28, STATE + 0x300)
        branch.put(STATE + 0x100, *joined)
        branch.put(STATE + 0x300, *gaming)
        for tab in range(4):
            branch.cpu.reg_write(EAX, tab)
            branch.run(0x428DD2, 0x428E73)
            expected = (16 * tab + 3, 16 * tab + 7, 16 * tab, 16 * tab + 4, 16 * tab + 8, 16 * tab + 12,
                        16 * tab + 9, 16 * tab + 13, 64 + 2 * tab, 65 + 2 * tab, 72 + 2 * tab, 73 + 2 * tab)
            for index, slot in enumerate(expected):
                branch.put(STACK + 0x1C, STACK + 0xAC + 4 * index)
                branch.cpu.reg_write(ESP, STACK)
                branch.run(0x428E73, 0x428E9C)
                branch.cpu.reg_write(ESP, STACK)
                self.assertEqual(branch.cpu.reg_read(EDX), 1000 + slot)
                self.assertEqual(branch.cpu.reg_read(ECX), 2000 + slot)
        branch.run(0x4291FA, 0x429245)
        self.assertEqual([branch.dword(STACK + offset) for offset in range(0x60, 0x80, 4)], list(range(96, 104)))
        branch.cpu.reg_write(EAX, 0)
        branch.cpu.reg_write(EDI, STATE)
        branch.run(0x4295A7, 0x4295BA)
        self.assertEqual((branch.cpu.reg_read(EDX), branch.cpu.reg_read(ECX)), (1112, 2112))

    def test_ranked_queue_type_table(self):
        base = (7, 15, 1, 9, 0x41, 0x49, 0x1041, 0x1049, 0x11, 0x19, 0x51, 0x59)
        for tab, mask in enumerate((0, 0x80, 0x20, 0xA0)):
            for control, expected in zip(QUEUE_IDS, base):
                branch = Branch()
                branch.put(STACK, STOP, 0, tab, control)
                branch.run(0x43F590)
                self.assertEqual(branch.cpu.reg_read(EAX), expected | mask, (tab, control))

    def test_ranked_visibility_and_enable(self):
        four = set(range(6))
        three = set(range(8, 12))
        both = four | three
        cases = (
            ('general-split', 0, (13, 1799), (13, 1800), 0, 0, 0, four, four),
            ('advanced-split', 1, (8, 1500), (9, 1500), 59, 0, 0, three, three),
            ('advanced-paid-boundary', 1, (8, 1500), (8, 1500), 60, 0, 0, both, both),
            ('advanced-phoenix-exclusion', 1, (16, 2000), (16, 1999), 60, 0, 0, three, three),
            ('special-split', 2, (13, 1800), (13, 1799), 60, 0, 0, four, four),
            ('phoenix-split', 3, (16, 1999), (16, 2000), 60, 0, 0, three, three),
            ('phoenix-client-without-expire', 3, (16, 2000), (16, 2000), 0, 0, 0, both, both),
            ('general-light-and-reserved', 0, (9, 1500), (9, 1500), 60, 1, 1 << 2,
             set(range(12)), set(range(12)) - {2}),
            ('option-is-tab-specific', 2, (13, 1800), (13, 1800), 60, 1, 0, both, both),
            ('special-light', 2, (13, 1800), (13, 1800), 60, 4, 0, set(range(12)), set(range(12))),
        )
        for name, tab, pf4, pf3, expire, opt, reservations, visible, enabled in cases:
            with self.subTest(name=name):
                self.assertEqual(ranked_case(tab, pf4, pf3, expire, opt, reservations), (visible, enabled))

    def test_cross_tab_reservation_rejoin_and_cancel_message(self):
        branch = Branch()
        branch.stub(SEND_FORMAT, 3, branch.wire, caller_cleanup=True)
        for tab, control in ((0, 0xFB1), (2, 0xFB8)):
            branch.put(STACK + 0x14, control)
            branch.put(STACK + 0x38, 0)
            branch.cpu.reg_write(EDI, tab)
            branch.put(STACK, STOP, 0, tab, control)
            branch.run(0x43F590)
            branch.cpu.reg_write(ESP, STACK)
            branch.run(0x43AF7D, 0x43D330)
        expected = ['<JOIN t="0,1" />', '<JOIN t="0,57" />']
        self.assertEqual(branch.calls, expected)
        self.assertEqual(branch.dword(STATE + 0xC), 1 << 2)
        self.assertEqual(branch.dword(STATE + 0x14), 1 << 9)
        branch.calls.clear()
        branch.cpu.reg_write(ESP, STACK - 8)
        branch.run(0x43E6CD, 0x43E77D)
        self.assertEqual(branch.calls, expected)
        branch.stub(0x415AC0, 1, branch.wire, caller_cleanup=True)
        branch.run(0x42706F, 0x427725)
        self.assertEqual(branch.calls[-1], '<JOIN />')

    def test_ordinary_room_and_broadcast_visibility(self):
        for room in (1234, 8141):
            with self.subTest(room=room):
                regular = set(range(12)) - {6, 7}
                self.assertEqual(ranked_case(0, (0, 1500), (17, 2200), 0, 0xF, 1 << 2, room),
                                 (regular, regular - {2}))

    def test_lobby_requests_preserve_contest_leading_zeroes(self):
        branch = Branch()
        branch.put(STACK + 0x18, 12)
        branch.stub(SEND_FORMAT, 3, lambda fmt, *values:
                    branch.wire(fmt, *(values[:1] if fmt == 0x4F6F10 else values)), caller_cleanup=True)
        for requested in (0, 123, 10007):
            branch.cpu.reg_write(EBX, requested)
            branch.run(0x43E608, 0x43E654)
        self.assertEqual(branch.calls, ['<CHAT text="%2Flobby%200" />', '<CHAT text="%2Flobby%20123" />',
                                        '<CS lobby="C00070012" />'])

    def test_lobby_confirmation_and_missing_or_invalid_id(self):
        for reply, expected in (
            ('<CHAT lobby="0124" />', (124, 0)),
            ('<CHAT lobby="0000" />', (0, 0)),
            ('<CHAT lobby="C00070012" />', (10007, 12)),
            ('<CHAT />', None),
            ('<CHAT lobby="C0007" />', None),
            ('<CHAT lobby="L00070012" />', None),
        ):
            with self.subTest(reply=reply):
                branch = Branch()
                branch.put(STATE + 0x2C, 123)
                branch.put(STACK + 0x14F0, STATE + 0x200)
                branch.put(STACK + 0xC4, STATE + 0x100)
                branch.cpu.reg_write(ECX, STATE + 0x200)
                branch.cpu.mem_write(STATE + 0x200, reply.encode('ascii') + b'\0')
                lobby = b''

                def read_attribute(out, message, left, right):
                    nonlocal lobby
                    self.assertEqual((branch.text(left), branch.text(right)), (' lobby="', '"'))
                    lobby = ElementTree.fromstring(branch.text(message)).get('lobby', '').encode('ascii')
                    branch.cpu.mem_write(branch.dword(out), lobby + b'\0')
                    return out

                def substring(out, start, length):
                    branch.cpu.mem_write(out, lobby[start:start + length] + b'\0')
                    return out

                branch.stub(0x4C48C0, 4, read_attribute, caller_cleanup=True)
                branch.stub(0x435D20, 0, lambda: len(lobby))
                branch.stub(0x4448C0, 1, lambda index: STATE + 0x100 + index)
                branch.stub(0x428540, 3, substring)
                branch.stub(0x435D10, 0, lambda: STATE + 0x100 if branch.cpu.reg_read(ECX) == STACK + 0xC4
                            else branch.cpu.reg_read(ECX))
                branch.stub(0x4CD52B, 3, lambda value, end, base: int(branch.text(value), base), caller_cleanup=True)
                branch.stub(0x401EC0, 0, lambda: None)
                branch.stub(0x466BF0, 2, lambda *args: None, caller_cleanup=True)
                branch.stub(0x43F310, 4, lambda owner, room, key, switch:
                            branch.calls.append((room & 0xFFFF, key & 0xFFFF)), caller_cleanup=True)
                branch.run(0x43E076, 0x43E1EB)
                actual = struct.unpack('<HH', branch.cpu.mem_read(STATE + 0x2C, 4))
                self.assertEqual(actual, (123, 0) if expected is None else expected)
                self.assertEqual(branch.calls, [] if expected is None else [expected])

    def test_contest_action_visibility(self):
        for name, blocked, registered, fee, now, expected in (
            ('blocked', 1, 0, 10, 20, (0, 0)),
            ('register', 0, 0, 10, 20, (0, 5)),
            ('no-fee-not-registered', 0, 0, 0, 20, (0, 0)),
            ('before-start', 0, 1, 10, 9, (0, 0)),
            ('start-inclusive', 0, 1, 10, 10, (5, 0)),
            ('ready', 0, 1, 10, 20, (5, 0)),
            ('deadline', 0, 1, 10, 30, (0, 0)),
            ('reserved', 0, 0x101, 10, 20, None),
        ):
            with self.subTest(name=name):
                branch = Branch()
                branch.cpu.reg_write(EAX, blocked)
                branch.put(STATE + 0x4E, registered)
                branch.put(STATE + 0x7C, fee)
                branch.put(STATE + 0x70, now, 0)
                branch.put(STACK + 0x3C, 10, 0)
                branch.put(STACK + 0xA0, 30, 0)
                branch.stub(0x41A2D0, 3, lambda out, fmt, value: out, caller_cleanup=True)
                branch.stub(0x4CC938, 1, lambda string: 1, caller_cleanup=True)
                branch.imported(0x4E5038, 5, lambda *args: None)
                branch.imported(0x4E54B4, 3, lambda *args: None)
                branch.imported(0x4E544C, 2, lambda control, show: branch.calls.append(('show', control, show)))
                branch.imported(0x4E53C0, 2, lambda control, enabled: branch.calls.append(('enable', control, enabled)))
                branch.run(0x43C2EC, 0x43C53B)
                calls = [('enable', 0xFBB, 0)] if expected is None else [
                    ('show', control, show) for control, show in zip((0xFBB, 0xFBC), expected)]
                self.assertEqual(branch.calls, calls)

    def test_ranked_rule_panel_timers(self):
        for capture, is_visible, expected in (
            (0xFB1, False, [('kill', 0x67), ('set', 0x66, 500)]),
            (0xFB1, True, [('kill', 0x67), ('rules', 0xFB1)]),
            (0, False, [('kill', 0x66), ('set', 0x67, 500)]),
        ):
            with self.subTest(capture=capture, is_visible=is_visible):
                branch = Branch()
                branch.put(STACK + 0x14EC, capture)
                branch.imported(0x4E53CC, 1, lambda window: 1)
                branch.imported(0x4E5448, 1, lambda window: int(is_visible))
                branch.imported(0x4E537C, 2, lambda window, timer: branch.calls.append(('kill', timer)))
                branch.imported(0x4E5490, 4, lambda window, timer, ms, proc: branch.calls.append(('set', timer, ms)))
                branch.stub(0x43F6B0, 4, lambda window, control, rank3, rank4:
                            branch.calls.append(('rules', control)), caller_cleanup=True)
                branch.run(0x43C7E6, 0x43D330)
                self.assertEqual(branch.calls, expected)

    def test_toutenkou_nuki_candidates(self):
        tile_cases = [((tile,), [f'n{tile}']) for tile in (0, 1, 2, 3, 16, 17, 18, 19, 32, 33, 34, 35, 120, 121, 122, 123)]
        tile_cases += [((4, 119, 124), []), ((1, 3, 16, 18, 34, 35, 122, 123), ['n122', 'n34', 'n18', 'n16', 'n1'])]
        for hand, expected in tile_cases:
            with self.subTest(hand=hand):
                branch = Branch()
                branch.put(STACK + 0x38, STATE)
                branch.stub(0x485E70, 2, lambda tiles, tile: int(tile in hand), caller_cleanup=True)

                def format_candidate(out, fmt, tile):
                    candidate = branch.text(fmt) % tile
                    branch.cpu.mem_write(out, candidate.encode('ascii') + b'\0')
                    return out

                branch.stub(0x4183E0, 3, format_candidate, caller_cleanup=True)
                branch.stub(0x402470, 2, lambda text, size: branch.calls.append(branch.text(text)))
                branch.run(0x49727E, 0x497683)
                self.assertEqual(branch.calls, expected)

    def test_toutenkou_switch_and_type(self):
        for attributes in (0xFFFFFFFF, 0x80):
            branch = Branch()
            branch.imported(0x4E527C, 1, lambda filename: branch.calls.append(branch.text(filename)) or attributes)
            branch.imported(0x4E53F0, 3, lambda parent, control, title: branch.calls.append((control, branch.text(title))))
            branch.run(0x426D98, 0x426DCF)
            self.assertEqual(branch.calls[0], 'toutenkou.test')
            self.assertEqual(branch.calls[1][0], 0xFA4)
            self.assertEqual('紅天' in branch.calls[1][1], attributes != 0xFFFFFFFF)
        branch = Branch()
        branch.put(STACK, STOP, 2, 0, 0xFAF)
        branch.run(0x43F590)
        self.assertEqual(branch.cpu.reg_read(EAX), 0x811)


if __name__ == '__main__':
    unittest.main()
