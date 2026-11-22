"""Record and trim messages from the repository's original Windows client."""

import argparse
import base64
import hashlib
import json
import re
import sys
from datetime import datetime
from pathlib import Path
from queue import Empty, Queue
from threading import Event, Lock


ROOT = Path(__file__).resolve().parents[1]
HOOK = Path(__file__).with_name("origin-session-hook.js")
ORIGIN_EXE = ROOT / "Tenhou.Client.Origin" / "tenhou" / "tenhou.exe"
ORIGIN_LAUNCHER = ROOT / "Tenhou.Client.Origin" / "launcher.exe"
ORIGIN_SHA256 = "96164cdb63e043541c27572a22b895072a8f9a0a0ee97d877a3cb340e327ffe6"
FORMAT = "tenhou-origin-session/1"
RAW_FORMAT = "tenhou-origin-session-raw/1"
COMMANDS = """\
  mark [@序号] 备注         给指定事件打标。省略序号时标记当前最后一条事件
  show [起始序号 结束序号]    查看闭区间。省略区间时查看最新 20 条
  export 起始 结束 文件     导出闭区间及对应标记为新 JSONL，路径可含空格
  help / quit              显示命令 / 退出
"""
LIVE_COMMANDS = """\
  attach [PID]            附加到原版。省略 PID 时选择唯一运行的 tenhou.exe
  spawn [启动器路径]      启动原版 launcher.exe 并在 tenhou.exe 首条指令前开启记录（适合采集断线再开）
  detach                  解除附加
  start / pause           开始 / 暂停记录消息
"""
SENSITIVE_ATTR = re.compile(
    r'(\s(?:name|uname|auth|expire|gpid|n0|n1|n2|n3|log|seed|rankingcs)\s*=\s*)(?:"([^"]*)"|\'([^\']*)\')'
)
CHAT_TEXT_ATTR = re.compile(
    r'(\stext\s*=\s*)(?:"([^"]*)"|\'([^\']*)\')'
)


def verify_origin_exe(image_path):
    path = Path(image_path)
    if not path.is_file() or path.name.casefold() != "tenhou.exe":
        raise ValueError("指定路径不是有效的 tenhou.exe")
    with path.open("rb") as binary:
        if hashlib.file_digest(binary, "sha256").hexdigest() != ORIGIN_SHA256:
            raise ValueError("原版 EXE 的 SHA-256 与仓库取证版本不符")
    return path.resolve()


def verify_launcher_exe(image_path):
    path = Path(image_path)
    if path.name.casefold() == "tenhou.exe":
        raise ValueError("spawn 需启动 launcher.exe；原版 tenhou.exe 必须由启动器拉起")
    if not path.is_file() or path.name.casefold() != "launcher.exe":
        raise ValueError("指定路径不是有效的 launcher.exe")
    return path.resolve()


def redact_raw_text(text):
    def replace_sensitive(match):
        value = match.group(2) if match.group(2) is not None else match.group(3)
        if value == "NoName":
            return match.group(0)
        return f'{match.group(1)}"{"<empty>" if value == "" else "<present>"}"'

    redacted = SENSITIVE_ATTR.sub(replace_sensitive, text)
    if redacted.startswith("<CHAT "):
        def replace_chat_text(match):
            value = match.group(2) if match.group(2) is not None else match.group(3)
            if re.fullmatch(r"%2Flobby%20\d{1,4}", value, re.IGNORECASE):
                return match.group(0)
            return f'{match.group(1)}"<present>"'

        redacted = CHAT_TEXT_ATTR.sub(replace_chat_text, redacted)
    return redacted


