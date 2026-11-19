import importlib.util
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

    @unittest.skipUnless(shutil.which("node"), "Node.js needed to exercise hook callbacks")
    def test_hook_filters_source_and_redacts_before_emitting(self):
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
                rpc, send: record => events.push(record),
            });
            function receive(frame, site = 0x415f4e) {
                const bytes = Buffer.from(frame, 'latin1');
                callbacks.SendMessageA.onEnter.call({returnAddress: ptr(site), context: {esi: ptr(bytes.length)}},
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
            receive('<UNKNOWN_SECRET id="other-secret" />');
            rpc.exports.setRecording(false);
            receive('<Z />');
            console.log(JSON.stringify(events));
        '''
        result = subprocess.run(["node", "-e", script, str(SCRIPTS / "origin-session-hook.js")],
                                capture_output=True, text=True, encoding="utf-8")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertNotIn("secret", result.stdout.casefold())
        self.assertNotIn("20991231", result.stdout)
        rows = json.loads(result.stdout)
        self.assertEqual([row["tag"] for row in rows],
                         ["AUTH", "HELO", "CHAT", "CSREG", "LN", "RANK", "REINIT", "TAIKYOKU", "UNKNOWN"])
        self.assertEqual(rows[0]["attrs"], {"val": "<present>"})
        self.assertEqual(rows[0]["send"]["requested"] - rows[0]["send"]["accepted"], 1)
        self.assertEqual(rows[1]["attrs"]["PF4"], "1,2,3")
        self.assertEqual(rows[2]["attrs"], {"uname": "<present>", "lobby": "C00112233"})
        self.assertEqual(rows[3]["attrs"], {"FAILED": True})
        self.assertEqual(rows[4]["attrs"], {"n": "A1Bc"})
        self.assertEqual(rows[5]["attrs"], {"v2": "1200,0,-1,2.5"})
        self.assertEqual(rows[6]["attrs"], {"hai": "1,2,3", "kawa0": "1,255,33", "m0": "8736"})
