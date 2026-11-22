import base64
import contextlib
import importlib.util
import io
import json
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


SCRIPTS = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("origin_capture", SCRIPTS / "Capture-OriginSession.py")
CAPTURE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CAPTURE)


class OriginSessionCapture(unittest.TestCase):
    def test_raw_export_can_be_reviewed_without_changing_bytes(self):
        data = b'<EXTRA value="a&b%2F\x80\xff" />\0'
        with tempfile.TemporaryDirectory() as folder:
            source = Path(folder) / "raw.jsonl"
            trimmed = Path(folder) / "cut.jsonl"
            journal = CAPTURE.Journal(source, raw=True)
            sequence = journal.add("frame", direction="in", byte_length=len(data),
                                   data_b64=base64.b64encode(data).decode("ascii"))
            review = CAPTURE.Journal(source, review=True)
            review.mark("资料已显示", sequence)
            self.assertEqual(review.export(sequence, sequence, trimmed), 2)
            result = CAPTURE.Journal(trimmed, review=True)
            self.assertTrue(result.raw)
            self.assertEqual(result.records[0], journal.records[0])
            self.assertEqual(result.records[1]["anchor"], sequence)
            output = io.StringIO()
            with contextlib.redirect_stdout(output):
                CAPTURE.display(result.records[0])
            self.assertIn(repr(data), output.getvalue())

    def test_crop_preserves_anchors_and_original_sequences(self):
        with tempfile.TemporaryDirectory() as folder:
            source = Path(folder) / "full.jsonl"
            trimmed = Path(folder) / "interval with spaces.jsonl"
            journal = CAPTURE.Journal(source)
            first = journal.add("frame", direction="out", tag="JOIN", attrs={"t": "0,9"})
            last = journal.add("frame", direction="in", tag="GO", attrs={"type": "9"})
            outside = journal.add("detach", reason="process-terminated")
            journal.mark("收到开桌", last)
            journal.mark("已退出", outside)

            review = CAPTURE.Journal(source, review=True)
            offline_mark = review.mark("点击预约", first)
            self.assertEqual(review.export(first, last, trimmed), 4)
            result = CAPTURE.Journal(trimmed, review=True)
            self.assertEqual(result.header["origin_sha256"], CAPTURE.ORIGIN_SHA256)
            self.assertEqual(result.header["range"], [first, last])
            self.assertEqual([row["seq"] for row in result.records], [first, last, outside + 1, offline_mark])
            self.assertNotIn("点击预约", source.read_text(encoding="utf-8"))
            with self.assertRaises(FileExistsError):
                review.export(first, last, source)

    def test_spawn_launches_launcher_and_arms_child_tenhou_before_resume(self):
        events = []

        class FakeChild:
            def __init__(self, pid, parent_pid, path):
                self.pid = pid
                self.parent_pid = parent_pid
                self.path = str(path)

        class FakeExports:
            def set_recording(self, enabled, raw):
                events.append(("set_recording", enabled, raw))

        class FakeScript:
            exports_sync = FakeExports()

            def on(self, name, _cb):
                events.append(("script.on", name))

            def load(self):
                events.append(("script.load",))

        class FakeSession:
            def __init__(self, pid):
                self.pid = pid
                self.is_detached = False

            def on(self, name, _cb):
                events.append(("session.on", self.pid, name))

            def enable_child_gating(self):
                events.append(("enable_child_gating", self.pid))

            def disable_child_gating(self):
                events.append(("disable_child_gating", self.pid))

            def create_script(self, _source):
                events.append(("create_script", self.pid))
                return FakeScript()

            def detach(self):
                self.is_detached = True
                events.append(("detach", self.pid))

        class FakeProcess:
            def __init__(self, path):
                self.parameters = {"path": str(path)}

        class FakeDevice:
            def __init__(self):
                self.child_handlers = []

            def on(self, name, cb):
                events.append(("device.on", name))
                if name == "child-added":
                    self.child_handlers.append(cb)

            def off(self, name, cb):
                events.append(("device.off", name))
                if name == "child-added" and cb in self.child_handlers:
                    self.child_handlers.remove(cb)

            def spawn(self, argv, cwd=None):
                events.append(("spawn", Path(argv[0]).name, Path(cwd).name))
                return 1001

            def attach(self, pid):
                events.append(("attach", pid))
                return FakeSession(pid)

            def enumerate_processes(self, pids=None, scope=None):
                events.append(("enumerate_processes", tuple(pids or ()), scope))
                if pids == [4321]:
                    return [FakeProcess(CAPTURE.ORIGIN_EXE)]
                return []

            def resume(self, pid):
                events.append(("resume", pid))
                if pid == 1001:
                    for handler in list(self.child_handlers):
                        handler(FakeChild(4321, 1001, "tenhou.exe"))

            def kill(self, pid):
                events.append(("kill", pid))

        class FakeFrida:
            def get_local_device(self):
                return FakeDevice()

        with tempfile.TemporaryDirectory() as folder:
            journal = CAPTURE.Journal(Path(folder) / "spawn-raw.jsonl", raw=True)
            capture = CAPTURE.Capture(FakeFrida(), journal)
            output = io.StringIO()
            with contextlib.redirect_stdout(output):
                capture.spawn(CAPTURE.ORIGIN_LAUNCHER)
            self.assertTrue(capture.recording)
            self.assertEqual(
                events,
                [
                    ("device.on", "child-added"),
                    ("spawn", "launcher.exe", "Tenhou.Client.Origin"),
                    ("attach", 1001),
                    ("session.on", 1001, "detached"),
                    ("enable_child_gating", 1001),
                    ("resume", 1001),
                    ("enumerate_processes", (4321,), "metadata"),
                    ("attach", 4321),
                    ("session.on", 4321, "detached"),
                    ("create_script", 4321),
                    ("script.on", "message"),
                    ("script.load",),
                    ("set_recording", True, True),
                    ("resume", 4321),
                    ("device.off", "child-added"),
                    ("disable_child_gating", 1001),
                    ("detach", 1001),
                ],
            )
            self.assertEqual([row["kind"] for row in journal.records], ["attach", "start"])
            self.assertEqual(journal.records[0]["pid"], 4321)
            capture.close()

    def test_spawn_rejects_tenhou_exe_and_reports_early_launcher_exit(self):
        class FakeSession:
            is_detached = False

            def on(self, name, cb):
                if name == "detached":
                    self.on_detached = cb

            def enable_child_gating(self):
                pass

            def disable_child_gating(self):
                pass

            def detach(self):
                self.is_detached = True

        class FakeDevice:
            def on(self, _name, _cb):
                pass

            def off(self, _name, _cb):
                pass

            def spawn(self, _argv, cwd=None):
                return 2002

            def attach(self, _pid):
                self.session = FakeSession()
                return self.session

            def resume(self, _pid):
                self.session.is_detached = True
                self.session.on_detached("process-terminated", None)

            def kill(self, _pid):
                pass

        class FakeFrida:
            def get_local_device(self):
                return FakeDevice()

        with tempfile.TemporaryDirectory() as folder:
            journal = CAPTURE.Journal(Path(folder) / "spawn-fail.jsonl", raw=True)
            capture = CAPTURE.Capture(FakeFrida(), journal)
            with self.assertRaisesRegex(ValueError, "launcher.exe"):
                capture.spawn(CAPTURE.ORIGIN_EXE)
            with contextlib.redirect_stdout(io.StringIO()), self.assertRaisesRegex(ValueError, "未拉起 tenhou.exe"):
                capture.spawn(CAPTURE.ORIGIN_LAUNCHER)

    def test_decode_outputs_readable_text_redaction_and_binary_streams(self):
        frames = [
            ("out", b'<HELO name="ID12345678-12345678" gpid="secret-gpid" sx="M" tid="w0" />\0'),
            ("in", b'<CHAT uname="alice" text="%2Flobby%204409" lobby="4409" />\0'),
            ("in", b'<CHAT uname="alice" text="private-chat" lobby="4409" />\0'),
            ("in", b'<GO type="17" lobby="4409" gpid="secret-gpid" />\0'),
        ]
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            source = root / "raw.jsonl"
            journal = CAPTURE.Journal(source, raw=True)
            for direction, payload in frames:
                journal.add("frame", direction=direction, byte_length=len(payload),
                            data_b64=base64.b64encode(payload).decode("ascii"))
            review = CAPTURE.Journal(source, review=True)
            txt = root / "decoded.txt"
            bin_dir = root / "bins"
            lines, extracted = CAPTURE.decode_journal(review, out_path=txt, bin_dir=bin_dir, redact=True)
            self.assertEqual((lines, extracted), (4, 4))
            decoded = txt.read_text(encoding="utf-8")
            self.assertNotIn("ID12345678", decoded)
            self.assertNotIn("secret-gpid", decoded)
            self.assertNotIn("alice", decoded)
            self.assertNotIn("private-chat", decoded)
            self.assertIn("%2Flobby%204409", decoded)
            self.assertIn('type="17"', decoded)
            self.assertEqual((bin_dir / "00001-out.bin").read_bytes(), frames[0][1])
            self.assertEqual((bin_dir / "out.bin").read_bytes(), frames[0][1])
            self.assertEqual((bin_dir / "in.bin").read_bytes(), b"".join(p for d, p in frames if d == "in"))

    @unittest.skipUnless(shutil.which("node"), "Node.js needed to exercise hook callbacks")
    def test_hook_filters_sources_and_preserves_bytes_in_raw_mode(self):
        script = r'''
            const fs = require('node:fs');
            const vm = require('node:vm');
            const ptr = n => ({
                add: m => ptr(n + m), equals: other => n === other.toUInt32(),
                readU32: () => 7, toUInt32: () => n, toInt32: () => n,
            });
            const callbacks = {};
            const rpc = {exports: {}};
            const events = [];
            vm.runInNewContext(fs.readFileSync(process.argv[1], 'utf8'), {
                Process: {mainModule: {base: ptr(0x400000)}, getModuleByName: () => ({getExportByName: n => n})},
                Interceptor: {attach: (name, handlers) => callbacks[name] = handlers},
                rpc, send: (record, data) => events.push(data ? {...record, data: data.toString('base64')} : record),
            });
            function receive(frame, site = 0x415f4e) {
                const bytes = Buffer.from(frame + '\0', 'latin1');
                callbacks.SendMessageA.onEnter.call({returnAddress: ptr(site), context: {esi: ptr(bytes.length - 1)}},
                    [null, ptr(0x8209), null, {readByteArray: length => bytes.subarray(0, length)}]);
            }
            function transmit(socket, frame, shortWrite = false) {
                const bytes = Buffer.from(frame + '\0');
                const call = {};
                callbacks.send.onEnter.call(call,
                    [ptr(socket), {readByteArray: length => bytes.subarray(0, length)}, ptr(bytes.length)]);
                callbacks.send.onLeave.call(call, ptr(bytes.length - Number(shortWrite)));
            }
            receive('<Z />');
            rpc.exports.setRecording(true);
            transmit(8, '<Z />');
            receive('<Z />', 0x415f4d);
            transmit(7, '<AUTH val="token-secret" />', true);
            receive('<HELO uname="name-secret" auth="challenge-secret" PF4="1,2,3" expire="20991231" />');
            receive('<CHAT uname="name-secret" text="chat-secret" lobby="C00112233" />');
            receive('<CSREG FAILED />');
            receive('<LN n="A1Bc" />');
            receive('<RANK v2="1200,0,-1,2.5" />');
            receive('<REINIT hai="1,2,3" kawa0="1,255,33" m0="8736" />');
            receive('<TAIKYOKU log="log-secret" />');
            receive('<GO type="9" gpid="game-secret" />');
            transmit(7, '<N type="3" hai0="8" hai1="12" />');
            transmit(7, '<REACH hai="52" />');
            receive('<BYE who="2" />');
            receive('<RANKING v2="1,0,0,0,66.8,,0,4726,0,5027,0,0,0,0,29991" rankingcs="ranking-secret" />');
            receive('<UNKNOWN_SECRET id="other-secret" />');
            rpc.exports.setRecording(false);
            receive('<Z />');
            const redacted = events.splice(0);
            rpc.exports.setRecording(true, true);
            transmit(8, '<Z />');
            receive('<Z />', 0x415f4d);
            transmit(7, '<AUTH val="fixture-token" />', true);
            receive('<FUTURE x="a&b%2F\x80\xff" x="duplicate" />');
            receive('<RANK v2="' + '1,'.repeat(33000) + '1" />');
            rpc.exports.setRecording(false, true);
            receive('<Z />');
            console.log(JSON.stringify({redacted, raw: events}));
        '''
        result = subprocess.run(["node", "-e", script, str(SCRIPTS / "origin-session-hook.js")],
                                capture_output=True, text=True, encoding="utf-8")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertNotIn("secret", result.stdout.casefold())
        self.assertNotIn("20991231", result.stdout)
        events = json.loads(result.stdout)
        rows = events["redacted"]
        self.assertEqual([row["tag"] for row in rows],
                         ["AUTH", "HELO", "CHAT", "CSREG", "LN", "RANK", "REINIT", "TAIKYOKU",
                          "GO", "N", "REACH", "BYE", "RANKING", "UNKNOWN"])
        self.assertEqual(rows[0]["attrs"], {"val": "<present>"})
        self.assertEqual(rows[0]["send"]["requested"] - rows[0]["send"]["accepted"], 1)
        self.assertEqual(rows[1]["attrs"]["PF4"], "1,2,3")
        self.assertEqual(rows[2]["attrs"], {"uname": "<present>", "lobby": "C00112233"})
        self.assertEqual(rows[3]["attrs"], {"FAILED": True})
        self.assertEqual(rows[4]["attrs"], {"n": "A1Bc"})
        self.assertEqual(rows[5]["attrs"], {"v2": "1200,0,-1,2.5"})
        self.assertEqual(rows[6]["attrs"], {"hai": "1,2,3", "kawa0": "1,255,33", "m0": "8736"})
        self.assertEqual(rows[8]["attrs"], {"type": "9", "gpid": "<present>"})
        self.assertEqual(rows[9]["attrs"], {"type": "3", "hai0": "8", "hai1": "12"})
        self.assertEqual(rows[10]["attrs"], {"hai": "52"})
        self.assertEqual(rows[11]["attrs"], {"who": "2"})
        self.assertEqual(rows[12]["attrs"],
                         {"v2": "1,0,0,0,66.8,,0,4726,0,5027,0,0,0,0,29991", "rankingcs": "<present>"})
        raw = events["raw"]
        self.assertEqual([row["direction"] for row in raw], ["out", "in", "in"])
        expected_frames = [
            b'<AUTH val="fixture-token" />\0',
            b'<FUTURE x="a&b%2F\x80\xff" x="duplicate" />\0',
            b'<RANK v2="' + b"1," * 33000 + b'1" />\0',
        ]
        for row, expected in zip(raw, expected_frames, strict=True):
            self.assertEqual(row["kind"], "frame")
            self.assertEqual(row["byte_length"], len(expected))
            self.assertEqual(base64.b64decode(row["data"]), expected)
        self.assertEqual(raw[0]["send"],
                         {"requested": len(expected_frames[0]), "accepted": len(expected_frames[0]) - 1})