def format_decoded_row(row, *, redact=False):
    if row["kind"] == "frame" and "data_b64" in row:
        raw_bytes = base64.b64decode(row["data_b64"], validate=True)
        has_nul = raw_bytes.endswith(b"\x00")
        body = raw_bytes[:-1] if has_nul else raw_bytes
        text = body.decode("utf-8", errors="backslashreplace")
        if redact:
            text = redact_raw_text(text)
        nul_suffix = " +NUL" if has_nul else " (no-NUL)"
        send_info = f' [send {row["send"]["accepted"]}/{row["send"]["requested"]}]' if "send" in row else ""
        return f'#{row["seq"]} {row["at"]} {row["direction"]} [{len(raw_bytes)}B{nul_suffix}]{send_info} {text}'
    if row["kind"] == "frame":
        body = " ".join(f"{k}={json.dumps(v, ensure_ascii=False)}" for k, v in row["attrs"].items())
        return f'#{row["seq"]} {row["at"]} {row["direction"]} {row["tag"]} {body}'.rstrip()
    if row["kind"] == "mark":
        return f'#{row["seq"]} {row["at"]} 标记 @{row["anchor"]} {row["note"]}'
    meta = " ".join(str(row[key]) for key in ("kind", "pid", "reason") if key in row)
    return f'#{row["seq"]} {row["at"]} {meta}'.rstrip()


def decode_journal(journal, *, out_path=None, bin_dir=None, redact=False):
    lines = [format_decoded_row(row, redact=redact) for row in journal.records]
    if out_path is not None:
        out_path.parent.mkdir(parents=True, exist_ok=True)
        out_path.write_text("\n".join(lines) + ("\n" if lines else ""), encoding="utf-8")
    else:
        for line in lines:
            print(line)
    extracted = 0
    if bin_dir is not None:
        if not journal.raw:
            raise ValueError("仅原始采集记录（tenhou-origin-session-raw/1）包含可导出的二进制帧")
        bin_dir.mkdir(parents=True, exist_ok=True)
        in_stream = bytearray()
        out_stream = bytearray()
        for row in journal.records:
            if row["kind"] != "frame" or "data_b64" not in row:
                continue
            raw_bytes = base64.b64decode(row["data_b64"], validate=True)
            frame_file = bin_dir / f'{row["seq"]:05d}-{row["direction"]}.bin'
            frame_file.write_bytes(raw_bytes)
            if row["direction"] == "in":
                in_stream.extend(raw_bytes)
            else:
                out_stream.extend(raw_bytes)
            extracted += 1
        (bin_dir / "in.bin").write_bytes(bytes(in_stream))
        (bin_dir / "out.bin").write_bytes(bytes(out_stream))
    return len(lines), extracted


def timestamp(tick=None):
    instant = datetime.now() if tick is None else datetime.fromtimestamp(tick / 1000)
    return instant.astimezone().isoformat(timespec="milliseconds")


def write_line(output, record):
    print(json.dumps(record, ensure_ascii=False, separators=(",", ":")), file=output)


class Journal:
    def __init__(self, path, review=False, *, raw=False):
        self.path = path.resolve()
        self.review = review
        self.raw = raw
        self.lock = Lock()
        if review:
            with self.path.open(encoding="utf-8") as source:
                self.header = json.loads(source.readline())
                if not isinstance(self.header, dict) or self.header.get("format") not in (FORMAT, RAW_FORMAT):
                    raise ValueError("不是原版采集记录")
                self.raw = self.header["format"] == RAW_FORMAT
                self.records = [json.loads(line) for line in source if line.strip()]
        else:
            self.header = {
                "format": RAW_FORMAT if raw else FORMAT,
                "origin_version": "130/0005", "origin_sha256": ORIGIN_SHA256,
                "created_at": timestamp(), "outbound": "Winsock send", "inbound": "0x415F48 / WM 0x8209",
            }
            self.records = []
            self.path.parent.mkdir(parents=True, exist_ok=True)
            with self.path.open("x", encoding="utf-8") as output:
                write_line(output, self.header)

    def add(self, kind, *, tick=None, **fields):
        with self.lock:
            sequence = self.records[-1]["seq"] + 1 if self.records else 1
            record = {"seq": sequence, "at": timestamp(tick), "kind": kind, **fields}
            if not self.review:
                with self.path.open("a", encoding="utf-8") as output:
                    write_line(output, record)
            self.records.append(record)
            return sequence

    def mark(self, text, anchor=None):
        with self.lock:
            if anchor is None and self.records:
                anchor = self.records[-1].get("anchor", self.records[-1]["seq"])
            if not any(record["seq"] == anchor and record["kind"] != "mark" for record in self.records):
                raise ValueError("请指定记录内的一条事件作为标记位置")
        return self.add("mark", anchor=anchor, note=text)

    def select(self, first, last):
        with self.lock:
            selected = [row for row in self.records if first <= row.get("anchor", row["seq"]) <= last]
        if not selected:
            raise ValueError("所选区间内没有事件")
        return selected

    def latest(self):
        with self.lock:
            return self.records[-20:]

    def export(self, first, last, destination):
        rows = self.select(first, last)
        destination.parent.mkdir(parents=True, exist_ok=True)
        with destination.open("x", encoding="utf-8") as output:
            write_line(output, {**self.header, "range": [first, last], "exported_at": timestamp()})
            for row in rows:
                write_line(output, row)
        return len(rows)


class Capture:
    def __init__(self, frida, journal):
        self.device = frida.get_local_device()
        self.journal = journal
        self.session = None
        self.script = None
        self.recording = False

    def _attach_pid(self, pid, *, start_recording=False):
        session = self.device.attach(pid)
        detached = Event()
        stream = self.journal.add("attach", pid=pid)

        def on_detach(reason, _crash):
            detached.set()
            self.journal.add("detach", stream=stream, reason=reason)

        def on_message(message, data):
            if message["type"] != "send":
                self.journal.add("gap", stream=stream, reason="hook-error")
                return
            payload = message["payload"]
            if self.journal.raw and payload["kind"] == "frame":
                if data is None or len(data) != payload["byte_length"]:
                    payload = {**payload, "kind": "gap", "reason": "binary-transfer"}
                else:
                    payload = {**payload, "data_b64": base64.b64encode(data).decode("ascii")}
            self.journal.add(stream=stream, **payload)

        session.on("detached", on_detach)
        try:
            script = session.create_script(HOOK.read_text(encoding="utf-8"))
            script.on("message", on_message)
            script.load()
            self.session, self.script, self.stream, self.detached = session, script, stream, detached
            self.recording = False
            if start_recording:
                self.record(True)
        except Exception:
            if not session.is_detached:
                session.detach()
            self.session = None
            self.script = None
            self.recording = False
            raise

    def attach(self, pid=None):
        if self.session and not self.session.is_detached:
            raise ValueError("当前原版进程仍在运行。可用 detach 解除后再附加")
        if pid is None:
            clients = [p for p in self.device.enumerate_processes() if p.name.casefold() == "tenhou.exe"]
            if len(clients) != 1:
                pids = ", ".join(str(p.pid) for p in clients)
                raise ValueError(f"请启动原版并停在登录页，或用 spawn 启动。多个客户端时使用 attach PID（当前候选 {pids or '无'}）")
            pid = clients[0].pid
        processes = self.device.enumerate_processes(pids=[pid], scope="metadata")
        if not processes:
            raise ValueError(f"PID {pid} 已退出或不存在")
        image = processes[0].parameters.get("path")
        if not image:
            raise ValueError("指定进程不是 tenhou.exe 或无法读取进程镜像路径")
        verify_origin_exe(image)
        self._attach_pid(pid, start_recording=False)
        print(f"已附加 PID {pid}，输入 start 开始记录")

    def _child_image_path(self, child):
        path = getattr(child, "path", None)
        if path and Path(path).is_absolute() and Path(path).is_file():
            return path
        processes = self.device.enumerate_processes(pids=[child.pid], scope="metadata")
        if processes:
            resolved = processes[0].parameters.get("path")
            if resolved:
                return resolved
        return path

    def spawn(self, exe=None):
        if self.session and not self.session.is_detached:
            raise ValueError("当前原版进程仍在运行。可用 detach 解除后再启动")
        target = verify_launcher_exe(exe or ORIGIN_LAUNCHER)
        events = Queue()

        def on_child_added(child):
            events.put(("child", child))

        launcher_pid = None
        launcher_session = None
        child_pid = None
        self.device.on("child-added", on_child_added)
        try:
            launcher_pid = self.device.spawn([str(target)], cwd=str(target.parent))
            launcher_session = self.device.attach(launcher_pid)
            launcher_session.on("detached", lambda reason, _crash: events.put(("detached", reason)))
            launcher_session.enable_child_gating()
            self.device.resume(launcher_pid)
            print(f"已启动启动器 PID {launcher_pid}，等待拉起 tenhou.exe...")
            while True:
                try:
                    kind, payload = events.get(timeout=0.2)
                except Empty:
                    continue
                if kind == "detached":
                    raise ValueError(f"启动器已退出（{payload}），未拉起 tenhou.exe")
                if payload.parent_pid != launcher_pid:
                    continue
                image = self._child_image_path(payload)
                if not image or Path(image).name.casefold() != "tenhou.exe":
                    self.device.resume(payload.pid)
                    continue
                child_pid = payload.pid
                verify_origin_exe(image)
                self._attach_pid(child_pid, start_recording=True)
                self.device.resume(child_pid)
                print(f"已由启动器拉起并从首条指令开始记录 PID {child_pid}")
                return
        except BaseException:
            if child_pid is not None:
                if self.session is not None and not self.session.is_detached:
                    self.close()
                try:
                    self.device.kill(child_pid)
                except Exception:
                    pass
            if launcher_pid is not None:
                try:
                    self.device.kill(launcher_pid)
                except Exception:
                    pass
            raise
        finally:
            self.device.off("child-added", on_child_added)
            if launcher_session is not None and not launcher_session.is_detached:
                try:
                    launcher_session.disable_child_gating()
                except Exception:
                    pass
                try:
                    launcher_session.detach()
                except Exception:
                    pass

    def record(self, enabled):
        if not self.session or self.session.is_detached or self.detached.is_set():
            raise ValueError("请先 attach 原版进程")
        if self.recording != enabled:
            if enabled:
                self.journal.add("start", stream=self.stream)
            self.script.exports_sync.set_recording(enabled, self.journal.raw)
            if not enabled:
                self.journal.add("pause", stream=self.stream)
            self.recording = enabled
        print("正在记录" if enabled else "已暂停")

    def close(self):
        if self.session and not self.session.is_detached:
            self.session.detach()
        self.session = None


def sequence_range(first, last):
    first, last = int(first), int(last)
    if not 1 <= first <= last:
        raise ValueError("请输入有效区间，起始序号须小于或等于结束序号")
    return first, last


def display(row):
    if row["kind"] == "frame":
        if "data_b64" in row:
            data = base64.b64decode(row["data_b64"], validate=True)
            message = f'{row["direction"]} [{len(data)} bytes] {data!r}'
        else:
            body = " ".join(f"{k}={json.dumps(v, ensure_ascii=False)}" for k, v in row["attrs"].items())
            message = f'{row["direction"]} {row["tag"]} {body}'
        if "send" in row:
            message += f' [send {row["send"]["accepted"]}/{row["send"]["requested"]}]'
    elif row["kind"] == "mark":
        message = f'标记 @{row["anchor"]} {row["note"]}'
    else:
        message = " ".join(str(row[key]) for key in ("kind", "pid", "reason") if key in row)
    print(f'#{row["seq"]} {row["at"]} {message}'.rstrip())


def prompt(journal, capture, errors):
    print("输入 help 查看命令。" if capture else "审阅模式，标记随 export 写入新文件。输入 help 查看命令。")
    while True:
        try:
            command, _, argument = input("origin> ").strip().partition(" ")
            argument = argument.strip()
            if command == "quit":
                break
            if command == "help":
                print((LIVE_COMMANDS if capture else "") + COMMANDS, end="")
            elif command == "attach" and capture:
                capture.attach(int(argument) if argument else None)
            elif command == "spawn" and capture:
                capture.spawn(Path(argument.strip('"')) if argument else None)
            elif command == "detach" and capture:
                capture.close()
                print("已解除附加")
            elif command in ("start", "pause") and capture:
                capture.record(command == "start")
            elif command == "mark":
                anchor = None
                if argument.startswith("@"):
                    position, _, argument = argument.partition(" ")
                    anchor = int(position[1:])
                if not argument.strip():
                    raise ValueError("需要填写备注")
                print(f"已标记 #{journal.mark(argument.strip(), anchor)}")
            elif command == "show":
                if argument:
                    first, last = argument.split()
                    rows = journal.select(*sequence_range(first, last))
                else:
                    rows = journal.latest()
                for row in rows:
                    display(row)
            elif command == "export":
                first, last, path = argument.split(maxsplit=2)
                destination = Path(path.strip('"'))
                count = journal.export(*sequence_range(first, last), destination)
                print(f"已导出 {count} 条至 {destination.resolve()}")
            elif command:
                print("此模式下无该命令。输入 help 查看用法。")
        except (EOFError, KeyboardInterrupt):
            print()
            break
        except errors as error:
            print(error, file=sys.stderr)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    modes = parser.add_subparsers(dest="mode", required=True)
    live = modes.add_parser("capture", help="attach to or spawn the original client and record")
    live.add_argument("--pid", type=int, help="original tenhou.exe process ID")
    live.add_argument("--spawn", action="store_true", help="spawn launcher.exe and start recording tenhou.exe before entry")
    live.add_argument("--exe", type=Path, help="path to launcher.exe for --spawn (default: repository origin launcher)")
    live.add_argument("--out", type=Path, help="new JSONL output path")
    live.add_argument("--raw", action="store_true", help="keep original frame bytes, including credentials")
    review = modes.add_parser("review", help="view, annotate and export a captured interval")
    review.add_argument("path", type=Path)
    decode = modes.add_parser("decode", help="decode raw Base64 JSONL frames to readable text and/or binary files")
    decode.add_argument("path", type=Path)
    decode.add_argument("--out", type=Path, help="write decoded text transcript to file instead of stdout")
    decode.add_argument("--bin-dir", type=Path, help="extract decoded per-frame .bin files and in.bin/out.bin streams")
    decode.add_argument("--redact", action="store_true", help="redact credentials and personal fields in decoded text")
    args = parser.parse_args()
    errors = (OSError, ValueError)
    if args.mode == "capture":
        if args.spawn and args.pid is not None:
            parser.error("--spawn 与 --pid 不能同时使用")
        try:
            import frida
        except ImportError:
            parser.error("实时采集需要 Frida，请运行 python -m pip install frida==17.27.1")
        errors += (frida.ProcessNotFoundError, frida.PermissionDeniedError, frida.ProcessNotRespondingError,
                   frida.NotSupportedError, frida.InvalidArgumentError, frida.InvalidOperationError,
                   frida.TransportError, frida.TimedOutError, frida.core.RPCException)

    try:
        if args.mode == "review":
            prompt(Journal(args.path, review=True), None, errors)
        elif args.mode == "decode":
            journal = Journal(args.path, review=True)
            lines, extracted = decode_journal(journal, out_path=args.out, bin_dir=args.bin_dir, redact=args.redact)
            if args.out is not None or args.bin_dir is not None:
                summary = f"已解码 {lines} 条事件"
                if args.out is not None:
                    summary += f"，文本写入 {args.out.resolve()}"
                if args.bin_dir is not None:
                    summary += f"，导出 {extracted} 个二进制帧及双向流至 {args.bin_dir.resolve()}"
                print(summary)
        else:
            prefix = "session-raw" if args.raw else "session"
            filename = datetime.now().strftime(f"{prefix}-%Y%m%d-%H%M%S-%f.jsonl")
            path = args.out or ROOT / "build" / "origin-capture" / filename
            journal = Journal(path, raw=args.raw)
            capture = Capture(frida, journal)
            print(f"记录文件 {journal.path}")
            if args.raw:
                print("原始记录可能包含登录 ID 和认证值，分享前请手工脱敏。")
            try:
                try:
                    if args.spawn:
                        capture.spawn(args.exe)
                    else:
                        capture.attach(args.pid)
                except errors as error:
                    print(error, file=sys.stderr)
                prompt(journal, capture, errors)
            finally:
                capture.close()
    except errors as error:
        parser.exit(1, f"{error}\n")


if __name__ == "__main__":
    main()
